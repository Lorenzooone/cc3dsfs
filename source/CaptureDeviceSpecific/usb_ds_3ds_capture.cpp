#include "usb_ds_3ds_capture.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <cstring>
#include <chrono>
#include <iostream>

// Adapted from sample code provided by Loopy

#define REAL_SERIAL_NUMBER_SIZE 32
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

#define FIRST_3D_VERSION 6
#define CAPTURE_SKIP_TIMEOUT_SECONDS 1.0

enum usb_capture_status {
	USB_CAPTURE_SUCCESS = 0,
	USB_CAPTURE_SKIP,
	USB_CAPTURE_PIPE_ERROR,
	USB_CAPTURE_FRAMEINFO_ERROR,
	USB_CAPTURE_ERROR
};

struct usb_device {
	bool is_3ds;
	int vid;
	int pid;
	int default_config;
	int capture_interface;
	int control_timeout;
	int bulk_timeout;
	int ep2_in;
	int cmdin_status;
	int cmdin_frameinfo;
	int cmdout_capture_start;
	int cmdout_capture_stop;
	int cmdin_i2c_read;
	int cmdout_i2c_write;
	int i2caddr_3dsconfig;
	int bitstream_3dscfg_ver;
};

static const usb_device usb_3ds_desc = {
.is_3ds = true,
.vid = 0x16D0, .pid = 0x06A3,
.default_config = 1, .capture_interface = 0,
.control_timeout = 30, .bulk_timeout = 50,
.ep2_in = 2 | LIBUSB_ENDPOINT_IN,
.cmdin_status = 0, .cmdin_frameinfo = 0,
.cmdout_capture_start = 0x40, .cmdout_capture_stop = 0,
.cmdin_i2c_read = 0x21, .cmdout_i2c_write = 0x21,
.i2caddr_3dsconfig = 0x14, .bitstream_3dscfg_ver = 1
};

static const usb_device usb_old_ds_desc = {
.is_3ds = false,
.vid = 0x16D0, .pid = 0x0647,
.default_config = 1, .capture_interface = 0,
.control_timeout = 500, .bulk_timeout = 500,
.ep2_in = 2 | LIBUSB_ENDPOINT_IN,
.cmdin_status = 0x31, .cmdin_frameinfo = 0x30,
.cmdout_capture_start = 0x30, .cmdout_capture_stop = 0x31,
.cmdin_i2c_read = 0, .cmdout_i2c_write = 0,
.i2caddr_3dsconfig = 0, .bitstream_3dscfg_ver = 0
};

static const usb_device* usb_devices_desc_list[] = {
	&usb_3ds_desc,
	&usb_old_ds_desc,
};

// Read vendor request from control endpoint.  Returns bytes transferred (<0 = libusb error)
static int vend_in(libusb_device_handle *handle, const usb_device* usb_device_desc, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t *buf) {
	return libusb_control_transfer(handle, ((uint8_t)LIBUSB_REQUEST_TYPE_VENDOR | (uint8_t)LIBUSB_ENDPOINT_IN), bRequest, wValue, wIndex, buf, wLength, usb_device_desc->control_timeout);
}

// Write vendor request to control endpoint.  Returns bytes transferred (<0 = libusb error)
static int vend_out(libusb_device_handle *handle, const usb_device* usb_device_desc, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t *buf) {
	return libusb_control_transfer(handle, ((uint8_t)LIBUSB_REQUEST_TYPE_VENDOR | (uint8_t)LIBUSB_ENDPOINT_OUT), bRequest, wValue, wIndex, buf, wLength, usb_device_desc->control_timeout);
}

// Read from bulk endpoint.  Returns libusb error code
static int bulk_in(libusb_device_handle *handle, const usb_device* usb_device_desc, uint8_t *buf, int length, int *transferred) {
	return libusb_bulk_transfer(handle, usb_device_desc->ep2_in, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Read FPGA configuration regs
static bool read_config(libusb_device_handle *handle, const usb_device* usb_device_desc, uint8_t cfgAddr, uint8_t *buf, int count) {
	if(!usb_device_desc->is_3ds)
		return false;
	if((count <= 0) || (count >= 256))
		return false;
	if(vend_out(handle, usb_device_desc, usb_device_desc->cmdout_i2c_write, usb_device_desc->i2caddr_3dsconfig, 0, 1, &cfgAddr) <= 0)
		return false;
   	return vend_in(handle, usb_device_desc, usb_device_desc->cmdin_i2c_read, usb_device_desc->i2caddr_3dsconfig, 0, count, buf) == count;
}

static uint8_t capture_get_version(libusb_device_handle *handle, const usb_device* usb_device_desc) {
	if(!usb_device_desc->is_3ds)
		return 0;
	uint8_t version=0;
	read_config(handle, usb_device_desc, usb_device_desc->bitstream_3dscfg_ver, &version, 1);
	return version;
}

static bool capture_get_has_3d(libusb_device_handle *handle, const usb_device* usb_device_desc) {
	if(!usb_device_desc->is_3ds)
		return false;
	return capture_get_version(handle, usb_device_desc) >= FIRST_3D_VERSION;
}

static const usb_device* _get_usb_device_desc(CaptureDevice* device) {
	return (usb_device*)device->descriptor;
}

static const usb_device* get_usb_device_desc(CaptureData* capture_data) {
	return _get_usb_device_desc(&capture_data->status.device);
}

static std::string get_serial(libusb_device_handle *handle, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	uint8_t data[SERIAL_NUMBER_SIZE];
	std::string serial_str = std::to_string(curr_serial_extra_id);
	if(libusb_get_string_descriptor_ascii(handle, usb_descriptor->iSerialNumber, data, REAL_SERIAL_NUMBER_SIZE-1) >= 0) {
		data[REAL_SERIAL_NUMBER_SIZE] = '\0';
		serial_str = std::string((const char*)data);
	}
	else
		curr_serial_extra_id += 1;
	return serial_str;
}

static int do_usb_config_device(libusb_device_handle *handle, const usb_device* usb_device_desc) {
	libusb_check_and_detach_kernel_driver(handle, usb_device_desc->capture_interface);
	int result = libusb_check_and_set_configuration(handle, usb_device_desc->default_config);
	if(result != LIBUSB_SUCCESS)
		return result;
	libusb_check_and_detach_kernel_driver(handle, usb_device_desc->capture_interface);
	return result;
}

static int insert_device(std::vector<CaptureDevice> &devices_list, const usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	uint8_t data[SERIAL_NUMBER_SIZE];
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return LIBUSB_ERROR_NOT_FOUND;
	int result = libusb_open(usb_device, &handle);
	if(result || (handle == NULL))
		return result;
	result = do_usb_config_device(handle, usb_device_desc);
	if(result != LIBUSB_SUCCESS) {
		libusb_close(handle);
		return result;
	}
	result = libusb_claim_interface(handle, usb_device_desc->capture_interface);
	if(result == LIBUSB_SUCCESS)
		libusb_release_interface(handle, usb_device_desc->capture_interface);
	if(result < 0) {
		libusb_close(handle);
		return result;
	}
	std::string serial_str = get_serial(handle, usb_descriptor, curr_serial_extra_id);
	if(usb_device_desc->is_3ds)
		devices_list.emplace_back(serial_str, "3DS", CAPTURE_CONN_USB, (void*)usb_device_desc, true, capture_get_has_3d(handle, usb_device_desc), true, HEIGHT_3DS, TOP_WIDTH_3DS + BOT_WIDTH_3DS, O3DS_SAMPLES_IN, 90, 0, 0, TOP_WIDTH_3DS, 0, VIDEO_DATA_RGB);
	else
		devices_list.emplace_back(serial_str, "DS", CAPTURE_CONN_USB, (void*)usb_device_desc, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS, VIDEO_DATA_RGB16);
	libusb_close(handle);
	return result;
}

static libusb_device_handle* usb_find_by_serial_number(const usb_device* usb_device_desc, std::string &serial_number) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	libusb_device_handle *final_handle = NULL;

	int curr_serial_extra_id = 0;
	for(int i = 0; i < num_devices; i++) {
		libusb_device_handle *handle = NULL;
		uint8_t data[SERIAL_NUMBER_SIZE];
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if((usb_descriptor.idVendor != usb_device_desc->vid) || (usb_descriptor.idProduct != usb_device_desc->pid))
			continue;
		result = libusb_open(usb_devices[i], &handle);
		if(result || (handle == NULL))
			continue;
		std::string device_serial_number = get_serial(handle, &usb_descriptor, curr_serial_extra_id);
		if(serial_number == device_serial_number) {
			final_handle = handle;
			break;
		}
		libusb_close(handle);
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
	return final_handle;
}

static void capture_end(libusb_device_handle *dev, const usb_device* usb_device_desc, bool interface_claimed = true) {
	if(interface_claimed)
		libusb_release_interface(dev, usb_device_desc->capture_interface);
	libusb_close(dev);
}

static uint64_t _usb_get_video_in_size(const usb_device* usb_device_desc, bool enabled_3d) {
	if(!usb_device_desc->is_3ds)
		return sizeof(USBOldDSVideoInputData);
	if(enabled_3d)
		return sizeof(RGB83DSVideoInputData_3D);
	return sizeof(RGB83DSVideoInputData);
}

static uint64_t get_capture_size(const usb_device* usb_device_desc, bool enabled_3d) {
	if(!usb_device_desc->is_3ds)
		return sizeof(USBOldDSVideoInputData);
	if(enabled_3d)
		return sizeof(USB3DSCaptureReceived_3D) - EXTRA_DATA_BUFFER_USB_SIZE;
	return sizeof(USB3DSCaptureReceived) - EXTRA_DATA_BUFFER_USB_SIZE;
}

static usb_capture_status capture_read_oldds_3ds(CaptureData* capture_data, size_t *read_amount) {
	*read_amount = 0;
	CaptureDataSingleBuffer* full_data_buf = capture_data->data_buffers.GetWriterBuffer(0);
	CaptureReceived* data_buffer = &full_data_buf->capture_buf;
	libusb_device_handle* handle = (libusb_device_handle*)capture_data->handle;
	const usb_device* usb_device_desc = get_usb_device_desc(capture_data);
	const bool enabled_3d = get_3d_enabled(&capture_data->status);
	int bytesIn = 0;
	int transferred = 0;
	int result = 0;
	uint64_t video_size = _usb_get_video_in_size(usb_device_desc, enabled_3d);
	uint64_t full_in_size = get_capture_size(usb_device_desc, enabled_3d);
	uint8_t *video_data_ptr = (uint8_t*)data_buffer->usb_received_3ds.video_in.screen_data;
	if(!usb_device_desc->is_3ds)
		video_data_ptr = (uint8_t*)data_buffer->usb_received_old_ds.video_in.screen_data;

	uint8_t dummy;
	result = vend_out(handle, usb_device_desc, usb_device_desc->cmdout_capture_start, 0, 0, 0, &dummy);
	if(result < 0)
		return (result == LIBUSB_ERROR_TIMEOUT) ? USB_CAPTURE_SKIP: USB_CAPTURE_ERROR;

	// Both DS and 3DS old CCs send data until end of frame, followed by 0-length packets
	do {
		int transferSize = ((full_in_size - bytesIn) + (EXTRA_DATA_BUFFER_USB_SIZE - 1)) & ~(EXTRA_DATA_BUFFER_USB_SIZE - 1); // multiple of maxPacketSize
		result = bulk_in(handle, usb_device_desc, video_data_ptr + bytesIn, transferSize, &transferred);
		if(result == LIBUSB_SUCCESS)
			bytesIn += transferred;
	} while((bytesIn < full_in_size) && (result == LIBUSB_SUCCESS) && (transferred > 0));

	if(result==LIBUSB_ERROR_PIPE) {
		libusb_clear_halt(handle, usb_device_desc->ep2_in);
		return USB_CAPTURE_PIPE_ERROR;
	} else if(result == LIBUSB_ERROR_TIMEOUT || (usb_device_desc->is_3ds && (bytesIn < video_size))) {
		return USB_CAPTURE_SKIP;
	} else if(result != LIBUSB_SUCCESS) {
		return USB_CAPTURE_ERROR;
	}

	if(!usb_device_desc->is_3ds) {
		#ifndef SIMPLE_DS_FRAME_SKIP
		if(bytesIn < video_size) {
			if(vend_in(handle, usb_device_desc, usb_device_desc->cmdin_frameinfo, 0, 0, sizeof(data_buffer->usb_received_old_ds.frameinfo), (uint8_t*)&data_buffer->usb_received_old_ds.frameinfo) < 0)
				return USB_CAPTURE_FRAMEINFO_ERROR;
		}
		else {
			data_buffer->usb_received_old_ds.frameinfo.valid = 1;
			for(int i = 0; i < (HEIGHT_DS >> 3) << 1; i++)
				data_buffer->usb_received_old_ds.frameinfo.half_line_flags[i] = 0xFF;
		}
		#else
		if(bytesIn < video_size)
			return USB_CAPTURE_SKIP;
		#endif
	}

	*read_amount = bytesIn;
	return USB_CAPTURE_SUCCESS;
}

static void process_usb_capture_result(usb_capture_status result, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, bool* done, CaptureData* capture_data, size_t read_amount) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - (*clock_start);
	bool wrote_to_buffer = false;

	switch(result) {
		case USB_CAPTURE_SKIP:
			/*
			if(diff.count() >= CAPTURE_SKIP_TIMEOUT_SECONDS) {
				capture_error_print(true, capture_data, "Disconnected: Too long since last read");
				done = true;
			}
			*/
			break;
		case USB_CAPTURE_PIPE_ERROR:
			capture_error_print(true, capture_data, "Disconnected: Pipe error");
			*done = true;
			break;
		case USB_CAPTURE_FRAMEINFO_ERROR:
			capture_error_print(true, capture_data, "Disconnected: Frameinfo error");
			*done = true;
			break;
		case USB_CAPTURE_SUCCESS:
			*clock_start = curr_time;
			wrote_to_buffer = true;
			if(capture_data->status.cooldown_curr_in)
				capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
			capture_data->data_buffers.WriteToBuffer(NULL, read_amount, diff.count(), &capture_data->status.device, 0);
			capture_data->status.video_wait.unlock();
			capture_data->status.audio_wait.unlock();
			break;
		default:
			capture_error_print(true, capture_data, "Disconnected: Error");
			*done = true;
			break;
	}

	if(!wrote_to_buffer)
		capture_data->data_buffers.ReleaseWriterBuffer(0, false);
}

void list_devices_usb_ds_3ds(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	const size_t num_usb_desc = sizeof(usb_devices_desc_list) / sizeof(usb_devices_desc_list[0]);
	bool no_access_elems[num_usb_desc];
	int curr_serial_extra_ids[num_usb_desc];
	for (int i = 0; i < num_usb_desc; i++) {
		no_access_elems[i] = false;
		curr_serial_extra_ids[i] = 0;
	}
	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for(int j = 0; j < num_usb_desc; j++)
			if(insert_device(devices_list, usb_devices_desc_list[j], usb_devices[i], &usb_descriptor, curr_serial_extra_ids[j]) != LIBUSB_ERROR_NOT_FOUND) {
				if (result == LIBUSB_ERROR_ACCESS)
					no_access_elems[j] = true;
				break;
			}
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);

	for(int i = 0; i < num_usb_desc; i++)
		if(no_access_elems[i])
			no_access_list.emplace_back(usb_devices_desc_list[i]->vid, usb_devices_desc_list[i]->pid);
}

bool connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	if(!usb_is_initialized())
		return false;
	const usb_device* usb_device_desc = _get_usb_device_desc(device);
	libusb_device_handle *dev = usb_find_by_serial_number(usb_device_desc, device->serial_number);
	if(!dev) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	if(do_usb_config_device(dev, usb_device_desc) != LIBUSB_SUCCESS) {
		capture_error_print(true, capture_data, "Configuration failed");
		capture_end(dev, usb_device_desc, false);
		return false;
	}
	if(libusb_claim_interface(dev, usb_device_desc->capture_interface) != LIBUSB_SUCCESS) {
		capture_error_print(true, capture_data, "Interface claim failed");
		capture_end(dev, usb_device_desc, false);
		return false;
	}

	if(usb_device_desc->is_3ds) {
		//FW bug(?) workaround- first read times out sometimes
		uint8_t dummy;
		vend_out(dev, usb_device_desc, usb_device_desc->cmdout_capture_start, 0, 0, 0, &dummy);
		default_sleep(usb_device_desc->bulk_timeout);
	}

	capture_data->handle = (void*)dev;

	return true;
}

uint64_t usb_get_video_in_size(CaptureData* capture_data) {
	return _usb_get_video_in_size(get_usb_device_desc(capture_data), get_3d_enabled(&capture_data->status));
}

uint64_t usb_get_video_in_size(CaptureData* capture_data, bool override_3d) {
	return _usb_get_video_in_size(get_usb_device_desc(capture_data), override_3d);
}

void usb_capture_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;

	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	bool done = false;
	size_t read_amount = 0;

	while((!done) && capture_data->status.connected && capture_data->status.running) {
		usb_capture_status result = capture_read_oldds_3ds(capture_data, &read_amount);

		process_usb_capture_result(result, &clock_start, &done, capture_data, read_amount);
	}
}

void usb_capture_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;

	capture_data->data_buffers.ReleaseWriterBuffer(0, false);
	const usb_device* usb_device_desc = get_usb_device_desc(capture_data);
	capture_end((libusb_device_handle*)capture_data->handle, usb_device_desc);
}

void usb_ds_3ds_init() {
	return usb_init();
}

void usb_ds_3ds_close() {
	usb_close();
}


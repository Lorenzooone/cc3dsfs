#include "usb_ds_3ds_capture.hpp"
#include "devicecapture.hpp"

#include <libusb.h>
#include <cstring>
#include <thread>
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
	int bpp;
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

struct usb_ds_status {
	uint32_t framecount;
	uint8_t lcd_on;
	uint8_t capture_in_progress;
};

static const usb_device usb_3ds_desc = {
.is_3ds = true, .bpp = IN_VIDEO_BPP_SIZE_3DS,
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
.is_3ds = false, .bpp = IN_VIDEO_BPP_SIZE_DS_BASIC,
.vid = 0x16D0, .pid = 0x0647,
.default_config = 1, .capture_interface = 0,
.control_timeout = 500, .bulk_timeout = 500,
.ep2_in = 2 | LIBUSB_ENDPOINT_IN,
.cmdin_status = 0x31, .cmdin_frameinfo = 0x30,
.cmdout_capture_start = 0x30, .cmdout_capture_stop = 0x31,
.cmdin_i2c_read = 0, .cmdout_i2c_write = 0,
.i2caddr_3dsconfig = 0, .bitstream_3dscfg_ver = 0
};

static bool usb_initialized = false;
static libusb_context *usb_ctx = NULL; // libusb session context

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
	const usb_device* usb_device_desc = &usb_3ds_desc;
	if(!device->is_3ds) {
		if(device->rgb_bits_size == usb_old_ds_desc.bpp)
			usb_device_desc = &usb_old_ds_desc;
	}
	return usb_device_desc;
}

static const usb_device* get_usb_device_desc(CaptureData* capture_data) {
	return _get_usb_device_desc(&capture_data->status.device);
}

static bool insert_device(std::vector<CaptureDevice> &devices_list, const usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	uint8_t data[SERIAL_NUMBER_SIZE];
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return false;
	int result = libusb_open(usb_device, &handle);
	if(result || (handle == NULL))
		return true;
	std::string serial_str = std::to_string(curr_serial_extra_id);
	if(libusb_get_string_descriptor_ascii(handle, usb_descriptor->iSerialNumber, data, REAL_SERIAL_NUMBER_SIZE-1) >= 0) {
		data[REAL_SERIAL_NUMBER_SIZE] = '\0';
		serial_str = std::string((const char*)data);
	}
	else
		curr_serial_extra_id += 1;
	if(usb_device_desc->is_3ds)
		devices_list.emplace_back(serial_str, "3DS", CAPTURE_CONN_USB, true, capture_get_has_3d(handle, usb_device_desc), true, HEIGHT_3DS, TOP_WIDTH_3DS + BOT_WIDTH_3DS, O3DS_SAMPLES_IN, usb_device_desc->bpp, 90, 0, 0, TOP_WIDTH_3DS, 0);
	else {
		bool has_audio = false;
		int max_samples_in = 0;
		if(usb_device_desc->bpp > IN_VIDEO_BPP_SIZE_DS_BASIC) {
			has_audio = true;
			max_samples_in = DS_SAMPLES_IN;
		}
		devices_list.emplace_back(serial_str, "DS", CAPTURE_CONN_USB, false, false, has_audio, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, max_samples_in, usb_device_desc->bpp, 0, 0, 0, 0, HEIGHT_DS);
	}
	libusb_close(handle);
	return true;
}

static libusb_device_handle* usb_find_by_serial_number(const usb_device* usb_device_desc, std::string &serial_number) {
	if(!usb_initialized)
		return NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(usb_ctx, &usb_devices);
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
		std::string device_serial_number = std::to_string(curr_serial_extra_id);
		if(libusb_get_string_descriptor_ascii(handle, usb_descriptor.iSerialNumber, data, REAL_SERIAL_NUMBER_SIZE-1) >= 0) {
			data[REAL_SERIAL_NUMBER_SIZE] = '\0';
			device_serial_number = std::string((const char*)data);
		}
		else
			curr_serial_extra_id += 1;
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

static usb_capture_status capture_read_oldds_3ds(libusb_device_handle *handle, const usb_device* usb_device_desc, CaptureReceived* data_buffer, int &bytesIn, bool enabled_3d) {
	bytesIn = 0;
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
		if(bytesIn < video_size) {
			if(vend_in(handle, usb_device_desc, usb_device_desc->cmdin_frameinfo, 0, 0, sizeof(data_buffer->usb_received_old_ds.frameinfo), (uint8_t*)&data_buffer->usb_received_old_ds.frameinfo) < 0)
				return USB_CAPTURE_FRAMEINFO_ERROR;
		}
		else {
			data_buffer->usb_received_old_ds.frameinfo.valid = 1;
			for(int i = 0; i < (HEIGHT_DS >> 3) << 1; i++)
				data_buffer->usb_received_old_ds.frameinfo.half_line_flags[i] = 0xFF;
		}
	}

	return USB_CAPTURE_SUCCESS;
}

static inline uint8_t xbits_to_8bits(uint8_t value, int num_bits) {
	int left_shift = 8 - num_bits;
	int right_shift = num_bits - left_shift;
	return (value << left_shift) | (value >> right_shift);
}

static inline void usb_oldDSconvertVideoToOutputRGBA(USBOldDSPixelData data, uint8_t* target) {
	target[0] = xbits_to_8bits(data.r, OLD_DS_PIXEL_R_BITS);
	target[1] = xbits_to_8bits(data.g, OLD_DS_PIXEL_G_BITS);
	target[2] = xbits_to_8bits(data.b, OLD_DS_PIXEL_B_BITS);
}

static inline void usb_oldDSconvertVideoToOutputHalfLine(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, int input_halfline, int output_halfline) {
	//de-interleave pixels
	for(int i = 0; i < WIDTH_DS / 2 ; i++) {
		uint32_t input_halfline_pixel = (input_halfline * (WIDTH_DS / 2)) + i;
		uint32_t output_halfline_pixel = (output_halfline * (WIDTH_DS / 2)) + i;
		usb_oldDSconvertVideoToOutputRGBA(p_in->video_in.screen_data[input_halfline_pixel * 2], p_out->screen_data[output_halfline_pixel + (WIDTH_DS * HEIGHT_DS)]);
		usb_oldDSconvertVideoToOutputRGBA(p_in->video_in.screen_data[(input_halfline_pixel * 2) + 1], p_out->screen_data[output_halfline_pixel]);
	}
}

static void usb_oldDSconvertVideoToOutput(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out) {
	if(!p_in->frameinfo.valid) { //LCD was off
		memset(p_out->screen_data, 0, WIDTH_DS * (2 * HEIGHT_DS) * 3);
		return;
	}

	// Handle first line being off, if needed
	memset(p_out->screen_data, 0, WIDTH_DS * 3);

	int input_halfline = 0;
	for(int i = 0; i < 2; i++) {
		if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
			usb_oldDSconvertVideoToOutputHalfLine(p_in, p_out, input_halfline++, i);
	}

	for(int i = 2; i < HEIGHT_DS * 2; i++) {
		if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
			usb_oldDSconvertVideoToOutputHalfLine(p_in, p_out, input_halfline++, i);
		else { // deal with missing half-line
			memcpy(p_out->screen_data[i * (WIDTH_DS / 2)], p_out->screen_data[(i - 2) * (WIDTH_DS / 2)], (WIDTH_DS / 2) * 3);
			memcpy(p_out->screen_data[(i * (WIDTH_DS / 2)) + (WIDTH_DS * HEIGHT_DS)], p_out->screen_data[((i - 2) * (WIDTH_DS / 2)) + (WIDTH_DS * HEIGHT_DS)], (WIDTH_DS / 2) * 3);
		}
	}
}

static void usb_3DSconvertVideoToOutput(USB3DSCaptureReceived *p_in, VideoOutputData *p_out) {
	memcpy(p_out->screen_data, p_in->video_in.screen_data, IN_VIDEO_HEIGHT_3DS * IN_VIDEO_WIDTH_3DS * 3);
}

void list_devices_usb_ds_3ds(std::vector<CaptureDevice> &devices_list) {
	if(!usb_initialized)
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(usb_ctx, &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	
	int curr_serial_extra_id_3ds = 0;
	int curr_serial_extra_id_old_ds = 0;
	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if(insert_device(devices_list, &usb_3ds_desc, usb_devices[i], &usb_descriptor, curr_serial_extra_id_3ds))
			continue;
		if(insert_device(devices_list, &usb_old_ds_desc, usb_devices[i], &usb_descriptor, curr_serial_extra_id_old_ds))
			continue;
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
}

bool connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	if(!usb_initialized)
		return false;
	const usb_device* usb_device_desc = _get_usb_device_desc(device);
	libusb_device_handle *dev = usb_find_by_serial_number(usb_device_desc, device->serial_number);
	if(!dev) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	if(libusb_set_configuration(dev, usb_device_desc->default_config) != LIBUSB_SUCCESS) {
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
	return _usb_get_video_in_size(get_usb_device_desc(capture_data), capture_data->status.enabled_3d);
}

void usb_capture_main_loop(CaptureData* capture_data) {
	if(!usb_initialized)
		return;
	const usb_device* usb_device_desc = get_usb_device_desc(capture_data);
	int inner_curr_in = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	bool done = false;

	while((!done) && capture_data->status.connected && capture_data->status.running) {

		int read_amount = 0;
		usb_capture_status result = capture_read_oldds_3ds((libusb_device_handle*)capture_data->handle, usb_device_desc, &capture_data->capture_buf[inner_curr_in], read_amount, capture_data->status.enabled_3d);

		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;

		switch(result) {
			case USB_CAPTURE_SKIP:
				if(diff.count() >= CAPTURE_SKIP_TIMEOUT_SECONDS) {
					capture_error_print(true, capture_data, "Disconnected: Too long since last read");
					done = true;
				}
				break;
			case USB_CAPTURE_PIPE_ERROR:
				capture_error_print(true, capture_data, "Disconnected: Pipe error");
				done = true;
				break;
			case USB_CAPTURE_FRAMEINFO_ERROR:
				capture_error_print(true, capture_data, "Disconnected: Frameinfo error");
				done = true;
				break;
			case USB_CAPTURE_SUCCESS:
				clock_start = curr_time;
				capture_data->time_in_buf[inner_curr_in] = diff.count();
				capture_data->read[inner_curr_in] = read_amount;

				inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
				if(capture_data->status.cooldown_curr_in)
					capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
				capture_data->status.curr_in = inner_curr_in;
				capture_data->status.video_wait.unlock();
				capture_data->status.audio_wait.unlock();
				break;
			default:
				capture_error_print(true, capture_data, "Disconnected: Error");
				done = true;
				break;
		}
	}
}

void usb_capture_cleanup(CaptureData* capture_data) {
	if(!usb_initialized)
		return;
	const usb_device* usb_device_desc = get_usb_device_desc(capture_data);
	capture_end((libusb_device_handle*)capture_data->handle, usb_device_desc);
}

void usb_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureDevice* capture_device, bool enabled_3d) {
	if(!usb_initialized)
		return;
	if(capture_device->is_3ds) {
		if(!enabled_3d)
			usb_3DSconvertVideoToOutput(&p_in->usb_received_3ds, p_out);
	}
	else if(capture_device->rgb_bits_size == IN_VIDEO_BPP_SIZE_DS_BASIC)
		usb_oldDSconvertVideoToOutput(&p_in->usb_received_old_ds, p_out);
}

void usb_init() {
	if(usb_initialized)
		return;
	int result = libusb_init(&usb_ctx); // open session
	if (result < 0) {
		usb_ctx = NULL;
		usb_initialized = false;
		return;
	}
	usb_initialized = true;
}

void usb_close() {
	if(usb_initialized)
		libusb_exit(usb_ctx);
	usb_ctx = NULL;
	usb_initialized = false;
}


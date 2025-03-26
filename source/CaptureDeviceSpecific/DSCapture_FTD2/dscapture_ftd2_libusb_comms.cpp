#include "dscapture_ftd2_libusb_comms.hpp"
#include "dscapture_ftd2_general.hpp"
#include "dscapture_ftd2_compatibility.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define MANUFACTURER_SIZE 128
#define DESCRIPTION_SIZE 128
#define SERIAL_NUMBER_SIZE 128

#define ENABLE_AUDIO true

#define FTDI_VID 0x0403
#define FT232H_PID 0x6014

#define DEFAULT_INTERFACE 0
#define DEFAULT_CONFIGURATION 1

#define DEFAULT_EP_IN 2
#define DEFAULT_EP_OUT 0x81

struct ftd2_libusb_handle_data {
	libusb_device_handle *usb_handle = NULL;
	int timeout_r_ms = 500;
	int timeout_w_ms = 500;
	int ep_in = 0x81;
	int ep_out = 2;
	int chip_index = 1;
};

struct vid_pid_descriptor {
	int vid;
	int pid;
};

static const vid_pid_descriptor base_device = {
	.vid = FTDI_VID, .pid = FT232H_PID
};

static const vid_pid_descriptor* accepted_devices[] = {
&base_device,
};

static const vid_pid_descriptor* get_device_descriptor(int vid, int pid) {
	for(int i = 0; i < sizeof(accepted_devices) / sizeof(*accepted_devices); i++)
		if((vid == accepted_devices[i]->vid) && (pid == accepted_devices[i]->pid))
			return accepted_devices[i];
	return NULL;
}

void ftd2_libusb_init() {
	return usb_init();
}

void ftd2_libusb_end() {
	usb_close();
}

static void ftd2_libusb_usb_thread_function(bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 300000;
	while(*usb_thread_run)
		libusb_handle_events_timeout_completed(get_usb_ctx(), &tv, NULL);
}

void ftd2_libusb_start_thread(std::thread* thread_ptr, bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = true;
	*thread_ptr = std::thread(ftd2_libusb_usb_thread_function, usb_thread_run);
}

void ftd2_libusb_close_thread(std::thread* thread_ptr, bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = false;
	thread_ptr->join();
}

static int read_strings(libusb_device_handle *handle, libusb_device_descriptor *usb_descriptor, char* manufacturer, char* description, char* serial) {
	manufacturer[0] = 0;
	description[0] = 0;
	serial[0] = 0;
	int result = libusb_get_string_descriptor_ascii(handle, usb_descriptor->iManufacturer, (uint8_t*)manufacturer, MANUFACTURER_SIZE);
	if(result < 0)
		return result;
	result = libusb_get_string_descriptor_ascii(handle, usb_descriptor->iProduct, (uint8_t*)description, DESCRIPTION_SIZE);
	if(result < 0)
		return result;
	result = libusb_get_string_descriptor_ascii(handle, usb_descriptor->iSerialNumber, (uint8_t*)serial, SERIAL_NUMBER_SIZE);
	if(result < 0)
		return result;
	manufacturer[MANUFACTURER_SIZE - 1] = 0;
	description[DESCRIPTION_SIZE - 1] = 0;
	serial[SERIAL_NUMBER_SIZE - 1] = 0;
	return 0;
}

static void close_handle_ftd2_libusb(libusb_device_handle *dev_handle, bool claimed) {
	if(dev_handle == NULL)
		return;
	if(claimed)
		libusb_release_interface(dev_handle, DEFAULT_INTERFACE);
	libusb_close(dev_handle);
}

static int init_handle_and_populate_device_info_ftd2_libusb(libusb_device_handle **dev_handle, libusb_device* dev, char* description, char* SerialNumber, const vid_pid_descriptor** curr_descriptor) {
	char manufacturer[MANUFACTURER_SIZE];
	libusb_device_descriptor desc = {0};
	int retval = libusb_get_device_descriptor(dev, &desc);
	if(retval < 0)
		return retval;
	*curr_descriptor = NULL;
	*curr_descriptor = get_device_descriptor(desc.idVendor, desc.idProduct);
	if((desc.bcdUSB < 0x0200) || ((*curr_descriptor) == NULL))
		return LIBUSB_ERROR_OTHER;

	retval = libusb_open(dev, dev_handle);
	if(retval || ((*dev_handle) == NULL))
		return retval;
	retval = read_strings(*dev_handle, &desc, manufacturer, description, SerialNumber);
	if(retval < 0) {
		close_handle_ftd2_libusb(*dev_handle, false);
		return retval;
	}
	libusb_check_and_detach_kernel_driver(*dev_handle, DEFAULT_INTERFACE);
	retval = libusb_check_and_set_configuration(*dev_handle, DEFAULT_CONFIGURATION);
	if(retval != LIBUSB_SUCCESS)
		return retval;
	libusb_check_and_detach_kernel_driver(*dev_handle, DEFAULT_INTERFACE);
	retval = libusb_claim_interface(*dev_handle, DEFAULT_INTERFACE);
	if(retval != LIBUSB_SUCCESS) {
		close_handle_ftd2_libusb(*dev_handle, false);
		return retval;
	}
	return LIBUSB_SUCCESS;
}

static int check_single_device_valid_ftd2_libusb(libusb_device* dev, char* description, char* SerialNumber, const vid_pid_descriptor** curr_descriptor) {
	libusb_device_handle *dev_handle = NULL;
	int retval = init_handle_and_populate_device_info_ftd2_libusb(&dev_handle, dev, description, SerialNumber, curr_descriptor);
	if(retval != LIBUSB_SUCCESS)
		return retval;
	close_handle_ftd2_libusb(dev_handle, true);
	return retval;
}

void list_devices_ftd2_libusb(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	char description[DESCRIPTION_SIZE], SerialNumber[SERIAL_NUMBER_SIZE];
	int debug_multiplier = 1;
	bool insert_anyway = false;
	bool perm_error = false;
	const vid_pid_descriptor* curr_descriptor;

	for(int i = 0; i < num_devices; i++) {
		int retval = check_single_device_valid_ftd2_libusb(usb_devices[i], description, SerialNumber, &curr_descriptor);
		if(retval < 0) {
			if(retval == LIBUSB_ERROR_ACCESS)
				perm_error = true;
			continue;
		}
		std::string serial_number = std::string(SerialNumber);
		bool is_already_inserted = false;
		for(int j = 0; j < devices_list.size(); j++) {
			if((devices_list[j].cc_type == CAPTURE_CONN_FTD2) && (devices_list[j].serial_number == serial_number)) {
				is_already_inserted = true;
				break;
			}
		}
		if(is_already_inserted && (!insert_anyway))
			continue;
		for(int j = 0; j < get_num_ftd2_device_types(); j++) {
			if(description == get_ftd2_fw_desc(j)) {
				for(int u = 0; u < debug_multiplier; u++)
					devices_list.emplace_back(serial_number, "DS.2", "DS.2.l565", CAPTURE_CONN_FTD2, (void*)curr_descriptor, false, false, ENABLE_AUDIO, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, get_max_samples(false), 0, 0, 0, 0, HEIGHT_DS, VIDEO_DATA_RGB16, get_ftd2_fw_index(j), false, std::string(description));
				break;
			}
		}
	}
	if(perm_error)
		no_access_list.emplace_back("ftd2_libusb");

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
}

static int ftd2_libusb_ctrl_out(ftd2_libusb_handle_data* handle, uint8_t request, uint16_t value, uint16_t index, const uint8_t* data, size_t size) {
	if(handle == NULL)
		return -1;
	if(handle->usb_handle == NULL)
		return -1;
	return libusb_control_transfer(handle->usb_handle, 0x40, request, value, index, (uint8_t*)data, size, handle->timeout_w_ms);
}

static int ftd2_libusb_ctrl_in(ftd2_libusb_handle_data* handle, uint8_t request, uint16_t value, uint16_t index, uint8_t* data, size_t size) {
	if(handle == NULL)
		return -1;
	if(handle->usb_handle == NULL)
		return -1;
	return libusb_control_transfer(handle->usb_handle, 0xC0, request, value, index, data, size, handle->timeout_r_ms);
}

static int ftd2_libusb_ctrl_out_with_index(ftd2_libusb_handle_data* handle, uint8_t request, uint16_t value, uint16_t index, uint8_t* data, size_t size) {
	if(handle == NULL)
		return -1;
	return ftd2_libusb_ctrl_out(handle, request, value, index | handle->chip_index, data, size);
}

static int ftd2_libusb_bulk_out(ftd2_libusb_handle_data* handle, const uint8_t* data, size_t size, int* transferred) {
	if(handle == NULL)
		return -1;
	if(handle->usb_handle == NULL)
		return -1;
	return libusb_bulk_transfer(handle->usb_handle, handle->ep_out, (uint8_t*)data, size, transferred, handle->timeout_w_ms);
}

static int ftd2_libusb_bulk_in(ftd2_libusb_handle_data* handle, uint8_t* data, size_t size, int* transferred) {
	if(handle == NULL)
		return -1;
	if(handle->usb_handle == NULL)
		return -1;
	return libusb_bulk_transfer(handle->usb_handle, handle->ep_in, data, size, transferred, handle->timeout_r_ms);
}

int ftd2_libusb_async_bulk_in_prepare_and_submit(void* handle, void* transfer_in, uint8_t* buffer_raw, size_t size, void* cb_fn, void* cb_data, int timeout_multiplier) {
	ftd2_libusb_handle_data* in_handle = (ftd2_libusb_handle_data*)handle;
	if(in_handle == NULL)
		return -1;
	if(in_handle->usb_handle == NULL)
		return -1;
	if(!transfer_in)
		return -1;
	libusb_transfer* transfer = (libusb_transfer*)transfer_in;
	libusb_fill_bulk_transfer(transfer, in_handle->usb_handle, in_handle->ep_in, buffer_raw, size, (libusb_transfer_cb_fn)cb_fn, cb_data, in_handle->timeout_r_ms * timeout_multiplier);
	transfer->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
	libusb_submit_transfer(transfer);
	return 0;
}

int ftd2_libusb_reset(void* handle) {
	return ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 0, 0, 0, NULL, 0);
}

int ftd2_libusb_set_latency_timer(void* handle, unsigned char latency) {
	return ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 9, latency, 0, NULL, 0);
}

int ftd2_libusb_setflowctrl(void* handle, int flowctrl, unsigned char xon, unsigned char xoff) {
	uint16_t value = 0;
	if(flowctrl == FTD2_SIO_XON_XOFF_HS)
		value = xon | (xoff << 8);
	return ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 2, value, flowctrl, NULL, 0);
}

int ftd2_libusb_set_bitmode(void* handle, unsigned char bitmask, unsigned char mode) {
	uint16_t value = bitmask | (mode << 8);
	return ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 0xB, value, 0, NULL, 0);
}

int ftd2_libusb_purge(void* handle, bool do_read, bool do_write) {
	int result = 0;
	if(do_write)
		result = ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 0, 1, 0, NULL, 0);
	if(result < 0)
		return result;
	if(do_read)
		result = ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 0, 2, 0, NULL, 0);
	if(result < 0)
		return result;
	return 0;
}

int ftd2_libusb_read_eeprom(void* handle, int eeprom_addr, int *eeprom_val) {
	uint8_t val[sizeof(uint16_t)];
	int ret = ftd2_libusb_ctrl_in((ftd2_libusb_handle_data*)handle, 0x90, 0, eeprom_addr, val, sizeof(val));
	*eeprom_val = read_le16(val, 0);
	return ret;
}

int ftd2_libusb_set_chars(void* handle, unsigned char eventch, unsigned char event_enable, unsigned char errorch, unsigned char error_enable) {
	if(event_enable)
		event_enable = 1;
	uint16_t value = eventch | (event_enable << 8);
	int ret = ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 6, value, 0, NULL, 0);
	if(ret < 0)
		return ret;
	if(error_enable)
		error_enable = 1;
	value = errorch | (error_enable << 8);
	return ftd2_libusb_ctrl_out_with_index((ftd2_libusb_handle_data*)handle, 7, value, 0, NULL, 0);
}

int ftd2_libusb_set_usb_chunksizes(void* handle, unsigned int chunksize_in, unsigned int chunksize_out) {
	return 0;
}

void ftd2_libusb_set_timeouts(void* handle, int timeout_in_ms, int timeout_out_ms) {
	if(handle == NULL)
		return;
	ftd2_libusb_handle_data* in_handle = (ftd2_libusb_handle_data*)handle;
	in_handle->timeout_r_ms = timeout_in_ms;
	in_handle->timeout_w_ms = timeout_out_ms;
}

int ftd2_libusb_write(void* handle, const uint8_t* data, size_t size, size_t* bytesOut) {
	int transferred = 0;
	int result = ftd2_libusb_bulk_out((ftd2_libusb_handle_data*)handle, data, size, &transferred);
	*bytesOut = transferred;
	return result;
}

size_t ftd2_libusb_get_expanded_length(const int max_packet_size, size_t length, size_t header_packet_size) {
	// Add the small headers every 512 bytes...
	if(length == 0)
		return header_packet_size;
	return length + ((length + (max_packet_size - header_packet_size) - 1) / (max_packet_size - header_packet_size)) * header_packet_size;
}

size_t ftd2_libusb_get_expanded_length(size_t length) {
	return ftd2_libusb_get_expanded_length(MAX_PACKET_SIZE_USB2, length, FTD2_INTRA_PACKET_HEADER_SIZE);
}

size_t ftd2_libusb_get_actual_length(const int max_packet_size, size_t length, size_t header_packet_size) {
	// Remove the small headers every 512 bytes...
	// The "- header_packet_size" instead of "-1" covers for partial header transfers...
	int num_iters = (length + max_packet_size - header_packet_size) / max_packet_size;
	if(num_iters > 0)
		length -= (num_iters * header_packet_size);
	else
		length = 0;
	return length;
}

size_t ftd2_libusb_get_actual_length(size_t length) {
	return ftd2_libusb_get_actual_length(MAX_PACKET_SIZE_USB2, length, FTD2_INTRA_PACKET_HEADER_SIZE);
}

void ftd2_libusb_copy_buffer_to_target(uint8_t* buffer_written, uint8_t* buffer_target, const int max_packet_size, size_t length, size_t header_packet_size) {
	// Remove the small headers every 512 bytes...
	// The "- header_packet_size" instead of "-1" covers for partial header transfers...
	int num_iters = (length + max_packet_size - header_packet_size) / max_packet_size;
	if(num_iters <= 0)
		return;

	length -= (num_iters * header_packet_size);
	for(int i = 0; i < num_iters; i++) {
		int rem_size = length - ((max_packet_size - header_packet_size) * i);
		if(rem_size > ((int)(max_packet_size - header_packet_size)))
			rem_size = max_packet_size - header_packet_size;
		if(rem_size <= 0)
			break;
		memcpy(buffer_target + ((max_packet_size - header_packet_size) * i), buffer_written + header_packet_size + (max_packet_size * i), rem_size);
	}
}

void ftd2_libusb_copy_buffer_to_target(uint8_t* buffer_written, uint8_t* buffer_target, size_t length) {
	return ftd2_libusb_copy_buffer_to_target(buffer_written, buffer_target, MAX_PACKET_SIZE_USB2, length, FTD2_INTRA_PACKET_HEADER_SIZE);
}

// Read from bulk
static int ftd2_libusb_direct_read(void* handle, uint8_t* buffer_raw, uint8_t* buffer_normal, size_t length, size_t* transferred) {
	length = ftd2_libusb_get_expanded_length(length);
	int transferred_internal = 0;
	int retval = ftd2_libusb_bulk_in((ftd2_libusb_handle_data*)handle, buffer_raw, length, &transferred_internal);
	if(retval < 0)
		return retval;
	ftd2_libusb_copy_buffer_to_target(buffer_raw, buffer_normal, transferred_internal);
	*transferred = ftd2_libusb_get_actual_length(transferred_internal);
	return LIBUSB_SUCCESS;
}

int ftd2_libusb_read(void* handle, uint8_t* data, size_t size, size_t* bytesIn) {
	size_t new_size = ftd2_libusb_get_expanded_length(size);
	uint8_t* buffer_raw = new uint8_t[new_size];
	int result = ftd2_libusb_direct_read(handle, buffer_raw, data, size, bytesIn);
	delete []buffer_raw;
	return result;
}

int ftd2_libusb_force_read_with_timeout(void* handle, uint8_t* buffer_raw, uint8_t* buffer_normal, size_t length, double timeout) {
	size_t total_transferred = 0;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(total_transferred < length) {
		size_t received = 0;
		int retval = ftd2_libusb_direct_read(handle, buffer_raw, buffer_normal, length - total_transferred, &received);
		if(ftd2_is_error(retval, true))
			return retval;
		total_transferred += received;
		if(received == 0)
			break;
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - start_time;
		if(diff.count() > timeout)
			return LIBUSB_ERROR_TIMEOUT;
	}
	return LIBUSB_SUCCESS;
}

int get_ftd2_libusb_read_queue_size(void* handle, size_t* bytesIn) {
	uint8_t buffer[64];
	*bytesIn = 0;
	ftd2_libusb_handle_data* in_handle = (ftd2_libusb_handle_data*)handle;
	int timeout_in_ms = in_handle->timeout_r_ms;
	in_handle->timeout_r_ms = 0;
	size_t curr_bytes_in = 0;
	bool done = false;
	int retval = 0;
	while(!done) {
		retval = ftd2_libusb_read(handle, buffer, 64, &curr_bytes_in);
		if(retval <= 0)
			done = true;
		else
			*bytesIn += curr_bytes_in;
	}
	in_handle->timeout_r_ms = timeout_in_ms;
	return LIBUSB_SUCCESS;
}

int ftd2_libusb_open_serial(CaptureDevice* device, void** handle) {
	if(!usb_is_initialized())
		return LIBUSB_ERROR_OTHER;

	*handle = NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	char description[DESCRIPTION_SIZE], SerialNumber[SERIAL_NUMBER_SIZE];
	int debug_multiplier = 1;
	bool insert_anyway = false;
	bool perm_error = false;
	const vid_pid_descriptor* curr_descriptor;
	int ret = LIBUSB_ERROR_OTHER;

	for(int i = 0; i < num_devices; i++) {
		ftd2_libusb_handle_data out_handle;
		int retval = init_handle_and_populate_device_info_ftd2_libusb(&out_handle.usb_handle, usb_devices[i], description, SerialNumber, &curr_descriptor);
		if(retval != LIBUSB_SUCCESS)
			continue;
		if(curr_descriptor != ((const vid_pid_descriptor*)device->descriptor)) {
			close_handle_ftd2_libusb(out_handle.usb_handle, true);
			continue;
		}
		std::string serial_number = std::string(SerialNumber);
		std::string desc = std::string(description);
		if((serial_number != device->serial_number) || (desc != device->path)) {
			close_handle_ftd2_libusb(out_handle.usb_handle, true);
			continue;
		}
		ftd2_libusb_handle_data* final_handle = new ftd2_libusb_handle_data;
		*final_handle = out_handle;
		*handle = final_handle;
		ret = LIBUSB_SUCCESS;
		break;
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
	return ret;
}

int ftd2_libusb_close(void* handle) {
	if(handle == NULL)
		return LIBUSB_SUCCESS;
	ftd2_libusb_handle_data* in_handle = (ftd2_libusb_handle_data*)handle;
	close_handle_ftd2_libusb(in_handle->usb_handle, true);
	delete in_handle;
	return LIBUSB_SUCCESS;
}

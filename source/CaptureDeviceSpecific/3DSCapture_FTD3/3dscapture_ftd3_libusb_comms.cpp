#include "3dscapture_ftd3_libusb_comms.hpp"
#include "3dscapture_ftd3_shared_general.hpp"
#include "devicecapture.hpp"

#include <cstring>
#include <libusb.h>
#include "usb_generic.hpp"

// This was created to remove the dependency from the FTD3XX library
// under Linux and MacOS (as well as WinUSB).
// There were issues with said library which forced the use of an
// older release (double free when a device is disconnected, for example).
// There were problems with automatic downloads being blocked as well.
// This is something which could not go on indefinitively.

// The functions present in this file represent the needs of this
// particular program at this moment in time.
// It may not include all the functions needed to replace the FTD3XX
// library for you... If that's the case, Wireshark is your friend.

#define FTD3_COMMAND_CREATE_PIPE_ID 0x82
#define FTD3_COMMAND_BULK_PIPE_ID 0x01

#define FTD3_COMMAND_TIMEOUT 500

#define FTD3_COMMAND_INTERFACE 0
#define FTD3_BULK_INTERFACE 1

#define FTD3_COMMAND_ABORT_ID 0
#define FTD3_COMMAND_RW_OPERATION_PREPARE_ID 1
#define FTD3_COMMAND_SET_STREAM_PIPE_ID 2
#define FTD3_COMMAND_CREATE_ID 3
#define FTD3_COMMAND_DESTROY_ID 3

#define FTDI_VID 0x0403

const uint16_t ftd3_valid_vids[] = {FTDI_VID};
const size_t ftd3_num_vids = sizeof(ftd3_valid_vids) / sizeof(ftd3_valid_vids[0]);
const uint16_t ftd3_valid_pids[] = {0x601e, 0x601f, 0x602a, 0x602b, 0x602c, 0x602d, 0x602f};
const size_t ftd3_num_pids = sizeof(ftd3_valid_pids) / sizeof(ftd3_valid_pids[0]);

#pragma pack(push, 1)
struct ftd3_command_preamble_data {
	uint32_t unk;
	uint8_t pipe;
	uint8_t command;
	uint16_t unk2;
} PACKED;

struct ftd3_command_with_ptr_data {
	ftd3_command_preamble_data preamble_data;
	uint64_t random_ptr;
	uint32_t unk;
} PACKED;

struct ftd3_command_with_len_data {
	ftd3_command_preamble_data preamble_data;
	uint32_t len;
	uint64_t unk;
} PACKED;
#pragma pack(pop)

// Read from ctrl_in
int ftd3_libusb_ctrl_in(ftd3_device_device_handlers* handlers, uint32_t timeout, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred) {
	int ret = libusb_control_transfer((libusb_device_handle*)handlers->usb_handle, 0xC0, request, value, index, buf, length, timeout);
	if(ret >= 0)
		*transferred = ret;
	return ret;
}

// Write to ctrl_out
int ftd3_libusb_ctrl_out(ftd3_device_device_handlers* handlers, uint32_t timeout, const uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred) {
	int ret = libusb_control_transfer((libusb_device_handle*)handlers->usb_handle, 0x40, request, value, index, (uint8_t*)buf, length, timeout);
	if(ret >= 0)
		*transferred = ret;
	return ret;
}

// Read from bulk
int ftd3_libusb_bulk_in(ftd3_device_device_handlers* handlers, int endpoint_in, uint32_t timeout, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer((libusb_device_handle*)handlers->usb_handle, endpoint_in, buf, length, transferred, timeout);
}

// Write to bulk
int ftd3_libusb_bulk_out(ftd3_device_device_handlers* handlers, int endpoint_out, uint32_t timeout, const uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer((libusb_device_handle*)handlers->usb_handle, endpoint_out, (uint8_t*)buf, length, transferred, timeout);
}

void ftd3_libusb_cancell_callback(ftd3_async_callback_data* cb_data) {
	cb_data->transfer_data_access.lock();
	if(cb_data->transfer_data)
		libusb_cancel_transfer((libusb_transfer*)cb_data->transfer_data);
	cb_data->transfer_data_access.unlock();
}

void STDCALL ftd3_libusb_async_callback(libusb_transfer* transfer) {
	ftd3_async_callback_data* cb_data = (ftd3_async_callback_data*)transfer->user_data;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = NULL;
	cb_data->transfer_data_access.unlock();
	cb_data->function(cb_data->actual_user_data, transfer->actual_length, transfer->status);
}

// Read from bulk
int ftd3_libusb_async_in_start(ftd3_device_device_handlers* handlers, int endpoint_in, uint32_t timeout, uint8_t* buf, int length, ftd3_async_callback_data* cb_data) {
	libusb_transfer *transfer_in = libusb_alloc_transfer(0);
	if(!transfer_in)
		return LIBUSB_ERROR_OTHER;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = transfer_in;
	libusb_fill_bulk_transfer(transfer_in, (libusb_device_handle*)handlers->usb_handle, endpoint_in, buf, length, ftd3_libusb_async_callback, cb_data, timeout);
	transfer_in->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
	int retval = libusb_submit_transfer(transfer_in);
	cb_data->transfer_data_access.unlock();
	return retval;
}

static int ftd3_libusb_send_command(libusb_device_handle* handle, uint8_t* data, size_t length) {
	int num_transferred = 0;
	int result = libusb_bulk_transfer(handle, FTD3_COMMAND_BULK_PIPE_ID, data, length, &num_transferred, FTD3_COMMAND_TIMEOUT);
	if(result < 0)
		return result;
	if(num_transferred != length)
		return -1;
	return result;
}

static int ftd3_libusb_send_ptr_data_command(libusb_device_handle* handle, uint8_t pipe, uint8_t command) {
	ftd3_command_with_ptr_data command_with_ptr_data;
	memset((uint8_t*)&command_with_ptr_data, 0, sizeof(ftd3_command_with_ptr_data));
	command_with_ptr_data.preamble_data.pipe = pipe;
	command_with_ptr_data.preamble_data.command = command;
	return ftd3_libusb_send_command(handle, (uint8_t*)&command_with_ptr_data, sizeof(ftd3_command_with_ptr_data));
}

static int ftd3_libusb_send_len_data_command(libusb_device_handle* handle, uint8_t pipe, uint8_t command, uint32_t length) {
	ftd3_command_with_len_data command_with_len_data;
	memset((uint8_t*)&command_with_len_data, 0, sizeof(ftd3_command_with_len_data));
	command_with_len_data.preamble_data.pipe = pipe;
	command_with_len_data.preamble_data.command = command;
	command_with_len_data.len = to_le(length);
	return ftd3_libusb_send_command(handle, (uint8_t*)&command_with_len_data, sizeof(ftd3_command_with_len_data));
}

static int ftd3_libusb_send_create_command(libusb_device_handle* handle) {
	return ftd3_libusb_send_ptr_data_command(handle, FTD3_COMMAND_CREATE_PIPE_ID, FTD3_COMMAND_CREATE_ID);
}

int ftd3_libusb_abort_pipe(ftd3_device_device_handlers* handlers, int pipe) {
	int result = ftd3_libusb_send_ptr_data_command((libusb_device_handle*)handlers->usb_handle, pipe, FTD3_COMMAND_ABORT_ID);
	if(result < 0)
		return result;
	result = ftd3_libusb_send_ptr_data_command((libusb_device_handle*)handlers->usb_handle, pipe, FTD3_COMMAND_ABORT_ID);
	if(result < 0)
		return result;
	return ftd3_libusb_send_ptr_data_command((libusb_device_handle*)handlers->usb_handle, pipe, FTD3_COMMAND_DESTROY_ID);
}

int ftd3_libusb_write_pipe(ftd3_device_device_handlers* handlers, int pipe, const uint8_t* data, size_t length, int* num_transferred) {
	int result = ftd3_libusb_send_len_data_command((libusb_device_handle*)handlers->usb_handle, pipe, FTD3_COMMAND_RW_OPERATION_PREPARE_ID, length);
	if(result < 0)
		return result;
	return ftd3_libusb_bulk_out(handlers, pipe, FTD3_COMMAND_TIMEOUT, data, length, num_transferred);
}

int ftd3_libusb_read_pipe(ftd3_device_device_handlers* handlers, int pipe, uint8_t* data, size_t length, int* num_transferred) {
	int result = ftd3_libusb_send_len_data_command((libusb_device_handle*)handlers->usb_handle, pipe, FTD3_COMMAND_RW_OPERATION_PREPARE_ID, length);
	if(result < 0)
		return result;
	return ftd3_libusb_bulk_in(handlers, pipe, FTD3_COMMAND_TIMEOUT, data, length, num_transferred);
}

int ftd3_libusb_set_stream_pipe(ftd3_device_device_handlers* handlers, int pipe, size_t length) {
	return ftd3_libusb_send_len_data_command((libusb_device_handle*)handlers->usb_handle, pipe, FTD3_COMMAND_SET_STREAM_PIPE_ID, length);
}

static bool ftd3_libusb_setup_connection(libusb_device_handle* handle, bool* claimed_cmd, bool* claimed_bulk) {
	*claimed_cmd = false;
	*claimed_bulk = false;
	int result = libusb_claim_interface(handle, FTD3_COMMAND_INTERFACE);
	if(result != LIBUSB_SUCCESS)
		return false;
	*claimed_cmd = true;
	result = libusb_claim_interface(handle, FTD3_BULK_INTERFACE);
	if(result != LIBUSB_SUCCESS)
		return false;
	*claimed_bulk = true;
	result = ftd3_libusb_send_create_command(handle);
	if(result < 0)
		return false;
	return true;
}

static void read_strings(libusb_device_handle *handle, libusb_device_descriptor *usb_descriptor, char* manufacturer, char* product, char* serial) {
	manufacturer[0] = 0;
	product[0] = 0;
	serial[0] = 0;
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iManufacturer, (uint8_t*)manufacturer, 0x100);
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iProduct, (uint8_t*)product, 0x100);
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iSerialNumber, (uint8_t*)serial, 0x100);
	manufacturer[0xFF] = 0;
	product[0xFF] = 0;
	serial[0xFF] = 0;
}

static int ftd3_libusb_insert_device(std::vector<CaptureDevice> &devices_list, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id, const std::vector<std::string> &valid_descriptions) {
	libusb_device_handle *handle = NULL;
	int result = libusb_open(usb_device, &handle);
	if((result < 0) || (handle == NULL))
		return result;
	char manufacturer[0x100];
	char product[0x100];
	char serial[0x100];
	read_strings(handle, usb_descriptor, manufacturer, product, serial);
	bool found = false;
	for(int i = 0; i < valid_descriptions.size(); i++)
		if(product == valid_descriptions[i]) {
			found = true;
			break;
		}
	if(!found) {
		libusb_close(handle);
		return result;
	}
	bool claimed_cmd = false;
	bool claimed_bulk = false;
	bool result_setup = ftd3_libusb_setup_connection(handle, &claimed_cmd, &claimed_bulk);
	uint32_t usb_speed = 0x200;
	libusb_speed read_speed = (libusb_speed)libusb_get_device_speed(usb_device);
	if((read_speed >= LIBUSB_SPEED_SUPER) || ((read_speed == LIBUSB_SPEED_UNKNOWN) && (usb_descriptor->bcdUSB >= 0x300)))
		usb_speed = 0x300;
	if(result_setup)
		ftd3_insert_device(devices_list, (std::string)(serial), curr_serial_extra_id, usb_speed, false);
	if(claimed_cmd)
		libusb_release_interface(handle, FTD3_COMMAND_INTERFACE);
	if(claimed_bulk)
		libusb_release_interface(handle, FTD3_BULK_INTERFACE);
	libusb_close(handle);
	return result;
}

static ftd3_device_device_handlers* ftd3_libusb_serial_find_in_list(libusb_device **usb_devices, int num_devices, std::string wanted_serial_number, int &curr_serial_extra_id, const std::vector<std::string> &valid_descriptions) {
	for(int i = 0; i < num_devices; i++) {
		libusb_device_descriptor usb_descriptor{};
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for(int j = 0; j < ftd3_num_vids; j++) {
			if(usb_descriptor.idVendor != ftd3_valid_vids[j])
				continue;
			for(int k = 0; k < ftd3_num_pids; k++) {
				if(usb_descriptor.idProduct != ftd3_valid_pids[k])
					continue;
				ftd3_device_device_handlers handlers;
				result = libusb_open(usb_devices[i], (libusb_device_handle**)&handlers.usb_handle);
				if((result < 0) || (handlers.usb_handle == NULL))
					continue;
				char manufacturer[0x100];
				char product[0x100];
				char serial[0x100];
				read_strings((libusb_device_handle*)handlers.usb_handle, &usb_descriptor, manufacturer, product, serial);
				for(int l = 0; l < valid_descriptions.size(); l++) {
					if(product != valid_descriptions[l])
						continue;
					std::string device_serial_number = ftd3_get_serial((std::string)(serial), curr_serial_extra_id);
					bool claimed_cmd = false;
					bool claimed_bulk = false;
					if((wanted_serial_number == device_serial_number) && (ftd3_libusb_setup_connection((libusb_device_handle*)handlers.usb_handle, &claimed_cmd, &claimed_bulk))) {
						ftd3_device_device_handlers* final_handlers = new ftd3_device_device_handlers;
						final_handlers->usb_handle = handlers.usb_handle;
						final_handlers->driver_handle = NULL;
						return final_handlers;
					}
					if(claimed_cmd)
						libusb_release_interface((libusb_device_handle*)handlers.usb_handle, FTD3_COMMAND_INTERFACE);
					if(claimed_bulk)
						libusb_release_interface((libusb_device_handle*)handlers.usb_handle, FTD3_BULK_INTERFACE);
				}
				libusb_close((libusb_device_handle*)handlers.usb_handle);
			}
		}
	}
	return NULL;
}

ftd3_device_device_handlers* ftd3_libusb_serial_reconnection(std::string wanted_serial_number, int &curr_serial_extra_id, const std::vector<std::string> &valid_descriptions) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);

	ftd3_device_device_handlers* final_handlers = ftd3_libusb_serial_find_in_list(usb_devices, num_devices, wanted_serial_number, curr_serial_extra_id, valid_descriptions);

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);

	return final_handlers;
}

void ftd3_libusb_end_connection(ftd3_device_device_handlers* handlers, bool interface_claimed) {
	if(handlers->usb_handle == NULL)
		return;
	if(interface_claimed) {
		libusb_release_interface((libusb_device_handle*)handlers->usb_handle, FTD3_COMMAND_INTERFACE);
		libusb_release_interface((libusb_device_handle*)handlers->usb_handle, FTD3_BULK_INTERFACE);
	}
	libusb_close((libusb_device_handle*)handlers->usb_handle);
	handlers->usb_handle = NULL;
}

void ftd3_libusb_list_devices(std::vector<CaptureDevice> &devices_list, bool* no_access_elems, bool* not_supported_elems, int *curr_serial_extra_id_ftd3, const std::vector<std::string> &valid_descriptions) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for(int j = 0; j < ftd3_num_vids; j++) {
			if(usb_descriptor.idVendor != ftd3_valid_vids[j])
				continue;
			for(int k = 0; k < ftd3_num_pids; k++) {
				if(usb_descriptor.idProduct != ftd3_valid_pids[k])
					continue;
				result = ftd3_libusb_insert_device(devices_list, usb_devices[i], &usb_descriptor, *curr_serial_extra_id_ftd3, valid_descriptions);
				// Apparently this is how it fails if FTD3XX is the driver on Windows...
				if(result == LIBUSB_ERROR_NOT_FOUND)
					*not_supported_elems = true;
				if(result == LIBUSB_ERROR_ACCESS)
					*no_access_elems = true;
				if(result == LIBUSB_ERROR_NOT_SUPPORTED)
					*not_supported_elems = true;
				break;
			}
		}
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
}

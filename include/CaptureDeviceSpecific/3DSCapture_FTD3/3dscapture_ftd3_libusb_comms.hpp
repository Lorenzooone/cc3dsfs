#ifndef __3DSCAPTURE_FTD3_LIBUSB_COMMS_HPP
#define __3DSCAPTURE_FTD3_LIBUSB_COMMS_HPP

#include <vector>
#include "capture_structs.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3_compatibility.hpp"

typedef void (*ftd3_async_callback_function)(void* user_data, int transfer_length, int transfer_status);

struct ftd3_async_callback_data {
	ftd3_async_callback_function function;
	void* actual_user_data;
	void* transfer_data;
	std::mutex transfer_data_access;
};

int ftd3_libusb_ctrl_in(ftd3_device_device_handlers* handlers, uint32_t timeout, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred);
int ftd3_libusb_ctrl_out(ftd3_device_device_handlers* handlers, uint32_t timeout, const uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred);
int ftd3_libusb_bulk_in(ftd3_device_device_handlers* handlers, int endpoint_in, uint32_t timeout, uint8_t* buf, int length, int* transferred);
int ftd3_libusb_bulk_out(ftd3_device_device_handlers* handlers, int endpoint_out, uint32_t timeout, const uint8_t* buf, int length, int* transferred);
int ftd3_libusb_async_in_start(ftd3_device_device_handlers* handlers, int endpoint_in, uint32_t timeout, uint8_t* buf, int length, ftd3_async_callback_data* cb_data);
void ftd3_libusb_cancell_callback(ftd3_async_callback_data* cb_data);

void ftd3_libusb_list_devices(std::vector<CaptureDevice> &devices_list, bool* no_access_elems, bool* not_supported_elems, int *curr_serial_extra_id_ftd3, const std::vector<std::string> &valid_descriptions);
ftd3_device_device_handlers* ftd3_libusb_serial_reconnection(std::string wanted_serial_number, int &curr_serial_extra_id, const std::vector<std::string> &valid_descriptions);

void ftd3_libusb_end_connection(ftd3_device_device_handlers* handlers, bool interface_claimed);
int ftd3_libusb_abort_pipe(ftd3_device_device_handlers* handlers, int pipe);
int ftd3_libusb_write_pipe(ftd3_device_device_handlers* handlers, int pipe, const uint8_t* data, size_t length, int* num_transferred);
int ftd3_libusb_read_pipe(ftd3_device_device_handlers* handlers, int pipe, uint8_t* data, size_t length, int* num_transferred);
int ftd3_libusb_set_stream_pipe(ftd3_device_device_handlers* handlers, int pipe, size_t length);

#endif

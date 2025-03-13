#ifndef __3DSCAPTURE_FTD3_COMPATIBILITY_HPP
#define __3DSCAPTURE_FTD3_COMPATIBILITY_HPP

#include "capture_structs.hpp"
#include "devicecapture.hpp"

#define FTD3_CONCURRENT_BUFFERS NUM_CONCURRENT_DATA_BUFFER_WRITERS

struct ftd3_device_device_handlers {
	void* usb_handle = NULL;
	void* driver_handle = NULL;
};

void ftd3_list_devices_compat(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, const std::vector<std::string> &valid_descriptions);
ftd3_device_device_handlers* ftd3_reconnect_compat(std::string wanted_serial_number, const std::vector<std::string> &valid_descriptions);
void ftd3_close_compat(ftd3_device_device_handlers* handlers);
bool ftd3_is_error_compat(ftd3_device_device_handlers* handlers, int error_num);
int ftd3_abort_pipe_compat(ftd3_device_device_handlers* handlers, int pipe);
int ftd3_set_stream_pipe_compat(ftd3_device_device_handlers* handlers, int pipe, size_t length);
int ftd3_write_pipe_compat(ftd3_device_device_handlers* handlers, int pipe, const uint8_t* data, size_t size, int* num_transferred);
int ftd3_read_pipe_compat(ftd3_device_device_handlers* handlers, int pipe, uint8_t* data, size_t size, int* num_transferred);
bool ftd3_get_is_bad_compat(ftd3_device_device_handlers* handlers);
bool ftd3_get_skip_initial_pipe_abort_compat(ftd3_device_device_handlers* handlers);
void ftd3_main_loop_compat(CaptureData* capture_data, int pipe);

#endif

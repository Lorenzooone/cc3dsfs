#include "3dscapture_ftd3_compatibility.hpp"
#include "3dscapture_ftd3_libusb_comms.hpp"
#include "3dscapture_ftd3_driver_comms.hpp"
#include "3dscapture_ftd3_driver_acquisition.hpp"
#include "3dscapture_ftd3_libusb_acquisition.hpp"
#include "devicecapture.hpp"

#ifdef USE_FTD3XX
//#include "ftd3xx_symbols_renames.h"
#ifdef _WIN32
#define FTD3XX_STATIC
#endif
#include <ftd3xx.h>
#endif

#ifdef USE_FTD3_LIBUSB
#define CHECK_DRIVER_VALUE false
#else
#define CHECK_DRIVER_VALUE true
#endif

void ftd3_list_devices_compat(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, const std::vector<std::string> &valid_descriptions) {
	bool no_access_elems = false;
	bool not_supported_elems = false;
	int curr_serial_extra_id_ftd3 = 0;
	#ifdef USE_FTD3_LIBUSB
	ftd3_libusb_list_devices(devices_list, &no_access_elems, &not_supported_elems, &curr_serial_extra_id_ftd3, valid_descriptions);
	#endif
	#ifdef USE_FTD3XX
	if(CHECK_DRIVER_VALUE || not_supported_elems)
		ftd3_driver_list_devices(devices_list, &curr_serial_extra_id_ftd3, valid_descriptions);
	#endif
	if(no_access_elems)
		no_access_list.emplace_back("FTD3");
}

ftd3_device_device_handlers* ftd3_reconnect_compat(std::string wanted_serial_number, const std::vector<std::string> &valid_descriptions) {
	int curr_serial_extra_id_ftd3 = 0;
	ftd3_device_device_handlers* final_handlers = NULL;
	#ifdef USE_FTD3_LIBUSB
	final_handlers = ftd3_libusb_serial_reconnection(wanted_serial_number, curr_serial_extra_id_ftd3, valid_descriptions);
	if(final_handlers != NULL)
		return final_handlers;
	#endif
	#ifdef USE_FTD3XX
	final_handlers = ftd3_driver_serial_reconnection(wanted_serial_number);
	if(final_handlers != NULL)
		return final_handlers;
	#endif
	return final_handlers;
}

bool ftd3_is_error_compat(ftd3_device_device_handlers* handlers, int error_num) {
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		return error_num < 0;
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle)
		return FT_FAILED(error_num);
	#endif
	return true;
}

void ftd3_close_compat(ftd3_device_device_handlers* handlers) {
	#ifdef USE_FTD3_LIBUSB
	ftd3_libusb_end_connection(handlers, true);
	#endif
	#ifdef USE_FTD3XX
	ftd3_driver_end_connection(handlers);
	#endif
}

int ftd3_abort_pipe_compat(ftd3_device_device_handlers* handlers, int pipe) {
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		return ftd3_libusb_abort_pipe(handlers, pipe);
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle)
		return FT_AbortPipe(handlers->driver_handle, pipe);
	#endif
	return 0;
}

int ftd3_set_stream_pipe_compat(ftd3_device_device_handlers* handlers, int pipe, size_t length) {
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		return ftd3_libusb_set_stream_pipe(handlers, pipe, length);
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle)
		return FT_SetStreamPipe(handlers->driver_handle, false, false, pipe, (ULONG)length);
	#endif
	return 0;
}

int ftd3_write_pipe_compat(ftd3_device_device_handlers* handlers, int pipe, const uint8_t* data, size_t size, int* num_transferred) {
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		return ftd3_libusb_write_pipe(handlers, pipe, data, size, num_transferred);
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle) {
		ULONG transferred_ftd3xx = 0;
		int result = FT_WritePipe(handlers->driver_handle, pipe, (uint8_t*)data, (ULONG)size, &transferred_ftd3xx, 0);
		if(FT_FAILED(result))
			return result;
		*num_transferred = (int)transferred_ftd3xx;
		return result;
	}
	#endif
	return 0;
}

int ftd3_read_pipe_compat(ftd3_device_device_handlers* handlers, int pipe, uint8_t* data, size_t size, int* num_transferred) {
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		return ftd3_libusb_read_pipe(handlers, pipe, data, size, num_transferred);
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle) {
		ULONG transferred_ftd3xx = 0;
		int result = FT_ReadPipe(handlers->driver_handle, pipe, data, (ULONG)size, &transferred_ftd3xx, 0);
		if(FT_FAILED(result))
			return result;
		*num_transferred = (int)transferred_ftd3xx;
		return result;
	}
	#endif
	return 0;
}

bool ftd3_get_is_bad_compat(ftd3_device_device_handlers* handlers) {
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		return false;
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle)
		return ftd3_driver_get_is_bad();
	#endif
	return false;
}

bool ftd3_get_skip_initial_pipe_abort_compat(ftd3_device_device_handlers* handlers) {
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		return false;
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle)
		return ftd3_driver_get_skip_initial_pipe_abort();
	#endif
	return false;
}

void ftd3_main_loop_compat(CaptureData* capture_data, int pipe) {
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;
	#ifdef USE_FTD3_LIBUSB
	if(handlers->usb_handle)
		ftd3_libusb_capture_main_loop(capture_data, pipe);
	#endif
	#ifdef USE_FTD3XX
	if(handlers->driver_handle)
		ftd3_driver_capture_main_loop(capture_data, pipe);
	#endif
}

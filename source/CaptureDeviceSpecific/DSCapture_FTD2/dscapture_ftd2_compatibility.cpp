#include "dscapture_ftd2_libusb_acquisition.hpp"
#include "dscapture_ftd2_libusb_comms.hpp"
#include "dscapture_ftd2_driver_acquisition.hpp"
#include "dscapture_ftd2_driver_comms.hpp"
#include "devicecapture.hpp"
#include "dscapture_ftd2_compatibility.hpp"

#ifdef USE_FTD2_DRIVER
#define FTD2XX_STATIC
#include "ftd2xx_symbols_renames.h"
#include <ftd2xx.h>
#endif

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define FT_FAILED(x) ((x) != FT_OK)

void list_devices_ftd2_compatibility(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	#ifdef USE_FTD2_DRIVER
	list_devices_ftd2_driver(devices_list, no_access_list);
	#endif
	#ifdef USE_FTD2_LIBUSB
	list_devices_ftd2_libusb(devices_list, no_access_list);
	#endif
}

void ftd2_capture_main_loop(CaptureData* capture_data) {
	#ifdef USE_FTD2_DRIVER
	bool is_ftd2_libusb = capture_data->status.device.descriptor != NULL;
	if(!is_ftd2_libusb)
		return ftd2_capture_main_loop_driver(capture_data);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_capture_main_loop_libusb(capture_data);
	#endif
}

int ftd2_get_queue_status(void* handle, bool is_ftd2_libusb, size_t* bytes_in) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb) {
		DWORD bytesIn = 0;
		FT_STATUS ftStatus = FT_GetQueueStatus(handle, &bytesIn);
		*bytes_in = bytesIn;
		return ftStatus;
	}
	#endif
	#ifdef USE_FTD2_LIBUSB
	return get_ftd2_libusb_read_queue_size(handle, bytes_in);
	#else
	return FT_OK;
	#endif
}

bool ftd2_is_error(int value, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_FAILED(value);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return value < 0;
	#else
	return FT_OK;
	#endif
}

int ftd2_write(void* handle, bool is_ftd2_libusb, const uint8_t* data, size_t size, size_t *sent) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb) {
		DWORD inner_sent = 0;
		FT_STATUS ftStatus = FT_Write(handle, (void*)data, (DWORD)size, &inner_sent);
		*sent = inner_sent;
		return ftStatus;
	}
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_write(handle, data, size, sent);
	#else
	return FT_OK;
	#endif
}

int ftd2_read(void* handle, bool is_ftd2_libusb, uint8_t* data, size_t size, size_t *bytesIn) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb) {
		DWORD inner_bytes_in = 0;
		FT_STATUS ftStatus = FT_Read(handle, (void*)data, (DWORD)size, &inner_bytes_in);
		*bytesIn = inner_bytes_in;
		return ftStatus;
	}
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_read(handle, data, size, bytesIn);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_timeouts(void* handle, bool is_ftd2_libusb, int timeout_read, int timeout_write) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetTimeouts(handle, timeout_read, timeout_write);
	#endif
	#ifdef USE_FTD2_LIBUSB
	ftd2_libusb_set_timeouts(handle, timeout_read, timeout_write);
	return 0;
	#else
	return FT_OK;
	#endif
}

int ftd2_reset_device(void* handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_ResetDevice(handle);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_reset(handle);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_usb_parameters(void* handle, bool is_ftd2_libusb, size_t size_in, size_t size_out) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetUSBParameters(handle, (ULONG)size_in, (ULONG)size_out);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_set_usb_chunksizes(handle, (unsigned int)size_in, (unsigned int)size_out);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_chars(void* handle, bool is_ftd2_libusb, unsigned char eventch, unsigned char event_enable, unsigned char errorch, unsigned char error_enable) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetChars(handle, eventch, event_enable, errorch, error_enable);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_set_chars(handle, eventch, event_enable, errorch, error_enable);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_latency_timer(void* handle, bool is_ftd2_libusb, unsigned char latency) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetLatencyTimer(handle, latency);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_set_latency_timer(handle, latency);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_flow_ctrl_rts_cts(void* handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetFlowControl(handle, FT_FLOW_RTS_CTS, 0, 0);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_setflowctrl(handle, FTD2_SIO_RTS_CTS_HS, 0, 0);
	#else
	return FT_OK;
	#endif
}

int ftd2_reset_bitmode(void* handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetBitMode(handle, 0x00, FT_BITMODE_RESET);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_set_bitmode(handle, 0x00, FTD2_MPSSE_BITMODE_RESET);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_mpsse_bitmode(void* handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetBitMode(handle, 0x00, FT_BITMODE_MPSSE);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_set_bitmode(handle, 0x00, FTD2_MPSSE_BITMODE_MPSSE);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_fifo_bitmode(void* handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_SetBitMode(handle, 0x00, FT_BITMODE_SYNC_FIFO);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_set_bitmode(handle, 0x00, FTD2_MPSSE_BITMODE_SYNCFIFO);
	#else
	return FT_OK;
	#endif
}

int ftd2_purge_all(void* handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_purge(handle, true, true);
	#else
	return FT_OK;
	#endif
}

int ftd2_read_ee(void* handle, bool is_ftd2_libusb, int eeprom_addr, int *eeprom_val) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb) {
		WORD val = 0;
		int retval = FT_ReadEE(handle, eeprom_addr, &val);
		*eeprom_val = val;
		return retval;
	}
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_read_eeprom(handle, eeprom_addr, eeprom_val);
	#else
	return FT_OK;
	#endif
}

int ftd2_close(void* handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return FT_Close(handle);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_close(handle);
	#else
	return FT_OK;
	#endif
}

int ftd2_open(CaptureDevice* device, void** handle, bool is_ftd2_libusb) {
	#ifdef USE_FTD2_DRIVER
	if(!is_ftd2_libusb)
		return ftd2_driver_open_serial(device, handle);
	#endif
	#ifdef USE_FTD2_LIBUSB
	return ftd2_libusb_open_serial(device, handle);
	#else
	return FT_OTHER_ERROR;
	#endif
}

void ftd2_init() {
	#ifdef USE_FTD2_LIBUSB
	ftd2_libusb_init();
	#endif
}

void ftd2_end() {
	#ifdef USE_FTD2_LIBUSB
	ftd2_libusb_end();
	#endif
}

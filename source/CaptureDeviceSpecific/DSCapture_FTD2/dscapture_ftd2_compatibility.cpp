#include "dscapture_libftdi2.hpp"
#include "dscapture_ftd2_driver.hpp"
#include "devicecapture.hpp"
#include "dscapture_ftd2_compatibility.hpp"

#ifdef USE_FTD2_LIBFTDI
#include <ftdi.h>
#endif
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
	#ifdef USE_FTD2_LIBFTDI
	list_devices_libftdi(devices_list, no_access_list);
	#endif
}

void ftd2_capture_main_loop(CaptureData* capture_data) {
	bool is_libftdi = capture_data->status.device.descriptor != NULL;
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return ftd2_capture_main_loop_driver(capture_data);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return ftd2_capture_main_loop_libftdi(capture_data);
	#endif
}

int ftd2_get_queue_status(void* handle, bool is_libftdi, size_t* bytes_in) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi) {
		DWORD bytesIn = 0;
		FT_STATUS ftStatus = FT_GetQueueStatus(handle, &bytesIn);
		*bytes_in = bytesIn;
		return ftStatus;
	}
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return get_libftdi_read_queue_size(handle, bytes_in);
	#else
	return FT_OK;
	#endif
}

bool ftd2_is_error(int value, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_FAILED(value);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return value < 0;
	#else
	return FT_OK;
	#endif
}

int ftd2_write(void* handle, bool is_libftdi, const uint8_t* data, size_t size, size_t *sent) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi) {
		DWORD inner_sent = 0;
		FT_STATUS ftStatus = FT_Write(handle, (void*)data, size, &inner_sent);
		*sent = inner_sent;
		return ftStatus;
	}
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_write(handle, data, size, sent);
	#else
	return FT_OK;
	#endif
}

int ftd2_read(void* handle, bool is_libftdi, uint8_t* data, size_t size, size_t *bytesIn) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi) {
		DWORD inner_bytes_in = 0;
		FT_STATUS ftStatus = FT_Read(handle, (void*)data, size, &inner_bytes_in);
		*bytesIn = inner_bytes_in;
		return ftStatus;
	}
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_read(handle, data, size, bytesIn);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_timeouts(void* handle, bool is_libftdi, int timeout_read, int timeout_write) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetTimeouts(handle, timeout_read, timeout_write);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	libftdi_set_timeouts(handle, timeout_read, timeout_write);
	return 0;
	#else
	return FT_OK;
	#endif
}

int ftd2_reset_device(void* handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_ResetDevice(handle);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_reset(handle);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_usb_parameters(void* handle, bool is_libftdi, size_t size_in, size_t size_out) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetUSBParameters(handle, size_in, size_out);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_set_usb_chunksizes(handle, size_in, size_out);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_chars(void* handle, bool is_libftdi, unsigned char eventch, unsigned char event_enable, unsigned char errorch, unsigned char error_enable) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetChars(handle, eventch, event_enable, errorch, error_enable);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_set_chars(handle, eventch, event_enable, errorch, error_enable);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_latency_timer(void* handle, bool is_libftdi, unsigned char latency) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetLatencyTimer(handle, latency);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_set_latency_timer(handle, latency);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_flow_ctrl_rts_cts(void* handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetFlowControl(handle, FT_FLOW_RTS_CTS, 0, 0);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_setflowctrl(handle, SIO_RTS_CTS_HS, 0, 0);
	#else
	return FT_OK;
	#endif
}

int ftd2_reset_bitmode(void* handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetBitMode(handle, 0x00, FT_BITMODE_RESET);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_set_bitmode(handle, 0x00, BITMODE_RESET);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_mpsse_bitmode(void* handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetBitMode(handle, 0x00, FT_BITMODE_MPSSE);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_set_bitmode(handle, 0x00, BITMODE_MPSSE);
	#else
	return FT_OK;
	#endif
}

int ftd2_set_fifo_bitmode(void* handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_SetBitMode(handle, 0x00, FT_BITMODE_SYNC_FIFO);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_set_bitmode(handle, 0x00, BITMODE_SYNCFF);
	#else
	return FT_OK;
	#endif
}

int ftd2_purge_all(void* handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_purge(handle, true, true);
	#else
	return FT_OK;
	#endif
}

int ftd2_read_ee(void* handle, bool is_libftdi, int eeprom_addr, int *eeprom_val) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi) {
		WORD val = 0;
		int retval = FT_ReadEE(handle, eeprom_addr, &val);
		*eeprom_val = val;
		return retval;
	}
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_read_eeprom(handle, eeprom_addr, eeprom_val);
	#else
	return FT_OK;
	#endif
}

int ftd2_close(void* handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return FT_Close(handle);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_close(handle);
	#else
	return FT_OK;
	#endif
}

int ftd2_open(CaptureDevice* device, void** handle, bool is_libftdi) {
	#ifdef USE_FTD2_DRIVER
	if(!is_libftdi)
		return ftd2_driver_open_serial(device, handle);
	#endif
	#ifdef USE_FTD2_LIBFTDI
	return libftdi_open_serial(device, handle);
	#else
	return FT_OTHER_ERROR;
	#endif
}

void ftd2_init() {
	#ifdef USE_FTD2_LIBFTDI
	libftdi_init();
	#endif
}

void ftd2_end() {
	#ifdef USE_FTD2_LIBFTDI
	libftdi_end();
	#endif
}

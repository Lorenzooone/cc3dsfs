#include "3dscapture_ftd3_driver_comms.hpp"
#include "3dscapture_ftd3_shared_general.hpp"
#include "devicecapture.hpp"
//#include "ftd3xx_symbols_renames.h"

#ifdef _WIN32
#define FTD3XX_STATIC
#endif
#include <ftd3xx.h>

#include <cstring>
#include <thread>
#include <chrono>

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

static bool _ftd3_driver_get_is_bad();
static bool _ftd3_driver_get_skip_initial_pipe_abort();

const bool is_bad_ftd3xx = _ftd3_driver_get_is_bad();
const bool skip_initial_pipe_abort = _ftd3_driver_get_skip_initial_pipe_abort();

static bool _ftd3_driver_get_is_bad() {
	#ifdef _WIN32
	#ifdef _M_ARM64
	return true;
	#else
	return false;
	#endif
	#endif
	// For some reason, the version scheme seems broken...
	// For now, just return false... :/
	return false;

	bool is_bad_ftd3xx = false;
	DWORD ftd3xx_lib_version;

	if(FT_FAILED(FT_GetLibraryVersion(&ftd3xx_lib_version))) {
		ftd3xx_lib_version = 0;
	}
	if(ftd3xx_lib_version >= 0x0100001A) {
		is_bad_ftd3xx = true;
	}
	return is_bad_ftd3xx;
}

static bool _ftd3_driver_get_skip_initial_pipe_abort() {
	#ifdef _WIN32
	return false;
	#endif

	bool skip_initial_pipe_abort = false;
	DWORD ftd3xx_lib_version;

	if(FT_FAILED(FT_GetLibraryVersion(&ftd3xx_lib_version))) {
		ftd3xx_lib_version = 0;
	}
	if(ftd3xx_lib_version >= 0x0100001A) {
		skip_initial_pipe_abort = true;
	}
	return skip_initial_pipe_abort;
}

bool ftd3_driver_get_is_bad() {
	return is_bad_ftd3xx;
}

bool ftd3_driver_get_skip_initial_pipe_abort() {
	return skip_initial_pipe_abort;
}

void ftd3_driver_list_devices(std::vector<CaptureDevice> &devices_list, int *curr_serial_extra_id_ftd3, const std::vector<std::string> &valid_descriptions) {
	FT_STATUS ftStatus;
	DWORD numDevs = 0;
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	size_t num_inserted = 0;
	if(FT_FAILED(ftStatus) || (numDevs == 0))
		return;

	FT_HANDLE ftHandle = NULL;
	DWORD Flags = 0;
	DWORD Type = 0;
	DWORD ID = 0;
	char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
	char Description[65] = { 0 };
	for (DWORD i = 0; i < numDevs; i++)
	{
		ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, NULL,
		SerialNumber, Description, &ftHandle);
		if(FT_FAILED(ftStatus))
			continue;
		for(int j = 0; j < valid_descriptions.size(); j++) {
			if(Description == valid_descriptions[j]) {
				ftStatus = FT_Create(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
				if(FT_FAILED(ftStatus))
					break;
				ftd3_insert_device(devices_list, std::string(SerialNumber), *curr_serial_extra_id_ftd3, true);
				FT_Close(ftHandle);
				break;
			}
		}
	}
}

ftd3_device_device_handlers* ftd3_driver_serial_reconnection(std::string wanted_serial_number) {
	ftd3_device_device_handlers* final_handlers = NULL;
	ftd3_device_device_handlers inside_handlers;
	char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
	strncpy(SerialNumber, wanted_serial_number.c_str(), REAL_SERIAL_NUMBER_SIZE);
	SerialNumber[REAL_SERIAL_NUMBER_SIZE] = 0;
	if(!FT_FAILED(FT_Create(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &inside_handlers.driver_handle))) {
		final_handlers = new ftd3_device_device_handlers;
		final_handlers->usb_handle = NULL;
		final_handlers->driver_handle = inside_handlers.driver_handle;
	}
	return final_handlers;
}

void ftd3_driver_end_connection(ftd3_device_device_handlers* handlers) {
	if(handlers->driver_handle == NULL)
		return;
	FT_Close(handlers->driver_handle);
	handlers->driver_handle = NULL;
}

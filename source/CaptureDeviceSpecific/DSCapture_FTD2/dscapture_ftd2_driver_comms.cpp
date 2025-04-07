#include "dscapture_ftd2_driver_comms.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"
#include "dscapture_ftd2_general.hpp"
#include "dscapture_ftd2_compatibility.hpp"

#include "ftd2xx_symbols_renames.h"
#define FTD2XX_STATIC
#include <ftd2xx.h>

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define FT_FAILED(x) ((x) != FT_OK)

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

void list_devices_ftd2_driver(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	FT_STATUS ftStatus;
	DWORD numDevs = 0;
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	size_t num_inserted = 0;
	if (!FT_FAILED(ftStatus) && numDevs > 0)
	{
		const int debug_multiplier = 1;
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
			if((!FT_FAILED(ftStatus)) && (Flags & FT_FLAGS_HISPEED) && (Type == FT_DEVICE_232H))
				insert_device_ftd2_shared(devices_list, Description, debug_multiplier, std::string(SerialNumber), NULL, "", "d");
		}
	}
}

int ftd2_driver_open_serial(CaptureDevice* device, void** handle) {
	char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
	strncpy_s(SerialNumber, SERIAL_NUMBER_SIZE, device->serial_number.c_str(), SERIAL_NUMBER_SIZE);
	SerialNumber[REAL_SERIAL_NUMBER_SIZE] = 0;
	return FT_OpenEx(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, handle);
}

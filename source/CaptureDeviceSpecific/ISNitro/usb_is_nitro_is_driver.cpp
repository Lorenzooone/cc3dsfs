#include "usb_is_nitro_communications.hpp"
#include "usb_is_nitro_setup_general.hpp"
#include "usb_is_nitro_is_driver.hpp"

#include <algorithm>
#include <filesystem>
#ifdef _WIN32
#include <setupapi.h>

// Code based off of Gericom's sample code. Distributed under the MIT License. Copyright (c) 2024 Gericom
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

const GUID is_nitro_driver_guid = { .Data1 = 0xB78D7ADA, .Data2 = 0xDDF4, .Data3 = 0x418F, .Data4 = {0x8C, 0x7C, 0x4A, 0xC8, 0x80, 0x30, 0xF5, 0x42} };

static bool read_is_driver_device_info(HANDLE handle, uint8_t* buffer, size_t size, DWORD* num_read) {
	return DeviceIoControl(handle, 0x22000C, buffer, size, buffer, size, num_read, NULL);
}

static bool is_driver_device_reset(HANDLE handle) {
	DWORD num_read;
	return DeviceIoControl(handle, 0x220004, NULL, 0, NULL, 0, &num_read, NULL);
}

static bool is_driver_pipe_reset(HANDLE handle) {
	DWORD num_read;
	return DeviceIoControl(handle, 0x220008, NULL, 0, NULL, 0, &num_read, NULL);
}

static std::string is_driver_get_device_path(HDEVINFO DeviceInfoSet, SP_DEVICE_INTERFACE_DATA* DeviceInterfaceData) {
	std::string result = "";
	// Call this with an empty buffer to the required size of the structure.
	ULONG requiredSize;
	SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, DeviceInterfaceData, NULL, 0, &requiredSize, NULL);
	if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return result;

	PSP_DEVICE_INTERFACE_DETAIL_DATA devInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)(new uint8_t[requiredSize]);
	if(!devInterfaceDetailData)
		return result;

	devInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	if(SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, DeviceInterfaceData, devInterfaceDetailData, requiredSize, &requiredSize, NULL))
		result = (std::string)(devInterfaceDetailData->DevicePath);

	delete []((uint8_t*)devInterfaceDetailData);
	return result;
}

static bool is_driver_get_device_pid_vid(std::string path, uint16_t& out_vid, uint16_t& out_pid) {
	HANDLE handle = CreateFile(path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	uint8_t buffer[0x412];
	memset(buffer, 0, sizeof(buffer));
	DWORD num_read = 0;
	bool result = read_is_driver_device_info(handle, buffer, sizeof(buffer), &num_read);
	CloseHandle(handle);
	if(!result)
		return false;
	if (num_read < 11)
		return false;
	out_vid = buffer[8] + (buffer[9] << 8);
	out_pid = buffer[10] + (buffer[11] << 8);
	return true;
}

static void set_handle_timeout(HANDLE handle, int timeout) {
	COMMTIMEOUTS timeouts;
	if (GetCommTimeouts(handle, &timeouts)) {
		timeouts.ReadTotalTimeoutConstant = timeout;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		SetCommTimeouts(handle, &timeouts);
	}
}

static bool is_driver_setup_connection(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, std::string path) {
	handlers->usb_handle = NULL;
	handlers->mutex = NULL;
	handlers->write_handle = INVALID_HANDLE_VALUE;
	handlers->read_handle = INVALID_HANDLE_VALUE;
	std::string mutex_name = "Global\\ISU_" + path.substr(4);
	std::replace(mutex_name.begin(), mutex_name.end(), '\\', '@');
	handlers->mutex = CreateMutex(NULL, true, mutex_name.c_str());
	if ((handlers->mutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS)) {
		CloseHandle(handlers->mutex);
		handlers->mutex = NULL;
	}
	if (handlers->mutex == NULL)
		return false;
	handlers->write_handle = CreateFile((path + (char)(std::filesystem::path::preferred_separator)+"Pipe00").c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	handlers->read_handle = CreateFile((path + (char)(std::filesystem::path::preferred_separator)+"Pipe01").c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	if ((handlers->write_handle == INVALID_HANDLE_VALUE) || (handlers->read_handle == INVALID_HANDLE_VALUE))
		return false;
	if (!is_driver_device_reset(handlers->write_handle))
		return false;
	if (!is_driver_pipe_reset(handlers->write_handle))
		return false;
	if (!is_driver_pipe_reset(handlers->read_handle))
		return false;
	set_handle_timeout(handlers->read_handle, usb_device_desc->bulk_timeout);
	set_handle_timeout(handlers->write_handle, usb_device_desc->bulk_timeout);
	return true;
}
#endif

is_nitro_device_handlers* is_driver_serial_reconnection(const is_nitro_usb_device* usb_device_desc, CaptureDevice* device) {
	is_nitro_device_handlers* final_handlers = NULL;
	#ifdef _WIN32
	if (device->path != "") {
		is_nitro_device_handlers handlers;
		if (is_driver_setup_connection(&handlers, usb_device_desc, device->path)) {
			final_handlers = new is_nitro_device_handlers;
			final_handlers->usb_handle = NULL;
			final_handlers->mutex = handlers.mutex;
			final_handlers->read_handle = handlers.read_handle;
			final_handlers->write_handle = handlers.write_handle;
		}
		else
			is_driver_end_connection(&handlers);
	}
	#endif
	return final_handlers;
}

void is_driver_end_connection(is_nitro_device_handlers* handlers) {
	#ifdef _WIN32
	if (handlers->write_handle != INVALID_HANDLE_VALUE)
		CloseHandle(handlers->write_handle);
	handlers->write_handle = INVALID_HANDLE_VALUE;
	if (handlers->read_handle != INVALID_HANDLE_VALUE)
		CloseHandle(handlers->read_handle);
	handlers->read_handle = INVALID_HANDLE_VALUE;
	if(handlers->mutex != NULL)
		CloseHandle(handlers->mutex);
	handlers->mutex = NULL;
	#endif
}

void is_driver_list_devices(std::vector<CaptureDevice> &devices_list, bool* not_supported_elems, int *curr_serial_extra_id_is_nitro, const size_t num_is_nitro_desc) {
	#ifdef _WIN32
	HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
		&is_nitro_driver_guid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	uint32_t i = 0;
	while (SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &is_nitro_driver_guid, i++, &DeviceInterfaceData)) {
		std::string path = is_driver_get_device_path(DeviceInfoSet, &DeviceInterfaceData);
		if (path == "")
			continue;
		uint16_t vid = 0;
		uint16_t pid = 0;
		if(!is_driver_get_device_pid_vid(path, vid, pid))
			continue;
		for (int j = 0; j < num_is_nitro_desc; j++) {
			const is_nitro_usb_device* usb_device_desc = GetISNitroDesc(j);
			if(not_supported_elems[j] && (usb_device_desc->vid == vid) && (usb_device_desc->pid == pid)) {
				is_nitro_device_handlers handlers;
				if(is_driver_setup_connection(&handlers, usb_device_desc, path))
					is_nitro_insert_device(devices_list, &handlers, usb_device_desc, curr_serial_extra_id_is_nitro[j], path);
				is_driver_end_connection(&handlers);
				break;
			}
		}
	}

	if (DeviceInfoSet) {
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	}
	#endif
}

// Write to bulk
int is_driver_bulk_out(is_nitro_device_handlers* handlers, uint8_t* buf, int length, int* transferred) {
	bool result = true;
	#ifdef _WIN32
	DWORD num_bytes = 0;
	result = WriteFile(handlers->write_handle, buf, length, &num_bytes, NULL);
	*transferred = num_bytes;
	#endif
	return result ? LIBUSB_SUCCESS : LIBUSB_ERROR_OTHER;
}

// Read from bulk
int is_driver_bulk_in(is_nitro_device_handlers* handlers, uint8_t* buf, int length, int* transferred) {
	bool result = true;
	#ifdef _WIN32
	DWORD num_bytes = 0;
	result = ReadFile(handlers->read_handle, buf, length, &num_bytes, NULL);
	*transferred = num_bytes;
	#endif
	return result ? LIBUSB_SUCCESS : LIBUSB_ERROR_OTHER;
}
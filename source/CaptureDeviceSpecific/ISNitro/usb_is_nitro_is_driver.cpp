#include "usb_is_nitro_communications.hpp"
#include "usb_is_nitro_setup_general.hpp"
#include "usb_is_nitro_is_driver.hpp"
#include "usb_is_nitro_acquisition_general.hpp"
#include "frontend.hpp"
#include "utils.hpp"

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

static void is_nitro_is_driver_function(bool* usb_thread_run, ISNitroCaptureReceivedData* is_nitro_capture_recv_data, is_nitro_device_handlers* handlers) {
	while(*usb_thread_run) {
		for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
			isn_async_callback_data* cb_data = (isn_async_callback_data*)&is_nitro_capture_recv_data[i].cb_data;
			bool read_data_ready = false;
			cb_data->transfer_data_access.lock();
			read_data_ready = cb_data->is_data_ready;
			cb_data->is_data_ready = false;
			cb_data->transfer_data_access.unlock();
			if(is_nitro_capture_recv_data[i].in_use && read_data_ready)
				cb_data->function(&is_nitro_capture_recv_data[i], cb_data->actual_length, cb_data->status_value);
		}
		int dummy = 0;
		is_nitro_capture_recv_data[0].cb_data.is_transfer_data_ready_mutex->general_timed_lock(&dummy);
	}
}

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

static bool is_driver_setup_connection(is_nitro_device_handlers* handlers, std::string path) {
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
	handlers->write_handle = CreateFile((path + (char)(std::filesystem::path::preferred_separator)+"Pipe00").c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	handlers->read_handle = CreateFile((path + (char)(std::filesystem::path::preferred_separator)+"Pipe01").c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if ((handlers->write_handle == INVALID_HANDLE_VALUE) || (handlers->read_handle == INVALID_HANDLE_VALUE))
		return false;
	if (!is_driver_device_reset(handlers->write_handle))
		return false;
	if (!is_driver_pipe_reset(handlers->write_handle))
		return false;
	if (!is_driver_pipe_reset(handlers->read_handle))
		return false;
	return true;
}

static int wait_result_io_operation(HANDLE handle, OVERLAPPED* overlapped_var, int* transferred, const is_nitro_usb_device* usb_device_desc) {
	DWORD num_bytes = 0;
	int retval = 0;
	int error = 0;
	do {
		retval = GetOverlappedResultEx(handle, overlapped_var, &num_bytes, usb_device_desc->bulk_timeout, true);
		error = GetLastError();
	} while((retval == 0) && (error == WAIT_IO_COMPLETION));
	*transferred = num_bytes;
	if (retval == 0) {
		if (error == WAIT_TIMEOUT) {
			CancelIoEx(handle, overlapped_var);
			return LIBUSB_ERROR_TIMEOUT;
		}
		return LIBUSB_ERROR_OTHER;
	}
	return LIBUSB_SUCCESS;
}

static void BasicCompletionOutputRoutine(isn_async_callback_data* cb_data, int num_bytes, int error) {
	cb_data->transfer_data_access.lock();
	if ((error == LIBUSB_SUCCESS) && (num_bytes != cb_data->requested_length))
		error = LIBUSB_ERROR_INTERRUPTED;
	cb_data->transfer_data = NULL;
	cb_data->actual_length = num_bytes;
	cb_data->status_value = error;
	cb_data->is_data_ready = true;
	cb_data->is_transfer_data_ready_mutex->specific_unlock(cb_data->internal_index);
	cb_data->is_transfer_done_mutex->specific_unlock(cb_data->internal_index);
	cb_data->transfer_data_access.unlock();
}

static void OverlappedCompletionNothingRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, OVERLAPPED* overlapped_var) {
	return;
}

#endif

is_nitro_device_handlers* is_driver_serial_reconnection(CaptureDevice* device) {
	is_nitro_device_handlers* final_handlers = NULL;
	#ifdef _WIN32
	if (device->path != "") {
		is_nitro_device_handlers handlers;
		if (is_driver_setup_connection(&handlers, device->path)) {
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
				if(is_driver_setup_connection(&handlers, path))
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
int is_driver_bulk_out(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	#ifdef _WIN32
	OVERLAPPED overlapped_var;
	memset(&overlapped_var, 0, sizeof(OVERLAPPED));
	DWORD num_bytes = 0;
	int retval = WriteFileEx(handlers->write_handle, buf, length, &overlapped_var, OverlappedCompletionNothingRoutine);
	if(retval == 0)
		return LIBUSB_ERROR_BUSY;
	return wait_result_io_operation(handlers->write_handle, &overlapped_var, transferred, usb_device_desc);
	#endif
	return LIBUSB_SUCCESS;
}

// Read from bulk
int is_driver_bulk_in(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	#ifdef _WIN32
	OVERLAPPED overlapped_var;
	memset(&overlapped_var, 0, sizeof(OVERLAPPED));
	int retval = ReadFileEx(handlers->read_handle, buf, length, &overlapped_var, OverlappedCompletionNothingRoutine);
	if (retval == 0)
		return LIBUSB_ERROR_BUSY;
	return wait_result_io_operation(handlers->read_handle, &overlapped_var, transferred, usb_device_desc);
	#endif
	return LIBUSB_SUCCESS;
}

int is_drive_async_in_start(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, uint8_t* buf, int length, isn_async_callback_data* cb_data) {
	#ifdef _WIN32
	cb_data->transfer_data_access.lock();
	cb_data->is_data_ready = false;
	cb_data->transfer_data = cb_data->base_transfer_data;
	cb_data->handle = handlers->read_handle;
	cb_data->requested_length = length;
	cb_data->buffer = buf;
	OVERLAPPED* overlap_data = (OVERLAPPED*)cb_data->transfer_data;
	memset(overlap_data, 0, sizeof(OVERLAPPED));
	cb_data->is_transfer_done_mutex->specific_try_lock(cb_data->internal_index);
	cb_data->is_transfer_data_ready_mutex->specific_try_lock(cb_data->internal_index);
	int retval = ReadFileEx(cb_data->handle, cb_data->buffer, cb_data->requested_length, (OVERLAPPED*)cb_data->transfer_data, OverlappedCompletionNothingRoutine);
	cb_data->transfer_data_access.unlock();
	if (retval == 0)
		return LIBUSB_ERROR_BUSY;
	int num_bytes = 0;
	int error = wait_result_io_operation(handlers->read_handle, (OVERLAPPED*)overlap_data, &num_bytes, usb_device_desc);
	BasicCompletionOutputRoutine(cb_data, num_bytes, error);
	#endif
	return LIBUSB_SUCCESS;
}

void is_nitro_is_driver_start_thread(std::thread* thread_ptr, bool* usb_thread_run, ISNitroCaptureReceivedData* is_nitro_capture_recv_data, is_nitro_device_handlers* handlers, ConsumerMutex* AsyncMutexPtr) {
	#ifdef _WIN32
	*usb_thread_run = true;
	for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		is_nitro_capture_recv_data[i].cb_data.base_transfer_data = new OVERLAPPED;
	*thread_ptr = std::thread(is_nitro_is_driver_function, usb_thread_run, is_nitro_capture_recv_data, handlers);
	#endif
}

void is_nitro_is_driver_close_thread(std::thread* thread_ptr, bool* usb_thread_run, ISNitroCaptureReceivedData* is_nitro_capture_recv_data) {
	#ifdef _WIN32
	*usb_thread_run = false;
	is_nitro_capture_recv_data[0].cb_data.is_transfer_data_ready_mutex->specific_unlock(0);
	thread_ptr->join();
	for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		delete is_nitro_capture_recv_data[i].cb_data.base_transfer_data;
	#endif
}

void is_nitro_is_driver_cancel_callback(isn_async_callback_data* cb_data) {
	#ifdef _WIN32
	// Nothing
	#endif
}
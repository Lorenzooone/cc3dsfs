#include "cypress_shared_driver_comms.hpp"

#include <algorithm>
#include <filesystem>
#ifdef _WIN32
#include <setupapi.h>
#include <devioctl.h>

#define BASE_IOCTL_INDEX 0

#define DIR_HOST_TO_DEVICE 0
#define DIR_DEVICE_TO_HOST 1

#define IOCTL_ADAPT_SEND_EP0_CONTROL_TRANSFER CTL_CODE(FILE_DEVICE_UNKNOWN, BASE_IOCTL_INDEX + 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ADAPT_SEND_NON_EP0_DIRECT CTL_CODE(FILE_DEVICE_UNKNOWN, BASE_IOCTL_INDEX + 18, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_ADAPT_RESET_PIPE CTL_CODE(FILE_DEVICE_UNKNOWN, BASE_IOCTL_INDEX + 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ADAPT_SET_TRANSFER_SIZE CTL_CODE(FILE_DEVICE_UNKNOWN, BASE_IOCTL_INDEX + 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ADAPT_ABORT_PIPE CTL_CODE(FILE_DEVICE_UNKNOWN, BASE_IOCTL_INDEX + 17, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define USB_REQUEST_GET_STATUS                    0x00
#define USB_REQUEST_CLEAR_FEATURE                 0x01

#define USB_REQUEST_SET_FEATURE                   0x03

#define USB_REQUEST_SET_ADDRESS                   0x05
#define USB_REQUEST_GET_DESCRIPTOR                0x06
#define USB_REQUEST_SET_DESCRIPTOR                0x07
#define USB_REQUEST_GET_CONFIGURATION             0x08
#define USB_REQUEST_SET_CONFIGURATION             0x09
#define USB_REQUEST_GET_INTERFACE                 0x0A
#define USB_REQUEST_SET_INTERFACE                 0x0B
#define USB_REQUEST_SYNC_FRAME                    0x0C

#define USB_DEVICE_DESCRIPTOR_TYPE                0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE         0x02
#define USB_STRING_DESCRIPTOR_TYPE                0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE             0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE              0x05

#define TARGET_DEVICE 0
#define REQUEST_STANDARD 0
#define REQUEST_VENDOR 2

#pragma pack(push, 1)

struct PACKED USB_COMMON_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
};

struct PACKED USB_STRING_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
	WCHAR bString[1];
};

struct PACKED USB_DEVICE_DESCRIPTOR {
	UCHAR bLength;
	UCHAR bDescriptorType;
	USHORT bcdUSB;
	UCHAR bDeviceClass;
	UCHAR bDeviceSubClass;
	UCHAR bDeviceProtocol;
	UCHAR bMaxPacketSize0;
	USHORT idVendor;
	USHORT idProduct;
	USHORT bcdDevice;
	UCHAR iManufacturer;
	UCHAR iProduct;
	UCHAR iSerialNumber;
	UCHAR bNumConfigurations;
};

// Is this fine? Without any check for endianness?
// Shouldn't all of this be packed?!
struct PACKED CYPRESS_WORD_SPLIT {
	UCHAR lowByte;
	UCHAR hiByte;
};

struct PACKED CYPRESS_BM_REQ_TYPE {
	UCHAR   Recipient:2;
	UCHAR   Reserved:3;
	UCHAR   Type:2;
	UCHAR   Direction:1;
};

struct PACKED CYPRESS_SETUP_PACKET {
	union {
		CYPRESS_BM_REQ_TYPE bmReqType;
		UCHAR bmRequest;
	};
	UCHAR bRequest;
	union {
		CYPRESS_WORD_SPLIT wVal;
		USHORT wValue;
	};
	union {
		CYPRESS_WORD_SPLIT wIndx;
		USHORT wIndex;
	};
	union {
		CYPRESS_WORD_SPLIT wLen;
		USHORT wLength;
	};
	ULONG ulTimeOut;
};

struct PACKED CYPRESS_ISO_ADV_PARAMS {
	USHORT isoId;
	USHORT isoCmd;
	ULONG ulParam1;
	ULONG ulParam2;
};

struct PACKED CYPRESS_SINGLE_TRANSFER {
	union {
		CYPRESS_SETUP_PACKET SetupPacket;
		CYPRESS_ISO_ADV_PARAMS IsoParams;
	};

	UCHAR reserved;

	UCHAR ucEndpointAddress;
	ULONG NtStatus;
	ULONG UsbdStatus;
	ULONG IsoPacketOffset;
	ULONG IsoPacketLength;
	ULONG BufferOffset;
	ULONG BufferLength;
};

struct PACKED CYPRESS_SET_TRANSFER_SIZE_INFO {
    UCHAR EndpointAddress;
    ULONG TransferSize;
};

#pragma pack(pop)

static GUID cypress_driver_guid = {.Data1 = 0xae18aa60, .Data2 = 0x7f6a, .Data3 = 0x11d4, .Data4 = {0x97, 0xdd, 0x0, 0x1, 0x2, 0x29, 0xb9, 0x59}};
static GUID cypress_optimize_driver_guid = {.Data1 = 0xae18aa60, .Data2 = 0x7f6a, .Data3 = 0x11d4, .Data4 = {0x97, 0xdd, 0x0, 0x1, 0x2, 0x29, 0xb9, 0x59}};

static GUID* get_driver_guid(CypressWindowsDriversEnum driver) {
	switch(driver) {
		case CYPRESS_WINDOWS_DEFAULT_USB_DRIVER:
			return &cypress_driver_guid;
		case CYPRESS_WINDOWS_OPTIMIZE_USB_DRIVER:
			return &cypress_optimize_driver_guid;
		default:
			return NULL;
	}
}

static bool cypress_driver_process_async_transfers(std::vector<cy_async_callback_data*> *cb_data_vector, cy_device_device_handlers* handlers) {
	bool found = false;
	for(size_t i = 0; i < cb_data_vector->size(); i++) {
		cy_async_callback_data* cb_data = cb_data_vector->at(i);
		bool possible_result = false;
		bool is_data_ready = false;
		cb_data->transfer_data_access.lock();
		possible_result = cb_data->transfer_data != NULL;
		is_data_ready = cb_data->is_data_ready;
		cb_data->transfer_data_access.unlock();
		if((*cb_data->in_use_ptr) && is_data_ready)
			found = true;
		if((*cb_data->in_use_ptr) && possible_result) {
			DWORD bytes_transferred = 0;
			int error = 0;
			if(!GetOverlappedResult(handlers->read_handle, (OVERLAPPED*)cb_data->transfer_data, &bytes_transferred, false))
				error = GetLastError();
			if(error == ERROR_IO_INCOMPLETE) {
				const auto curr_time = std::chrono::high_resolution_clock::now();
				const std::chrono::duration<double> diff = curr_time - cb_data->start_request;
				if(diff.count() > cb_data->timeout_s)
					cb_data->error_function(cb_data->actual_user_data, LIBUSB_ERROR_TIMEOUT);
			}
			if(error != ERROR_IO_INCOMPLETE) {
				CYPRESS_SINGLE_TRANSFER* transfer_info = (CYPRESS_SINGLE_TRANSFER*)cb_data->extra_transfer_data;
				if(!error) {
					error = transfer_info->UsbdStatus;
					if(!error)
						error = transfer_info->NtStatus;
				}
				cb_data->actual_length = bytes_transferred;
				cb_data->status_value = error;
				cb_data->transfer_data = NULL;
				cb_data->is_data_ready = true;
				found = true;
			}
		}
	}
	return found;
}

static void cypress_driver_function(bool* usb_thread_run, std::vector<cy_async_callback_data*> *cb_data_vector, cy_device_device_handlers* handlers) {
	int ret = 0;
	HANDLE* to_wait_array = new HANDLE[cb_data_vector->size()];
	for(size_t i = 0; i < cb_data_vector->size(); i++)
		to_wait_array[i] = ((OVERLAPPED*)cb_data_vector->at(i)->base_transfer_data)->hEvent;

	while(*usb_thread_run) {
		if(cypress_driver_process_async_transfers(cb_data_vector, handlers)) {
			for(size_t i = 0; i < cb_data_vector->size(); i++) {
				cy_async_callback_data* cb_data = cb_data_vector->at(i);
				bool is_data_ready = false;
				cb_data->transfer_data_access.lock();
				is_data_ready = cb_data->is_data_ready;
				cb_data->transfer_data_access.unlock();
				if((*cb_data->in_use_ptr) && is_data_ready) {
					cb_data->is_data_ready = false;
					cb_data->function(cb_data->actual_user_data, (int)cb_data->actual_length, cb_data->status_value);
				}
			}
		}
		else
			WaitForMultipleObjects((DWORD)cb_data_vector->size(), to_wait_array, false, 300);
	}
	delete []to_wait_array;
}

static bool cypress_driver_bulk_async_start(HANDLE handle, uint8_t* buffer, size_t size, DWORD* num_read, uint8_t ep_num, OVERLAPPED* overlap, CYPRESS_SINGLE_TRANSFER* single_transfer) {
	memset(single_transfer, 0, sizeof(CYPRESS_SINGLE_TRANSFER));
	single_transfer->ucEndpointAddress = ep_num;
	bool retval = DeviceIoControl(handle, IOCTL_ADAPT_SEND_NON_EP0_DIRECT, single_transfer, sizeof(CYPRESS_SINGLE_TRANSFER), buffer, (DWORD)size, num_read, overlap);
	return retval;
}

static bool cypress_driver_bulk_sync_start(HANDLE handle, uint8_t* buffer, size_t size, DWORD* num_read, uint8_t ep_num) {
	CYPRESS_SINGLE_TRANSFER single_transfer;
	return cypress_driver_bulk_async_start(handle, buffer, size, num_read, ep_num, NULL, &single_transfer);
}

static bool cypress_driver_generic_ctrl_transfer(HANDLE handle, uint8_t* buffer, size_t inner_size, uint8_t val_hi, uint8_t val_low, uint16_t index, uint16_t timeout_s, uint8_t recipient, uint8_t type, uint8_t direction, uint16_t request_code, DWORD* num_read) {
	size_t full_size = inner_size + sizeof(CYPRESS_SINGLE_TRANSFER);
	uint8_t* full_buffer = new uint8_t[full_size];
	memset(full_buffer, 0, full_size);
	CYPRESS_SINGLE_TRANSFER* single_transfer = (CYPRESS_SINGLE_TRANSFER*)full_buffer;
	single_transfer->SetupPacket.bmReqType.Direction = direction;
	single_transfer->SetupPacket.bmReqType.Type = type;
	single_transfer->SetupPacket.bmReqType.Recipient = recipient;
	single_transfer->SetupPacket.bRequest = (UCHAR)request_code;
	single_transfer->SetupPacket.wVal.hiByte = val_hi;
	single_transfer->SetupPacket.wVal.lowByte = val_low;
	single_transfer->SetupPacket.wIndex = index;
	single_transfer->SetupPacket.wLength = (USHORT)inner_size;
	single_transfer->SetupPacket.ulTimeOut = timeout_s;
	single_transfer->BufferLength = single_transfer->SetupPacket.wLength;
	single_transfer->BufferOffset = sizeof(CYPRESS_SINGLE_TRANSFER);
	if(direction == DIR_HOST_TO_DEVICE)
		memcpy(full_buffer + single_transfer->BufferOffset, buffer, inner_size);
	bool retval = DeviceIoControl(handle, IOCTL_ADAPT_SEND_EP0_CONTROL_TRANSFER, full_buffer, (DWORD)full_size, full_buffer, (DWORD)full_size, num_read, NULL);
	if((*num_read) >= sizeof(CYPRESS_SINGLE_TRANSFER))
		*num_read -= sizeof(CYPRESS_SINGLE_TRANSFER);
	else
		*num_read = 0;
	if(direction == DIR_DEVICE_TO_HOST)
		memcpy(buffer, full_buffer + single_transfer->BufferOffset, inner_size);
	delete []full_buffer;
	if((*num_read) != inner_size)
		return false;
	return retval;
}

static bool read_cypress_driver_generic_info(HANDLE handle, uint8_t* buffer, size_t inner_size, uint8_t val_hi, uint8_t val_low, uint16_t index, DWORD* num_read) {
	return cypress_driver_generic_ctrl_transfer(handle, buffer, inner_size, val_hi, val_low, index, 5, TARGET_DEVICE, REQUEST_STANDARD, DIR_DEVICE_TO_HOST, USB_REQUEST_GET_DESCRIPTOR, num_read);
}

static bool read_cypress_driver_device_info(HANDLE handle, uint8_t* buffer, size_t inner_size, DWORD* num_read) {
	return read_cypress_driver_generic_info(handle, buffer, inner_size, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, num_read);
}

static bool cypress_driver_get_string_language_info(HANDLE handle, USHORT &language_id, USHORT preferred_language = 0x0409) {
	USB_COMMON_DESCRIPTOR cmnDescriptor;
	DWORD num_read = 0;
	language_id = 0;
	bool retval = read_cypress_driver_generic_info(handle, (uint8_t*)&cmnDescriptor, sizeof(cmnDescriptor), USB_STRING_DESCRIPTOR_TYPE, 0, 0, &num_read);
	if(!retval)
		return false;
	if(cmnDescriptor.bLength == 2)
		return true;
	uint8_t* buffer = new uint8_t[cmnDescriptor.bLength];
	retval = read_cypress_driver_generic_info(handle, buffer, cmnDescriptor.bLength, USB_STRING_DESCRIPTOR_TYPE, 0, 0, &num_read);
	if(retval) {
		USB_STRING_DESCRIPTOR* IDs = (USB_STRING_DESCRIPTOR*)buffer;
		int num_langs = (cmnDescriptor.bLength - 2) / 2;
		language_id = IDs[0].bString[0];
		for(int i = 0; i < num_langs; i++)
			if(IDs[i].bString[0] == preferred_language)
				language_id = preferred_language;
	}
	delete []buffer;
	return retval;
}

static bool cypress_driver_get_string_info(HANDLE handle, UCHAR string_index, USHORT language_id, std::string &out_str) {
	USB_COMMON_DESCRIPTOR cmnDescriptor;
	DWORD num_read = 0;
	out_str = "";
	bool retval = read_cypress_driver_generic_info(handle, (uint8_t*)&cmnDescriptor, sizeof(cmnDescriptor), USB_STRING_DESCRIPTOR_TYPE, string_index, language_id, &num_read);
	if(!retval)
		return false;
	uint8_t* buffer = new uint8_t[cmnDescriptor.bLength];
	retval = read_cypress_driver_generic_info(handle, buffer, cmnDescriptor.bLength, USB_STRING_DESCRIPTOR_TYPE, string_index, language_id, &num_read);
	if(retval && num_read >= 2) {
		uint8_t bytes = buffer[0];
		if(bytes > num_read)
			bytes = (uint8_t)num_read;
		uint8_t signature = buffer[1];
		if((signature == 3) && (bytes > 2))
			out_str = read_string(buffer + 2, bytes - 2);
	}
	delete []buffer;
	return retval;
}

static std::string cypress_driver_get_device_path(HDEVINFO DeviceInfoSet, SP_DEVICE_INTERFACE_DATA* DeviceInterfaceData) {
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

static bool cypress_driver_get_device_pid_vid_ids(HANDLE handle, uint16_t& out_vid, uint16_t& out_pid, UCHAR &imanufacturer, UCHAR &iserial, USHORT &bcdDevice) {
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	USB_DEVICE_DESCRIPTOR out_descriptor;
	memset(&out_descriptor, 0, sizeof(out_descriptor));
	DWORD num_read = 0;
	bool result = read_cypress_driver_device_info(handle, (uint8_t*)&out_descriptor, sizeof(USB_DEVICE_DESCRIPTOR), &num_read);
	if(!result)
		return false;
	out_vid = out_descriptor.idVendor;
	out_pid = out_descriptor.idProduct;
	imanufacturer = out_descriptor.iManufacturer;
	//iproduct = out_descriptor.iProduct;
	iserial = out_descriptor.iSerialNumber;
	bcdDevice = out_descriptor.bcdDevice;
	return true;
}

static bool cypress_driver_get_device_pid_vid_imanufacturer_iserial_number(std::string path, uint16_t& out_vid, uint16_t& out_pid, UCHAR &imanufacturer, UCHAR &iserial, USHORT &bcd_device) {
	HANDLE handle = CreateFile(path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	bool result = cypress_driver_get_device_pid_vid_ids(handle, out_vid, out_pid, imanufacturer, iserial, bcd_device);
	CloseHandle(handle);
	if(!result)
		return false;
	return true;
}

static bool cypress_driver_get_device_string_manufacturer_product_serial_number(std::string path, std::string &manufacturer, std::string &serial, UCHAR &imanufacturer, UCHAR &iserial) {
	HANDLE handle = CreateFile(path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	USHORT language = 0;
	cypress_driver_get_string_language_info(handle, language);
	cypress_driver_get_string_info(handle, imanufacturer, language, manufacturer);
	cypress_driver_get_string_info(handle, iserial, language, serial);
	CloseHandle(handle);
	return true;
}

static bool cypress_driver_setup_connection(cy_device_device_handlers* handlers, std::string path, bool do_pipe_clear_reset) {
	handlers->usb_handle = NULL;
	handlers->mutex = NULL;
	handlers->write_handle = INVALID_HANDLE_VALUE;
	handlers->read_handle = INVALID_HANDLE_VALUE;
	handlers->path = path;
	std::string mutex_name = "Global\\ISU_" + path.substr(4);
	std::replace(mutex_name.begin(), mutex_name.end(), '\\', '@');
	handlers->mutex = CreateMutex(NULL, true, mutex_name.c_str());
	if((handlers->mutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS)) {
		CloseHandle(handlers->mutex);
		handlers->mutex = NULL;
	}
	if(handlers->mutex == NULL)
		return false;
	handlers->write_handle = CreateFile(path.c_str(), (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	handlers->read_handle = handlers->write_handle;
	if((handlers->write_handle == INVALID_HANDLE_VALUE) || (handlers->read_handle == INVALID_HANDLE_VALUE))
		return false;
	if(do_pipe_clear_reset) {
		/*
		if(!is_driver_device_reset(handlers->write_handle))
			return false;
		if(!is_driver_pipe_reset(handlers->write_handle))
			return false;
		if(!is_driver_pipe_reset(handlers->read_handle))
			return false;
		*/
	}
	return true;
}

static void cypress_driver_close_handle(void** handle, void* default_value = INVALID_HANDLE_VALUE) {
	#ifdef _WIN32
	if ((*handle) == default_value)
		return;
	CloseHandle(*handle);
	*handle = default_value;
	#endif
}

static void cypress_driver_pipe_reset(HANDLE handle, uint8_t endpoint) {
    DWORD dwBytes = 0;
    DeviceIoControl(handle, IOCTL_ADAPT_RESET_PIPE, &endpoint, sizeof(uint8_t), NULL, 0, &dwBytes, 0);
}

#endif

cy_device_device_handlers* cypress_driver_serial_reconnection(CaptureDevice* device) {
	cy_device_device_handlers* final_handlers = NULL;
	#ifdef _WIN32
	if(device->path != "") {
		cy_device_device_handlers handlers;
		const cy_device_usb_device* usb_device_info = (const cy_device_usb_device*)device->descriptor;
		if(cypress_driver_setup_connection(&handlers, device->path, usb_device_info->do_pipe_clear_reset)) {
			final_handlers = new cy_device_device_handlers;
			final_handlers->usb_handle = NULL;
			final_handlers->mutex = handlers.mutex;
			final_handlers->read_handle = handlers.read_handle;
			final_handlers->write_handle = handlers.write_handle;
			final_handlers->path = device->path;
		}
		else
			cypress_driver_end_connection(&handlers);
	}
	#endif
	return final_handlers;
}

void cypress_driver_list_devices(std::vector<CaptureDevice> &devices_list, bool* not_supported_elems, int *curr_serial_extra_id_cypress, std::vector<const cy_device_usb_device*> &device_descriptions, CypressWindowsDriversEnum driver) {
	#ifdef _WIN32
	GUID* to_check_guid = get_driver_guid(driver);
	if(to_check_guid == NULL)
		return;
	HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
		to_check_guid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	uint32_t i = 0;
	while(SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, to_check_guid, i++, &DeviceInterfaceData)) {
		std::string path = cypress_driver_get_device_path(DeviceInfoSet, &DeviceInterfaceData);
		if (path == "")
			continue;
		uint16_t vid = 0;
		uint16_t pid = 0;
		USHORT bcd_device = 0;
		UCHAR imanufacturer = 0;
		UCHAR iserial = 0;
		if(!cypress_driver_get_device_pid_vid_imanufacturer_iserial_number(path, vid, pid, imanufacturer, iserial, bcd_device))
			continue;
		for(size_t j = 0; j < device_descriptions.size(); j++) {
			const cy_device_usb_device* usb_device_desc = device_descriptions[j];
			uint16_t masked_wanted_bcd_device = usb_device_desc->bcd_device_mask & usb_device_desc->bcd_device_wanted_value;
			if(not_supported_elems[j] && (usb_device_desc->vid == vid) && (usb_device_desc->pid == pid) && (masked_wanted_bcd_device == (bcd_device & usb_device_desc->bcd_device_mask))) {
				std::string manufacturer;
				std::string serial;
				bool result = cypress_driver_get_device_string_manufacturer_product_serial_number(path, manufacturer, serial, imanufacturer, iserial);
				if(!result)
					continue;
				cy_device_device_handlers handlers;
				bool result_conn_check = cypress_driver_setup_connection(&handlers, path, usb_device_desc->do_pipe_clear_reset);
				cypress_driver_end_connection(&handlers);
				if(result_conn_check)
					cypress_insert_device(devices_list, usb_device_desc, serial, bcd_device, curr_serial_extra_id_cypress[j], path);
				break;
			}
		}
	}

	if(DeviceInfoSet)
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	#endif
}

cy_device_device_handlers* cypress_driver_find_by_serial_number(const cy_device_usb_device* usb_device_desc, std::string wanted_serial_number, int &curr_serial_extra_id, CaptureDevice* new_device, CypressWindowsDriversEnum driver) {
	cy_device_device_handlers* final_handlers = NULL;
	#ifdef _WIN32
	GUID* to_check_guid = get_driver_guid(driver);
	if(to_check_guid == NULL)
		return final_handlers;
	HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
		to_check_guid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	uint32_t i = 0;
	uint16_t masked_wanted_bcd_device = usb_device_desc->bcd_device_mask & usb_device_desc->bcd_device_wanted_value;
	while(SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, to_check_guid, i++, &DeviceInterfaceData)) {
		std::string path = cypress_driver_get_device_path(DeviceInfoSet, &DeviceInterfaceData);
		if (path == "")
			continue;
		uint16_t vid = 0;
		uint16_t pid = 0;
		uint16_t bcd_device = 0;
		UCHAR imanufacturer = 0;
		UCHAR iserial = 0;
		if(!cypress_driver_get_device_pid_vid_imanufacturer_iserial_number(path, vid, pid, imanufacturer, iserial, bcd_device))
			continue;
		if((usb_device_desc->vid == vid) && (usb_device_desc->pid == pid) && (masked_wanted_bcd_device == (bcd_device & usb_device_desc->bcd_device_mask))) {
			std::string manufacturer;
			std::string serial;
			if(!cypress_driver_get_device_string_manufacturer_product_serial_number(path, manufacturer, serial, imanufacturer, iserial))
				continue;
			std::string read_serial = cypress_get_serial(usb_device_desc, serial, bcd_device, curr_serial_extra_id);
			if(wanted_serial_number != read_serial)
				continue;
			cy_device_device_handlers handlers;
			if(cypress_driver_setup_connection(&handlers, path, usb_device_desc->do_pipe_clear_reset)) {
				final_handlers = new cy_device_device_handlers;
				final_handlers->usb_handle = NULL;
				final_handlers->mutex = handlers.mutex;
				final_handlers->read_handle = handlers.read_handle;
				final_handlers->write_handle = handlers.write_handle;
				final_handlers->path = path;
				if(new_device != NULL)
					*new_device = cypress_create_device(usb_device_desc, wanted_serial_number, path);
			}
			else
				cypress_driver_end_connection(&handlers);
			break;
		}
	}

	if(DeviceInfoSet)
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	#endif
	return final_handlers;
}

int cypress_driver_ctrl_transfer_in(cy_device_device_handlers* handlers, uint8_t* buffer, size_t inner_size, uint16_t value, uint16_t index, uint16_t request_code, int* num_read) {
	#ifdef _WIN32
	DWORD dummy = 0;
	bool retval = cypress_driver_generic_ctrl_transfer(handlers->read_handle, buffer, inner_size, (value >> 8) & 0xFF, value & 0xFF, index, 1, TARGET_DEVICE, REQUEST_VENDOR, DIR_DEVICE_TO_HOST, request_code, &dummy);
	*num_read = dummy;
	return retval ? 0 : -1;
	#else
	return 0;
	#endif
}

int cypress_driver_ctrl_transfer_out(cy_device_device_handlers* handlers, uint8_t* buffer, size_t inner_size, uint16_t value, uint16_t index, uint16_t request_code, int* num_read) {
	#ifdef _WIN32
	DWORD dummy = 0;
	bool retval = cypress_driver_generic_ctrl_transfer(handlers->write_handle, buffer, inner_size, (value >> 8) & 0xFF, value & 0xFF, index, 1, TARGET_DEVICE, REQUEST_VENDOR, DIR_HOST_TO_DEVICE, request_code, &dummy);
	*num_read = dummy;
	return retval ? 0 : -1;
	#else
	return 0;
	#endif
}

void cypress_driver_end_connection(cy_device_device_handlers* handlers) {
	#ifdef _WIN32
	cypress_driver_close_handle(&handlers->write_handle);
	handlers->read_handle = handlers->write_handle;
	cypress_driver_close_handle(&handlers->mutex, NULL);
	#endif
}

int cypress_driver_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	int result = LIBUSB_SUCCESS;
	#ifdef _WIN32
	DWORD inner_transfered = 0;
	if(!cypress_driver_bulk_sync_start(handlers->read_handle, buf, length, &inner_transfered, usb_device_desc->ep_bulk_in))
		result = -1;
	*transferred = inner_transfered;
	#endif
	return result;
}

int cypress_driver_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	int result = LIBUSB_SUCCESS;
	#ifdef _WIN32
	DWORD inner_transfered = 0;
	if(!cypress_driver_bulk_sync_start(handlers->read_handle, buf, length, &inner_transfered, usb_device_desc->ep_ctrl_bulk_in))
		result = -1;
	*transferred = inner_transfered;
	#endif
	return result;
}

int cypress_driver_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	int result = LIBUSB_SUCCESS;
	#ifdef _WIN32
	DWORD inner_transfered = 0;
	if(!cypress_driver_bulk_sync_start(handlers->write_handle, buf, length, &inner_transfered, usb_device_desc->ep_ctrl_bulk_out))
		result = -1;
	*transferred = inner_transfered;
	#endif
	return result;
}

void cypress_driver_pipe_reset_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	#ifdef _WIN32
	cypress_driver_pipe_reset(handlers->read_handle, usb_device_desc->ep_ctrl_bulk_in);
	#endif
}

void cypress_driver_pipe_reset_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	#ifdef _WIN32
	cypress_driver_pipe_reset(handlers->read_handle, usb_device_desc->ep_bulk_in);
	#endif
}

void cypress_driver_pipe_reset_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	#ifdef _WIN32
	cypress_driver_pipe_reset(handlers->read_handle, usb_device_desc->ep_ctrl_bulk_out);
	#endif
}

void cypress_driver_find_used_serial(const cy_device_usb_device* usb_device_desc, bool* found, size_t num_free_fw_ids, int &curr_serial_extra_id, CypressWindowsDriversEnum driver) {
	#ifdef _WIN32
	GUID* to_check_guid = get_driver_guid(driver);
	if(to_check_guid == NULL)
		return;
	HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
		to_check_guid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	uint32_t i = 0;
	uint16_t masked_wanted_bcd_device = usb_device_desc->bcd_device_mask & usb_device_desc->bcd_device_wanted_value;
	while(SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, to_check_guid, i++, &DeviceInterfaceData)) {
		std::string path = cypress_driver_get_device_path(DeviceInfoSet, &DeviceInterfaceData);
		if (path == "")
			continue;
		uint16_t vid = 0;
		uint16_t pid = 0;
		uint16_t bcd_device = 0;
		UCHAR imanufacturer = 0;
		UCHAR iserial = 0;
		if(!cypress_driver_get_device_pid_vid_imanufacturer_iserial_number(path, vid, pid, imanufacturer, iserial, bcd_device))
			continue;
		if((usb_device_desc->vid == vid) && (usb_device_desc->pid == pid) && (masked_wanted_bcd_device == (bcd_device & usb_device_desc->bcd_device_mask))) {
			std::string manufacturer;
			std::string serial;
			if(!cypress_driver_get_device_string_manufacturer_product_serial_number(path, manufacturer, serial, imanufacturer, iserial))
				continue;
			std::string read_serial = cypress_get_serial(usb_device_desc, serial, bcd_device, curr_serial_extra_id);
			try {
				size_t pos = std::stoi(read_serial);
				if((pos < 0) || (pos >= num_free_fw_ids))
					continue;
				found[pos] = true;
			}
			catch(...) {
				continue;
			}
		}
	}

	if(DeviceInfoSet)
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	#endif
}

int cypress_driver_async_in_start(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, cy_async_callback_data* cb_data) {
	#ifdef _WIN32
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = cb_data->base_transfer_data;
	cb_data->handle = handlers->read_handle;
	cb_data->requested_length = length;
	cb_data->buffer = buf;
	cb_data->start_request = std::chrono::high_resolution_clock::now();
	cb_data->timeout_s = usb_device_desc->bulk_timeout / 1000.0f;
	OVERLAPPED* overlap_data = (OVERLAPPED*)cb_data->transfer_data;
	cb_data->is_transfer_done_mutex->specific_try_lock(cb_data->internal_index);
	int retval = cypress_driver_bulk_async_start(cb_data->handle, cb_data->buffer, cb_data->requested_length, NULL, usb_device_desc->ep_bulk_in, overlap_data, (CYPRESS_SINGLE_TRANSFER*)cb_data->extra_transfer_data);
	cb_data->transfer_data_access.unlock();
	if(!retval) {
		int error = GetLastError();
		if(error != ERROR_IO_PENDING)
			return LIBUSB_ERROR_BUSY;
	}
	#endif
	return LIBUSB_SUCCESS;
}

void cypress_driver_start_thread(std::thread* thread_ptr, bool* usb_thread_run, std::vector<cy_async_callback_data*> &cb_data_vector, cy_device_device_handlers* handlers) {
	#ifdef _WIN32
	*usb_thread_run = true;
	for (size_t i = 0; i < cb_data_vector.size(); i++) {
		cb_data_vector[i]->extra_transfer_data = new CYPRESS_SINGLE_TRANSFER;
		cb_data_vector[i]->base_transfer_data = new OVERLAPPED;
		((OVERLAPPED*)cb_data_vector[i]->base_transfer_data)->hEvent = CreateEvent(NULL, false, false, NULL);
	}
	*thread_ptr = std::thread(cypress_driver_function, usb_thread_run, &cb_data_vector, handlers);
	#endif
}

void cypress_driver_close_thread(std::thread* thread_ptr, bool* usb_thread_run, std::vector<cy_async_callback_data*> cb_data_vector) {
	#ifdef _WIN32
	*usb_thread_run = false;
	thread_ptr->join();
	for(size_t i = 0; i < cb_data_vector.size(); i++) {
		CloseHandle(((OVERLAPPED*)cb_data_vector[i]->base_transfer_data)->hEvent);
		delete ((OVERLAPPED*)cb_data_vector[i]->base_transfer_data);
		delete ((CYPRESS_SINGLE_TRANSFER*)cb_data_vector[i]->extra_transfer_data);
	}
	#endif
}

void cypress_driver_cancel_callback(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, cy_async_callback_data* cb_data) {
	#ifdef _WIN32
    DWORD transferred = 0;
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(OVERLAPPED));
	ov.hEvent = CreateEvent(NULL, false, false, NULL);
	uint8_t pipe = usb_device_desc->ep_bulk_in;
	bool ret = DeviceIoControl(handlers->read_handle, IOCTL_ADAPT_ABORT_PIPE, &pipe, sizeof(uint8_t), NULL, 0, &transferred, &ov);
	if((!ret) && (GetLastError() == ERROR_IO_PENDING))
		WaitForSingleObject(ov.hEvent, INFINITE);
	CloseHandle(ov.hEvent);
	#endif
}

void cypress_driver_set_transfer_size_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, size_t new_transfer_size) {
	#ifdef _WIN32
    DWORD transferred;
    CYPRESS_SET_TRANSFER_SIZE_INFO SetTransferInfo;

	new_transfer_size = ((new_transfer_size + usb_device_desc->max_usb_packet_size - 1) / usb_device_desc->max_usb_packet_size) * usb_device_desc->max_usb_packet_size;

    SetTransferInfo.EndpointAddress = usb_device_desc->ep_bulk_in;    
    SetTransferInfo.TransferSize = (ULONG)new_transfer_size;    

    DeviceIoControl(handlers->read_handle, IOCTL_ADAPT_SET_TRANSFER_SIZE, &SetTransferInfo, sizeof(CYPRESS_SET_TRANSFER_SIZE_INFO), &SetTransferInfo, sizeof(CYPRESS_SET_TRANSFER_SIZE_INFO), &transferred, NULL);
	#endif
}

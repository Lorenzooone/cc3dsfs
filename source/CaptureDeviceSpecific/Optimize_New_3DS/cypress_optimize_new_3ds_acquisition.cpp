#include "devicecapture.hpp"
#include "cypress_shared_driver_comms.hpp"
#include "cypress_shared_libusb_comms.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_optimize_new_3ds_communications.hpp"
#include "cypress_optimize_new_3ds_acquisition.hpp"
#include "cypress_optimize_new_3ds_acquisition_general.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>

// This code was developed by exclusively looking at Wireshark USB packet
// captures to learn the USB device's protocol.
// As such, it was developed in a clean room environment, to provide
// compatibility of the USB device with more (and more modern) hardware.
// Such an action is allowed under EU law.
// No reverse engineering of the original software was done to create this.

#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS NUM_CONCURRENT_DATA_BUFFER_WRITERS

// The driver only seems to support up to 4 concurrent reads. Not more...
#ifdef NUM_CAPTURE_RECEIVED_DATA_BUFFERS
#if NUM_CAPTURE_RECEIVED_DATA_BUFFERS > 4
#define NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS 4
#else
#define NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS NUM_CAPTURE_RECEIVED_DATA_BUFFERS
#endif
#endif

#define MAX_TIME_WAIT 1.0

#define OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER CYPRESS_WINDOWS_OPTIMIZE_NEW_USB_DRIVER

struct CypressOptimizeNew3DSDeviceCaptureReceivedData {
	volatile bool in_use;
	uint32_t index;
	SharedConsumerMutex* is_buffer_free_shared_mutex;
	int* status;
	uint32_t* last_index;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	cy_async_callback_data cb_data;
};

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status);
static int get_cypress_device_status(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data);
static void error_cypress_device_status(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val);

static cy_device_device_handlers* usb_find_by_serial_number(const cyopn_device_usb_device* usb_device_desc, std::string wanted_serial_number, CaptureDevice* new_device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device);

	if(final_handlers == NULL)
		final_handlers = cypress_driver_find_by_serial_number(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device, OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER);
	return final_handlers;
}

static int usb_find_free_fw_id(const cyopn_device_usb_device* usb_device_desc) {
	int curr_serial_extra_id = 0;
	const int num_free_fw_ids = 0x100;
	bool found[num_free_fw_ids];
	for(int i = 0; i < num_free_fw_ids; i++)
		found[i] = false;
	cypress_libusb_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id);
	cypress_driver_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id, OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER);

	for(int i = 0; i < num_free_fw_ids; i++)
		if(!found[i])
			return i;
	return 0;
}

static cy_device_device_handlers* usb_reconnect(const cyopn_device_usb_device* usb_device_desc, CaptureDevice* device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), device->serial_number, curr_serial_extra_id, NULL);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_serial_reconnection(device);
	return final_handlers;
}

static std::string _cypress_optimize_new_3ds_get_serial(const cyopn_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if((!usb_device_desc->has_bcd_device_serial) && (serial != ""))
		return serial;
	if(usb_device_desc->has_bcd_device_serial)
		return std::to_string(bcd_device & 0xFF);
	return std::to_string((curr_serial_extra_id++) + 1);
}

std::string cypress_optimize_new_3ds_get_serial(const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if(usb_device_desc == NULL)
		return "";
	return _cypress_optimize_new_3ds_get_serial((const cyopn_device_usb_device*)usb_device_desc, serial, bcd_device, curr_serial_extra_id);
}

static CaptureDevice _cypress_optimize_new_3ds_create_device(const cyopn_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	return CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_CYPRESS_NEW_OPTIMIZE, (void*)usb_device_desc, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS, VIDEO_DATA_RGB, path);
}

CaptureDevice cypress_optimize_new_3ds_create_device(const void* usb_device_desc, std::string serial, std::string path) {
	if(usb_device_desc == NULL)
		return CaptureDevice();
	return _cypress_optimize_new_3ds_create_device((const cyopn_device_usb_device*)usb_device_desc, serial, path);
}

static void cypress_optimize_new_3ds_connection_end(cy_device_device_handlers* handlers, const cyopn_device_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		cypress_libusb_end_connection(handlers, get_cy_usb_info(device_desc), interface_claimed);
	else
		cypress_driver_end_connection(handlers);
	delete handlers;
}

void list_devices_cyopn_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	const size_t num_cyopn_device_desc = GetNumCyOpNDeviceDesc();
	int* curr_serial_extra_id_cyopn_device = new int[num_cyopn_device_desc];
	bool* no_access_elems = new bool[num_cyopn_device_desc];
	bool* not_supported_elems = new bool[num_cyopn_device_desc];
	std::vector<const cy_device_usb_device*> usb_devices_to_check;
	for (int i = 0; i < num_cyopn_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_cyopn_device[i] = 0;
		const cyopn_device_usb_device* curr_device_desc = GetCyOpNDeviceDesc(i);
		usb_devices_to_check.push_back(get_cy_usb_info(curr_device_desc));
	}
	cypress_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_cyopn_device, usb_devices_to_check);

	bool any_not_supported = false;
	for(int i = 0; i < num_cyopn_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(int i = 0; i < num_cyopn_device_desc; i++)
		if(no_access_elems[i])
			no_access_list.emplace_back(usb_devices_to_check[i]->vid, usb_devices_to_check[i]->pid);
	if(any_not_supported)
		cypress_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_cyopn_device, usb_devices_to_check, OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER);

	delete[] curr_serial_extra_id_cyopn_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

bool cyopn_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const cyopn_device_usb_device* usb_device_info = (const cyopn_device_usb_device*)device->descriptor;
	cy_device_device_handlers* handlers = usb_reconnect(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	if(has_to_load_firmware(usb_device_info)) {
		const cyopn_device_usb_device* next_usb_device_info = GetNextDeviceDesc(usb_device_info);
		int free_fw_id = usb_find_free_fw_id(next_usb_device_info);
		int ret = load_firmware(handlers, usb_device_info, free_fw_id);
		if(!ret) {
			capture_error_print(true, capture_data, "Firmware load error");
			return false;
		}
		cypress_optimize_new_3ds_connection_end(handlers, usb_device_info);
		std::string new_serial_number = std::to_string(free_fw_id);
		CaptureDevice new_device;
		for(int i = 0; i < 20; i++) {
			default_sleep(500);
			handlers = usb_find_by_serial_number(next_usb_device_info, new_serial_number, &new_device);
			if(handlers != NULL)
				break;
		}
		if(handlers == NULL) {
			capture_error_print(true, capture_data, "Device reconnection error");
			return false;
		}
		*device = new_device;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

uint64_t cyopn_device_get_video_in_size(bool is_3d, InputVideoDataType video_data_type) {
	bool is_rgb888 = video_data_type == VIDEO_DATA_RGB;
	if(is_rgb888) {
		if(is_3d)
			return sizeof(USB888New3DSOptimizeCaptureReceived_3D);
		return sizeof(USB888New3DSOptimizeCaptureReceived);
	}
	if(is_3d)
		return sizeof(USB565New3DSOptimizeCaptureReceived_3D);
	return sizeof(USB565New3DSOptimizeCaptureReceived);
}


uint64_t cyopn_device_get_video_in_size(CaptureStatus* status, bool is_3d, InputVideoDataType video_data_type) {
	return cyopn_device_get_video_in_size(is_3d, video_data_type);
}


uint64_t cyopn_device_get_video_in_size(CaptureData* capture_data, bool is_3d, InputVideoDataType video_data_type) {
	return cyopn_device_get_video_in_size(&capture_data->status, is_3d, video_data_type);
}

void cyopn_device_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
}

void usb_cyopn_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;

	cypress_optimize_new_3ds_connection_end((cy_device_device_handlers*)capture_data->handle, (const cyopn_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}
void usb_cyopn_device_init() {
	return usb_init();
}

void usb_cyopn_device_close() {
	usb_close();
}


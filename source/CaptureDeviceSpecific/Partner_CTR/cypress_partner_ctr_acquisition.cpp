#include "devicecapture.hpp"
#include "cypress_shared_driver_comms.hpp"
#include "cypress_shared_libusb_comms.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_partner_ctr_communications.hpp"
#include "cypress_partner_ctr_acquisition.hpp"
#include "cypress_partner_ctr_acquisition_general.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>

#define PARTNER_CTR_CYPRESS_USB_WINDOWS_DRIVER CYPRESS_WINDOWS_DEFAULT_USB_DRIVER

static cy_device_device_handlers* usb_reconnect(const cypart_device_usb_device* usb_device_desc, CaptureDevice* device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), device->serial_number, curr_serial_extra_id, NULL);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_serial_reconnection(device);
	return final_handlers;
}

static std::string _cypress_partner_ctr_get_serial(cy_device_device_handlers* handlers, const cypart_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	serial = read_serial_ctr_capture(handlers, usb_device_desc);
	if(serial != "")
		return serial;
	return std::to_string((curr_serial_extra_id++) + 1);
}

std::string cypress_partner_ctr_get_serial(cy_device_device_handlers* handlers, const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if(usb_device_desc == NULL)
		return "";
	if(handlers == NULL)
		return "";
	const cypart_device_usb_device* in_usb_device_desc = (const cypart_device_usb_device*)usb_device_desc;
	return _cypress_partner_ctr_get_serial(handlers, in_usb_device_desc, serial, bcd_device, curr_serial_extra_id);
}

static CaptureDevice _cypress_partner_ctr_create_device(const cypart_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	CaptureDevice out_device = CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_PARTNER_CTR, (void*)usb_device_desc, true, true, true, TOP_WIDTH_3DS, HEIGHT_3DS * 2, TOP_WIDTH_3DS, HEIGHT_3DS * 3, N3DSXL_SAMPLES_IN, 180, 0, HEIGHT_3DS, 0, 2 * HEIGHT_3DS, 0, 0, false, usb_device_desc->video_data_type, 0x200, path);
	out_device.is_horizontally_flipped = true;
	out_device.continuous_3d_screens = false;
	return out_device;
}

CaptureDevice cypress_partner_ctr_create_device(const void* usb_device_desc, std::string serial, std::string path) {
	if(usb_device_desc == NULL)
		return CaptureDevice();
	return _cypress_partner_ctr_create_device((const cypart_device_usb_device*)usb_device_desc, serial, path);
}

static void cypress_partner_ctr_connection_end(cy_device_device_handlers* handlers, const cypart_device_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		cypress_libusb_end_connection(handlers, get_cy_usb_info(device_desc), interface_claimed);
	else
		cypress_driver_end_connection(handlers);
	delete handlers;
}

void list_devices_cypart_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan) {
	const size_t num_cypart_device_desc = GetNumCyPartnerCTRDeviceDesc();
	int* curr_serial_extra_id_cypart_device = new int[num_cypart_device_desc];
	bool* no_access_elems = new bool[num_cypart_device_desc];
	bool* not_supported_elems = new bool[num_cypart_device_desc];
	std::vector<const cy_device_usb_device*> usb_devices_to_check;
	for (size_t i = 0; i < num_cypart_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_cypart_device[i] = 0;
		const cypart_device_usb_device* curr_device_desc = GetCyPartnerCTRDeviceDesc((int)i);
		if(devices_allowed_scan[curr_device_desc->index_in_allowed_scan])
			usb_devices_to_check.push_back(get_cy_usb_info(curr_device_desc));
	}
	cypress_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_cypart_device, usb_devices_to_check);

	bool any_not_supported = false;
	for(size_t i = 0; i < num_cypart_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(size_t i = 0; i < num_cypart_device_desc; i++)
		if(no_access_elems[i])
			no_access_list.emplace_back(usb_devices_to_check[i]->vid, usb_devices_to_check[i]->pid);
	if(any_not_supported)
		cypress_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_cypart_device, usb_devices_to_check, PARTNER_CTR_CYPRESS_USB_WINDOWS_DRIVER);

	delete[] curr_serial_extra_id_cypart_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

bool cypart_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const cypart_device_usb_device* usb_device_info = (const cypart_device_usb_device*)device->descriptor;
	cy_device_device_handlers* handlers = usb_reconnect(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

uint64_t cypart_device_get_video_in_size(CaptureStatus* status) {
	return 0;
}


uint64_t cypart_device_get_video_in_size(CaptureData* capture_data) {
	return cypart_device_get_video_in_size(&capture_data->status);
}

void cypart_device_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
}

void usb_cypart_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	cypress_partner_ctr_connection_end((cy_device_device_handlers*)capture_data->handle, (const cypart_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}
void usb_cypart_device_init() {
	return usb_init();
}

void usb_cypart_device_close() {
	usb_close();
}


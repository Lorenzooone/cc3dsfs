#include "devicecapture.hpp"
#include "usb_partner_ctr_setup_general.hpp"
#include "usb_partner_ctr_libusb.hpp"
#include "usb_partner_ctr_driver.hpp"
#include "usb_partner_ctr_communications.hpp"
#include "usb_partner_ctr_acquisition.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>

#define SERIAL_NUMBER_SIZE (PARTNER_CTR_REAL_SERIAL_NUMBER_SIZE + 1)
#define MAX_TIME_WAIT 1.0
#define FRAME_BUFFER_SIZE 32

std::string get_serial(const partner_ctr_usb_device* usb_device_desc, partner_ctr_device_handlers* handlers, int& curr_serial_extra_id) {
	uint8_t data[SERIAL_NUMBER_SIZE];
	std::string serial_str = std::to_string(curr_serial_extra_id);
	bool conn_success = true;
	//if(conn_success && (GetPartnerCTRDeviceSerial(handlers, data, usb_device_desc) != LIBUSB_SUCCESS))
	//	conn_success = false;
	if(conn_success) {
		data[PARTNER_CTR_REAL_SERIAL_NUMBER_SIZE] = '\0';
		serial_str = std::string((const char*)data);
	}
	else
		curr_serial_extra_id += 1;
	return serial_str;
}

void partner_ctr_insert_device(std::vector<CaptureDevice>& devices_list, partner_ctr_device_handlers* handlers, const partner_ctr_usb_device* usb_device_desc, int& curr_serial_extra_id_partner_ctr, std::string path) {
	devices_list.emplace_back(get_serial(usb_device_desc, handlers, curr_serial_extra_id_partner_ctr), usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_PARTNER_CTR, (void*)usb_device_desc, false, false, true, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, usb_device_desc->max_audio_samples_size, 0, 0, 0, 0, HEIGHT_DS, usb_device_desc->video_data_type, path);
}

void partner_ctr_insert_device(std::vector<CaptureDevice>& devices_list, partner_ctr_device_handlers* handlers, const partner_ctr_usb_device* usb_device_desc, int& curr_serial_extra_id_partner_ctr) {
	partner_ctr_insert_device(devices_list, handlers, usb_device_desc, curr_serial_extra_id_partner_ctr, "");
}

static partner_ctr_device_handlers* usb_find_by_serial_number(const partner_ctr_usb_device* usb_device_desc, CaptureDevice* device) {
	partner_ctr_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = partner_ctr_libusb_serial_reconnection(usb_device_desc, device, curr_serial_extra_id);

	if(final_handlers == NULL)
		final_handlers = partner_ctr_driver_serial_reconnection(device);
	return final_handlers;
}

void list_devices_partner_ctr(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan) {
	const size_t num_partner_ctr_desc = GetNumPartnerCTRDeviceDesc();
	int* curr_serial_extra_id_partner_ctr = new int[num_partner_ctr_desc];
	bool* no_access_elems = new bool[num_partner_ctr_desc];
	bool* not_supported_elems = new bool[num_partner_ctr_desc];
	std::vector<const partner_ctr_usb_device*> device_descriptions;
	for(size_t i = 0; i < num_partner_ctr_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_partner_ctr[i] = 0;
		const partner_ctr_usb_device* usb_device_desc = GetPartnerCTRDeviceDesc((int)i);
		if(devices_allowed_scan[usb_device_desc->index_in_allowed_scan])
			device_descriptions.push_back(usb_device_desc);
			
	}
	partner_ctr_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_partner_ctr, device_descriptions);

	bool any_not_supported = false;
	for(size_t i = 0; i < num_partner_ctr_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(size_t i = 0; i < num_partner_ctr_desc; i++)
		if(no_access_elems[i]) {
			const partner_ctr_usb_device* usb_device = device_descriptions[i];
			no_access_list.emplace_back(usb_device->vid, usb_device->pid);
		}
	if(any_not_supported)
		partner_ctr_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_partner_ctr, device_descriptions);

	delete[] curr_serial_extra_id_partner_ctr;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

static void partner_ctr_connection_end(partner_ctr_device_handlers* handlers, const partner_ctr_usb_device *device_desc, bool interface_claimed = true) {
	if(handlers == NULL)
		return;
	if(handlers->usb_handle)
		partner_ctr_libusb_end_connection(handlers, device_desc, interface_claimed);
	else
		partner_ctr_driver_end_connection(handlers);
	delete handlers;
}

bool partner_ctr_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const partner_ctr_usb_device* usb_device_info = (const partner_ctr_usb_device*)device->descriptor;
	partner_ctr_device_handlers* handlers = usb_find_by_serial_number(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

void partner_ctr_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
}

void usb_partner_ctr_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	//for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
	//	capture_data->data_buffers.ReleaseWriterBuffer(i, false);
	partner_ctr_connection_end((partner_ctr_device_handlers*)capture_data->handle, (const partner_ctr_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}

void usb_partner_ctr_init() {
	return usb_init();
}

void usb_partner_ctr_close() {
	usb_close();
}


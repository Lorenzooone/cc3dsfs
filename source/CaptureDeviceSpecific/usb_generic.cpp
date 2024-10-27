#include "usb_generic.hpp"

static bool usb_initialized = false;
static libusb_context* usb_ctx = NULL; // libusb session context

void usb_init() {
	if(usb_initialized)
		return;
	int result = libusb_init(&usb_ctx); // open session
	if (result < 0) {
		usb_ctx = NULL;
		usb_initialized = false;
		return;
	}
	usb_initialized = true;
	return;
}

void usb_close() {
	if(usb_initialized) {
		libusb_exit(usb_ctx);
	}
	usb_ctx = NULL;
	usb_initialized = false;
}

bool usb_is_initialized() {
	return usb_initialized;
}

libusb_context* get_usb_ctx() {
	return usb_ctx;
}

int get_usb_total_filtered_devices(const uint16_t valid_vids[], size_t num_vids, const uint16_t valid_pids[], size_t num_pids) {
	if(!usb_is_initialized())
		return 0;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	int num_devices_found = 0;

	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		bool found_vid = false;
		for(int j = 0; j < num_vids; j++)
			if(usb_descriptor.idVendor == valid_vids[j]) {
				found_vid = true;
				break;
			}
		if(!found_vid)
			continue;
		for(int j = 0; j < num_pids; j++) {
			if(usb_descriptor.idProduct == valid_pids[j]) {
				num_devices_found += 1;
				break;
			}
		}
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);

	return num_devices_found;
}

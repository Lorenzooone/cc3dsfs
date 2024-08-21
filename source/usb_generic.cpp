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

#ifndef __USB_GENERIC_HPP
#define __USB_GENERIC_HPP

#include <libusb.h>

void usb_init();
void usb_close();
bool usb_is_initialized();
libusb_context* get_usb_ctx();

#endif

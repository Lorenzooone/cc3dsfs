#ifndef __USB_GENERIC_HPP
#define __USB_GENERIC_HPP

#ifdef USE_LIBUSB
#include <libusb.h>
libusb_context* get_usb_ctx();
#endif

void usb_init();
void usb_close();
bool usb_is_initialized();
void libusb_check_and_detach_kernel_driver(void* handle, int interface);
int libusb_check_and_set_configuration(void* handle, int wanted_configuration);

void libusb_register_to_event_thread();
void libusb_unregister_from_event_thread();

#endif

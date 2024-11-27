#ifndef __USB_GENERIC_HPP
#define __USB_GENERIC_HPP

#if defined(USE_IS_NITRO_USB) || defined(USE_DS_3DS_USB)
#include <libusb.h>
libusb_context* get_usb_ctx();
#endif

void usb_init();
void usb_close();
bool usb_is_initialized();
int get_usb_total_filtered_devices(const uint16_t valid_vids[], size_t num_vids, const uint16_t valid_pids[], size_t num_pids);

#endif

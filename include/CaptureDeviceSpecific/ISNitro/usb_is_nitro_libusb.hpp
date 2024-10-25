#ifndef __USB_IS_NITRO_LIBUSB_HPP
#define __USB_IS_NITRO_LIBUSB_HPP

#include <vector>
#include "capture_structs.hpp"
#include "usb_is_nitro_communications.hpp"

void is_nitro_libusb_list_devices(std::vector<CaptureDevice>& devices_list, bool* no_access_elems, bool* not_supported_elems, int* curr_serial_extra_id_is_nitro, const size_t num_is_nitro_desc);
is_nitro_device_handlers* is_nitro_libusb_serial_reconnection(const is_nitro_usb_device* usb_device_desc, CaptureDevice* device, int& curr_serial_extra_id);
void is_nitro_libusb_end_connection(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc, bool interface_claimed);
int is_nitro_libusb_bulk_out(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int is_nitro_libusb_bulk_in(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);

#endif
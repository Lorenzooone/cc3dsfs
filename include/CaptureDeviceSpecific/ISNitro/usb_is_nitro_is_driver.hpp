#ifndef __USB_IS_NITRO_IS_DRIVER_HPP
#define __USB_IS_NITRO_IS_DRIVER_HPP

#include <vector>
#include "capture_structs.hpp"
#include "usb_is_nitro_communications.hpp"

void is_driver_list_devices(std::vector<CaptureDevice>& devices_list, bool* not_supported_elems, int* curr_serial_extra_id_is_nitro, const size_t num_is_nitro_desc);
is_nitro_device_handlers* is_driver_serial_reconnection(const is_nitro_usb_device* usb_device_desc, CaptureDevice* device);
void is_driver_end_connection(is_nitro_device_handlers* handlers);
int is_driver_bulk_out(is_nitro_device_handlers* handlers, uint8_t* buf, int length, int* transferred);
int is_driver_bulk_in(is_nitro_device_handlers* handlers, uint8_t* buf, int length, int* transferred);

#endif
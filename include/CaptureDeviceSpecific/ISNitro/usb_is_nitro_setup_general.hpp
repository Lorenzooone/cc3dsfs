#ifndef __USB_IS_NITRO_SETUP_GENERAL_HPP
#define __USB_IS_NITRO_SETUP_GENERAL_HPP

#include <vector>
#include "capture_structs.hpp"
#include "usb_is_nitro_communications.hpp"

std::string get_serial(const is_nitro_usb_device* usb_device_desc, is_nitro_device_handlers* handlers, int& curr_serial_extra_id);
void is_nitro_insert_device(std::vector<CaptureDevice>& devices_list, is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, int& curr_serial_extra_id_is_nitro, std::string path);
void is_nitro_insert_device(std::vector<CaptureDevice>& devices_list, is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, int& curr_serial_extra_id_is_nitro);

#endif
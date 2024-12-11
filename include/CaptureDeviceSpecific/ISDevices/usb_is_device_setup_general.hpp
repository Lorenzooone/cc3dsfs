#ifndef __USB_IS_DEVICE_SETUP_GENERAL_HPP
#define __USB_IS_DEVICE_SETUP_GENERAL_HPP

#include <vector>
#include "capture_structs.hpp"
#include "usb_is_device_communications.hpp"

std::string get_serial(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers, int& curr_serial_extra_id);
void is_device_insert_device(std::vector<CaptureDevice>& devices_list, is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, int& curr_serial_extra_id_is_device, std::string path);
void is_device_insert_device(std::vector<CaptureDevice>& devices_list, is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, int& curr_serial_extra_id_is_device);

#endif

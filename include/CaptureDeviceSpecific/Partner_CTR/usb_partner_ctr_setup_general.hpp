#ifndef __USB_PARTNER_CTR_SETUP_GENERAL_HPP
#define __USB_PARTNER_CTR_SETUP_GENERAL_HPP

#include <vector>
#include "capture_structs.hpp"
#include "usb_partner_ctr_communications.hpp"

std::string get_serial(const partner_ctr_usb_device* usb_device_desc, partner_ctr_device_handlers* handlers, int& curr_serial_extra_id);
void partner_ctr_insert_device(std::vector<CaptureDevice>& devices_list, partner_ctr_device_handlers* handlers, const partner_ctr_usb_device* usb_device_desc, int& curr_serial_extra_id_partner_ctr, std::string path);
void partner_ctr_insert_device(std::vector<CaptureDevice>& devices_list, partner_ctr_device_handlers* handlers, const partner_ctr_usb_device* usb_device_desc, int& curr_serial_extra_id_partner_ctr);

#endif

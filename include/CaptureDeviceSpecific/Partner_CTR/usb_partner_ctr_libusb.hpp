#ifndef __USB_PARTNER_CTR_LIBUSB_HPP
#define __USB_PARTNER_CTR_LIBUSB_HPP

#include <vector>
#include "capture_structs.hpp"
#include "usb_partner_ctr_communications.hpp"

void partner_ctr_libusb_list_devices(std::vector<CaptureDevice>& devices_list, bool* no_access_elems, bool* not_supported_elems, int* curr_serial_extra_id_partner_ctr, std::vector<const partner_ctr_usb_device*> &device_descriptions);
partner_ctr_device_handlers* partner_ctr_libusb_serial_reconnection(const partner_ctr_usb_device* usb_device_desc, CaptureDevice* device, int& curr_serial_extra_id);
void partner_ctr_libusb_end_connection(partner_ctr_device_handlers* handlers, const partner_ctr_usb_device* device_desc, bool interface_claimed);

#endif

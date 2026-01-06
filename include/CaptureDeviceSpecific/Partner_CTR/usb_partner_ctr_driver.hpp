#ifndef __USB_PARTNER_CTR_DRIVER_HPP
#define __USB_PARTNER_CTR_DRIVER_HPP

#include <vector>
#include "capture_structs.hpp"
#include "utils.hpp"
#include "usb_partner_ctr_communications.hpp"

void partner_ctr_driver_list_devices(std::vector<CaptureDevice>& devices_list, bool* not_supported_elems, int* curr_serial_extra_id_partner_ctr, std::vector<const partner_ctr_usb_device*> &device_descriptions);
partner_ctr_device_handlers* partner_ctr_driver_serial_reconnection(CaptureDevice* device);
void partner_ctr_driver_end_connection(partner_ctr_device_handlers* handlers);

#endif

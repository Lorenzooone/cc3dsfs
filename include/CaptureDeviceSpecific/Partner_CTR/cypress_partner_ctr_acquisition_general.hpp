#ifndef __CYPRESS_PARTNER_CTR_ACQUISITION_GENERAL_HPP
#define __CYPRESS_PARTNER_CTR_ACQUISITION_GENERAL_HPP

#include "cypress_shared_communications.hpp"
#include "cypress_partner_ctr_communications.hpp"

std::string cypress_partner_ctr_get_serial(cy_device_device_handlers* handlers, const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id);
CaptureDevice cypress_partner_ctr_create_device(const void* usb_device_desc, std::string serial, std::string path);

#endif

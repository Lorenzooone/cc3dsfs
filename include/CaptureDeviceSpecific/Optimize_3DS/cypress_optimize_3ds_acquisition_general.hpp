#ifndef __CYPRESS_OPTIMIZE_3DS_ACQUISITION_GENERAL_HPP
#define __CYPRESS_OPTIMIZE_3DS_ACQUISITION_GENERAL_HPP

#include "cypress_shared_communications.hpp"
#include "cypress_optimize_3ds_communications.hpp"

std::string cypress_optimize_3ds_get_serial(const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id);
CaptureDevice cypress_optimize_3ds_create_device(const void* usb_device_desc, std::string serial, std::string path);

#endif

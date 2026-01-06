#ifndef __USB_PARTNER_CTR_ACQUISITION_HPP
#define __USB_PARTNER_CTR_ACQUISITION_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

void list_devices_partner_ctr(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan);
bool partner_ctr_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void partner_ctr_acquisition_main_loop(CaptureData* capture_data);
void usb_partner_ctr_acquisition_cleanup(CaptureData* capture_data);
void usb_partner_ctr_init();
void usb_partner_ctr_close();

#endif

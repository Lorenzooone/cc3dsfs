#ifndef __CYPRESS_PARTNER_CTR_ACQUISITION_HPP
#define __CYPRESS_PARTNER_CTR_ACQUISITION_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

void list_devices_cypart_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan);
bool cypart_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
uint64_t cypart_device_get_video_in_size(CaptureStatus* status);
uint64_t cypart_device_get_video_in_size(CaptureData* capture_data);
void cypart_device_acquisition_main_loop(CaptureData* capture_data);
void usb_cypart_device_acquisition_cleanup(CaptureData* capture_data);
void usb_cypart_device_init();
void usb_cypart_device_close();

#endif

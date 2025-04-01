#ifndef __CYPRESS_OPTIMIZE_NEW_3DS_ACQUISITION_HPP
#define __CYPRESS_OPTIMIZE_NEW_3DS_ACQUISITION_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

void list_devices_cyopn_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list);
bool cyopn_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
uint64_t cyopn_device_get_video_in_size(CaptureStatus* status, bool is_3d, InputVideoDataType video_data_type);
uint64_t cyopn_device_get_video_in_size(CaptureData* capture_data, bool is_3d, InputVideoDataType video_data_type);
void cyopn_device_acquisition_main_loop(CaptureData* capture_data);
void usb_cyopn_device_acquisition_cleanup(CaptureData* capture_data);
void usb_cyopn_device_init();
void usb_cyopn_device_close();

#endif

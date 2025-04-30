#ifndef __USB_DS_3DS_CAPTURE_HPP
#define __USB_DS_3DS_CAPTURE_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

#define SIMPLE_DS_FRAME_SKIP

void list_devices_usb_ds_3ds(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan);
bool connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void usb_capture_main_loop(CaptureData* capture_data);
void usb_capture_cleanup(CaptureData* capture_data);
uint64_t usb_get_video_in_size(CaptureData* capture_data);
uint64_t usb_get_video_in_size(CaptureData* capture_data, bool override_3d);
void usb_ds_3ds_init();
void usb_ds_3ds_close();

#endif

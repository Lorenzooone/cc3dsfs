#ifndef __USB_DS_3DS_CAPTURE_HPP
#define __USB_DS_3DS_CAPTURE_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

#define USE_USB

void list_devices_usb_ds_3ds(std::vector<CaptureDevice> &devices_list);
bool connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void usb_capture_main_loop(CaptureData* capture_data);
void usb_capture_cleanup(CaptureData* capture_data);
void usb_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureDevice* capture_device, bool enabled_3d);
uint64_t usb_get_video_in_size(CaptureData* capture_data);
void usb_init();
void usb_close();

#endif

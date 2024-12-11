#ifndef __USB_IS_DEVICE_ACQUISITION_HPP
#define __USB_IS_DEVICE_ACQUISITION_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

void list_devices_is_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list);
bool is_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void is_device_acquisition_main_loop(CaptureData* capture_data);
void usb_is_device_acquisition_cleanup(CaptureData* capture_data);
uint64_t usb_is_device_get_video_in_size(CaptureData* capture_data);
uint64_t usb_is_device_get_video_in_size(CaptureStatus* capture_status);
bool is_device_is_capture(CaptureDevice* device);
bool is_device_is_nitro(CaptureDevice* device);
void usb_is_device_init();
void usb_is_device_close();

#endif

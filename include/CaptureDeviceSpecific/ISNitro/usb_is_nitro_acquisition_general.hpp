#ifndef __USB_IS_NITRO_ACQUISITION_GENERAL_HPP
#define __USB_IS_NITRO_ACQUISITION_GENERAL_HPP

#include "usb_is_nitro_communications.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

uint64_t _is_nitro_get_video_in_size(CaptureScreensType capture_type);
int set_acquisition_mode(is_nitro_device_handlers* handlers, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, const is_nitro_usb_device* usb_device_desc);
int EndAcquisition(is_nitro_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type, const is_nitro_usb_device* usb_device_desc);
int is_nitro_read_frame_and_output(CaptureData* capture_data, int& inner_curr_in, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start);

#endif

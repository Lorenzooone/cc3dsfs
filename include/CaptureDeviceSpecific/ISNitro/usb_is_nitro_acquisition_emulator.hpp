#ifndef __USB_IS_NITRO_ACQUISITION_EMULATOR_HPP
#define __USB_IS_NITRO_ACQUISITION_EMULATOR_HPP

#include "usb_is_nitro_communications.hpp"
#include "capture_structs.hpp"

int initial_cleanup_emulator(const is_nitro_usb_device* usb_device_desc, is_nitro_device_handlers* handlers);
int EndAcquisitionEmulator(is_nitro_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type, const is_nitro_usb_device* usb_device_desc);
void is_nitro_acquisition_emulator_main_loop(CaptureData* capture_data, CaptureReceived* capture_buf);

#endif

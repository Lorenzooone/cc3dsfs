#ifndef __USB_IS_NITRO_ACQUISITION_EMULATOR_HPP
#define __USB_IS_NITRO_ACQUISITION_EMULATOR_HPP

#include "usb_is_device_communications.hpp"
#include "capture_structs.hpp"

int initial_cleanup_emulator(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers);
int EndAcquisitionEmulator(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_nitro_capture_recv_data, bool do_drain_frames, int start_frames, CaptureScreensType capture_type);
int EndAcquisitionEmulator(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type);
void is_nitro_acquisition_emulator_main_loop(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_nitro_capture_recv_data);

#endif

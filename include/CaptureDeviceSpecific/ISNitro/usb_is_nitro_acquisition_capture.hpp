#ifndef __USB_IS_NITRO_ACQUISITION_CAPTURE_HPP
#define __USB_IS_NITRO_ACQUISITION_CAPTURE_HPP

#include "usb_is_nitro_communications.hpp"
#include "capture_structs.hpp"

int initial_cleanup_capture(const is_nitro_usb_device* usb_device_desc, is_nitro_device_handlers* handlers);
int EndAcquisitionCapture(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc);
void is_nitro_acquisition_capture_main_loop(CaptureData* capture_data, CaptureReceived* capture_buf);

#endif

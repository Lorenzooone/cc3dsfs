#ifndef __USB_IS_TWL_ACQUISITION_CAPTURE_HPP
#define __USB_IS_TWL_ACQUISITION_CAPTURE_HPP

#include "usb_is_device_communications.hpp"
#include "capture_structs.hpp"

int initial_cleanup_twl_capture(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers);
int EndAcquisitionTWLCapture(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_twl_capture_recv_data);
int EndAcquisitionTWLCapture(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers);
void is_twl_acquisition_capture_main_loop(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_twl_capture_recv_data);

#endif

#include "devicecapture.hpp"
#include "usb_is_nitro_communications.hpp"
#include "usb_is_nitro_acquisition_general.hpp"
#include "usb_is_nitro_acquisition_capture.hpp"

// Code created by analyzing the USB packets sent and received by the IS Nitro Capture device.

// The code for video capture of the IS Nitro Emulator and of the IS Nitro Capture is wildly different.
// For this reason, the code was split into two device-specific files.

#define CAPTURE_SKIP_LID_REOPEN_FRAMES 32

static int StartAcquisitionCapture(is_nitro_device_handlers* handlers, CaptureScreensType capture_type, uint32_t* lid_state, const is_nitro_usb_device* usb_device_desc) {
	int ret = 0;
	ret = ReadLidState(handlers, lid_state, usb_device_desc);
	if(ret < 0)
		return ret;
	if((*lid_state) & 1)
		return LIBUSB_SUCCESS;
	ret = set_acquisition_mode(handlers, capture_type, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = SetForwardFramePermanent(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = UpdateFrameForwardEnable(handlers, true, true, usb_device_desc);
	if(ret < 0)
		return ret;
	return StartUsbCaptureDma(handlers, usb_device_desc);
}

static int LidReopenCaptureCheck(CaptureData* capture_data, CaptureScreensType &curr_capture_type, uint32_t* lid_state) {
	is_nitro_device_handlers* handlers = (is_nitro_device_handlers*)capture_data->handle;
	const is_nitro_usb_device* usb_device_desc = (const is_nitro_usb_device*)capture_data->status.device.descriptor;
	curr_capture_type = capture_data->status.capture_type;
	int ret = StartAcquisitionCapture(handlers, curr_capture_type, lid_state, usb_device_desc);
	if(ret < 0)
		return ret;
	if(!((*lid_state) & 1))
		capture_data->status.cooldown_curr_in = CAPTURE_SKIP_LID_REOPEN_FRAMES;
	return ret;
}

int initial_cleanup_capture(const is_nitro_usb_device* usb_device_desc, is_nitro_device_handlers* handlers) {
	//EndAcquisition(handlers, false, 0, CAPTURE_SCREENS_BOTH, usb_device_desc);
	return LIBUSB_SUCCESS;
}

int EndAcquisitionCapture(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc) {
	return StopUsbCaptureDma(handlers, usb_device_desc);
}

void is_nitro_acquisition_capture_main_loop(CaptureData* capture_data) {
	is_nitro_device_handlers* handlers = (is_nitro_device_handlers*)capture_data->handle;
	const is_nitro_usb_device* usb_device_desc = (const is_nitro_usb_device*)capture_data->status.device.descriptor;
	uint32_t lid_state = 0;
	CaptureScreensType curr_capture_type = capture_data->status.capture_type;
	int ret = StartAcquisitionCapture(handlers, curr_capture_type, &lid_state, usb_device_desc);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
	int inner_curr_in = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	auto clock_last_reset = std::chrono::high_resolution_clock::now();

	while(capture_data->status.connected && capture_data->status.running) {
		if(lid_state & 1) {
			default_sleep(20);
			ret = LidReopenCaptureCheck(capture_data, curr_capture_type, &lid_state);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Lid Reopen: Failed");
				return;
			}
		}
		if(lid_state & 1)
			continue;
		ret = is_nitro_read_frame_and_output(capture_data, inner_curr_in, curr_capture_type, clock_start);
		if(ret < 0) {
			ret = EndAcquisition(handlers, true, 0, curr_capture_type, usb_device_desc);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Disconnected: Read error");
				return;
			}
			ret = LidReopenCaptureCheck(capture_data, curr_capture_type, &lid_state);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Lid Reopen: Failed");
				return;
			}
			continue;
		}

		if(curr_capture_type != capture_data->status.capture_type) {
			ret = EndAcquisition(handlers, true, 0, curr_capture_type, usb_device_desc);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Capture End: Failed");
				return;
			}
			curr_capture_type = capture_data->status.capture_type;
			ret = StartAcquisitionCapture(handlers, curr_capture_type, &lid_state, usb_device_desc);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Capture Restart: Failed");
				return;
			}
		}
	}
	if(!(lid_state & 1))
		EndAcquisition(handlers, true, 0, curr_capture_type, usb_device_desc);
}

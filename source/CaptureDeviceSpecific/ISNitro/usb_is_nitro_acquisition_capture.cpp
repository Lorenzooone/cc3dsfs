#include "devicecapture.hpp"
#include "usb_is_nitro_communications.hpp"
#include "usb_is_nitro_acquisition_general.hpp"
#include "usb_is_nitro_acquisition_capture.hpp"

// Code created by analyzing the USB packets sent and received by the IS Nitro Capture device.

// The code for video capture of the IS Nitro Emulator and of the IS Nitro Capture is wildly different.
// For this reason, the code was split into two device-specific files.

#define CAPTURE_SKIP_LID_REOPEN_FRAMES 32
#define RESET_TIMEOUT 4.0
#define SLEEP_CHECKS_TIME 20

static int StartAcquisitionCapture(is_nitro_device_handlers* handlers, CaptureScreensType capture_type, bool* is_acquisition_off, const is_nitro_usb_device* usb_device_desc) {
	int ret = 0;
	ret = ReadLidState(handlers, is_acquisition_off, usb_device_desc);
	if(ret < 0)
		return ret;
	if(*is_acquisition_off)
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

static int LidReopenCaptureCheck(CaptureData* capture_data, CaptureScreensType &curr_capture_type, bool* is_acquisition_off) {
	is_nitro_device_handlers* handlers = (is_nitro_device_handlers*)capture_data->handle;
	const is_nitro_usb_device* usb_device_desc = (const is_nitro_usb_device*)capture_data->status.device.descriptor;
	curr_capture_type = capture_data->status.capture_type;
	int ret = StartAcquisitionCapture(handlers, curr_capture_type, is_acquisition_off, usb_device_desc);
	if(ret < 0)
		return ret;
	if(!(*is_acquisition_off))
		capture_data->status.cooldown_curr_in = CAPTURE_SKIP_LID_REOPEN_FRAMES;
	return ret;
}

static int CaptureResetHardware(CaptureData* capture_data, CaptureScreensType &curr_capture_type, bool *is_acquisition_off, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_reset) {
	is_nitro_device_handlers* handlers = (is_nitro_device_handlers*)capture_data->handle;
	const is_nitro_usb_device* usb_device_desc = (const is_nitro_usb_device*)capture_data->status.device.descriptor;
	bool reset_hardware = capture_data->status.reset_hardware;
	capture_data->status.reset_hardware = false;
	int ret = LIBUSB_SUCCESS;
	if(!reset_hardware)
		return ret;
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - clock_last_reset;
	// Do not reset too fast... In general.
	if(diff.count() < RESET_TIMEOUT)
		return ret;
	clock_last_reset = curr_time;

	if(!(*is_acquisition_off))
		ret = EndAcquisitionCapture(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = ResetCPUStart(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = ResetCPUEnd(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	if(!(*is_acquisition_off)) {
		curr_capture_type = capture_data->status.capture_type;
		ret = StartAcquisitionCapture(handlers, curr_capture_type, is_acquisition_off, usb_device_desc);
	}
	if(ret < 0)
		return ret;
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
	bool is_acquisition_off = true;
	CaptureScreensType curr_capture_type = capture_data->status.capture_type;
	int ret = StartAcquisitionCapture(handlers, curr_capture_type, &is_acquisition_off, usb_device_desc);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
	int inner_curr_in = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	auto clock_last_reset = std::chrono::high_resolution_clock::now();

	while(capture_data->status.connected && capture_data->status.running) {
		ret = CaptureResetHardware(capture_data, curr_capture_type, &is_acquisition_off, clock_last_reset);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Hardware reset: Failed");
			return;
		}
		if(is_acquisition_off) {
			default_sleep(SLEEP_CHECKS_TIME);
			ret = LidReopenCaptureCheck(capture_data, curr_capture_type, &is_acquisition_off);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Lid Reopen: Failed");
				return;
			}
		}
		if(is_acquisition_off)
			continue;
		ret = is_nitro_read_frame_and_output(capture_data, inner_curr_in, curr_capture_type, clock_start);
		if(ret < 0) {
			ret = EndAcquisition(handlers, true, 0, curr_capture_type, usb_device_desc);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Disconnected: Read error");
				return;
			}
			is_acquisition_off = true;
			continue;
		}

		if(curr_capture_type != capture_data->status.capture_type) {
			ret = EndAcquisition(handlers, true, 0, curr_capture_type, usb_device_desc);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Capture End: Failed");
				return;
			}
			curr_capture_type = capture_data->status.capture_type;
			ret = StartAcquisitionCapture(handlers, curr_capture_type, &is_acquisition_off, usb_device_desc);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Capture Restart: Failed");
				return;
			}
		}
	}
	if(!is_acquisition_off)
		EndAcquisition(handlers, true, 0, curr_capture_type, usb_device_desc);
}

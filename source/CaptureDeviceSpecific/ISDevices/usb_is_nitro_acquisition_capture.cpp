#include "devicecapture.hpp"
#include "usb_is_device_communications.hpp"
#include "usb_is_device_acquisition_general.hpp"
#include "usb_is_nitro_acquisition_capture.hpp"

// Code created by analyzing the USB packets sent and received by the IS Nitro Capture device.

// The code for video capture of the IS Nitro Emulator and of the IS Nitro Capture is wildly different.
// For this reason, the code was split into two device-specific files.

#define CAPTURE_SKIP_LID_REOPEN_FRAMES 32
#define RESET_TIMEOUT 4.0
#define SLEEP_CHECKS_TIME_MS 20
#define SLEEP_RESET_TIME_MS 2000

static int StartAcquisitionCapture(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, bool* is_acquisition_off, uint32_t &index) {
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
	int ret = 0;
	ret = ReadLidState(handlers, is_acquisition_off, usb_device_desc);
	if(ret < 0)
		return ret;
	if(*is_acquisition_off)
		return LIBUSB_SUCCESS;
	ret = set_acquisition_mode(handlers, capture_type, capture_speed, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = SetForwardFramePermanent(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = UpdateFrameForwardEnable(handlers, true, true, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = StartUsbCaptureDma(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	reset_is_device_status(is_device_capture_recv_data);
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		is_device_read_frame_request(capture_data, is_device_get_free_buffer(capture_data, is_device_capture_recv_data), capture_type, index++);
	return ret;
}

static int LidReopenCaptureCheck(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, CaptureScreensType &curr_capture_type, CaptureSpeedsType &curr_capture_speed, bool* is_acquisition_off, uint32_t &index) {
	curr_capture_type = capture_data->status.capture_type;
	curr_capture_speed = capture_data->status.capture_speed;
	int ret = StartAcquisitionCapture(capture_data, is_device_capture_recv_data, curr_capture_type, curr_capture_speed, is_acquisition_off, index);
	if(ret < 0)
		return ret;
	if(!(*is_acquisition_off))
		capture_data->status.cooldown_curr_in = CAPTURE_SKIP_LID_REOPEN_FRAMES;
	return ret;
}

static int CaptureResetHardware(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, CaptureScreensType &curr_capture_type, CaptureSpeedsType &curr_capture_speed, bool *is_acquisition_off, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_reset, uint32_t &index) {
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
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
		ret = EndAcquisitionCapture(capture_data, is_device_capture_recv_data);
	if(ret < 0)
		return ret;
	ret = ResetCPUStart(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = ResetCPUEnd(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	if(!(*is_acquisition_off)) {
		default_sleep(SLEEP_RESET_TIME_MS);
		curr_capture_type = capture_data->status.capture_type;
		curr_capture_speed = capture_data->status.capture_speed;
		ret = StartAcquisitionCapture(capture_data, is_device_capture_recv_data, curr_capture_type, curr_capture_speed, is_acquisition_off, index);
	}
	if(ret < 0)
		return ret;
	return ret;
}

int initial_cleanup_capture(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers) {
	//EndAcquisition(handlers, usb_device_desc, false, 0, CAPTURE_SCREENS_BOTH);
	return LIBUSB_SUCCESS;
}

int EndAcquisitionCapture(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	wait_all_is_device_transfers_done(capture_data, is_device_capture_recv_data);
	return EndAcquisitionCapture((const is_device_usb_device*)capture_data->status.device.descriptor, (is_device_device_handlers*)capture_data->handle);
}

int EndAcquisitionCapture(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers) {
	return StopUsbCaptureDma(handlers, usb_device_desc);
}

void is_nitro_acquisition_capture_main_loop(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
	bool is_acquisition_off = true;
	uint32_t index = 0;
	CaptureScreensType curr_capture_type = capture_data->status.capture_type;
	CaptureSpeedsType curr_capture_speed = capture_data->status.capture_speed;
	int ret = StartAcquisitionCapture(capture_data, is_device_capture_recv_data, curr_capture_type, curr_capture_speed, &is_acquisition_off, index);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
	auto clock_last_reset = std::chrono::high_resolution_clock::now();

	while(capture_data->status.connected && capture_data->status.running) {
		ret = CaptureResetHardware(capture_data, is_device_capture_recv_data, curr_capture_type, curr_capture_speed, &is_acquisition_off, clock_last_reset, index);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Hardware reset: Failed");
			return;
		}
		if(is_acquisition_off) {
			default_sleep(SLEEP_CHECKS_TIME_MS);
			ret = LidReopenCaptureCheck(capture_data, is_device_capture_recv_data, curr_capture_type, curr_capture_speed, &is_acquisition_off, index);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Lid Reopen: Failed");
				return;
			}
		}
		if(is_acquisition_off)
			continue;
		wait_one_is_device_buffer_free(capture_data, is_device_capture_recv_data);
		ret = get_is_device_status(is_device_capture_recv_data);
		if(ret < 0) {
			ret = EndAcquisition(capture_data, is_device_capture_recv_data, true, 0, curr_capture_type);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Disconnected: Read error");
				return;
			}
			is_acquisition_off = true;
			continue;
		}
		is_device_read_frame_request(capture_data, is_device_get_free_buffer(capture_data, is_device_capture_recv_data), curr_capture_type, index++);

		if((curr_capture_type != capture_data->status.capture_type) || (curr_capture_speed != capture_data->status.capture_speed)) {
			ret = EndAcquisition(capture_data, is_device_capture_recv_data, true, 0, curr_capture_type);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Capture End: Failed");
				return;
			}
			curr_capture_type = capture_data->status.capture_type;
			curr_capture_speed = capture_data->status.capture_speed;
			ret = StartAcquisitionCapture(capture_data, is_device_capture_recv_data, curr_capture_type, curr_capture_speed, &is_acquisition_off, index);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Capture Restart: Failed");
				return;
			}
		}
	}
	if(!is_acquisition_off)
		EndAcquisition(capture_data, is_device_capture_recv_data, true, 0, curr_capture_type);
}

#include "devicecapture.hpp"
#include "usb_is_device_communications.hpp"
#include "usb_is_device_acquisition_general.hpp"
#include "usb_is_twl_acquisition_capture.hpp"

// Code created by analyzing the USB packets sent and received by the IS TWL Capture device.

#define RESET_TIMEOUT 4.0
#define SLEEP_RESET_TIME_MS 2000
#define DEFAULT_FRAME_TIME_MS 16.7
#define SLEEP_FRAME_DIVIDER 4

static int process_frame_and_read(CaptureData* capture_data, CaptureReceived* capture_buf, CaptureScreensType curr_capture_type, CaptureSpeedsType curr_capture_speed, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, uint32_t &last_read_frame_index, uint32_t video_address, uint32_t video_length, uint32_t audio_address, uint32_t audio_length, bool &processed, float &last_frame_length) {
	processed = false;
	int multiplier = 1;
	switch(curr_capture_speed) {
		case CAPTURE_SPEEDS_HALF:
			multiplier = 2;
			break;
		case CAPTURE_SPEEDS_THIRD:
			multiplier = 3;
			break;
		case CAPTURE_SPEEDS_QUARTER:
			multiplier = 4;
			break;
		default:
			break;
	}
	int num_available_frames = multiplier;
	if(num_available_frames < multiplier)
		return 0;
	processed = true;
	int frame_next = num_available_frames - 1;
	last_read_frame_index += num_available_frames;
	video_address = video_address + (frame_next * sizeof(ISTWLCaptureVideoReceived));
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
	size_t video_processed_size = usb_is_device_get_video_in_size(curr_capture_type, IS_TWL_CAPTURE_DEVICE);
	int ret = ReadFrame(handlers, (uint8_t*)&capture_buf->is_twl_capture_received.video_capture_in, 0, video_processed_size, usb_device_desc);
	if(ret < 0)
		return ret;
	int audio_diff_from_max = usb_device_desc->max_audio_samples_size - audio_length;
	if(audio_diff_from_max >= 0)
		audio_diff_from_max = 0;
	else {
		audio_address -= audio_diff_from_max;
		audio_length = usb_device_desc->max_audio_samples_size;
	}
	ret = ReadFrame(handlers, (uint8_t*)&capture_buf->is_twl_capture_received.audio_capture_in, audio_address, audio_length, usb_device_desc);
	if(ret < 0)
		return ret;
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - (*clock_start);
	last_frame_length = diff.count();
	output_to_thread(capture_data, capture_buf, curr_capture_type, clock_start, video_processed_size + (audio_length / sizeof(ISTWLCaptureAudioReceived)) * (sizeof(ISTWLCaptureAudioReceived) - sizeof(uint32_t)));
	return 0;
}

int initial_cleanup_twl_capture(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers) {
	//EndAcquisition(handlers, usb_device_desc, false, 0, CAPTURE_SCREENS_BOTH);
	return LIBUSB_SUCCESS;
}

int EndAcquisitionTWLCapture(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	wait_all_is_device_transfers_done(capture_data, is_device_capture_recv_data);
	return EndAcquisitionTWLCapture((const is_device_usb_device*)capture_data->status.device.descriptor, (is_device_device_handlers*)capture_data->handle);
}

int EndAcquisitionTWLCapture(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers) {
	return StopUsbCaptureDma(handlers, usb_device_desc);
}

void is_twl_acquisition_capture_main_loop(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
	bool is_acquisition_off = true;
	uint32_t last_read_frame_index = 0;
	CaptureScreensType curr_capture_type = capture_data->status.capture_type;
	CaptureSpeedsType curr_capture_speed = capture_data->status.capture_speed;
	bool audio_enabled = true;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_frame = std::chrono::high_resolution_clock::now();
	float last_frame_length = 0.0;
	int ret = 0;
	uint32_t video_address = 0;
	uint32_t video_length = 0;
	uint32_t audio_address = 0;
	uint32_t audio_length = 0;
	ret = StartUsbCaptureDma(handlers, usb_device_desc);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
	/*
	ret = AskFrameLengthPos(handlers, &video_address, &video_length, true, &audio_address, &audio_length, audio_enabled, usb_device_desc);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Initial Frame Info Read: Failed");
		return;
	}
	ret = SetLastFrameInfo(handlers, video_address, video_length, audio_address, audio_length, usb_device_desc);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Initial Frame Info Set: Failed");
		return;
	}
	*/
	default_sleep(DEFAULT_FRAME_TIME_MS / SLEEP_FRAME_DIVIDER);

	while(capture_data->status.connected && capture_data->status.running) {
		bool processed = false;
		/*
		ret = AskFrameLengthPos(handlers, &video_address, &video_length, true, &audio_address, &audio_length, audio_enabled, usb_device_desc);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Frame Info Read: Failed");
			return;
		}
		if(video_length > 0) {
		*/
			ret = process_frame_and_read(capture_data, &is_device_capture_recv_data[0].buffer, curr_capture_type, curr_capture_speed, &clock_last_frame, last_read_frame_index, video_address, video_length, audio_address, audio_length, processed, last_frame_length);
			if(ret < 0) {
					capture_error_print(true, capture_data, "Frame Read: Error " + std::to_string(ret));
					return;
			}
			/*
			if(processed) {
				ret = SetLastFrameInfo(handlers, video_address, video_length, audio_address, audio_length, usb_device_desc);
				if(ret < 0) {
					capture_error_print(true, capture_data, "Frame Info Set: Failed");
					return;
				}
			}
		}
		*/
		if(last_frame_length > 0)
			default_sleep((last_frame_length * 1000) / SLEEP_FRAME_DIVIDER);
		else
			default_sleep(DEFAULT_FRAME_TIME_MS / SLEEP_FRAME_DIVIDER);
	}
	if(!is_acquisition_off)
		EndAcquisition(capture_data, is_device_capture_recv_data, true, 0, curr_capture_type);
}

#include "devicecapture.hpp"
#include "usb_is_device_communications.hpp"
#include "usb_is_device_acquisition_general.hpp"
#include "usb_is_twl_acquisition_capture.hpp"

// Code created by analyzing the USB packets sent and received by the IS TWL Capture device.

#define RESET_TIMEOUT 4.0
#define BATTERY_TIMEOUT 5.0
#define SLEEP_RESET_TIME_MS 2000
#define DEFAULT_FRAME_TIME_MS 16.7
#define SLEEP_FRAME_DIVIDER 4
#define CUTOFF_LAST_FRAME_TIME 0.250

#define AUDIO_ADDRESS_RING_BUFFER_END 0x01880000
#define VIDEO_ADDRESS_RING_BUFFER_END 0x01000000

static void output_to_thread_reset_processed_data(CaptureData* capture_data, int internal_index, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t video_processed_size, size_t &video_length_processed, size_t &audio_length_processed) {
	output_to_thread(capture_data, internal_index, curr_capture_type, clock_start, video_processed_size + ((audio_length_processed / sizeof(ISTWLCaptureAudioReceived)) * sizeof(ISTWLCaptureSoundData)));
	video_length_processed = 0;
	audio_length_processed = 0;
}

static int process_frame_and_read(CaptureData* capture_data, int internal_index, CaptureScreensType curr_capture_type, CaptureSpeedsType curr_capture_speed, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, uint32_t &last_read_frame_index, uint32_t video_address, uint32_t video_length, size_t &video_length_processed, uint32_t audio_address, uint32_t audio_length, size_t &audio_length_processed, bool &processed, float &last_frame_length, bool &reprocess) {
	const size_t video_processed_size = (size_t)usb_is_device_get_video_in_size(curr_capture_type, IS_TWL_CAPTURE_DEVICE);
	processed = false;
	if((video_length == 0) && (audio_length == 0)) {
		if(reprocess)
			output_to_thread_reset_processed_data(capture_data, internal_index, curr_capture_type, clock_start, video_processed_size, video_length_processed, audio_length_processed);
		reprocess = false;
		return 0;
	}
	reprocess = false;
	processed = true;
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
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
	video_length_processed += video_length;
	// Process audio regardless, to ensure no issues with getting to the end of the ring buffer...
	int max_audio_length = (multiplier * 5) * sizeof(ISTWLCaptureAudioReceived);
	int audio_diff_from_max = max_audio_length - audio_length;
	if(audio_diff_from_max >= 0)
		audio_diff_from_max = 0;
	else {
		audio_address += -audio_diff_from_max;
		audio_length = max_audio_length;
	}
	int audio_processed_diff_from_max = (int)(max_audio_length - audio_length_processed);
	if(audio_processed_diff_from_max >= 0)
		audio_processed_diff_from_max = 0;
	else
		audio_length_processed = max_audio_length - audio_length;
	CaptureDataSingleBuffer* target = capture_data->data_buffers.GetWriterBuffer(internal_index);
	CaptureReceived* capture_buf = &target->capture_buf;
	int ret = ReadFrame(handlers, ((uint8_t*)&capture_buf->is_twl_capture_received.audio_capture_in) + audio_length_processed, audio_address, audio_length, usb_device_desc);
	if(ret < 0)
		return ret;
	audio_length_processed += audio_length;
	// Have enough video frames been received? If yes, output!
	int num_curr_available_frames = (int)(video_length / sizeof(ISTWLCaptureVideoInternalReceived));
	int num_available_frames = (int)(video_length_processed / sizeof(ISTWLCaptureVideoInternalReceived));
	if(num_available_frames < multiplier)
		return 0;
	if(num_curr_available_frames > 0) {
		int frame_next = num_curr_available_frames - 1;
		last_read_frame_index += num_curr_available_frames;
		video_address = video_address + (frame_next * sizeof(ISTWLCaptureVideoInternalReceived));
		ret = ReadFrame(handlers, (uint8_t*)&capture_buf->is_twl_capture_received.video_capture_in, video_address, (int)video_processed_size, usb_device_desc);
		if(ret < 0)
			return ret;
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<float> diff = curr_time - (*clock_start);
		last_frame_length = diff.count();
	}
	// Possible issue: there is more video or audio data available
	// (end of ring buffer has been reached in this iteration),
	// but it's not being processed since there
	// are enough video frames right now...
	if(((audio_address + audio_length) >=  AUDIO_ADDRESS_RING_BUFFER_END) || ((video_address + video_length) >=  VIDEO_ADDRESS_RING_BUFFER_END)) {
		reprocess = true;
		return 0;
	}
	output_to_thread_reset_processed_data(capture_data, internal_index, curr_capture_type, clock_start, video_processed_size, video_length_processed, audio_length_processed);
	return 0;
}

static int CaptureResetHardware(CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_reset) {
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

	ret = ResetCPUStart(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = ResetCPUEnd(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	default_sleep(SLEEP_RESET_TIME_MS);
	return ret;
}

static int CaptureBatteryHandleHardware(CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_battery_set, int &curr_battery_percentage, bool &curr_ac_adapter_connected) {
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
	int loaded_battery_percentage = capture_data->status.is_battery_percentage;
	bool loaded_ac_adapter_connected = capture_data->status.is_ac_adapter_connected;

	const auto curr_time_battery = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff_battery = curr_time_battery - clock_last_battery_set;
	if(diff_battery.count() > BATTERY_TIMEOUT) {
		if(curr_battery_percentage != loaded_battery_percentage) {
			int ret = SetBatteryPercentage(handlers, usb_device_desc, loaded_battery_percentage);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Battery Set: Failed");
				return ret;
			}
			loaded_ac_adapter_connected = curr_battery_percentage < loaded_battery_percentage;
			clock_last_battery_set = curr_time_battery;
			curr_battery_percentage = loaded_battery_percentage;
		}
		if(curr_ac_adapter_connected != loaded_ac_adapter_connected) {
			int ret = SetACAdapterConnected(handlers, usb_device_desc, loaded_ac_adapter_connected);
			if(ret < 0) {
				capture_error_print(true, capture_data, "AC Adapter Set: Failed");
				return ret;
			}
			curr_ac_adapter_connected = loaded_ac_adapter_connected;
		}
	}
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
	int curr_battery_percentage = capture_data->status.is_battery_percentage;
	bool curr_ac_adapter_connected = capture_data->status.is_ac_adapter_connected;
	bool audio_enabled = true;
	bool reprocess = false;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_frame = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_reset = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_battery_set = std::chrono::high_resolution_clock::now();
	float last_frame_length = 0.0;
	int ret = 0;
	uint32_t video_address = 0;
	uint32_t video_length = 0;
	uint32_t audio_address = 0;
	uint32_t audio_length = 0;
	size_t video_length_processed = 0;
	size_t audio_length_processed = 0;
	is_device_twl_enc_dec_table enc_table, dec_table;
	ret = PrepareEncDecTable(handlers, &enc_table, &dec_table, usb_device_desc);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Enc Dec Table init: Failed");
		return;
	}
	ret = StartUsbCaptureDma(handlers, usb_device_desc, audio_enabled, &enc_table, &dec_table);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
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
	default_sleep((float)(DEFAULT_FRAME_TIME_MS / SLEEP_FRAME_DIVIDER));
	ret = SetBatteryPercentage(handlers, usb_device_desc, curr_battery_percentage);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Initial Battery Set: Failed");
		return;
	}
	ret = SetACAdapterConnected(handlers, usb_device_desc, curr_ac_adapter_connected);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Initial AC Adapter Set: Failed");
		return;
	}

	while(capture_data->status.connected && capture_data->status.running) {
		bool processed = false;
		ret = AskFrameLengthPos(handlers, &video_address, &video_length, true, &audio_address, &audio_length, audio_enabled, usb_device_desc);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Frame Info Read: Failed");
			return;
		}
		ret = process_frame_and_read(capture_data, 0, curr_capture_type, curr_capture_speed, &clock_last_frame, last_read_frame_index, video_address, video_length, video_length_processed, audio_address, audio_length, audio_length_processed, processed, last_frame_length, reprocess);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Frame Read: Error " + std::to_string(ret));
			return;
		}
		if(processed) {
			curr_capture_speed = capture_data->status.capture_speed;
			ret = SetLastFrameInfo(handlers, video_address, video_length, audio_address, audio_length, usb_device_desc);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Frame Info Set: Failed");
				return;
			}
		}
		if(!reprocess) {
			// Handle VRR, but also prevent issues with the console lid being closed
			if((last_frame_length > 0) && (last_frame_length < CUTOFF_LAST_FRAME_TIME))
				default_sleep((last_frame_length * 1000) / SLEEP_FRAME_DIVIDER);
			else
				default_sleep((float)(DEFAULT_FRAME_TIME_MS / SLEEP_FRAME_DIVIDER));
			ret = CaptureResetHardware(capture_data, clock_last_reset);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Hardware Reset: Failed");
				return;
			}
			ret = CaptureBatteryHandleHardware(capture_data, clock_last_battery_set, curr_battery_percentage, curr_ac_adapter_connected);
			if(ret < 0)
				return;
		}
	}
	if(!is_acquisition_off)
		EndAcquisition(capture_data, is_device_capture_recv_data, true, 0, curr_capture_type);
}

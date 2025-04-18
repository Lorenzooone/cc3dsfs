#include "devicecapture.hpp"
#include "usb_is_device_communications.hpp"
#include "usb_is_device_acquisition_general.hpp"
#include "usb_is_nitro_acquisition_emulator.hpp"

#include <chrono>

// Code based off of Gericom's sample code. Distributed under the MIT License. Copyright (c) 2024 Gericom
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// The code for video capture of the IS Nitro Emulator and of the IS Nitro Capture is wildly different.
// For this reason, the code was split into two device-specific files.

#define FRAME_BUFFER_SIZE 32
#define SLEEP_CHECKS_TIME_MS 20

#define SLEEP_TIME_DIVISOR 8

static int drain_frames(is_device_device_handlers* handlers, int num_frames, int start_frames, CaptureScreensType capture_type, const is_device_usb_device* usb_device_desc) {
	ISNitroEmulatorVideoInputData* video_in_buffer = new ISNitroEmulatorVideoInputData;
	for (int i = start_frames; i < num_frames; i++) {
		int ret = ReadFrame(handlers, (uint8_t*)video_in_buffer, (int)usb_is_device_get_video_in_size(capture_type, usb_device_desc->device_type), usb_device_desc);
		if(ret < 0) {
			delete video_in_buffer;
			return ret;
		}
	}
	delete video_in_buffer;
	return LIBUSB_SUCCESS;
}

static int StartAcquisitionEmulator(is_device_device_handlers* handlers, uint16_t &out_frame_count, float &single_frame_time, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, const is_device_usb_device* usb_device_desc) {
	int ret = 0;
	ret = DisableLca2(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = set_acquisition_mode(handlers, capture_type, capture_speed, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = SetForwardFrameCount(handlers, FRAME_BUFFER_SIZE, usb_device_desc);
	if(ret < 0)
		return ret;
	// Reset this in case it's high. At around 0xFFFF, reading from the USB DMA seems to fail...
	ret = UpdateFrameForwardEnable(handlers, true, true, usb_device_desc);
	if(ret < 0)
		return ret;
	ret = UpdateFrameForwardEnable(handlers, true, false, usb_device_desc);
	if(ret < 0)
		return ret;

	// Get to the closest next frame
	auto clock_start = std::chrono::high_resolution_clock::now();
	uint16_t oldFrameCount;
	uint16_t newFrameCount;
	ret = GetFrameCounter(handlers, &oldFrameCount, usb_device_desc);
	if(ret < 0)
		return ret;
	newFrameCount = oldFrameCount;
	while(newFrameCount == oldFrameCount) {
		ret = GetFrameCounter(handlers, &newFrameCount, usb_device_desc);
		if(ret < 0)
			return ret;
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;
		// If too much time has passed, the DS is probably either turned off or sleeping. If so, avoid locking up
		if(diff.count() > 0.2)
			break;
	}

	// Get to the next modulo 32 frame.
	// We also do this to measure the time that is needed for each frame...
	// To do so, a minimum of 4 frames is required (FRAME_BUFFER_SIZE - 1 + 4)
	clock_start = std::chrono::high_resolution_clock::now();
	ret = GetFrameCounter(handlers, &oldFrameCount, usb_device_desc);
	if(ret < 0)
		return ret;
	uint16_t targetFrameCount = (newFrameCount + FRAME_BUFFER_SIZE + 3) & (~(FRAME_BUFFER_SIZE - 1));
	while(oldFrameCount != targetFrameCount) {
		ret = GetFrameCounter(handlers, &oldFrameCount, usb_device_desc);
		if(ret < 0)
			return ret;
		// Placing a sleep of some kind here would be much better...
		// Though this is only executed for a small time when first connecting...
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;
		// If too much time has passed, the DS is probably either turned off or sleeping. If so, avoid locking up
		if(diff.count() > 1.0)
			break;
	}
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<float> diff = curr_time - clock_start;
	// Sometimes the upper 8 bits aren't updated... Use only the lower 8 bits.
	newFrameCount &= 0xFF;
	oldFrameCount &= 0xFF;
	int frame_diff = ((int)oldFrameCount) - ((int)newFrameCount);
	if(frame_diff < 0)
		frame_diff += 1 << 8;
	out_frame_count = oldFrameCount;
	// Determine how much time a single frame takes. We'll use it for sleeps
	if(frame_diff == 0)
		single_frame_time = 0;
	else
		single_frame_time = diff.count() / frame_diff;

	// Start the actual DMA
	if(single_frame_time > 0) {
		ret = StartUsbCaptureDma(handlers, usb_device_desc);
		if(ret < 0)
			return ret;
	}
	return ret;
}

static bool do_sleep(float single_frame_time, std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_reset, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, int curr_frame_counter, int last_frame_counter, float* out_time) {
	auto curr_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = curr_time - clock_last_reset;
	float expected_time = single_frame_time * curr_frame_counter;
	// Frames aren't ready asap. It takes a while for them to be, in slower modes...
	int adding_single_frame_time_divisor = 0;
	switch(capture_speed) {
		case CAPTURE_SPEEDS_HALF:
			adding_single_frame_time_divisor = 1;
			break;
		case CAPTURE_SPEEDS_THIRD:
			adding_single_frame_time_divisor = 2;
			break;
		case CAPTURE_SPEEDS_QUARTER:
			adding_single_frame_time_divisor = 3;
			break;
		default:
			break;
	}
	if((capture_type == CAPTURE_SCREENS_TOP) || (capture_type == CAPTURE_SCREENS_BOTTOM))
		adding_single_frame_time_divisor *= 2;
	if(adding_single_frame_time_divisor > 0) {
		float adding_single_frame_time_multiplier = (float)(adding_single_frame_time_divisor - 1);
		if(adding_single_frame_time_multiplier == 0)
			adding_single_frame_time_multiplier = 0.33f;
		expected_time += (single_frame_time * adding_single_frame_time_multiplier) / adding_single_frame_time_divisor;
	}
	float low_single_frame_time = 1.0f / ((int)((1.0 / single_frame_time) + 1));
	float low_expected_time = low_single_frame_time * curr_frame_counter;
	// If the current time is too low, sleep a bit to make sure we don't overrun the framerate counter
	// Don't do it regardless of the situation, and only in small increments...
	// Otherwise there is the risk of sleeping too much
	bool result = (diff.count() < expected_time) && ((expected_time - diff.count()) > (single_frame_time / SLEEP_TIME_DIVISOR));
	*out_time = single_frame_time;
	if(last_frame_counter & (FRAME_BUFFER_SIZE - 1)) {
		result = (diff.count() < low_expected_time) && ((low_expected_time - diff.count()) > (low_single_frame_time / SLEEP_TIME_DIVISOR));
		*out_time = low_single_frame_time;
	}
	return result;
}

static void frame_wait(float single_frame_time, std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_reset, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, int curr_frame_counter, int last_frame_counter) {
	if(curr_frame_counter == FRAME_BUFFER_SIZE)
		return;
	float sleep_time = 0;
	while(do_sleep(single_frame_time, clock_last_reset, capture_type, capture_speed, curr_frame_counter, last_frame_counter, &sleep_time))
		default_sleep(sleep_time * 1000.0f / SLEEP_TIME_DIVISOR);
}

static int reset_acquisition_frames(CaptureData* capture_data, uint16_t &curr_frame_counter, uint16_t &last_frame_counter, float &single_frame_time, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_reset, CaptureScreensType &curr_capture_type, CaptureScreensType wanted_capture_type, CaptureSpeedsType &curr_capture_speed, CaptureSpeedsType wanted_capture_speed, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	curr_frame_counter += 1;

	if(curr_frame_counter < FRAME_BUFFER_SIZE)
		return LIBUSB_SUCCESS;

	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
	wait_all_is_device_transfers_done(capture_data, is_device_capture_recv_data);
	int ret = get_is_device_status(is_device_capture_recv_data);
	if (ret < 0)
		return ret;

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

	ret = StopUsbCaptureDma(handlers, usb_device_desc);
	if(ret < 0)
		return ret;

	// If the user requests a mode change, accomodate them.
	// Though it may lag for a bit...
	if((wanted_capture_type != curr_capture_type) || (wanted_capture_speed != curr_capture_speed)) {
		curr_capture_type = wanted_capture_type;
		curr_capture_speed = wanted_capture_speed;
		ret = set_acquisition_mode(handlers, curr_capture_type, curr_capture_speed, usb_device_desc);
		if(ret < 0)
			return ret;
	}

	uint16_t internalFrameCount = 0;
	uint16_t full_internalFrameCount = 0;
	int frame_diff = 0;
	int diff_target = FRAME_BUFFER_SIZE * multiplier;
	std::chrono::time_point<std::chrono::high_resolution_clock> curr_time;
	std::chrono::duration<float> diff;
	do {
		// Is this not the first loop? Also, do not sleep too much...
		if((frame_diff > 0) && (((internalFrameCount & (FRAME_BUFFER_SIZE - 1)) < (FRAME_BUFFER_SIZE - 1)))) {
			float expected_time = single_frame_time * FRAME_BUFFER_SIZE;
			if(diff.count() < expected_time)
				default_sleep(single_frame_time * 1000.0f / SLEEP_TIME_DIVISOR);
		}
		// Check how many frames have passed...
		ret = GetFrameCounter(handlers, &internalFrameCount, usb_device_desc);
		full_internalFrameCount = internalFrameCount;
		// Sometimes the upper 8 bits aren't updated... Use only the lower 8 bits.
		internalFrameCount &= 0xFF;
		if(ret < 0)
			return ret;
		frame_diff = internalFrameCount - last_frame_counter;
		if(frame_diff < 0)
			frame_diff += 1 << 8;
		// If the frames haven't advanced, the DS is either turned off or sleeping. If so, avoid locking up
		if(frame_diff == 0)
			break;
		curr_time = std::chrono::high_resolution_clock::now();
		diff = curr_time - clock_last_reset;
		// If too much time has passed, the DS is probably either turned off or sleeping. If so, avoid locking up
		if(diff.count() > (1.0 * multiplier)) {
			frame_diff = 0;
			break;
		}
		// Exit if enough frames have passed, or if there currently is some delay.
		// Exiting early makes it possible to catch up to the DMA, if we're behind.
	} while(((frame_diff < diff_target) && (!(last_frame_counter & (FRAME_BUFFER_SIZE - 1)))) || ((internalFrameCount & (FRAME_BUFFER_SIZE - 1)) >= (FRAME_BUFFER_SIZE - ( 1 + multiplier))));
	// Determine how much time a single frame takes. We'll use it for sleeps
	curr_time = std::chrono::high_resolution_clock::now();
	diff = curr_time - clock_last_reset;
	if((frame_diff > 0) && (frame_diff < (diff_target - (1 + multiplier)))) {
		bool is_lid_closed = false;
		ret = ReadLidState(handlers, &is_lid_closed, usb_device_desc);
		if(ret < 0)
			return ret;
		if(is_lid_closed)
			frame_diff = 0;
	}
	if(frame_diff == 0)
		single_frame_time = 0;
	else
		single_frame_time = diff.count() / (frame_diff / ((float)multiplier));
	clock_last_reset = curr_time;

	// Save the current frame counter's 8 LSB
	last_frame_counter = internalFrameCount;

	// If we're nearing 0xFFFF for the frame counter, reset it.
	// It's a problematic value for DMA reading
	if(frame_diff && (full_internalFrameCount >= 0xF000)) {
		ret = UpdateFrameForwardEnable(handlers, true, true, usb_device_desc);
		if(ret < 0)
			return ret;
		clock_last_reset = std::chrono::high_resolution_clock::now();
	}
	ret = UpdateFrameForwardEnable(handlers, true, false, usb_device_desc);
	if(ret < 0)
		return ret;
	curr_frame_counter = 0;

	// Start the actual DMA
	if(single_frame_time > 0) {
		ret = StartUsbCaptureDma(handlers, usb_device_desc);
		if(ret < 0)
			return ret;
	}
	return LIBUSB_SUCCESS;
}

int initial_cleanup_emulator(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers) {
	return EndAcquisition(usb_device_desc, handlers, false, 0, CAPTURE_SCREENS_BOTH);
}

int EndAcquisitionEmulator(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, bool do_drain_frames, int start_frames, CaptureScreensType capture_type) {
	wait_all_is_device_transfers_done(capture_data, is_device_capture_recv_data);
	return EndAcquisitionEmulator((const is_device_usb_device*)capture_data->status.device.descriptor, (is_device_device_handlers*)capture_data->handle, do_drain_frames, start_frames, capture_type);
}

int EndAcquisitionEmulator(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type) {
	int ret = 0;
	if (do_drain_frames)
		ret = drain_frames(handlers, FRAME_BUFFER_SIZE, start_frames, capture_type, usb_device_desc);
	if (ret < 0)
		return ret;
	ret = StopUsbCaptureDma(handlers, usb_device_desc);
	if (ret < 0)
		return ret;
	return UpdateFrameForwardEnable(handlers, false, false, usb_device_desc);
}

void is_nitro_acquisition_emulator_main_loop(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	is_device_device_handlers* handlers = (is_device_device_handlers*)capture_data->handle;
	const is_device_usb_device* usb_device_desc = (const is_device_usb_device*)capture_data->status.device.descriptor;
	uint32_t index = 0;
	uint16_t last_frame_counter = 0;
	float single_frame_time = 0;
	uint16_t curr_frame_counter = 0;
	CaptureScreensType curr_capture_type = capture_data->status.capture_type;
	CaptureSpeedsType curr_capture_speed = capture_data->status.capture_speed;
	int ret = StartAcquisitionEmulator(handlers, last_frame_counter, single_frame_time, curr_capture_type, curr_capture_speed, usb_device_desc);
	if (ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
	auto clock_last_reset = std::chrono::high_resolution_clock::now();

	while (capture_data->status.connected && capture_data->status.running) {
		ret = get_is_device_status(is_device_capture_recv_data);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Disconnected: Read error");
			return;
		}
		if(single_frame_time > 0) {
			ISDeviceCaptureReceivedData* chosen_buffer = is_device_get_free_buffer(capture_data, is_device_capture_recv_data);
			if(chosen_buffer == NULL)
				error_is_device_status(is_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
			is_device_read_frame_request(capture_data, chosen_buffer, curr_capture_type, index++);
			frame_wait(single_frame_time, clock_last_reset, curr_capture_type, curr_capture_speed, curr_frame_counter + 1, last_frame_counter);
		}
		else {
			capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
			clock_last_reset = std::chrono::high_resolution_clock::now();
			default_sleep(SLEEP_CHECKS_TIME_MS);
		}
		capture_data->status.curr_delay = last_frame_counter % FRAME_BUFFER_SIZE;
		ret = reset_acquisition_frames(capture_data, curr_frame_counter, last_frame_counter, single_frame_time, clock_last_reset, curr_capture_type, capture_data->status.capture_type, curr_capture_speed, capture_data->status.capture_speed, is_device_capture_recv_data);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Disconnected: Frame counter reset error");
			break;
		}
	}
	EndAcquisition(capture_data, is_device_capture_recv_data, true, curr_frame_counter, curr_capture_type);
}

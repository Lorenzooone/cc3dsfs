#include "devicecapture.hpp"
#include "usb_is_nitro.hpp"
#include "usb_is_nitro_capture.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

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

#define SERIAL_NUMBER_SIZE (IS_NITRO_REAL_SERIAL_NUMBER_SIZE + 1)

#define FRAME_BUFFER_SIZE 32

enum usb_capture_status {
	USB_CAPTURE_SUCCESS = 0,
	USB_CAPTURE_SKIP,
	USB_CAPTURE_PIPE_ERROR,
	USB_CAPTURE_FRAMEINFO_ERROR,
	USB_CAPTURE_ERROR
};

static int drain_frames(libusb_device_handle *handle, int num_frames, int start_frames, CaptureScreensType capture_type);
static int StartCapture(libusb_device_handle *handle, uint16_t *out_frame_count, float* single_frame_time, CaptureScreensType capture_type);
static int EndCapture(libusb_device_handle *handle, bool do_drain_frames, int start_frames, CaptureScreensType capture_type);

static std::string get_serial(const is_nitro_usb_device* usb_device_desc, libusb_device_handle *handle, int &curr_serial_extra_id) {
	uint8_t data[SERIAL_NUMBER_SIZE];
	std::string serial_str = std::to_string(curr_serial_extra_id);
	bool conn_success = true;
	if(libusb_set_configuration(handle, usb_device_desc->default_config) != LIBUSB_SUCCESS)
		conn_success = false;
	if(conn_success && libusb_claim_interface(handle, usb_device_desc->default_interface) != LIBUSB_SUCCESS)
		conn_success = false;
	if(conn_success && EndCapture(handle, false, 0, CAPTURE_SCREENS_BOTH) != LIBUSB_SUCCESS)
		conn_success = false;
	if(conn_success && (GetDeviceSerial(handle, data) != LIBUSB_SUCCESS)) {
		int ret = 0;
		while(ret >= 0)
			ret = drain_frames(handle, FRAME_BUFFER_SIZE * 2, 0, CAPTURE_SCREENS_TOP);
		if((GetDeviceSerial(handle, data) != LIBUSB_SUCCESS))
			conn_success = false;
	}
	if(conn_success) {
		data[IS_NITRO_REAL_SERIAL_NUMBER_SIZE] = '\0';
		serial_str = std::string((const char*)data);
	}
	else
		curr_serial_extra_id += 1;
	if(conn_success)
		libusb_release_interface(handle, usb_device_desc->default_interface);
	return serial_str;
}

static bool insert_device(std::vector<CaptureDevice> &devices_list, const is_nitro_usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return false;
	if((usb_descriptor->iManufacturer != usb_device_desc->manufacturer_id) || (usb_descriptor->iProduct != usb_device_desc->product_id))
		return false;
	int result = libusb_open(usb_device, &handle);
	if(result || (handle == NULL))
		return true;
	devices_list.emplace_back(get_serial(usb_device_desc, handle, curr_serial_extra_id), "IS Nitro Emulator", CAPTURE_CONN_IS_NITRO, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 24, 0, 0, 0, 0, HEIGHT_DS);
	libusb_close(handle);
	return true;
}

static libusb_device_handle* usb_find_by_serial_number(const is_nitro_usb_device* usb_device_desc, std::string &serial_number) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	libusb_device_handle *final_handle = NULL;

	int curr_serial_extra_id = 0;
	for(int i = 0; i < num_devices; i++) {
		libusb_device_handle *handle = NULL;
		uint8_t data[SERIAL_NUMBER_SIZE];
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if((usb_descriptor.idVendor != usb_device_desc->vid) || (usb_descriptor.idProduct != usb_device_desc->pid))
			continue;
		if((usb_descriptor.iManufacturer != usb_device_desc->manufacturer_id) || (usb_descriptor.iProduct != usb_device_desc->product_id))
			continue;
		result = libusb_open(usb_devices[i], &handle);
		if(result || (handle == NULL))
			continue;
		std::string device_serial_number = get_serial(usb_device_desc, handle, curr_serial_extra_id);
		if(serial_number == device_serial_number) {
			final_handle = handle;
			break;
		}
		libusb_close(handle);
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
	return final_handle;
}

void list_devices_is_nitro(std::vector<CaptureDevice> &devices_list) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	int curr_serial_extra_id_is_nitro_emulator = 0;
	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if(insert_device(devices_list, &usb_is_nitro_desc, usb_devices[i], &usb_descriptor, curr_serial_extra_id_is_nitro_emulator))
			continue;
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
}

static void is_nitro_connection_end(libusb_device_handle *dev, bool interface_claimed = true) {
	if(interface_claimed)
		libusb_release_interface(dev, usb_is_nitro_desc.default_interface);
	libusb_close(dev);
}

bool is_nitro_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	if(!usb_is_initialized())
		return false;
	libusb_device_handle *dev = usb_find_by_serial_number(&usb_is_nitro_desc, device->serial_number);
	if(!dev) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	if(libusb_set_configuration(dev, usb_is_nitro_desc.default_config) != LIBUSB_SUCCESS) {
		capture_error_print(true, capture_data, "Configuration failed");
		is_nitro_connection_end(dev, false);
		return false;
	}
	if(libusb_claim_interface(dev, usb_is_nitro_desc.default_interface) != LIBUSB_SUCCESS) {
		capture_error_print(true, capture_data, "Interface claim failed");
		is_nitro_connection_end(dev, false);
		return false;
	}
	capture_data->handle = (void*)dev;

	return true;
}

static uint64_t _is_nitro_emulator_get_video_in_size(CaptureScreensType capture_type) {
	if((capture_type == CAPTURE_SCREENS_TOP) || (capture_type == CAPTURE_SCREENS_BOTTOM))
		return sizeof(ISNitroEmulatorVideoInputData) / 2;
	return sizeof(ISNitroEmulatorVideoInputData);
}

uint64_t usb_is_nitro_emulator_get_video_in_size(CaptureData* capture_data) {
	return _is_nitro_emulator_get_video_in_size(capture_data->status.capture_type);
}

static int drain_frames(libusb_device_handle *handle, int num_frames, int start_frames, CaptureScreensType capture_type) {
	ISNitroEmulatorVideoInputData video_in_buffer;
	for (int i = start_frames; i < num_frames; i++) {
		int ret = ReadFrame(handle, (uint8_t*)&video_in_buffer, _is_nitro_emulator_get_video_in_size(capture_type));
		if(ret < 0)
			return ret;
	}
	return LIBUSB_SUCCESS;
}

static int set_capture_mode(libusb_device_handle *handle, CaptureScreensType capture_type) {
	uint8_t capture_mode_flag = IS_NITRO_FORWARD_CONFIG_MODE_BOTH;
	if(capture_type == CAPTURE_SCREENS_TOP)
		capture_mode_flag = IS_NITRO_FORWARD_CONFIG_MODE_TOP;
	if(capture_type == CAPTURE_SCREENS_BOTTOM)
		capture_mode_flag = IS_NITRO_FORWARD_CONFIG_MODE_BOTTOM;
	return UpdateFrameForwardConfig(handle, IS_NITRO_FORWARD_CONFIG_COLOR_RGB24 | capture_mode_flag | IS_NITRO_FORWARD_CONFIG_RATE_FULL);
}

static int StartCapture(libusb_device_handle *handle, uint16_t &out_frame_count, float &single_frame_time, CaptureScreensType capture_type) {
	int ret = DisableLca2(handle);
	if(ret < 0)
		return ret;
	ret = set_capture_mode(handle, capture_type);
	if(ret < 0)
		return ret;
	ret = SetForwardFrameCount(handle, FRAME_BUFFER_SIZE);
	if(ret < 0)
		return ret;
	// Reset this in case it's high. At around 0xFFFF, reading from the USB DMA seems to fail...
	ret = UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_RESTART | IS_NITRO_FORWARD_ENABLE_ENABLE);
	if(ret < 0)
		return ret;
	ret = UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_ENABLE);
	if(ret < 0)
		return ret;

	// Get to the closest next frame
	auto clock_start = std::chrono::high_resolution_clock::now();
	uint16_t oldFrameCount;
	uint16_t newFrameCount;
	ret = GetFrameCounter(handle, &oldFrameCount);
	if(ret < 0)
		return ret;
	newFrameCount = oldFrameCount;
	while(newFrameCount == oldFrameCount) {
		ret = GetFrameCounter(handle, &newFrameCount);
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
	ret = GetFrameCounter(handle, &oldFrameCount);
	if(ret < 0)
		return ret;
	uint16_t targetFrameCount = (newFrameCount + FRAME_BUFFER_SIZE + 3) & (~(FRAME_BUFFER_SIZE - 1));
	while(oldFrameCount != targetFrameCount) {
		ret = GetFrameCounter(handle, &oldFrameCount);
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
	const std::chrono::duration<double> diff = curr_time - clock_start;
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
		ret = StartUsbCaptureDma(handle);
		if(ret < 0)
			return ret;
	}
	return ret;
}

static int EndCapture(libusb_device_handle *handle, bool do_drain_frames, int start_frames, CaptureScreensType capture_type) {
	int ret = 0;
	if(do_drain_frames)
		ret = drain_frames(handle, FRAME_BUFFER_SIZE, start_frames, capture_type);
	if(ret < 0)
		return ret;
	ret = StopUsbCaptureDma(handle);
	if(ret < 0)
		return ret;
	return UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_DISABLE);
}

static void frame_wait(float single_frame_time, std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_reset, int curr_frame_counter, int last_frame_counter) {
	if(curr_frame_counter == 0)
		return;
	auto curr_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = curr_time - clock_last_reset;
	float expected_time = single_frame_time * curr_frame_counter;
	// If the current time is too low, sleep a bit to make sure we don't overrun the framerate counter
	// Don't do it regardless of the situation, and only in small increments...
	// Otherwise there is the risk of sleeping too much
	while((diff.count() < expected_time) && ((expected_time - diff.count()) > (single_frame_time / 4)) && (!(last_frame_counter & (FRAME_BUFFER_SIZE - 1)))) {
		default_sleep((expected_time - diff.count()) / 4);
		curr_time = std::chrono::high_resolution_clock::now();
		diff = curr_time - clock_last_reset;
	}
}

int reset_capture_frames(libusb_device_handle* handle, uint16_t &curr_frame_counter, uint16_t &last_frame_counter, float &single_frame_time, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_reset, CaptureScreensType &curr_capture_type, CaptureScreensType wanted_capture_type, int multiplier) {
	curr_frame_counter += 1;

	if(curr_frame_counter == FRAME_BUFFER_SIZE) {
		int ret = StopUsbCaptureDma(handle);
		if(ret < 0)
			return ret;

		// If the user requests a mode change, accomodate them.
		// Though it may lag for a bit...
		if(wanted_capture_type != curr_capture_type) {
			curr_capture_type = wanted_capture_type;
			ret = set_capture_mode(handle, curr_capture_type);
			if(ret < 0)
				return ret;
		}

		uint16_t internalFrameCount = 0;
		uint16_t full_internalFrameCount = 0;
		int frame_diff = 0;
		int diff_target = FRAME_BUFFER_SIZE * multiplier;
		do {
			// Check how many frames have passed...
			ret = GetFrameCounter(handle, &internalFrameCount);
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
			const auto curr_time = std::chrono::high_resolution_clock::now();
			const std::chrono::duration<double> diff = curr_time - clock_last_reset;
			// If too much time has passed, the DS is probably either turned off or sleeping. If so, avoid locking up
			if(diff.count() > (1.0 * multiplier)) {
				frame_diff = 0;
				break;
			}
			// Exit if enough frames have passed, or if there currently is some delay.
			// Exiting early makes it possible to catch up to the DMA, if we're behind.
		} while((frame_diff < diff_target) && (!(last_frame_counter & (FRAME_BUFFER_SIZE - 1))));
		// Determine how much time a single frame takes. We'll use it for sleeps
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_last_reset;
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
			ret = UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_RESTART | IS_NITRO_FORWARD_ENABLE_ENABLE);
			if(ret < 0)
				return ret;
			clock_last_reset = std::chrono::high_resolution_clock::now();
		}
		ret = UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_ENABLE);
		if(ret < 0)
			return ret;
		curr_frame_counter = 0;

		// Start the actual DMA
		if(single_frame_time > 0) {
			ret = StartUsbCaptureDma(handle);
			if(ret < 0)
				return ret;
		}
	}
	return LIBUSB_SUCCESS;
}

void is_nitro_capture_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	libusb_device_handle *handle = (libusb_device_handle*)capture_data->handle;
	uint16_t last_frame_counter = 0;
	float single_frame_time = 0;
	uint16_t curr_frame_counter = 0;
	CaptureScreensType curr_capture_type = capture_data->status.capture_type;
	int ret = StartCapture(handle, last_frame_counter, single_frame_time, curr_capture_type);
	if(ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
	int inner_curr_in = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	auto clock_last_reset = std::chrono::high_resolution_clock::now();

	while(capture_data->status.connected && capture_data->status.running) {
		frame_wait(single_frame_time, clock_last_reset, curr_frame_counter, last_frame_counter);

		if(single_frame_time > 0) {
			ret = ReadFrame(handle, (uint8_t*)&capture_data->capture_buf[inner_curr_in], _is_nitro_emulator_get_video_in_size(curr_capture_type));
			if(ret < 0) {
				capture_error_print(true, capture_data, "Disconnected: Read error");
				break;
			}
			// Output to the other threads...
			const auto curr_time = std::chrono::high_resolution_clock::now();
			const std::chrono::duration<double> diff = curr_time - clock_start;
			clock_start = curr_time;
			capture_data->time_in_buf[inner_curr_in] = diff.count();
			capture_data->read[inner_curr_in] = _is_nitro_emulator_get_video_in_size(curr_capture_type);
			capture_data->capture_type[inner_curr_in] = curr_capture_type;

			inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
			if(capture_data->status.cooldown_curr_in)
				capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
			capture_data->status.curr_in = inner_curr_in;
			capture_data->status.video_wait.unlock();
			capture_data->status.audio_wait.unlock();
		}
		else {
			capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
			default_sleep(20);
		}
		capture_data->status.curr_delay = last_frame_counter % FRAME_BUFFER_SIZE;
		ret = reset_capture_frames(handle, curr_frame_counter, last_frame_counter, single_frame_time, clock_last_reset, curr_capture_type, capture_data->status.capture_type, 1);
    	if(ret < 0) {
			capture_error_print(true, capture_data, "Disconnected: Frame counter reset error");
    		break;
		}
	}
	EndCapture(handle, true, curr_frame_counter, curr_capture_type);
}

void usb_is_nitro_capture_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	is_nitro_connection_end((libusb_device_handle*)capture_data->handle);
}

void usb_is_nitro_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureScreensType capture_type) {
	if(!usb_is_initialized())
		return;
	int num_pixels = _is_nitro_emulator_get_video_in_size(capture_type) / 3;
	int out_start_pos = 0;
	int out_clear_pos = num_pixels;
	if(capture_type == CAPTURE_SCREENS_BOTTOM) {
		out_start_pos = num_pixels;
		out_clear_pos = 0;
	}
	if((capture_type == CAPTURE_SCREENS_BOTTOM) || (capture_type == CAPTURE_SCREENS_TOP))
		memset(p_out->screen_data[out_clear_pos], 0, num_pixels * 3);
	for(int i = 0; i < num_pixels; i++) {
		p_out->screen_data[i + out_start_pos][0] = p_in->is_nitro_capture_received.video_in.screen_data[i][2];
		p_out->screen_data[i + out_start_pos][1] = p_in->is_nitro_capture_received.video_in.screen_data[i][1];
		p_out->screen_data[i + out_start_pos][2] = p_in->is_nitro_capture_received.video_in.screen_data[i][0];
	}
}

void usb_is_nitro_init() {
	return usb_init();
}

void usb_is_nitro_close() {
	usb_close();
}


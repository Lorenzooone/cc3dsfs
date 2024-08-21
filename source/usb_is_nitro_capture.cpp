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

#define NUM_CONSECUTIVE_FRAMES 32

enum usb_capture_status {
	USB_CAPTURE_SUCCESS = 0,
	USB_CAPTURE_SKIP,
	USB_CAPTURE_PIPE_ERROR,
	USB_CAPTURE_FRAMEINFO_ERROR,
	USB_CAPTURE_ERROR
};

static int drain_frames(libusb_device_handle *handle, int num_frames, int start_frames);
static int StartCapture(libusb_device_handle *handle, int *out_frame_count);
static int EndCapture(libusb_device_handle *handle, bool do_drain_frames, int start_frames);

static std::string get_serial(const is_nitro_usb_device* usb_device_desc, libusb_device_handle *handle, int &curr_serial_extra_id) {
	uint8_t data[SERIAL_NUMBER_SIZE];
	std::string serial_str = std::to_string(curr_serial_extra_id);
	bool conn_success = true;
	if(libusb_set_configuration(handle, usb_device_desc->default_config) != LIBUSB_SUCCESS)
		conn_success = false;
	//if(libusb_reset_device(handle) != LIBUSB_SUCCESS)
	//	conn_success = false;
	if(conn_success && libusb_claim_interface(handle, usb_device_desc->default_interface) != LIBUSB_SUCCESS)
		conn_success = false;
	if(conn_success && EndCapture(handle, false, 0) != LIBUSB_SUCCESS)
		conn_success = false;
	if(conn_success && (GetDeviceSerial(handle, data) == LIBUSB_SUCCESS)) {
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

static uint64_t _is_nitro_emulator_get_video_in_size() {
	return sizeof(ISNitroEmulatorVideoInputData);
}

static uint64_t get_capture_size() {
	return sizeof(ISNitroCaptureReceived);
}

uint64_t usb_is_nitro_emulator_get_video_in_size(CaptureData* capture_data) {
	return _is_nitro_emulator_get_video_in_size();
}

static int drain_frames(libusb_device_handle *handle, int num_frames, int start_frames) {
	ISNitroEmulatorVideoInputData video_in_buffer;
	for (int i = start_frames; i < num_frames; i++) {
		int ret = ReadFrame(handle, (uint8_t*)&video_in_buffer, _is_nitro_emulator_get_video_in_size());
		if(ret < 0)
			return ret;
	}
	return LIBUSB_SUCCESS;
}

static int StartCapture(libusb_device_handle *handle, int *out_frame_count) {
	int ret = DisableLca2(handle);
	if(ret < 0)
		return ret;
	ret = UpdateFrameForwardConfig(handle, IS_NITRO_FORWARD_CONFIG_COLOR_RGB24 | IS_NITRO_FORWARD_CONFIG_MODE_BOTH | IS_NITRO_FORWARD_CONFIG_RATE_FULL);
	if(ret < 0)
		return ret;
	ret = SetForwardFrameCount(handle, NUM_CONSECUTIVE_FRAMES);
	if(ret < 0)
		return ret;
	ret = UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_DISABLE);
	if(ret < 0)
		return ret;
	uint16_t oldFrameCount;
	ret = GetFrameCounter(handle, &oldFrameCount);
	if(ret < 0)
		return ret;
	ret = UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_ENABLE | IS_NITRO_FORWARD_ENABLE_RESTART);
	if(ret < 0)
		return ret;

	uint16_t newFrameCount;
	while (true) {
		ret = GetFrameCounter(handle, &newFrameCount);
		if(ret < 0)
			return ret;
		if (newFrameCount < 64 && newFrameCount != oldFrameCount && newFrameCount != oldFrameCount + 1) {
			break;
		}
	}

	ret = StartUsbCaptureDma(handle);
	if(ret < 0)
		return ret;
	ret = drain_frames(handle, newFrameCount, 0);
	*out_frame_count = newFrameCount;
	return ret;
}

static int EndCapture(libusb_device_handle *handle, bool do_drain_frames, int start_frames) {
	int ret = 0;
	if(do_drain_frames)
		ret = drain_frames(handle, NUM_CONSECUTIVE_FRAMES, start_frames);
	if(ret < 0)
		return ret;
	ret = StopUsbCaptureDma(handle);
	if(ret < 0)
		return ret;
	return UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_DISABLE);
}

int reset_capture_frames(libusb_device_handle* handle, int &frame_counter, uint16_t &total_frame_counter) {
	total_frame_counter += 1;
	frame_counter += 1;

	if(frame_counter == NUM_CONSECUTIVE_FRAMES) {
		int ret = StopUsbCaptureDma(handle);
		if(ret < 0)
			return ret;

		/*
		uint16_t internalFrameCount = -1;
		int diff;
		do {
			if (internalFrameCount != -1)
				default_sleep(8000);
			ret = GetFrameCounter(handle, &internalFrameCount);
			if(ret < 0)
				return ret;

			diff = internalFrameCount - total_frame_counter;
			if(diff > 32768)
				diff -= 1 << 16;
		} while(diff <= 0);
		*/

		ret = UpdateFrameForwardEnable(handle, IS_NITRO_FORWARD_ENABLE_ENABLE);
		if(ret < 0)
			return ret;
		frame_counter = 0;
		ret = StartUsbCaptureDma(handle);
		if(ret < 0)
			return ret;
	}
	return LIBUSB_SUCCESS;
}

void is_nitro_capture_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	libusb_device_handle *handle = (libusb_device_handle*)capture_data->handle;
	int frame_counter = 0;
	int ret = StartCapture(handle, &frame_counter);
	uint16_t total_frame_counter = frame_counter;
	if(ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return;
	}
	int inner_curr_in = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();

	while(capture_data->status.connected && capture_data->status.running) {
    	ret = ReadFrame(handle, (uint8_t*)&capture_data->capture_buf[inner_curr_in], _is_nitro_emulator_get_video_in_size());
    	if(ret < 0) {
			capture_error_print(true, capture_data, "Disconnected: Read error");
    		break;
		}

		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;
		ret = reset_capture_frames(handle, frame_counter, total_frame_counter);
    	if(ret < 0) {
			capture_error_print(true, capture_data, "Disconnected: Frame counter reset error");
    		break;
		}
		clock_start = curr_time;
		capture_data->time_in_buf[inner_curr_in] = diff.count();
		capture_data->read[inner_curr_in] = _is_nitro_emulator_get_video_in_size();

		inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
		if(capture_data->status.cooldown_curr_in)
			capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
		capture_data->status.curr_in = inner_curr_in;
		capture_data->status.video_wait.unlock();
		capture_data->status.audio_wait.unlock();
	}
	EndCapture(handle, true, frame_counter);
}

void usb_is_nitro_capture_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	is_nitro_connection_end((libusb_device_handle*)capture_data->handle);
}

void usb_is_nitro_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureDevice* capture_device) {
	if(!usb_is_initialized())
		return;
	for(int i = 0; i < IN_VIDEO_HEIGHT_DS; i++)
		for(int j = 0; j < IN_VIDEO_WIDTH_DS; j++) {
			int pixel = (i * IN_VIDEO_WIDTH_DS) + j;
			p_out->screen_data[pixel][0] = p_in->is_nitro_capture_received.video_in.screen_data[pixel][2];
			p_out->screen_data[pixel][1] = p_in->is_nitro_capture_received.video_in.screen_data[pixel][1];
			p_out->screen_data[pixel][2] = p_in->is_nitro_capture_received.video_in.screen_data[pixel][0];
		}
}

void usb_is_nitro_init() {
	return usb_init();
}

void usb_is_nitro_close() {
	usb_close();
}


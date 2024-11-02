#include "devicecapture.hpp"
#include "usb_is_nitro_communications.hpp"
#include "usb_is_nitro_acquisition.hpp"
#include "usb_is_nitro_acquisition_general.hpp"
#include "usb_is_nitro_acquisition_capture.hpp"
#include "usb_is_nitro_acquisition_emulator.hpp"
#include "usb_is_nitro_setup_general.hpp"
#include "usb_is_nitro_libusb.hpp"
#include "usb_is_nitro_is_driver.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>

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

static bool initial_cleanup(const is_nitro_usb_device* usb_device_desc, is_nitro_device_handlers* handlers) {
	if(!usb_device_desc->is_capture)
		return initial_cleanup_emulator(usb_device_desc, handlers) != LIBUSB_SUCCESS;
	return initial_cleanup_capture(usb_device_desc, handlers) != LIBUSB_SUCCESS;
}

std::string get_serial(const is_nitro_usb_device* usb_device_desc, is_nitro_device_handlers* handlers, int& curr_serial_extra_id) {
	uint8_t data[SERIAL_NUMBER_SIZE];
	std::string serial_str = std::to_string(curr_serial_extra_id);
	bool conn_success = true;
	if(initial_cleanup(usb_device_desc, handlers))
		conn_success = false;
	if (conn_success && (GetDeviceSerial(handlers, data, usb_device_desc) != LIBUSB_SUCCESS))
		conn_success = false;
	if (conn_success) {
		data[IS_NITRO_REAL_SERIAL_NUMBER_SIZE] = '\0';
		serial_str = std::string((const char*)data);
	}
	else
		curr_serial_extra_id += 1;
	return serial_str;
}

void is_nitro_insert_device(std::vector<CaptureDevice>& devices_list, is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, int& curr_serial_extra_id_is_nitro, std::string path) {
	devices_list.emplace_back(get_serial(usb_device_desc, handlers, curr_serial_extra_id_is_nitro), usb_device_desc->name, path, CAPTURE_CONN_IS_NITRO, (void*)usb_device_desc, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS);
}

void is_nitro_insert_device(std::vector<CaptureDevice>& devices_list, is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, int& curr_serial_extra_id_is_nitro) {
	devices_list.emplace_back(get_serial(usb_device_desc, handlers, curr_serial_extra_id_is_nitro), usb_device_desc->name, CAPTURE_CONN_IS_NITRO, (void*)usb_device_desc, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS);
}

static is_nitro_device_handlers* usb_find_by_serial_number(const is_nitro_usb_device* usb_device_desc, CaptureDevice* device) {
	is_nitro_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = is_nitro_libusb_serial_reconnection(usb_device_desc, device, curr_serial_extra_id);

	if (final_handlers == NULL)
		final_handlers = is_driver_serial_reconnection(usb_device_desc, device);
	return final_handlers;
}

void list_devices_is_nitro(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	const size_t num_is_nitro_desc = GetNumISNitroDesc();
	int* curr_serial_extra_id_is_nitro = new int[num_is_nitro_desc];
	bool* no_access_elems = new bool[num_is_nitro_desc];
	bool* not_supported_elems = new bool[num_is_nitro_desc];
	for (int i = 0; i < num_is_nitro_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_is_nitro[i] = 0;
	}
	is_nitro_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_is_nitro, num_is_nitro_desc);

	bool any_not_supported = false;
	for(int i = 0; i < num_is_nitro_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(int i = 0; i < num_is_nitro_desc; i++)
		if(no_access_elems[i]) {
			const is_nitro_usb_device* usb_device = GetISNitroDesc(i);
			no_access_list.emplace_back(usb_device->vid, usb_device->pid);
		}
	if(any_not_supported)
		is_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_is_nitro, num_is_nitro_desc);

	delete[] curr_serial_extra_id_is_nitro;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

static void is_nitro_connection_end(is_nitro_device_handlers* handlers, const is_nitro_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		is_nitro_libusb_end_connection(handlers, device_desc, interface_claimed);
	else
		is_driver_end_connection(handlers);
	delete handlers;
}

bool is_nitro_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const is_nitro_usb_device* usb_device_info = (const is_nitro_usb_device*)device->descriptor;
	is_nitro_device_handlers* handlers = usb_find_by_serial_number(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

uint64_t _is_nitro_get_video_in_size(CaptureScreensType capture_type) {
	if((capture_type == CAPTURE_SCREENS_TOP) || (capture_type == CAPTURE_SCREENS_BOTTOM))
		return sizeof(ISNitroEmulatorVideoInputData) / 2;
	return sizeof(ISNitroEmulatorVideoInputData);
}

uint64_t usb_is_nitro_get_video_in_size(CaptureData* capture_data) {
	return _is_nitro_get_video_in_size(capture_data->status.capture_type);
}

int set_acquisition_mode(is_nitro_device_handlers* handlers, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, const is_nitro_usb_device* usb_device_desc) {
	is_nitro_forward_config_values_screens capture_mode_flag = IS_NITRO_FORWARD_CONFIG_MODE_BOTH;
	switch(capture_type) {
		case CAPTURE_SCREENS_TOP:
			capture_mode_flag = IS_NITRO_FORWARD_CONFIG_MODE_TOP;
			break;
		case CAPTURE_SCREENS_BOTTOM:
			capture_mode_flag = IS_NITRO_FORWARD_CONFIG_MODE_BOTTOM;
			break;
		default:
			break;
	}
	is_nitro_forward_config_values_rate capture_rate_flag = IS_NITRO_FORWARD_CONFIG_RATE_FULL;
	switch(capture_speed) {
		case CAPTURE_SPEEDS_HALF:
			capture_rate_flag = IS_NITRO_FORWARD_CONFIG_RATE_HALF;
			break;
		case CAPTURE_SPEEDS_THIRD:
			capture_rate_flag = IS_NITRO_FORWARD_CONFIG_RATE_THIRD;
			break;
		case CAPTURE_SPEEDS_QUARTER:
			capture_rate_flag = IS_NITRO_FORWARD_CONFIG_RATE_QUARTER;
			break;
		default:
			break;
	}
	return UpdateFrameForwardConfig(handlers, IS_NITRO_FORWARD_CONFIG_COLOR_RGB24, capture_mode_flag, capture_rate_flag, usb_device_desc);
}

int EndAcquisition(is_nitro_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type, const is_nitro_usb_device* usb_device_desc) {
	if(usb_device_desc->is_capture)
		return EndAcquisitionCapture(handlers, usb_device_desc);
	return EndAcquisitionEmulator(handlers, do_drain_frames, start_frames, capture_type, usb_device_desc);
}

int is_nitro_read_frame_and_output(CaptureData* capture_data, int &inner_curr_in, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start) {
	int ret = ReadFrame((is_nitro_device_handlers*)capture_data->handle, (uint8_t*)&capture_data->capture_buf[inner_curr_in], _is_nitro_get_video_in_size(curr_capture_type), (const is_nitro_usb_device*)capture_data->status.device.descriptor);
	if (ret < 0)
		return ret;
	// Output to the other threads...
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - clock_start;
	clock_start = curr_time;
	capture_data->time_in_buf[inner_curr_in] = diff.count();
	capture_data->read[inner_curr_in] = _is_nitro_get_video_in_size(curr_capture_type);
	capture_data->capture_type[inner_curr_in] = curr_capture_type;

	inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
	if (capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	capture_data->status.curr_in = inner_curr_in;
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
	return ret;
}

void is_nitro_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	capture_data->status.reset_hardware = false;
	if(((const is_nitro_usb_device*)(capture_data->status.device.descriptor))->is_capture)
		is_nitro_acquisition_capture_main_loop(capture_data);
	else
		is_nitro_acquisition_emulator_main_loop(capture_data);
}

void usb_is_nitro_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	is_nitro_connection_end((is_nitro_device_handlers*)capture_data->handle, (const is_nitro_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}

void usb_is_nitro_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureScreensType capture_type) {
	if(!usb_is_initialized())
		return;
	int num_pixels = _is_nitro_get_video_in_size(capture_type) / 3;
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

bool is_nitro_is_capture(CaptureDevice* device) {
	const is_nitro_usb_device* usb_device_info = (const is_nitro_usb_device*)device->descriptor;
	return usb_device_info->is_capture;
}

void usb_is_nitro_init() {
	return usb_init();
}

void usb_is_nitro_close() {
	usb_close();
}


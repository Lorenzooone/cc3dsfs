#include "devicecapture.hpp"
#include "usb_is_device_setup_general.hpp"
#include "usb_is_device_libusb.hpp"
#include "usb_is_device_is_driver.hpp"
#include "usb_is_device_communications.hpp"
#include "usb_is_device_acquisition.hpp"
#include "usb_is_device_acquisition_general.hpp"
#include "usb_is_nitro_acquisition_capture.hpp"
#include "usb_is_nitro_acquisition_emulator.hpp"
#include "usb_is_twl_acquisition_capture.hpp"
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

#define SERIAL_NUMBER_SIZE (IS_DEVICE_REAL_SERIAL_NUMBER_SIZE + 1)
#define MAX_TIME_WAIT 1.0
#define FRAME_BUFFER_SIZE 32

static void is_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status);

static bool initial_cleanup(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers) {
	switch(usb_device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return initial_cleanup_emulator(usb_device_desc, handlers) != LIBUSB_SUCCESS;
		case IS_NITRO_CAPTURE_DEVICE:
			return initial_cleanup_capture(usb_device_desc, handlers) != LIBUSB_SUCCESS;
		case IS_TWL_CAPTURE_DEVICE:
			return initial_cleanup_twl_capture(usb_device_desc, handlers) != LIBUSB_SUCCESS;
		default:
			return false;
	}
}

std::string get_serial(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers, int& curr_serial_extra_id) {
	uint8_t data[SERIAL_NUMBER_SIZE];
	std::string serial_str = std::to_string(curr_serial_extra_id);
	bool conn_success = true;
	if(initial_cleanup(usb_device_desc, handlers))
		conn_success = false;
	if(conn_success && (GetDeviceSerial(handlers, data, usb_device_desc) != LIBUSB_SUCCESS))
		conn_success = false;
	if(conn_success) {
		data[IS_DEVICE_REAL_SERIAL_NUMBER_SIZE] = '\0';
		serial_str = std::string((const char*)data);
	}
	else
		curr_serial_extra_id += 1;
	return serial_str;
}

void is_device_insert_device(std::vector<CaptureDevice>& devices_list, is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, int& curr_serial_extra_id_is_device, std::string path) {
	devices_list.emplace_back(get_serial(usb_device_desc, handlers, curr_serial_extra_id_is_device), usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_IS_NITRO, (void*)usb_device_desc, false, false, usb_device_desc->audio_enabled, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, usb_device_desc->max_audio_samples_size, 0, 0, 0, 0, HEIGHT_DS, usb_device_desc->video_data_type, path);
}

void is_device_insert_device(std::vector<CaptureDevice>& devices_list, is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, int& curr_serial_extra_id_is_device) {
	is_device_insert_device(devices_list, handlers, usb_device_desc, curr_serial_extra_id_is_device, "");
}

static is_device_device_handlers* usb_find_by_serial_number(const is_device_usb_device* usb_device_desc, CaptureDevice* device) {
	is_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = is_device_libusb_serial_reconnection(usb_device_desc, device, curr_serial_extra_id);

	if(final_handlers == NULL)
		final_handlers = is_driver_serial_reconnection(device);
	return final_handlers;
}

void list_devices_is_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan) {
	const size_t num_is_device_desc = GetNumISDeviceDesc();
	int* curr_serial_extra_id_is_device = new int[num_is_device_desc];
	bool* no_access_elems = new bool[num_is_device_desc];
	bool* not_supported_elems = new bool[num_is_device_desc];
	std::vector<const is_device_usb_device*> device_descriptions;
	for(size_t i = 0; i < num_is_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_is_device[i] = 0;
		const is_device_usb_device* usb_device_desc = GetISDeviceDesc((int)i);
		if(devices_allowed_scan[usb_device_desc->index_in_allowed_scan])
			device_descriptions.push_back(usb_device_desc);
			
	}
	is_device_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_is_device, device_descriptions);

	bool any_not_supported = false;
	for(size_t i = 0; i < num_is_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(size_t i = 0; i < num_is_device_desc; i++)
		if(no_access_elems[i]) {
			const is_device_usb_device* usb_device = device_descriptions[i];
			no_access_list.emplace_back(usb_device->vid, usb_device->pid);
		}
	if(any_not_supported)
		is_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_is_device, device_descriptions);

	delete[] curr_serial_extra_id_is_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

static void is_device_connection_end(is_device_device_handlers* handlers, const is_device_usb_device *device_desc, bool interface_claimed = true) {
	if(handlers == NULL)
		return;
	if(handlers->usb_handle)
		is_device_libusb_end_connection(handlers, device_desc, interface_claimed);
	else
		is_driver_end_connection(handlers);
	delete handlers;
}

bool is_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const is_device_usb_device* usb_device_info = (const is_device_usb_device*)device->descriptor;
	is_device_device_handlers* handlers = usb_find_by_serial_number(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

uint64_t usb_is_device_get_video_in_size(CaptureScreensType capture_type, is_device_type device_type) {
	if(device_type == IS_TWL_CAPTURE_DEVICE)
		return sizeof(ISTWLCaptureVideoReceived);
	if((capture_type == CAPTURE_SCREENS_TOP) || (capture_type == CAPTURE_SCREENS_BOTTOM))
		return sizeof(ISNitroEmulatorVideoInputData) / 2;
	return sizeof(ISNitroEmulatorVideoInputData);
}


uint64_t usb_is_device_get_video_in_size(CaptureStatus* status) {
	return usb_is_device_get_video_in_size(status->capture_type, ((const is_device_usb_device*)(status->device.descriptor))->device_type);
}


uint64_t usb_is_device_get_video_in_size(CaptureData* capture_data) {
	return usb_is_device_get_video_in_size(&capture_data->status);
}

int set_acquisition_mode(is_device_device_handlers* handlers, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, const is_device_usb_device* usb_device_desc) {
	is_device_forward_config_values_screens capture_mode_flag = IS_DEVICE_FORWARD_CONFIG_MODE_BOTH;
	switch(capture_type) {
		case CAPTURE_SCREENS_TOP:
			capture_mode_flag = IS_DEVICE_FORWARD_CONFIG_MODE_TOP;
			break;
		case CAPTURE_SCREENS_BOTTOM:
			capture_mode_flag = IS_DEVICE_FORWARD_CONFIG_MODE_BOTTOM;
			break;
		default:
			break;
	}
	is_device_forward_config_values_rate capture_rate_flag = IS_DEVICE_FORWARD_CONFIG_RATE_FULL;
	switch(capture_speed) {
		case CAPTURE_SPEEDS_HALF:
			capture_rate_flag = IS_DEVICE_FORWARD_CONFIG_RATE_HALF;
			break;
		case CAPTURE_SPEEDS_THIRD:
			capture_rate_flag = IS_DEVICE_FORWARD_CONFIG_RATE_THIRD;
			break;
		case CAPTURE_SPEEDS_QUARTER:
			capture_rate_flag = IS_DEVICE_FORWARD_CONFIG_RATE_QUARTER;
			break;
		default:
			break;
	}
	return UpdateFrameForwardConfig(handlers, IS_DEVICE_FORWARD_CONFIG_COLOR_RGB24, capture_mode_flag, capture_rate_flag, usb_device_desc);
}

int EndAcquisition(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, bool do_drain_frames, int start_frames, CaptureScreensType capture_type) {
	switch(((const is_device_usb_device*)(capture_data->status.device.descriptor))->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return EndAcquisitionEmulator(capture_data, is_device_capture_recv_data, do_drain_frames, start_frames, capture_type);
		case IS_NITRO_CAPTURE_DEVICE:
			return EndAcquisitionCapture(capture_data, is_device_capture_recv_data);
		case IS_TWL_CAPTURE_DEVICE:
			return EndAcquisitionTWLCapture(capture_data, is_device_capture_recv_data);
		default:
			return 0;
	}
}

int EndAcquisition(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type) {
	switch(usb_device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return EndAcquisitionEmulator(usb_device_desc, handlers, do_drain_frames, start_frames, capture_type);
		case IS_NITRO_CAPTURE_DEVICE:
			return EndAcquisitionCapture(usb_device_desc, handlers);
		case IS_TWL_CAPTURE_DEVICE:
			return EndAcquisitionTWLCapture(usb_device_desc, handlers);
		default:
			return 0;
	}
}

void output_to_thread(CaptureData* capture_data, int internal_index, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t read_size) {
	// Output to the other threads...
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - (*clock_start);
	*clock_start = curr_time;
	capture_data->data_buffers.WriteToBuffer(NULL, read_size, diff.count(), &capture_data->status.device, curr_capture_type, internal_index);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static void output_to_thread(CaptureData* capture_data, int internal_index, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start) {
	const is_device_usb_device* usb_device_info = (const is_device_usb_device*)capture_data->status.device.descriptor;
	output_to_thread(capture_data, internal_index, curr_capture_type, clock_start, (size_t)usb_is_device_get_video_in_size(curr_capture_type, usb_device_info->device_type));
}

int is_device_read_frame_and_output(CaptureData* capture_data, int internal_index, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start) {
	const is_device_usb_device* usb_device_info = (const is_device_usb_device*)capture_data->status.device.descriptor;
	CaptureDataSingleBuffer* target = capture_data->data_buffers.GetWriterBuffer(internal_index);
	CaptureReceived* buffer = &target->capture_buf;
	int ret = ReadFrame((is_device_device_handlers*)capture_data->handle, (uint8_t*)buffer, (int)usb_is_device_get_video_in_size(curr_capture_type, usb_device_info->device_type), usb_device_info);
	if(ret < 0) {
		capture_data->data_buffers.ReleaseWriterBuffer(internal_index, false);
		return ret;
	}
	output_to_thread(capture_data, internal_index, curr_capture_type, &clock_start);
	return ret;
}

void is_device_read_frame_request(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, CaptureScreensType curr_capture_type, uint32_t index) {
	if(is_device_capture_recv_data == NULL)
		return;
	if((*is_device_capture_recv_data->status) < 0)
		return;
	const is_device_usb_device* usb_device_info = (const is_device_usb_device*)capture_data->status.device.descriptor;
	is_device_capture_recv_data->index = index;
	is_device_capture_recv_data->curr_capture_type = curr_capture_type;
	is_device_capture_recv_data->cb_data.function = is_device_read_frame_cb;
	CaptureDataSingleBuffer* target = capture_data->data_buffers.GetWriterBuffer(is_device_capture_recv_data->cb_data.internal_index);
	CaptureReceived* buffer = &target->capture_buf;
	ReadFrameAsync((is_device_device_handlers*)capture_data->handle, (uint8_t*)buffer, (int)usb_is_device_get_video_in_size(curr_capture_type, usb_device_info->device_type), usb_device_info, &is_device_capture_recv_data->cb_data);
}

static void end_is_device_read_frame_cb(ISDeviceCaptureReceivedData* is_device_capture_recv_data, bool pre_release) {
	if(pre_release)
		is_device_capture_recv_data->capture_data->data_buffers.ReleaseWriterBuffer(is_device_capture_recv_data->cb_data.internal_index, false);
	is_device_capture_recv_data->in_use = false;
	is_device_capture_recv_data->is_buffer_free_shared_mutex->specific_unlock(is_device_capture_recv_data->cb_data.internal_index);
}

static void is_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status) {
	ISDeviceCaptureReceivedData* is_device_capture_recv_data = (ISDeviceCaptureReceivedData*)user_data;
	if((*is_device_capture_recv_data->status) < 0)
		return end_is_device_read_frame_cb(is_device_capture_recv_data, true);
	if(transfer_status != LIBUSB_TRANSFER_COMPLETED) {
		*is_device_capture_recv_data->status = LIBUSB_ERROR_OTHER;
		return end_is_device_read_frame_cb(is_device_capture_recv_data, true);
	}

	if(((int32_t)(is_device_capture_recv_data->index - (*is_device_capture_recv_data->last_index))) <= 0) {
		//*is_device_capture_recv_data->status = LIBUSB_ERROR_INTERRUPTED;
		return end_is_device_read_frame_cb(is_device_capture_recv_data, true);
	}
	*is_device_capture_recv_data->last_index = is_device_capture_recv_data->index;

	output_to_thread(is_device_capture_recv_data->capture_data, is_device_capture_recv_data->cb_data.internal_index, is_device_capture_recv_data->curr_capture_type, is_device_capture_recv_data->clock_start);
	end_is_device_read_frame_cb(is_device_capture_recv_data, false);
}

int is_device_get_num_free_buffers(ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	int num_free = 0;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!is_device_capture_recv_data[i].in_use)
			num_free += 1;
	return num_free;
}

static void close_all_reads_error(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, bool &async_read_closed, bool &reset_usb_device, bool do_reset) {
	if(get_is_device_status(is_device_capture_recv_data) >= 0)
		return;
	if(!async_read_closed) {
		for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
			CloseAsyncRead((is_device_device_handlers*)capture_data->handle, &is_device_capture_recv_data[i].cb_data);
		async_read_closed = true;
	}
	if(do_reset && (!reset_usb_device)) {
		int ret = ResetUSBDevice((is_device_device_handlers*)capture_data->handle);
		if((ret == LIBUSB_ERROR_NO_DEVICE) || (ret == LIBUSB_ERROR_NOT_FOUND)) {
			for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
				is_device_capture_recv_data[i].cb_data.transfer_data_access.lock();
				is_device_capture_recv_data[i].cb_data.transfer_data = NULL;
				is_device_capture_recv_data[i].cb_data.transfer_data_access.unlock();
				is_device_capture_recv_data[i].in_use = false;
			}
		}
		reset_usb_device = true;
	}
}

static bool has_too_much_time_passed(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - start_time;
	return diff.count() > MAX_TIME_WAIT;
}

static void error_too_much_time_passed(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, bool &async_read_closed, bool &reset_usb_device, const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	if(has_too_much_time_passed(start_time)) {
		error_is_device_status(is_device_capture_recv_data, -1);
		close_all_reads_error(capture_data, is_device_capture_recv_data, async_read_closed, reset_usb_device, true);
	}
}

void wait_all_is_device_transfers_done(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	if(is_device_capture_recv_data == NULL)
		return;
	bool async_read_closed = false;
	bool reset_usb_device = false;
	close_all_reads_error(capture_data, is_device_capture_recv_data, async_read_closed, reset_usb_device, false);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
		void* transfer_data;
		do {
			is_device_capture_recv_data[i].cb_data.transfer_data_access.lock();
			transfer_data = is_device_capture_recv_data[i].cb_data.transfer_data;
			is_device_capture_recv_data[i].cb_data.transfer_data_access.unlock();
			if(transfer_data) {
				error_too_much_time_passed(capture_data, is_device_capture_recv_data, async_read_closed, reset_usb_device, start_time);
				is_device_capture_recv_data[i].cb_data.is_transfer_done_mutex->specific_timed_lock(i);
			}
		} while(transfer_data);
	}
}

void wait_all_is_device_buffers_free(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	if(is_device_capture_recv_data == NULL)
		return;
	bool async_read_closed = false;
	bool reset_usb_device = false;
	close_all_reads_error(capture_data, is_device_capture_recv_data, async_read_closed, reset_usb_device, false);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		while(is_device_capture_recv_data[i].in_use) {
			error_too_much_time_passed(capture_data, is_device_capture_recv_data, async_read_closed, reset_usb_device, start_time);
			is_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
		}
}

void wait_one_is_device_buffer_free(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
			if(!is_device_capture_recv_data[i].in_use)
				done = true;
		}
		if(!done) {
			if(has_too_much_time_passed(start_time))
				return;
			if(get_is_device_status(is_device_capture_recv_data) < 0)
				return;
			int dummy = 0;
			is_device_capture_recv_data[0].is_buffer_free_shared_mutex->general_timed_lock(&dummy);
		}
	}
}

bool is_device_are_buffers_all_free(ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	return is_device_get_num_free_buffers(is_device_capture_recv_data) == NUM_CAPTURE_RECEIVED_DATA_BUFFERS;
}

ISDeviceCaptureReceivedData* is_device_get_free_buffer(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	wait_one_is_device_buffer_free(capture_data, is_device_capture_recv_data);
	if(get_is_device_status(is_device_capture_recv_data) < 0)
		return NULL;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!is_device_capture_recv_data[i].in_use) {
			is_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_try_lock(i);
			is_device_capture_recv_data[i].in_use = true;
			return &is_device_capture_recv_data[i];
		}
	return NULL;
}

int get_is_device_status(ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	return *is_device_capture_recv_data[0].status;
}

void error_is_device_status(ISDeviceCaptureReceivedData* is_device_capture_recv_data, int error_val) {
	if((error_val == 0) || (get_is_device_status(is_device_capture_recv_data) == 0))
		*is_device_capture_recv_data[0].status = error_val;
}

void reset_is_device_status(ISDeviceCaptureReceivedData* is_device_capture_recv_data) {
	error_is_device_status(is_device_capture_recv_data, 0);
}

void is_device_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	bool is_done_thread;
	ConsumerMutex has_data_been_processed;
	std::thread async_processing_thread;

	uint32_t last_index = -1;
	int status = 0;
	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	SharedConsumerMutex is_transfer_done_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	SharedConsumerMutex is_transfer_data_ready_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	ISDeviceCaptureReceivedData* is_device_capture_recv_data = new ISDeviceCaptureReceivedData[NUM_CAPTURE_RECEIVED_DATA_BUFFERS];
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
		is_device_capture_recv_data[i].in_use = false;
		is_device_capture_recv_data[i].index = i;
		is_device_capture_recv_data[i].capture_data = capture_data;
		is_device_capture_recv_data[i].last_index = &last_index;
		is_device_capture_recv_data[i].clock_start = &clock_start;
		is_device_capture_recv_data[i].is_buffer_free_shared_mutex = &is_buffer_free_shared_mutex;
		is_device_capture_recv_data[i].status = &status;
		is_device_capture_recv_data[i].cb_data.actual_user_data = &is_device_capture_recv_data[i];
		is_device_capture_recv_data[i].cb_data.transfer_data = NULL;
		is_device_capture_recv_data[i].cb_data.is_transfer_done_mutex = &is_transfer_done_shared_mutex;
		is_device_capture_recv_data[i].cb_data.internal_index = i;
		is_device_capture_recv_data[i].cb_data.is_transfer_data_ready_mutex = &is_transfer_data_ready_shared_mutex;
		is_device_capture_recv_data[i].cb_data.is_data_ready = false;
	}
	SetupISDeviceAsyncThread((is_device_device_handlers*)capture_data->handle, is_device_capture_recv_data, &async_processing_thread, &is_done_thread, &has_data_been_processed);
	capture_data->status.reset_hardware = false;
	switch(((const is_device_usb_device*)(capture_data->status.device.descriptor))->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			is_nitro_acquisition_emulator_main_loop(capture_data, is_device_capture_recv_data);
			break;
		case IS_NITRO_CAPTURE_DEVICE:
			is_nitro_acquisition_capture_main_loop(capture_data, is_device_capture_recv_data);
			break;
		case IS_TWL_CAPTURE_DEVICE:
			is_twl_acquisition_capture_main_loop(capture_data, is_device_capture_recv_data);
			break;
		default:
			break;
	}
	wait_all_is_device_buffers_free(capture_data, is_device_capture_recv_data);
	EndISDeviceAsyncThread((is_device_device_handlers*)capture_data->handle, is_device_capture_recv_data, &async_processing_thread, &is_done_thread, &has_data_been_processed);
	delete []is_device_capture_recv_data;
}

void usb_is_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		capture_data->data_buffers.ReleaseWriterBuffer(i, false);
	is_device_connection_end((is_device_device_handlers*)capture_data->handle, (const is_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}

bool is_device_is_capture(CaptureDevice* device) {
	if(device->cc_type != CAPTURE_CONN_IS_NITRO)
		return false;
	const is_device_usb_device* usb_device_info = (const is_device_usb_device*)device->descriptor;
	return (usb_device_info->device_type == IS_TWL_CAPTURE_DEVICE) || (usb_device_info->device_type == IS_NITRO_CAPTURE_DEVICE);
}

bool is_device_is_nitro(CaptureDevice* device) {
	if(device->cc_type != CAPTURE_CONN_IS_NITRO)
		return false;
	const is_device_usb_device* usb_device_info = (const is_device_usb_device*)device->descriptor;
	return (usb_device_info->device_type == IS_NITRO_EMULATOR_DEVICE) || (usb_device_info->device_type == IS_NITRO_CAPTURE_DEVICE);
}

bool is_device_is_twl(CaptureDevice* device) {
	if(device->cc_type != CAPTURE_CONN_IS_NITRO)
		return false;
	const is_device_usb_device* usb_device_info = (const is_device_usb_device*)device->descriptor;
	return (usb_device_info->device_type == IS_TWL_CAPTURE_DEVICE);
}

void usb_is_device_init() {
	return usb_init();
}

void usb_is_device_close() {
	usb_close();
}


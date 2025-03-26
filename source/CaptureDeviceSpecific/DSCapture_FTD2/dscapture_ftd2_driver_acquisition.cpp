#include "dscapture_ftd2_driver_acquisition.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"
#include "dscapture_ftd2_general.hpp"
#include "dscapture_ftd2_compatibility.hpp"

#include "ftd2xx_symbols_renames.h"
#define FTD2XX_STATIC
#include <ftd2xx.h>

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define FT_FAILED(x) ((x) != FT_OK)

// Code based on sample provided by Loopy.

static size_t get_initial_offset_buffer(uint16_t* in_u16, size_t real_length) {
	if(real_length <= 0)
		return 0;
	// This is because the actual data seems to always start with a SYNCH
	size_t ignored_halfwords = 0;
	while((ignored_halfwords < (real_length / 2)) && (in_u16[ignored_halfwords] == FTD2_OLDDS_SYNCH_VALUES))
		ignored_halfwords++;
	return ignored_halfwords * 2;
}

static void data_output_update(int curr_data_buffer_index, CaptureData* capture_data, int read_amount, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	CaptureDataSingleBuffer* curr_full_data_buf = capture_data->data_buffers.GetWriterBuffer(curr_data_buffer_index);
	CaptureReceived* buffer = &curr_full_data_buf->capture_buf;
	size_t buffer_real_len = remove_synch_from_final_length((uint32_t*)buffer, read_amount);
	size_t initial_offset = get_initial_offset_buffer((uint16_t*) buffer, buffer_real_len);
	capture_data->data_buffers.WriteToBuffer(NULL, buffer_real_len, diff.count(), &capture_data->status.device, initial_offset, curr_data_buffer_index);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

void ftd2_capture_main_loop_driver(CaptureData* capture_data) {
	bool is_ftd2_libusb = false;
	// Separate Capture Enable and put it here, for better control
	if(!enable_capture(capture_data->handle, is_ftd2_libusb)) {
		capture_error_print(true, capture_data, "Capture enable error");
		return;
	}
	int inner_curr_in = 0;
	int retval = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	int curr_data_buffer_index = 0;
	int next_data_buffer_index = 0;
	const size_t full_size = get_capture_size(capture_data->status.device.is_rgb_888);
	size_t next_size = full_size;
	size_t bytesIn;

	while (capture_data->status.connected && capture_data->status.running) {
		curr_data_buffer_index = next_data_buffer_index;
		CaptureDataSingleBuffer* curr_full_data_buf = capture_data->data_buffers.GetWriterBuffer(curr_data_buffer_index);
		CaptureReceived* curr_data_buffer = &curr_full_data_buf->capture_buf;
		retval = ftd2_read(capture_data->handle, is_ftd2_libusb, ((uint8_t*)curr_data_buffer) + (full_size - next_size), next_size, &bytesIn);
		if(ftd2_is_error(retval, is_ftd2_libusb)) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			break;
		}
		if(bytesIn < next_size)
			continue;
		next_data_buffer_index = (curr_data_buffer_index + 1) % NUM_CAPTURE_RECEIVED_DATA_BUFFERS;
		CaptureDataSingleBuffer* next_full_data_buf = capture_data->data_buffers.GetWriterBuffer(next_data_buffer_index);
		CaptureReceived* next_data_buffer = &next_full_data_buf->capture_buf;
		bool has_synch_failed = !synchronization_check((uint16_t*)curr_data_buffer, full_size, (uint16_t*)next_data_buffer, &next_size, true);
		if(has_synch_failed) {
			continue;
		}
		data_output_update(curr_data_buffer_index, capture_data, full_size, clock_start);
	}
}

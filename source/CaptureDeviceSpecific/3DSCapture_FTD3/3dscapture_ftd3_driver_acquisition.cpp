#include "3dscapture_ftd3_driver_acquisition.hpp"
#include "3dscapture_ftd3_driver_comms.hpp"
#include "3dscapture_ftd3_compatibility.hpp"
#include "3dscapture_ftd3_shared_general.hpp"
#include "devicecapture.hpp"
#include "devicecapture.hpp"
//#include "ftd3xx_symbols_renames.h"

#ifdef _WIN32
#define FTD3XX_STATIC
#define FT_ASYNC_CALL FT_ReadPipeEx
#else
#define FT_ASYNC_CALL FT_ReadPipeAsync
#endif
#include <ftd3xx.h>

#include <chrono>

struct FTD3XXReceivedDataBuffer {
	CaptureReceived capture_buf;
	OVERLAPPED overlap;
	ULONG read_buffer;
};

static void fast_capture_call(FTD3XXReceivedDataBuffer* received_buffer, CaptureData* capture_data, int fifo_channel) {
	int inner_curr_in = 0;
	FT_STATUS ftStatus;
	void* handle = ((ftd3_device_device_handlers*)capture_data->handle)->driver_handle;
	for (inner_curr_in = 0; inner_curr_in < FTD3_CONCURRENT_BUFFERS; ++inner_curr_in) {
		ftStatus = FT_InitializeOverlapped(handle, &received_buffer[inner_curr_in].overlap);
		if(ftStatus) {
			capture_error_print(true, capture_data, "Disconnected: Initialize failed");
			return;
		}
	}

	for (inner_curr_in = 0; inner_curr_in < FTD3_CONCURRENT_BUFFERS - 1; ++inner_curr_in) {
		ftStatus = FT_ASYNC_CALL(handle, fifo_channel, (UCHAR*)&received_buffer[inner_curr_in].capture_buf, ftd3_get_capture_size(capture_data), &received_buffer[inner_curr_in].read_buffer, &received_buffer[inner_curr_in].overlap);
		if(ftStatus != FT_IO_PENDING) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return;
		}
	}

	inner_curr_in = FTD3_CONCURRENT_BUFFERS - 1;

	auto clock_start = std::chrono::high_resolution_clock::now();

	while (capture_data->status.connected && capture_data->status.running) {

		ftStatus = FT_ASYNC_CALL(handle, fifo_channel, (UCHAR*)&received_buffer[inner_curr_in].capture_buf, ftd3_get_capture_size(capture_data), &received_buffer[inner_curr_in].read_buffer, &received_buffer[inner_curr_in].overlap);
		if(ftStatus != FT_IO_PENDING) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return;
		}

		inner_curr_in = (inner_curr_in + 1) % FTD3_CONCURRENT_BUFFERS;

		ftStatus = FT_GetOverlappedResult(handle, &received_buffer[inner_curr_in].overlap, &received_buffer[inner_curr_in].read_buffer, true);
		if(FT_FAILED(ftStatus)) {
			capture_error_print(true, capture_data, "Disconnected: USB error");
			return;
		}

		data_output_update(&received_buffer[inner_curr_in].capture_buf, received_buffer[inner_curr_in].read_buffer, capture_data, clock_start);
	}
}

static bool safe_capture_call(FTD3XXReceivedDataBuffer* received_buffer, CaptureData* capture_data, int fifo_channel) {
	auto clock_start = std::chrono::high_resolution_clock::now();
	void* handle = ((ftd3_device_device_handlers*)capture_data->handle)->driver_handle;

	while(capture_data->status.connected && capture_data->status.running) {

		#ifdef _WIN32
		FT_STATUS ftStatus = FT_ReadPipeEx(handle, fifo_channel, (UCHAR*)&received_buffer->capture_buf, ftd3_get_capture_size(capture_data), &received_buffer->read_buffer, NULL);
		#else
		FT_STATUS ftStatus = FT_ReadPipeEx(handle, fifo_channel, (UCHAR*)&received_buffer->capture_buf, ftd3_get_capture_size(capture_data), &received_buffer->read_buffer, 1000);
		#endif
		if(FT_FAILED(ftStatus)) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return true;
		}

		data_output_update(&received_buffer->capture_buf, received_buffer->read_buffer, capture_data, clock_start);
	}

	return false;
}

static FTD3XXReceivedDataBuffer* init_received_buffer() {
	if(ftd3_driver_get_is_bad())
		return new FTD3XXReceivedDataBuffer;

	return new FTD3XXReceivedDataBuffer[FTD3_CONCURRENT_BUFFERS];
}

static void close_received_buffer(FTD3XXReceivedDataBuffer* received_buffer, CaptureData* capture_data) {
	if(ftd3_driver_get_is_bad()) {
		delete received_buffer;
		return;
	}

	void* handle = ((ftd3_device_device_handlers*)capture_data->handle)->driver_handle;
	for(int inner_curr_in = 0; inner_curr_in < FTD3_CONCURRENT_BUFFERS; ++inner_curr_in) {
		FT_STATUS ftStatus = FT_GetOverlappedResult(handle, &received_buffer[inner_curr_in].overlap, &received_buffer[inner_curr_in].read_buffer, true);
		if(FT_ReleaseOverlapped(handle, &received_buffer[inner_curr_in].overlap)) {
			capture_error_print(true, capture_data, "Disconnected: Release failed");
		}
	}
	delete []received_buffer;
}

void ftd3_driver_capture_main_loop(CaptureData* capture_data, int pipe) {
	FTD3XXReceivedDataBuffer* received_buffer = init_received_buffer();
	int fifo_channel = pipe;
	#ifndef _WIN32
		fifo_channel = 0;
	#endif
	if(!ftd3_driver_get_is_bad())
		fast_capture_call(received_buffer, capture_data, fifo_channel);
	else
		safe_capture_call(received_buffer, capture_data, fifo_channel);
	close_received_buffer(received_buffer, capture_data);
}

#include "3dscapture_ftd3.hpp"
#include "devicecapture.hpp"
//#include "ftd3xx_symbols_renames.h"

#ifdef _WIN32
#define FTD3XX_STATIC
#define FT_ASYNC_CALL FT_ReadPipeEx
#else
#define FT_ASYNC_CALL FT_ReadPipeAsync
#endif
#include <ftd3xx.h>
#ifdef USE_LIBUSB
#include "usb_generic.hpp"
#endif

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define BULK_OUT 0x02
#define BULK_IN 0x82

#ifdef _WIN32
#define FIFO_CHANNEL 0x82
#else
#define FIFO_CHANNEL 0
#endif

#define FTD3XX_VID 0x0403

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

#define FTD3XX_CONCURRENT_BUFFERS 4

struct FTD3XXReceivedDataBuffer {
	CaptureReceived capture_buf;
	OVERLAPPED overlap;
	ULONG read_buffer;
};

const uint16_t ftd3xx_valid_vids[] = {FTD3XX_VID};
const uint16_t ftd3xx_valid_pids[] = {0x601e, 0x601f, 0x602a, 0x602b, 0x602c, 0x602d, 0x602f};

static bool get_is_bad_ftd3xx();
static bool get_skip_initial_pipe_abort();

const bool is_bad_ftd3xx = get_is_bad_ftd3xx();
const bool skip_initial_pipe_abort = get_skip_initial_pipe_abort();

static bool get_is_bad_ftd3xx() {
	#ifdef _WIN32
	#ifdef _M_ARM64
	return true;
	#else
	return false;
	#endif
	#endif
	// For some reason, the version scheme seems broken...
	// For now, just return false... :/
	return false;

	bool is_bad_ftd3xx = false;
	DWORD ftd3xx_lib_version;

	if(FT_FAILED(FT_GetLibraryVersion(&ftd3xx_lib_version))) {
		ftd3xx_lib_version = 0;
	}
	if(ftd3xx_lib_version >= 0x0100001A) {
		is_bad_ftd3xx = true;
	}
	return is_bad_ftd3xx;
}

static bool get_skip_initial_pipe_abort() {
	#ifdef _WIN32
	return false;
	#endif

	bool skip_initial_pipe_abort = false;
	DWORD ftd3xx_lib_version;

	if(FT_FAILED(FT_GetLibraryVersion(&ftd3xx_lib_version))) {
		ftd3xx_lib_version = 0;
	}
	if(ftd3xx_lib_version >= 0x0100001A) {
		skip_initial_pipe_abort = true;
	}
	return skip_initial_pipe_abort;
}

void list_devices_ftd3(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	FT_STATUS ftStatus;
	DWORD numDevs = 0;
	std::string valid_descriptions[] = {"N3DSXL", "N3DSXL.2"};
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	size_t num_inserted = 0;
	if (!FT_FAILED(ftStatus) && numDevs > 0)
	{
		const int debug_multiplier = 1;
		FT_HANDLE ftHandle = NULL;
		DWORD Flags = 0;
		DWORD Type = 0;
		DWORD ID = 0;
		char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
		char Description[65] = { 0 };
		for (DWORD i = 0; i < numDevs; i++)
		{
			ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, NULL,
			SerialNumber, Description, &ftHandle);
			if (!FT_FAILED(ftStatus))
			{
				for(int j = 0; j < sizeof(valid_descriptions) / sizeof(*valid_descriptions); j++) {
					if(Description == valid_descriptions[j]) {
						for(int u = 0; u < debug_multiplier; u++)
							devices_list.emplace_back(std::string(SerialNumber), "N3DSXL", CAPTURE_CONN_FTD3, (void*)NULL, true, true, true, HEIGHT_3DS, TOP_WIDTH_3DS + BOT_WIDTH_3DS, N3DSXL_SAMPLES_IN, 90, 0, 0, TOP_WIDTH_3DS, 0, VIDEO_DATA_RGB);
						break;
					}
				}
			}
		}
	}
	if(num_inserted == 0) {
		#ifdef USE_LIBUSB
		if(get_usb_total_filtered_devices(ftd3xx_valid_vids, sizeof(ftd3xx_valid_vids) / sizeof(ftd3xx_valid_vids[0]), ftd3xx_valid_pids, sizeof(ftd3xx_valid_pids) / sizeof(ftd3xx_valid_pids[0])) != numDevs)
			no_access_list.emplace_back("FTD3XX");
		#endif
	}
}

uint64_t ftd3_get_video_in_size(CaptureData* capture_data) {
	if(!capture_data->status.enabled_3d)
		return sizeof(RGB83DSVideoInputData);
	return sizeof(RGB83DSVideoInputData_3D);
}

static uint64_t get_capture_size(CaptureData* capture_data) {
	if(!capture_data->status.enabled_3d)
		return sizeof(FTD3_3DSCaptureReceived) & (~(EXTRA_DATA_BUFFER_FTD3XX_SIZE - 1));
	return sizeof(FTD3_3DSCaptureReceived_3D) & (~(EXTRA_DATA_BUFFER_FTD3XX_SIZE - 1));
}

static void preemptive_close_connection(CaptureData* capture_data) {
	FT_AbortPipe(capture_data->handle, BULK_IN);
	FT_Close(capture_data->handle);
}

bool connect_ftd3(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {

	char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
	strncpy(SerialNumber, device->serial_number.c_str(), REAL_SERIAL_NUMBER_SIZE);
	SerialNumber[REAL_SERIAL_NUMBER_SIZE] = 0;
	if (FT_Create(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &capture_data->handle)) {
		capture_error_print(print_failed, capture_data, "Create failed");
		return false;
	}

	UCHAR buf[4] = {0x40, 0x80, 0x00, 0x00};
	ULONG written = 0;

	if (FT_WritePipe(capture_data->handle, BULK_OUT, buf, 4, &written, 0)) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	buf[1] = 0x00;

	if (FT_WritePipe(capture_data->handle, BULK_OUT, buf, 4, &written, 0)) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if (FT_SetStreamPipe(capture_data->handle, false, false, BULK_IN, get_capture_size(capture_data))) {
		capture_error_print(print_failed, capture_data, "Stream failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if(!skip_initial_pipe_abort) {
		if(FT_AbortPipe(capture_data->handle, BULK_IN)) {
			capture_error_print(print_failed, capture_data, "Abort failed");
			preemptive_close_connection(capture_data);
			return false;
		}

		if (FT_SetStreamPipe(capture_data->handle, false, false, BULK_IN, get_capture_size(capture_data))) {
			capture_error_print(print_failed, capture_data, "Stream failed");
			preemptive_close_connection(capture_data);
			return false;
		}
	}

	return true;
}

static inline void data_output_update(FTD3XXReceivedDataBuffer* received_buffer, CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	capture_data->data_buffers.WriteToBuffer(&received_buffer->capture_buf, received_buffer->read_buffer, diff.count(), &capture_data->status.device);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static void fast_capture_call(FTD3XXReceivedDataBuffer* received_buffer, CaptureData* capture_data) {
	int inner_curr_in = 0;
	FT_STATUS ftStatus;
	for (inner_curr_in = 0; inner_curr_in < FTD3XX_CONCURRENT_BUFFERS; ++inner_curr_in) {
		ftStatus = FT_InitializeOverlapped(capture_data->handle, &received_buffer[inner_curr_in].overlap);
		if (ftStatus) {
			capture_error_print(true, capture_data, "Disconnected: Initialize failed");
			return;
		}
	}

	for (inner_curr_in = 0; inner_curr_in < FTD3XX_CONCURRENT_BUFFERS - 1; ++inner_curr_in) {
		ftStatus = FT_ASYNC_CALL(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&received_buffer[inner_curr_in].capture_buf, get_capture_size(capture_data), &received_buffer[inner_curr_in].read_buffer, &received_buffer[inner_curr_in].overlap);
		if (ftStatus != FT_IO_PENDING) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return;
		}
	}

	inner_curr_in = FTD3XX_CONCURRENT_BUFFERS - 1;

	auto clock_start = std::chrono::high_resolution_clock::now();

	while (capture_data->status.connected && capture_data->status.running) {

		ftStatus = FT_ASYNC_CALL(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&received_buffer[inner_curr_in].capture_buf, get_capture_size(capture_data), &received_buffer[inner_curr_in].read_buffer, &received_buffer[inner_curr_in].overlap);
		if (ftStatus != FT_IO_PENDING) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return;
		}

		inner_curr_in = (inner_curr_in + 1) % FTD3XX_CONCURRENT_BUFFERS;

		ftStatus = FT_GetOverlappedResult(capture_data->handle, &received_buffer[inner_curr_in].overlap, &received_buffer[inner_curr_in].read_buffer, true);
		if(FT_FAILED(ftStatus)) {
			capture_error_print(true, capture_data, "Disconnected: USB error");
			return;
		}

		data_output_update(&received_buffer[inner_curr_in], capture_data, clock_start);
	}
}

static bool safe_capture_call(FTD3XXReceivedDataBuffer* received_buffer, CaptureData* capture_data) {
	auto clock_start = std::chrono::high_resolution_clock::now();

	while(capture_data->status.connected && capture_data->status.running) {

		#ifdef _WIN32
		FT_STATUS ftStatus = FT_ReadPipeEx(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&received_buffer->capture_buf, get_capture_size(capture_data), &received_buffer->read_buffer, NULL);
		#else
		FT_STATUS ftStatus = FT_ReadPipeEx(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&received_buffer->capture_buf, get_capture_size(capture_data), &received_buffer->read_buffer, 1000);
		#endif
		if(FT_FAILED(ftStatus)) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return true;
		}

		data_output_update(received_buffer, capture_data, clock_start);
	}

	return false;
}

static FTD3XXReceivedDataBuffer* init_received_buffer() {
	if(is_bad_ftd3xx)
		return new FTD3XXReceivedDataBuffer;

	return new FTD3XXReceivedDataBuffer[FTD3XX_CONCURRENT_BUFFERS];
}

static void close_received_buffer(FTD3XXReceivedDataBuffer* received_buffer, CaptureData* capture_data) {
	if(is_bad_ftd3xx) {
		delete received_buffer;
		return;
	}

	for(int inner_curr_in = 0; inner_curr_in < FTD3XX_CONCURRENT_BUFFERS; ++inner_curr_in) {
		FT_STATUS ftStatus = FT_GetOverlappedResult(capture_data->handle, &received_buffer[inner_curr_in].overlap, &received_buffer[inner_curr_in].read_buffer, true);
		if(FT_ReleaseOverlapped(capture_data->handle, &received_buffer[inner_curr_in].overlap)) {
			capture_error_print(true, capture_data, "Disconnected: Release failed");
		}
	}
	delete []received_buffer;
}

void ftd3_capture_main_loop(CaptureData* capture_data) {
	FTD3XXReceivedDataBuffer* received_buffer = init_received_buffer();
	if(!is_bad_ftd3xx)
		fast_capture_call(received_buffer, capture_data);
	else
		safe_capture_call(received_buffer, capture_data);
	close_received_buffer(received_buffer, capture_data);
}

void ftd3_capture_cleanup(CaptureData* capture_data) {
	if(FT_AbortPipe(capture_data->handle, BULK_IN)) {
		capture_error_print(true, capture_data, "Disconnected: Abort failed");
	}

	if (FT_Close(capture_data->handle)) {
		capture_error_print(true, capture_data, "Disconnected: Close failed");
	}
}

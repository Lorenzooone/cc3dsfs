#include "3dscapture_ftd3_libusb_acquisition.hpp"
#include "3dscapture_ftd3_libusb_comms.hpp"
#include "3dscapture_ftd3_compatibility.hpp"
#include "3dscapture_ftd3_shared_general.hpp"
#include "devicecapture.hpp"

#include <thread>
#include <libusb.h>
#include "usb_generic.hpp"

#define MAX_TIME_WAIT 0.5

// This was created to remove the dependency from the FTD3XX library
// under Linux and MacOS (as well as WinUSB).
// There were issues with said library which forced the use of an
// older release (double free when a device is disconnected, for example).
// There were problems with automatic downloads being blocked as well.
// This is something which could not go on indefinitively.

// Optimize data capturing for libusb with async transfers and callbacks,
// instead of waiting for the usb transfer to be done...

struct FTD3LibusbCaptureReceivedData {
	bool in_use;
	bool* pause_output;
	uint32_t index;
	int internal_index;
	CaptureData* capture_data;
	uint32_t* last_index;
	bool is_3d;
	std::chrono::time_point<std::chrono::high_resolution_clock> *clock_start;
	SharedConsumerMutex *is_buffer_free_shared_mutex;
	int* status;
	ftd3_async_callback_data cb_data;
};

static void ftd3_libusb_read_frame_cb(void* user_data, int transfer_length, int transfer_status);

static void ftd3_device_usb_thread_function(bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 300000;
	while(*usb_thread_run)
		libusb_handle_events_timeout_completed(get_usb_ctx(), &tv, NULL);
}

static void ftd3_libusb_start_thread(std::thread* thread_ptr, bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = true;
	*thread_ptr = std::thread(ftd3_device_usb_thread_function, usb_thread_run);
}

static void ftd3_libusb_close_thread(std::thread* thread_ptr, bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = false;
	thread_ptr->join();
}

static int get_ftd3_libusb_status(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data) {
	return *ftd3_libusb_capture_recv_data[0].status;
}

static void error_ftd3_libusb_status(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data, int error_val) {
	if((error_val == 0) || (get_ftd3_libusb_status(ftd3_libusb_capture_recv_data) == 0))
		*ftd3_libusb_capture_recv_data[0].status = error_val;
}

static void reset_ftd3_libusb_status(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data) {
	error_ftd3_libusb_status(ftd3_libusb_capture_recv_data, 0);
}

static void ftd3_libusb_read_frame_request(CaptureData* capture_data, FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data, uint32_t index, int pipe, bool is_3d) {
	if(ftd3_libusb_capture_recv_data == NULL)
		return;
	if((*ftd3_libusb_capture_recv_data->status) < 0)
		return;
	ftd3_libusb_capture_recv_data->index = index;
	ftd3_libusb_capture_recv_data->cb_data.function = ftd3_libusb_read_frame_cb;
	CaptureDataSingleBuffer* data_buf = capture_data->data_buffers.GetWriterBuffer(ftd3_libusb_capture_recv_data->internal_index);
	uint8_t* buffer = (uint8_t*)&data_buf->capture_buf;
	ftd3_libusb_capture_recv_data->is_3d = is_3d;
	ftd3_libusb_async_in_start((ftd3_device_device_handlers*)capture_data->handle, pipe, MAX_TIME_WAIT * 1000, buffer, ftd3_get_capture_size(is_3d), &ftd3_libusb_capture_recv_data->cb_data);
}

static void end_ftd3_libusb_read_frame_cb(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data, bool early_release) {
	if(early_release)
		ftd3_libusb_capture_recv_data->capture_data->data_buffers.ReleaseWriterBuffer(ftd3_libusb_capture_recv_data->internal_index, false);
	ftd3_libusb_capture_recv_data->in_use = false;
	ftd3_libusb_capture_recv_data->is_buffer_free_shared_mutex->specific_unlock(ftd3_libusb_capture_recv_data->internal_index);
}

static void ftd3_libusb_read_frame_cb(void* user_data, int transfer_length, int transfer_status) {
	FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data = (FTD3LibusbCaptureReceivedData*)user_data;
	if((*ftd3_libusb_capture_recv_data->status) < 0)
		return end_ftd3_libusb_read_frame_cb(ftd3_libusb_capture_recv_data, true);
	if(transfer_status != LIBUSB_TRANSFER_COMPLETED) {
		*ftd3_libusb_capture_recv_data->status = LIBUSB_ERROR_OTHER;
		return end_ftd3_libusb_read_frame_cb(ftd3_libusb_capture_recv_data, true);
	}

	if(((int32_t)(ftd3_libusb_capture_recv_data->index - (*ftd3_libusb_capture_recv_data->last_index))) <= 0) {
		//*ftd3_libusb_capture_recv_data->status = LIBUSB_ERROR_INTERRUPTED;
		return end_ftd3_libusb_read_frame_cb(ftd3_libusb_capture_recv_data, true);
	}
	*ftd3_libusb_capture_recv_data->last_index = ftd3_libusb_capture_recv_data->index;

	if(*ftd3_libusb_capture_recv_data->pause_output)
		return end_ftd3_libusb_read_frame_cb(ftd3_libusb_capture_recv_data, true);

	data_output_update(ftd3_libusb_capture_recv_data->internal_index, transfer_length, ftd3_libusb_capture_recv_data->capture_data, *ftd3_libusb_capture_recv_data->clock_start, ftd3_libusb_capture_recv_data->is_3d);
	end_ftd3_libusb_read_frame_cb(ftd3_libusb_capture_recv_data, false);
}

static int ftd3_libusb_get_num_free_buffers(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data) {
	int num_free = 0;
	for(int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++)
		if(!ftd3_libusb_capture_recv_data[i].in_use)
			num_free += 1;
	return num_free;
}

static void close_all_reads_error(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data, bool &async_read_closed) {
	if(get_ftd3_libusb_status(ftd3_libusb_capture_recv_data) >= 0)
		return;
	if(!async_read_closed) {
		for (int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++)
			ftd3_libusb_cancell_callback(&ftd3_libusb_capture_recv_data[i].cb_data);
		async_read_closed = true;
	}
}

static bool has_too_much_time_passed(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - start_time;
	return diff.count() > MAX_TIME_WAIT;
}

static void error_too_much_time_passed(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data, bool &async_read_closed, const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	if(has_too_much_time_passed(start_time)) {
		error_ftd3_libusb_status(ftd3_libusb_capture_recv_data, -1);
		close_all_reads_error(ftd3_libusb_capture_recv_data, async_read_closed);
	}
}

static void wait_all_ftd3_libusb_buffers_free(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data) {
	if(ftd3_libusb_capture_recv_data == NULL)
		return;
	bool async_read_closed = false;
	close_all_reads_error(ftd3_libusb_capture_recv_data, async_read_closed);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++)
		while(ftd3_libusb_capture_recv_data[i].in_use) {
			error_too_much_time_passed(ftd3_libusb_capture_recv_data, async_read_closed, start_time);
			ftd3_libusb_capture_recv_data[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
		}
}

static void wait_one_ftd3_libusb_buffer_free(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data) {
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		for(int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++) {
			if(!ftd3_libusb_capture_recv_data[i].in_use)
				done = true;
		}
		if(!done) {
			if(has_too_much_time_passed(start_time))
				return;
			if(get_ftd3_libusb_status(ftd3_libusb_capture_recv_data) < 0)
				return;
			int dummy = 0;
			ftd3_libusb_capture_recv_data[0].is_buffer_free_shared_mutex->general_timed_lock(&dummy);
		}
	}
}

static bool ftd3_libusb_are_buffers_all_free(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data) {
	return ftd3_libusb_get_num_free_buffers(ftd3_libusb_capture_recv_data) == FTD3_CONCURRENT_BUFFERS;
}

static FTD3LibusbCaptureReceivedData* ftd3_libusb_get_free_buffer(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data) {
	wait_one_ftd3_libusb_buffer_free(ftd3_libusb_capture_recv_data);
	if(get_ftd3_libusb_status(ftd3_libusb_capture_recv_data) < 0)
		return NULL;
	for(int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++)
		if(!ftd3_libusb_capture_recv_data[i].in_use) {
			ftd3_libusb_capture_recv_data[i].is_buffer_free_shared_mutex->specific_try_lock(i);
			ftd3_libusb_capture_recv_data[i].in_use = true;
			return &ftd3_libusb_capture_recv_data[i];
		}
	return NULL;
}

static void ftd3_libusb_capture_main_loop_processing(FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data, int pipe) {
	int index = 0;
	CaptureData* capture_data = ftd3_libusb_capture_recv_data[0].capture_data;

	bool could_use_3d = get_3d_enabled(&capture_data->status, true);
	bool stored_3d_status = true;
	bool result_3d_setup = ftd3_capture_3d_setup(capture_data, true, stored_3d_status);
	if(!result_3d_setup)
		return;

	for(int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++)
		ftd3_libusb_read_frame_request(capture_data, ftd3_libusb_get_free_buffer(ftd3_libusb_capture_recv_data), index++, pipe, could_use_3d && stored_3d_status);

	while(capture_data->status.connected && capture_data->status.running) {
		if(could_use_3d && (stored_3d_status != capture_data->status.requested_3d)) {
			*ftd3_libusb_capture_recv_data->pause_output = true;
			wait_all_ftd3_libusb_buffers_free(ftd3_libusb_capture_recv_data);
			if(get_ftd3_libusb_status(ftd3_libusb_capture_recv_data) < 0) {
				capture_error_print(true, capture_data, "Disconnected: Read failed");
				return;
			}

			result_3d_setup = ftd3_capture_3d_setup(capture_data, false, stored_3d_status);
			if(!result_3d_setup)
				return;

			*ftd3_libusb_capture_recv_data->pause_output = false;
			for(int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++)
				ftd3_libusb_read_frame_request(capture_data, ftd3_libusb_get_free_buffer(ftd3_libusb_capture_recv_data), index++, pipe, could_use_3d && stored_3d_status);

			*ftd3_libusb_capture_recv_data[0].clock_start = std::chrono::high_resolution_clock::now();
		}
		if(get_ftd3_libusb_status(ftd3_libusb_capture_recv_data) < 0) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return;
		}
		FTD3LibusbCaptureReceivedData* free_buffer = ftd3_libusb_get_free_buffer(ftd3_libusb_capture_recv_data);
		ftd3_libusb_read_frame_request(capture_data, free_buffer, index++, pipe, could_use_3d && stored_3d_status);
	}
}

void ftd3_libusb_capture_main_loop(CaptureData* capture_data, int pipe) {
	if(!usb_is_initialized())
		return;
	bool is_done_thread;
	std::thread async_processing_thread;

	uint32_t last_index = -1;
	int status = 0;
	bool pause_output = false;
	SharedConsumerMutex is_buffer_free_shared_mutex(FTD3_CONCURRENT_BUFFERS);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	FTD3LibusbCaptureReceivedData* ftd3_libusb_capture_recv_data = new FTD3LibusbCaptureReceivedData[FTD3_CONCURRENT_BUFFERS];
	for(int i = 0; i < FTD3_CONCURRENT_BUFFERS; i++) {
		ftd3_libusb_capture_recv_data[i].in_use = false;
		ftd3_libusb_capture_recv_data[i].index = i;
		ftd3_libusb_capture_recv_data[i].pause_output = &pause_output;
		ftd3_libusb_capture_recv_data[i].internal_index = i;
		ftd3_libusb_capture_recv_data[i].capture_data = capture_data;
		ftd3_libusb_capture_recv_data[i].last_index = &last_index;
		ftd3_libusb_capture_recv_data[i].clock_start = &clock_start;
		ftd3_libusb_capture_recv_data[i].is_buffer_free_shared_mutex = &is_buffer_free_shared_mutex;
		ftd3_libusb_capture_recv_data[i].status = &status;
		ftd3_libusb_capture_recv_data[i].cb_data.actual_user_data = &ftd3_libusb_capture_recv_data[i];
		ftd3_libusb_capture_recv_data[i].cb_data.transfer_data = NULL;
	}
	ftd3_libusb_start_thread(&async_processing_thread, &is_done_thread);
	ftd3_libusb_capture_main_loop_processing(ftd3_libusb_capture_recv_data, pipe);
	wait_all_ftd3_libusb_buffers_free(ftd3_libusb_capture_recv_data);
	ftd3_libusb_close_thread(&async_processing_thread, &is_done_thread);
	delete []ftd3_libusb_capture_recv_data;
}

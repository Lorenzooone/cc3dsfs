#include "dscapture_ftd2_libusb_acquisition.hpp"
#include "dscapture_ftd2_libusb_comms.hpp"
#include "dscapture_ftd2_general.hpp"
#include "dscapture_ftd2_compatibility.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define EXPECTED_IGNORED_HALFWORDS 1

#define IGNORE_FIRST_FEW_FRAMES_SYNC (NUM_CAPTURE_RECEIVED_DATA_BUFFERS * 2)
#define RESYNC_TIMEOUT 0.050

static void ftd2_libusb_cancel_callback(ftd2_async_callback_data* cb_data) {
	cb_data->transfer_data_access.lock();
	if(cb_data->transfer_data)
		libusb_cancel_transfer((libusb_transfer*)cb_data->transfer_data);
	cb_data->transfer_data_access.unlock();
}

static void STDCALL ftd2_libusb_read_callback(libusb_transfer* transfer) {
	ftd2_async_callback_data* cb_data = (ftd2_async_callback_data*)transfer->user_data;
	FTD2CaptureReceivedData* user_data = (FTD2CaptureReceivedData*)cb_data->actual_user_data;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = NULL;
	cb_data->is_transfer_done_mutex->specific_unlock(cb_data->internal_index);
	cb_data->transfer_data_access.unlock();
	cb_data->function((void*)user_data, transfer->actual_length, transfer->status);
}

// Read from bulk
static void ftd2_libusb_schedule_read(ftd2_async_callback_data* cb_data, uint8_t* buffer_raw, int length) {
	const int max_packet_size = MAX_PACKET_SIZE_USB2;
	libusb_transfer *transfer_in = libusb_alloc_transfer(0);
	if(!transfer_in)
		return;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = transfer_in;
	cb_data->is_transfer_done_mutex->specific_try_lock(cb_data->internal_index);
	length = ftd2_libusb_get_expanded_length(length);
	cb_data->requested_length = length;
	ftd2_libusb_async_bulk_in_prepare_and_submit(cb_data->handle, (void*)transfer_in, buffer_raw, length, (void*)ftd2_libusb_read_callback, (void*)cb_data, NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	cb_data->transfer_data_access.unlock();
}

static int get_ftd2_libusb_status(FTD2CaptureReceivedData* received_data_buffers) {
	return *received_data_buffers[0].status;
}

static void reset_ftd2_libusb_status(FTD2CaptureReceivedData* received_data_buffers) {
	*received_data_buffers[0].status = 0;
}

static void error_ftd2_libusb_status(FTD2CaptureReceivedData* received_data_buffers, int error) {
	*received_data_buffers[0].status = error;
}

static int ftd2_libusb_get_num_free_buffers(FTD2CaptureReceivedData* received_data_buffers) {
	int num_free = 0;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!received_data_buffers[i].in_use)
			num_free += 1;
	return num_free;
}

static void wait_all_ftd2_libusb_transfers_done(FTD2CaptureReceivedData* received_data_buffers) {
	if (received_data_buffers == NULL)
		return;
	if (*received_data_buffers[0].status < 0) {
		for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
			ftd2_libusb_cancel_callback(&received_data_buffers[i].cb_data);
	}
	for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
		void* transfer_data;
		do {
			received_data_buffers[i].cb_data.transfer_data_access.lock();
			transfer_data = received_data_buffers[i].cb_data.transfer_data;
			received_data_buffers[i].cb_data.transfer_data_access.unlock();
			if(transfer_data)
				received_data_buffers[i].cb_data.is_transfer_done_mutex->specific_timed_lock(i);
		} while(transfer_data);
	}
}

static void wait_all_ftd2_libusb_buffers_free(FTD2CaptureReceivedData* received_data_buffers) {
	if(received_data_buffers == NULL)
		return;
	if(*received_data_buffers[0].status < 0) {
		for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
			ftd2_libusb_cancel_callback(&received_data_buffers[i].cb_data);
	}
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		while(received_data_buffers[i].in_use)
			received_data_buffers[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
}

static void wait_one_ftd2_libusb_buffer_free(FTD2CaptureReceivedData* received_data_buffers) {
	bool done = false;
	while(!done) {
		for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
			if(!received_data_buffers[i].in_use)
				done = true;
		}
		if(!done) {
			if(*received_data_buffers[0].status < 0)
				return;
			int dummy = 0;
			received_data_buffers[0].is_buffer_free_shared_mutex->general_timed_lock(&dummy);
		}
	}
}

static bool ftd2_libusb_are_buffers_all_free(FTD2CaptureReceivedData* received_data_buffers) {
	return ftd2_libusb_get_num_free_buffers(received_data_buffers) == NUM_CAPTURE_RECEIVED_DATA_BUFFERS;
}

static FTD2CaptureReceivedData* ftd2_libusb_get_free_buffer(FTD2CaptureReceivedData* received_data_buffers) {
	wait_one_ftd2_libusb_buffer_free(received_data_buffers);
	if(*received_data_buffers[0].status < 0)
		return NULL;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!received_data_buffers[i].in_use) {
			received_data_buffers[i].is_buffer_free_shared_mutex->specific_try_lock(i);
			received_data_buffers[i].in_use = true;
			return &received_data_buffers[i];
		}
	return NULL;
}

static void ftd2_libusb_start_read(FTD2CaptureReceivedData* received_data_buffer, int index, size_t size) {
	if(received_data_buffer == NULL)
		return;
	CaptureDataSingleBuffer* full_data_buf = received_data_buffer->capture_data->data_buffers.GetWriterBuffer(received_data_buffer->cb_data.internal_index);
	CaptureReceived* data_buffer = &full_data_buf->capture_buf;
	received_data_buffer->buffer_raw = (uint8_t*)&data_buffer->ftd2_received_old_ds_normal_plus_raw.raw_data;
	received_data_buffer->buffer_target = (uint32_t*)data_buffer;
	received_data_buffer->index = index;
	ftd2_libusb_schedule_read(&received_data_buffer->cb_data, received_data_buffer->buffer_raw, size);
}

static void ftd2_libusb_copy_buffer_to_target_and_skip(uint8_t* buffer_written, uint8_t* buffer_target, const int max_packet_size, size_t length, size_t header_packet_size, size_t ignored_bytes) {
	// This could be made faster for small "ignored_bytes", however this scales well...
	// Plus, most of the time is used up by the memcpy routine, so...

	// Remove the small headers every 512 bytes...
	// The "- header_packet_size" instead of "-1" covers for partial header transfers...
	int num_iters = (length + max_packet_size - header_packet_size) / max_packet_size;
	if(num_iters <= 0)
		return;

	size_t inner_length = length - (num_iters * header_packet_size);
	size_t fully_ignored_iters = ignored_bytes / (max_packet_size - header_packet_size);
	size_t partially_ignored_iters = (ignored_bytes + (max_packet_size - header_packet_size) - 1) / (max_packet_size - header_packet_size);
	num_iters -= fully_ignored_iters;
	if(num_iters <= 0)
		return;

	buffer_written += fully_ignored_iters * max_packet_size;
	// Skip inside a packet, since it's misaligned
	if(partially_ignored_iters != fully_ignored_iters) {
		size_t offset_bytes = ignored_bytes % (max_packet_size - header_packet_size);
		int rem_size = inner_length - ((max_packet_size - header_packet_size) * fully_ignored_iters);
		if(rem_size > ((int)((max_packet_size - header_packet_size))))
			rem_size = max_packet_size - header_packet_size;
		rem_size -= offset_bytes;
		if(rem_size > 0) {
			memcpy(buffer_target, buffer_written + header_packet_size + offset_bytes, rem_size);
			buffer_written += max_packet_size;
			buffer_target += rem_size;
		}
	}
	if(length <= (max_packet_size * partially_ignored_iters))
		return;
	ftd2_libusb_copy_buffer_to_target(buffer_written, buffer_target, max_packet_size, length - (max_packet_size * partially_ignored_iters), header_packet_size);
}

static size_t ftd2_libusb_copy_buffer_to_target_and_skip_synch(uint8_t* in_buffer, uint32_t* out_buffer, int read_length, size_t* sync_offset) {
	// This is because the actual data seems to always start with a SYNCH
	const size_t max_packet_size = MAX_PACKET_SIZE_USB2;
	const size_t header_packet_size = FTD2_INTRA_PACKET_HEADER_SIZE;
	size_t ignored_halfwords = 0;
	uint16_t* in_u16 = (uint16_t*)in_buffer;
	size_t real_length = ftd2_libusb_get_actual_length(max_packet_size, read_length, header_packet_size);
	while((ignored_halfwords < (real_length / 2)) && (in_u16[ignored_halfwords + 1 + (ignored_halfwords / (max_packet_size / 2))] == FTD2_OLDDS_SYNCH_VALUES))
		ignored_halfwords++;
	size_t copy_offset = ignored_halfwords * 2;
	ftd2_libusb_copy_buffer_to_target_and_skip(in_buffer, (uint8_t*)out_buffer, max_packet_size, read_length, header_packet_size, copy_offset);
	if((copy_offset == 0) || (copy_offset >= (max_packet_size - header_packet_size))) {
		size_t internal_sync_offset = 0;
		bool is_synced = synchronization_check((uint16_t*)out_buffer, real_length, NULL, &internal_sync_offset);
		if(!is_synced) {
			*sync_offset = (internal_sync_offset / (max_packet_size - header_packet_size)) * (max_packet_size - header_packet_size);
			return 0;
		}
		else
			*sync_offset = 0;
	}
	else
		*sync_offset = 0;
	uint16_t* out_u16 = (uint16_t*)out_buffer;
	for(int i = 0; i < ignored_halfwords; i++)
		out_u16[(real_length / 2) - ignored_halfwords + i] = FTD2_OLDDS_SYNCH_VALUES;
	return remove_synch_from_final_length(out_buffer, real_length);
}

static void end_ftd2_libusb_read_frame_cb(FTD2CaptureReceivedData* received_data_buffer, bool has_succeded) {
	if(!has_succeded)
		received_data_buffer->capture_data->data_buffers.ReleaseWriterBuffer(received_data_buffer->cb_data.internal_index, false);
	received_data_buffer->in_use = false;
	received_data_buffer->is_buffer_free_shared_mutex->specific_unlock(received_data_buffer->cb_data.internal_index);
}

static void output_to_thread(CaptureData* capture_data, int internal_index, uint8_t* buffer_raw, uint32_t* buffer_target, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time, int read_length, size_t* sync_offset) {
	// For some reason, there is usually one.
	// Though make this generic enough
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	// Copy data to buffer, with special memcpy which accounts for ftd2 header data and skips synch bytes
	size_t real_length = ftd2_libusb_copy_buffer_to_target_and_skip_synch(buffer_raw, buffer_target, read_length, sync_offset);
	capture_data->data_buffers.WriteToBuffer(NULL, real_length, diff.count(), &capture_data->status.device, internal_index);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static void ftd2_libusb_capture_process_data(void* in_user_data, int transfer_length, int transfer_status) {
	// Note: sometimes the data returned has length 0...
	// It's because the code is too fast...
	FTD2CaptureReceivedData* user_data = (FTD2CaptureReceivedData*)in_user_data;
	if((*user_data->status) < 0)
		return end_ftd2_libusb_read_frame_cb(user_data, false);
	if(transfer_status != LIBUSB_TRANSFER_COMPLETED) {
		*user_data->status = LIBUSB_ERROR_OTHER;
		return end_ftd2_libusb_read_frame_cb(user_data, false);
	}
	if(transfer_length < user_data->cb_data.requested_length)
		return end_ftd2_libusb_read_frame_cb(user_data, false);
	if(((int32_t)(user_data->index - (*user_data->last_used_index))) <= 0) {
		//*user_data->status = LIBUSB_ERROR_INTERRUPTED;
		return end_ftd2_libusb_read_frame_cb(user_data, false);
	}
	*user_data->last_used_index = user_data->index;

	// For some reason, saving the raw buffer and then passing it
	// like this is way faster than loading it.
	// Even if it's still needed to get the writer buffer...
	output_to_thread(user_data->capture_data, user_data->cb_data.internal_index, user_data->buffer_raw, user_data->buffer_target, *user_data->clock_start, transfer_length, user_data->curr_offset);
	end_ftd2_libusb_read_frame_cb(user_data, true);
}

static void resync_offset(FTD2CaptureReceivedData* received_data_buffers, uint32_t &index, size_t full_size) {
	size_t wanted_offset = *received_data_buffers[0].curr_offset;
	if(wanted_offset == 0)
		return;
	wait_all_ftd2_libusb_buffers_free(received_data_buffers);
	if(get_ftd2_libusb_status(received_data_buffers) != 0)
		return;
	wanted_offset = *received_data_buffers[0].curr_offset;
	if(wanted_offset == 0)
		return;
	*received_data_buffers[0].curr_offset = 0;
	#if defined(__APPLE__) || defined(_WIN32)
	// Literally throw a die... Seems to work!
	default_sleep(1);
	return;
	#endif
	FTD2OldDSCaptureReceivedRaw* buffer_raw = new FTD2OldDSCaptureReceivedRaw;
	FTD2OldDSCaptureReceived* buffer = new FTD2OldDSCaptureReceived;
	CaptureData* capture_data = received_data_buffers[0].capture_data;
	bool is_synced = false;
	size_t chosen_transfer_size = ftd2_libusb_get_actual_length(MAX_PACKET_SIZE_USB2 * 4);
	while((!is_synced) && (capture_data->status.connected && capture_data->status.running)) {
		int retval = ftd2_libusb_force_read_with_timeout(received_data_buffers[0].cb_data.handle, (uint8_t*)buffer_raw, (uint8_t*)buffer, chosen_transfer_size, RESYNC_TIMEOUT);
		if(ftd2_is_error(retval, true)) {
			//error_ftd2_libusb_status(received_data_buffers, retval);
			delete buffer_raw;
			delete buffer;
			return;
		}
		size_t internal_sync_offset = 0;
		is_synced = synchronization_check((uint16_t*)buffer, chosen_transfer_size, NULL, &internal_sync_offset, true);
		if((!is_synced) && (internal_sync_offset < chosen_transfer_size))
			is_synced = true;
		else
			is_synced = false;
	}
	int retval = ftd2_libusb_force_read_with_timeout(received_data_buffers[0].cb_data.handle, (uint8_t*)buffer_raw, (uint8_t*)buffer, full_size - chosen_transfer_size, RESYNC_TIMEOUT);
	if(ftd2_is_error(retval, true)) {
		error_ftd2_libusb_status(received_data_buffers, retval);
		delete buffer_raw;
		delete buffer;
		return;
	}
	capture_data->status.cooldown_curr_in = IGNORE_FIRST_FEW_FRAMES_SYNC;
	delete buffer_raw;
	delete buffer;
}

void ftd2_capture_main_loop_libusb(CaptureData* capture_data) {
	const bool is_ftd2_libusb = true;
	bool is_done = false;
	int inner_curr_in = 0;
	int retval = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	FTD2CaptureReceivedData* received_data_buffers = new FTD2CaptureReceivedData[NUM_CAPTURE_RECEIVED_DATA_BUFFERS];
	int curr_data_buffer = 0;
	int next_data_buffer = 0;
	int status = 0;
	uint32_t last_used_index = -1;
	uint32_t index = 0;
	size_t curr_offset = 0;
	const size_t full_size = get_capture_size(capture_data->status.device.is_rgb_888);
	size_t bytesIn;
	bool usb_thread_run = false;
	std::thread processing_thread;
	ftd2_libusb_start_thread(&processing_thread, &usb_thread_run);

	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	SharedConsumerMutex is_transfer_done_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
		received_data_buffers[i].actual_length = 0;
		received_data_buffers[i].is_data_ready = false;
		received_data_buffers[i].in_use = false;
		received_data_buffers[i].is_buffer_free_shared_mutex = &is_buffer_free_shared_mutex;
		received_data_buffers[i].status = &status;
		received_data_buffers[i].index = 0;
		received_data_buffers[i].curr_offset = &curr_offset;
		received_data_buffers[i].last_used_index = &last_used_index;
		received_data_buffers[i].capture_data = capture_data;
		received_data_buffers[i].clock_start = &clock_start;
		received_data_buffers[i].cb_data.function = ftd2_libusb_capture_process_data;
		received_data_buffers[i].cb_data.actual_user_data = &received_data_buffers[i];
		received_data_buffers[i].cb_data.transfer_data = NULL;
		received_data_buffers[i].cb_data.handle = capture_data->handle;
		received_data_buffers[i].cb_data.is_transfer_done_mutex = &is_transfer_done_mutex;
		received_data_buffers[i].cb_data.internal_index = i;
		received_data_buffers[i].cb_data.requested_length = 0;
	}

	if(!enable_capture(capture_data->handle, is_ftd2_libusb)) {
		capture_error_print(true, capture_data, "Capture enable error");
		is_done = true;
	}

	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		ftd2_libusb_start_read(ftd2_libusb_get_free_buffer(received_data_buffers), index++, full_size);


	while(capture_data->status.connected && capture_data->status.running) {
		if(get_ftd2_libusb_status(received_data_buffers) != 0) {
			capture_error_print(true, capture_data, "Disconnected: Read error");
			is_done = true;
		}
		if(is_done)
			break;
		ftd2_libusb_start_read(ftd2_libusb_get_free_buffer(received_data_buffers), index++, full_size);
		resync_offset(received_data_buffers, index, full_size);
	}
	wait_all_ftd2_libusb_buffers_free(received_data_buffers);
	ftd2_libusb_close_thread(&processing_thread, &usb_thread_run);
	delete []received_data_buffers;
}

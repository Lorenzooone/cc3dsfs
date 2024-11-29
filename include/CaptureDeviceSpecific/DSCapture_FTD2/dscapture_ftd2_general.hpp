#ifndef __DSCAPTURE_FTD2_GENERAL_HPP
#define __DSCAPTURE_FTD2_GENERAL_HPP

#include "utils.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

#include <chrono>

// Sometimes the CC returns a 2 length packet, if you're too fast
#define BASE_NUM_CAPTURE_RECEIVED_DATA_BUFFERS 4
#define NUM_CAPTURE_RECEIVED_DATA_0_MULTIPLIER 2
#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS (BASE_NUM_CAPTURE_RECEIVED_DATA_BUFFERS * NUM_CAPTURE_RECEIVED_DATA_0_MULTIPLIER)

typedef void (*fdt2_async_callback_function)(void* user_data, int transfer_length, int transfer_status);

struct ftd2_async_callback_data {
	FTD2OldDSCaptureReceivedRaw buffer_raw;
	fdt2_async_callback_function function;
	void* actual_user_data;
	void* transfer_data;
	void* handle;
	std::mutex transfer_data_access;
	SharedConsumerMutex* is_transfer_done_mutex;
	size_t requested_length;
	int internal_index;
};

struct FTD2CaptureReceivedData {
	size_t actual_length;
	volatile bool in_use;
	volatile bool is_data_ready;
	SharedConsumerMutex* is_buffer_free_shared_mutex;
	int* status;
	uint32_t index;
	uint32_t* last_used_index;
	size_t* curr_offset;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	ftd2_async_callback_data cb_data;
};

int get_num_ftd2_device_types();
const std::string get_ftd2_fw_desc(int index);
const int get_ftd2_fw_index(int index);
uint64_t get_max_samples(bool is_rgb888);
uint64_t get_capture_size(bool is_rgb888);
size_t remove_synch_from_final_length(uint32_t* out_buffer, size_t real_length);
bool enable_capture(void* handle, bool is_libftdi);
bool synchronization_check(uint16_t* data_buffer, size_t size, uint16_t* next_data_buffer, size_t* next_size, bool special_check = false);

#endif

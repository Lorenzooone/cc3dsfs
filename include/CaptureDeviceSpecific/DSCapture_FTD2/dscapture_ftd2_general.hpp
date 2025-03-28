#ifndef __DSCAPTURE_FTD2_GENERAL_HPP
#define __DSCAPTURE_FTD2_GENERAL_HPP

#include "utils.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

#include <chrono>

// Sometimes the CC returns a 2 length packet, if you're too fast
#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS NUM_CONCURRENT_DATA_BUFFER_WRITERS

typedef void (*fdt2_async_callback_function)(void* user_data, int transfer_length, int transfer_status);

struct ftd2_async_callback_data {
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
	uint8_t* buffer_raw;
	uint32_t* buffer_target;
	size_t* curr_offset;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	ftd2_async_callback_data cb_data;
};

void insert_device_ftd2_shared(std::vector<CaptureDevice> &devices_list, char* description, int debug_multiplier, std::string serial_number, void* descriptor, std::string path, std::string extra_particle);
int get_num_ftd2_device_types();
const std::string get_ftd2_fw_desc(int index);
const int get_ftd2_fw_index(int index);
uint64_t get_max_samples(bool is_rgb888);
uint64_t get_capture_size(bool is_rgb888);
size_t remove_synch_from_final_length(uint32_t* out_buffer, size_t real_length);
bool enable_capture(void* handle, bool is_ftd2_libusb);
bool synchronization_check(uint16_t* data_buffer, size_t size, uint16_t* next_data_buffer, size_t* next_size, bool special_check = false);

#endif

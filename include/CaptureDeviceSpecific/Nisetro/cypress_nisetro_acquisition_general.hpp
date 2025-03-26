#ifndef __CYPRESS_NISETRO_ACQUISITION_GENERAL_HPP
#define __CYPRESS_NISETRO_ACQUISITION_GENERAL_HPP

#include "cypress_shared_communications.hpp"
#include "cypress_nisetro_communications.hpp"

#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS NUM_CONCURRENT_DATA_BUFFER_WRITERS

struct CypressDeviceCaptureReceivedData {
	volatile bool in_use;
	uint32_t index;
	SharedConsumerMutex* is_buffer_free_shared_mutex;
	size_t* scheduled_special_read;
	uint32_t* active_special_read_index;
	bool* is_active_special_read;
	bool* recalibration_request;
	int* status;
	uint32_t* last_index;
	int* errors_since_last_output;
	int* consecutive_output_to_thread;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	cy_async_callback_data cb_data;
};

std::string cypress_nisetro_get_serial(const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id);
CaptureDevice cypress_nisetro_create_device(const void* usb_device_desc, std::string serial, std::string path);

#endif

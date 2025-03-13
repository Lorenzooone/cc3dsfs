#ifndef __CYPRESS_NISETRO_ACQUISITION_GENERAL_HPP
#define __CYPRESS_NISETRO_ACQUISITION_GENERAL_HPP

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
	cyni_async_callback_data cb_data;
};

std::string get_serial(const cyni_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id);
CaptureDevice cypress_create_device(const cyni_device_usb_device* usb_device_desc, std::string serial, std::string path);
CaptureDevice cypress_create_device(const cyni_device_usb_device* usb_device_desc, std::string serial);
void cypress_insert_device(std::vector<CaptureDevice>& devices_list, const cyni_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id, std::string path);
void cypress_insert_device(std::vector<CaptureDevice>& devices_list, const cyni_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id);
void cypress_output_to_thread(CaptureData* capture_data, CaptureReceived* capture_buf, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t read_size);
int cypress_device_read_frame_and_output(CaptureData* capture_data, CaptureReceived* capture_buf, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start);
int cypress_device_read_frame_request(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data, CaptureScreensType curr_capture_type, uint32_t index);
int cypress_device_get_num_free_buffers(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data);
void wait_all_cypress_device_buffers_free(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data);
void wait_one_cypress_device_buffer_free(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data);
bool cypress_device_are_buffers_all_free(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data);
CypressDeviceCaptureReceivedData* cypress_device_get_free_buffer(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data);
int get_cypress_device_status(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data);
void error_cypress_device_status(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val);
void reset_cypress_device_status(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data);

#endif

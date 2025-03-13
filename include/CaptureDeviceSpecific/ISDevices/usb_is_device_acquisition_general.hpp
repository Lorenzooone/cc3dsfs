#ifndef __USB_IS_DEVICE_ACQUISITION_GENERAL_HPP
#define __USB_IS_DEVICE_ACQUISITION_GENERAL_HPP

#include "usb_is_device_communications.hpp"
#include "utils.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS NUM_CONCURRENT_DATA_BUFFER_WRITERS

struct ISDeviceCaptureReceivedData {
	volatile bool in_use;
	uint32_t index;
	SharedConsumerMutex* is_buffer_free_shared_mutex;
	int* status;
	uint32_t* last_index;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	CaptureScreensType curr_capture_type;
	isd_async_callback_data cb_data;
};

int set_acquisition_mode(is_device_device_handlers* handlers, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, const is_device_usb_device* usb_device_desc);
int EndAcquisition(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, bool do_drain_frames, int start_frames, CaptureScreensType capture_type);
int EndAcquisition(const is_device_usb_device* usb_device_desc, is_device_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type);
int is_device_read_frame_and_output(CaptureData* capture_data, int internal_index, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start);
void is_device_read_frame_request(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data, CaptureScreensType curr_capture_type, uint32_t index);
uint64_t usb_is_device_get_video_in_size(CaptureScreensType capture_type, is_device_type device_type);
void output_to_thread(CaptureData* capture_data, int internal_index, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t read_size);

ISDeviceCaptureReceivedData* is_device_get_free_buffer(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data);
bool is_device_are_buffers_all_free(ISDeviceCaptureReceivedData* is_device_capture_recv_data);
int is_device_get_num_free_buffers(ISDeviceCaptureReceivedData* is_device_capture_recv_data);
void wait_all_is_device_transfers_done(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data);
void wait_all_is_device_buffers_free(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data);
void wait_one_is_device_buffer_free(CaptureData* capture_data, ISDeviceCaptureReceivedData* is_device_capture_recv_data);
int get_is_device_status(ISDeviceCaptureReceivedData* is_device_capture_recv_data);
void error_is_device_status(ISDeviceCaptureReceivedData* is_device_capture_recv_data, int error_value);
void reset_is_device_status(ISDeviceCaptureReceivedData* is_device_capture_recv_data);

#endif

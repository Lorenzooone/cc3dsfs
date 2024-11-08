#ifndef __USB_IS_NITRO_ACQUISITION_GENERAL_HPP
#define __USB_IS_NITRO_ACQUISITION_GENERAL_HPP

#include "usb_is_nitro_communications.hpp"
#include "utils.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS 4

struct ISNitroCaptureReceivedData {
	volatile bool in_use;
	uint32_t index;
	CaptureReceived buffer;
	SharedConsumerMutex* is_buffer_free_shared_mutex;
	int* status;
	uint32_t* last_index;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	CaptureScreensType curr_capture_type;
	isn_async_callback_data cb_data;
};

uint64_t _is_nitro_get_video_in_size(CaptureScreensType capture_type);
int set_acquisition_mode(is_nitro_device_handlers* handlers, CaptureScreensType capture_type, CaptureSpeedsType capture_speed, const is_nitro_usb_device* usb_device_desc);
int EndAcquisition(CaptureData* capture_data, ISNitroCaptureReceivedData* is_nitro_capture_recv_data, bool do_drain_frames, int start_frames, CaptureScreensType capture_type);
int EndAcquisition(const is_nitro_usb_device* usb_device_desc, is_nitro_device_handlers* handlers, bool do_drain_frames, int start_frames, CaptureScreensType capture_type);
int is_nitro_read_frame_and_output(CaptureData* capture_data, CaptureReceived* capture_buf, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start);
void is_nitro_read_frame_request(CaptureData* capture_data, ISNitroCaptureReceivedData* is_nitro_capture_recv_data, CaptureScreensType curr_capture_type, uint32_t index);

ISNitroCaptureReceivedData* is_nitro_get_free_buffer(CaptureData* capture_data, ISNitroCaptureReceivedData* is_nitro_capture_recv_data);
bool is_nitro_are_buffers_all_free(ISNitroCaptureReceivedData* is_nitro_capture_recv_data);
int is_nitro_get_num_free_buffers(ISNitroCaptureReceivedData* is_nitro_capture_recv_data);
void wait_all_is_nitro_transfers_done(CaptureData* capture_data, ISNitroCaptureReceivedData* is_nitro_capture_recv_data);
void wait_all_is_nitro_buffers_free(CaptureData* capture_data, ISNitroCaptureReceivedData* is_nitro_capture_recv_data);
void wait_one_is_nitro_buffer_free(CaptureData* capture_data, ISNitroCaptureReceivedData* is_nitro_capture_recv_data);
int get_is_nitro_status(ISNitroCaptureReceivedData* is_nitro_capture_recv_data);
void reset_is_nitro_status(ISNitroCaptureReceivedData* is_nitro_capture_recv_data);

#endif

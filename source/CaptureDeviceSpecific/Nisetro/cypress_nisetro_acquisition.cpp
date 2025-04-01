#include "devicecapture.hpp"
#include "cypress_shared_driver_comms.hpp"
#include "cypress_shared_libusb_comms.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_nisetro_communications.hpp"
#include "cypress_nisetro_acquisition.hpp"
#include "cypress_nisetro_acquisition_general.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>

#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS NUM_CONCURRENT_DATA_BUFFER_WRITERS

// The driver only seems to support up to 4 concurrent reads. Not more...
#ifdef NUM_CAPTURE_RECEIVED_DATA_BUFFERS
#if NUM_CAPTURE_RECEIVED_DATA_BUFFERS > 4
#define NUM_NISETRO_CYPRESS_BUFFERS 4
#else
#define NUM_NISETRO_CYPRESS_BUFFERS NUM_CAPTURE_RECEIVED_DATA_BUFFERS
#endif
#endif

#define MAX_TIME_WAIT 1.0
#define MAX_ERRORS_ALLOWED 100
#define NUM_CONSECUTIVE_NEEDED_OUTPUT 6

#define NISETRO_CYPRESS_USB_WINDOWS_DRIVER CYPRESS_WINDOWS_DEFAULT_USB_DRIVER

struct CypressNisetroDeviceCaptureReceivedData {
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

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status);
static int get_cypress_device_status(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data);
static void error_cypress_device_status(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val);

static cy_device_device_handlers* usb_find_by_serial_number(const cyni_device_usb_device* usb_device_desc, std::string wanted_serial_number, CaptureDevice* new_device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device);

	if(final_handlers == NULL)
		final_handlers = cypress_driver_find_by_serial_number(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device, NISETRO_CYPRESS_USB_WINDOWS_DRIVER);
	return final_handlers;
}

static int usb_find_free_fw_id(const cyni_device_usb_device* usb_device_desc) {
	int curr_serial_extra_id = 0;
	const int num_free_fw_ids = 0x100;
	bool found[num_free_fw_ids];
	for(int i = 0; i < num_free_fw_ids; i++)
		found[i] = false;
	cypress_libusb_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id);
	cypress_driver_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id, NISETRO_CYPRESS_USB_WINDOWS_DRIVER);

	for(int i = 0; i < num_free_fw_ids; i++)
		if(!found[i])
			return i;
	return 0;
}

static cy_device_device_handlers* usb_reconnect(const cyni_device_usb_device* usb_device_desc, CaptureDevice* device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), device->serial_number, curr_serial_extra_id, NULL);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_serial_reconnection(device);
	return final_handlers;
}

static std::string _cypress_nisetro_get_serial(const cyni_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if((!usb_device_desc->has_bcd_device_serial) && (serial != ""))
		return serial;
	if(usb_device_desc->has_bcd_device_serial)
		return std::to_string(bcd_device & 0xFF);
	return std::to_string((curr_serial_extra_id++) + 1);
}

std::string cypress_nisetro_get_serial(const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if(usb_device_desc == NULL)
		return "";
	return _cypress_nisetro_get_serial((const cyni_device_usb_device*)usb_device_desc, serial, bcd_device, curr_serial_extra_id);
}

static CaptureDevice _cypress_nisetro_create_device(const cyni_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	return CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_CYPRESS_NISETRO, (void*)usb_device_desc, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS, usb_device_desc->video_data_type, path);
}

CaptureDevice cypress_nisetro_create_device(const void* usb_device_desc, std::string serial, std::string path) {
	if(usb_device_desc == NULL)
		return CaptureDevice();
	return _cypress_nisetro_create_device((const cyni_device_usb_device*)usb_device_desc, serial, path);
}

static void cypress_nisetro_connection_end(cy_device_device_handlers* handlers, const cyni_device_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		cypress_libusb_end_connection(handlers, get_cy_usb_info(device_desc), interface_claimed);
	else
		cypress_driver_end_connection(handlers);
	delete handlers;
}

void list_devices_cyni_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	const size_t num_cyni_device_desc = GetNumCyNiDeviceDesc();
	int* curr_serial_extra_id_cyni_device = new int[num_cyni_device_desc];
	bool* no_access_elems = new bool[num_cyni_device_desc];
	bool* not_supported_elems = new bool[num_cyni_device_desc];
	std::vector<const cy_device_usb_device*> usb_devices_to_check;
	for (int i = 0; i < num_cyni_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_cyni_device[i] = 0;
		const cyni_device_usb_device* curr_device_desc = GetCyNiDeviceDesc(i);
		usb_devices_to_check.push_back(get_cy_usb_info(curr_device_desc));
	}
	cypress_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_cyni_device, usb_devices_to_check);

	bool any_not_supported = false;
	for(int i = 0; i < num_cyni_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(int i = 0; i < num_cyni_device_desc; i++)
		if(no_access_elems[i])
			no_access_list.emplace_back(usb_devices_to_check[i]->vid, usb_devices_to_check[i]->pid);
	if(any_not_supported)
		cypress_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_cyni_device, usb_devices_to_check, NISETRO_CYPRESS_USB_WINDOWS_DRIVER);

	delete[] curr_serial_extra_id_cyni_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

bool cyni_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)device->descriptor;
	cy_device_device_handlers* handlers = usb_reconnect(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	if(has_to_load_firmware(usb_device_info)) {
		const cyni_device_usb_device* next_usb_device_info = GetNextDeviceDesc(usb_device_info);
		int free_fw_id = usb_find_free_fw_id(next_usb_device_info);
		int ret = load_firmware(handlers, usb_device_info, free_fw_id);
		if(!ret) {
			capture_error_print(true, capture_data, "Firmware load error");
			return false;
		}
		cypress_nisetro_connection_end(handlers, usb_device_info);
		std::string new_serial_number = std::to_string(free_fw_id);
		CaptureDevice new_device;
		for(int i = 0; i < 20; i++) {
			default_sleep(500);
			handlers = usb_find_by_serial_number(next_usb_device_info, new_serial_number, &new_device);
			if(handlers != NULL)
				break;
		}
		if(handlers == NULL) {
			capture_error_print(true, capture_data, "Device reconnection error");
			return false;
		}
		*device = new_device;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

uint64_t cyni_device_get_video_in_size(cypress_nisetro_device_type device_type) {
	return sizeof(CypressNisetroDSCaptureReceived);
}


uint64_t cyni_device_get_video_in_size(CaptureStatus* status) {
	return cyni_device_get_video_in_size(((const cyni_device_usb_device*)(status->device.descriptor))->device_type);
}


uint64_t cyni_device_get_video_in_size(CaptureData* capture_data) {
	return cyni_device_get_video_in_size(&capture_data->status);
}

static int find_first_vsync_byte(CaptureReceived* capture_buf, size_t read_size) {
	uint8_t* data_in = (uint8_t*)capture_buf->cypress_nisetro_capture_received.video_in.screen_data;
	if((data_in[0] & 0x80) && (!(data_in[read_size - 1] & 0x80)))
		return 0;
	int pos = 0;
	while((pos < read_size) && (data_in[pos] & 0x80))
		pos++;
	while((pos < read_size) && (!(data_in[pos] & 0x80)))
		pos++;
	return pos;
}

static void cypress_output_to_thread(CaptureData* capture_data, int internal_index, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t read_size, size_t* scheduled_special_read, bool* recalibration_request) {
	// Output to the other threads...
	CaptureDataSingleBuffer* data_buf = capture_data->data_buffers.GetWriterBuffer(internal_index);
	int offset = find_first_vsync_byte(&data_buf->capture_buf, read_size);
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	if(offset) {
		if(offset % get_cy_usb_info(usb_device_info)->max_usb_packet_size)
			*recalibration_request = true;
		else
			*scheduled_special_read = offset;
		capture_data->data_buffers.ReleaseWriterBuffer(internal_index, false);
		return;
	}
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - (*clock_start);
	*clock_start = curr_time;
	capture_data->data_buffers.WriteToBuffer(NULL, read_size, diff.count(), &capture_data->status.device, internal_index);
	if (capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static int cypress_device_read_frame_and_output(CaptureData* capture_data, int internal_index, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start, size_t* scheduled_special_read, bool* recalibration_request) {
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	size_t read_size = cyni_device_get_video_in_size(usb_device_info->device_type);
	if(*scheduled_special_read)
		read_size = *scheduled_special_read;
	CaptureDataSingleBuffer* data_buf = capture_data->data_buffers.GetWriterBuffer(internal_index);
	uint8_t* buffer = (uint8_t*)&data_buf->capture_buf;
	int ret = ReadFrame((cy_device_device_handlers*)capture_data->handle, buffer, read_size, usb_device_info);
	if (ret < 0)
		return ret;
	if(*scheduled_special_read) {
		*scheduled_special_read = 0;
		return ret;
	}
	cypress_output_to_thread(capture_data, internal_index, &clock_start, read_size, scheduled_special_read, recalibration_request);
	return ret;
}

static int cypress_device_read_frame_request(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t index) {
	if(cypress_device_capture_recv_data == NULL)
		return LIBUSB_SUCCESS;
	if((*cypress_device_capture_recv_data->status) < 0) {
		cypress_device_capture_recv_data->in_use = false;
		return LIBUSB_SUCCESS;
	}
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	cypress_device_capture_recv_data->index = index;
	cypress_device_capture_recv_data->cb_data.function = cypress_device_read_frame_cb;
	size_t read_size = cyni_device_get_video_in_size(usb_device_info->device_type);
	if(*cypress_device_capture_recv_data->scheduled_special_read) {
		read_size = *cypress_device_capture_recv_data->scheduled_special_read;
		*cypress_device_capture_recv_data->active_special_read_index = index;
		*cypress_device_capture_recv_data->is_active_special_read = true;
		*cypress_device_capture_recv_data->scheduled_special_read = 0;
	}
	CaptureDataSingleBuffer* data_buf = capture_data->data_buffers.GetWriterBuffer(cypress_device_capture_recv_data->cb_data.internal_index);
	uint8_t* buffer = (uint8_t*)&data_buf->capture_buf;
	return ReadFrameAsync((cy_device_device_handlers*)capture_data->handle, buffer, read_size, usb_device_info, &cypress_device_capture_recv_data->cb_data);
}

static void end_cypress_device_read_frame_cb(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool early_release) {
	if(early_release)
		cypress_device_capture_recv_data->capture_data->data_buffers.ReleaseWriterBuffer(cypress_device_capture_recv_data->cb_data.internal_index, false);
	cypress_device_capture_recv_data->in_use = false;
	cypress_device_capture_recv_data->is_buffer_free_shared_mutex->specific_unlock(cypress_device_capture_recv_data->cb_data.internal_index);
}

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status) {
	CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data = (CypressNisetroDeviceCaptureReceivedData*)user_data;
	if((*cypress_device_capture_recv_data->status) < 0)
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	if(transfer_status != LIBUSB_TRANSFER_COMPLETED) {
		int error = LIBUSB_ERROR_OTHER;
		if(transfer_status == LIBUSB_TRANSFER_TIMED_OUT)
			error = LIBUSB_ERROR_TIMEOUT;
		else {
			*cypress_device_capture_recv_data->errors_since_last_output += 1;
			*cypress_device_capture_recv_data->consecutive_output_to_thread = 0;
		}
		*cypress_device_capture_recv_data->status = error;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	}

	if(((int32_t)(cypress_device_capture_recv_data->index - (*cypress_device_capture_recv_data->last_index))) <= 0) {
		//*cypress_device_capture_recv_data->status = LIBUSB_ERROR_INTERRUPTED;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	}
	*cypress_device_capture_recv_data->consecutive_output_to_thread += 1;
	if((*cypress_device_capture_recv_data->consecutive_output_to_thread) > NUM_CONSECUTIVE_NEEDED_OUTPUT)
		*cypress_device_capture_recv_data->errors_since_last_output = 0;
	*cypress_device_capture_recv_data->last_index = cypress_device_capture_recv_data->index;

	// Realign, with multiple of max_usb_packet_size
	if((*cypress_device_capture_recv_data->is_active_special_read) && (((int32_t)(cypress_device_capture_recv_data->index - (*cypress_device_capture_recv_data->active_special_read_index))) >= 0)) {
		*cypress_device_capture_recv_data->is_active_special_read = false;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	}

	if((*cypress_device_capture_recv_data->scheduled_special_read) || (*cypress_device_capture_recv_data->is_active_special_read) || (*cypress_device_capture_recv_data->recalibration_request))
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);

	cypress_output_to_thread(cypress_device_capture_recv_data->capture_data, cypress_device_capture_recv_data->cb_data.internal_index, cypress_device_capture_recv_data->clock_start, transfer_length, cypress_device_capture_recv_data->scheduled_special_read, cypress_device_capture_recv_data->recalibration_request);
	end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, false);
}

static int cypress_device_get_num_free_buffers(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	int num_free = 0;
	for(int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++)
		if(!cypress_device_capture_recv_data[i].in_use)
			num_free += 1;
	return num_free;
}

static void close_all_reads_error(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyni_device_usb_device* usb_device_desc = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0) {
		if(!async_read_closed) {
			if(handlers->usb_handle) {
				for (int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++)
					CypressCloseAsyncRead(handlers, get_cy_usb_info(usb_device_desc), &cypress_device_capture_recv_data[i].cb_data);
			}
			else
				CypressCloseAsyncRead(handlers, get_cy_usb_info(usb_device_desc), &cypress_device_capture_recv_data[0].cb_data);
			async_read_closed = true;
		}
	}
}

static bool has_too_much_time_passed(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - start_time;
	return diff.count() > MAX_TIME_WAIT;
}

static void error_too_much_time_passed(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed, const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	if(has_too_much_time_passed(start_time)) {
		error_cypress_device_status(cypress_device_capture_recv_data, -1);
		close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	}
}

static void wait_all_cypress_device_buffers_free(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	if(cypress_device_capture_recv_data == NULL)
		return;
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	bool async_read_closed = false;
	close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++)
		while(cypress_device_capture_recv_data[i].in_use) {
			error_too_much_time_passed(capture_data, cypress_device_capture_recv_data, async_read_closed, start_time);
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
		}
}

static void wait_one_cypress_device_buffer_free(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		for(int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++) {
			if(!cypress_device_capture_recv_data[i].in_use)
				done = true;
		}
		if(!done) {
			if(has_too_much_time_passed(start_time))
				return;
			if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
				return;
			int dummy = 0;
			cypress_device_capture_recv_data[0].is_buffer_free_shared_mutex->general_timed_lock(&dummy);
		}
	}
}

static void wait_specific_cypress_device_buffer_free(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		if(!cypress_device_capture_recv_data->in_use)
			done = true;
		if(!done) {
			if(has_too_much_time_passed(start_time))
				return;
			if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
				return;
			int dummy = 0;
			cypress_device_capture_recv_data->is_buffer_free_shared_mutex->general_timed_lock(&dummy);
		}
	}
}

static bool cypress_device_are_buffers_all_free(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return cypress_device_get_num_free_buffers(cypress_device_capture_recv_data) == NUM_NISETRO_CYPRESS_BUFFERS;
}

static CypressNisetroDeviceCaptureReceivedData* cypress_device_get_free_buffer(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	wait_one_cypress_device_buffer_free(capture_data, cypress_device_capture_recv_data);
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
		return NULL;
	for(int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++)
		if(!cypress_device_capture_recv_data[i].in_use) {
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_try_lock(i);
			cypress_device_capture_recv_data[i].in_use = true;
			return &cypress_device_capture_recv_data[i];
		}
	return NULL;
}

static int get_cypress_device_status(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return *cypress_device_capture_recv_data[0].status;
}

static void error_cypress_device_status(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val) {
	if((error_val == 0) || (get_cypress_device_status(cypress_device_capture_recv_data) == 0))
		*cypress_device_capture_recv_data[0].status = error_val;
}

static void exported_error_cypress_device_status(void* data, int error_val) {
	if(data == NULL)
		return;
	return error_cypress_device_status((CypressNisetroDeviceCaptureReceivedData*)data, error_val);
}

static void reset_cypress_device_status(CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	error_cypress_device_status(cypress_device_capture_recv_data, 0);
}

static void recalibration_reads(cy_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data, CypressNisetroDeviceCaptureReceivedData* chosen_buffer, uint32_t &index) {
	// Enforce properly synchronized reads.
	// Reduces complexity and latency, at the cost of some extra
	// time between lid reopening and visible video output.
	// For differences which are multiple of max_usb_packet_size,
	// regular reads can be used. For non-multiples, use this,
	// which adds sleeps to better align.
	CaptureData* capture_data = cypress_device_capture_recv_data[0].capture_data;
	do {
		if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
			return;
		chosen_buffer->in_use = false;
		wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
		chosen_buffer->in_use = true;
		if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
			return;
		CypressNisetroDeviceCaptureReceivedData* next_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		default_sleep();
		*chosen_buffer->recalibration_request = false;
		chosen_buffer->in_use = true;
		int ret = cypress_device_read_frame_request(capture_data, chosen_buffer, index++);
		if(ret < 0) {
			error_cypress_device_status(cypress_device_capture_recv_data, ret);
			return;
		}
		ret = cypress_device_read_frame_request(capture_data, next_buffer, index++);
		if(ret < 0) {
			next_buffer->in_use = false;
			error_cypress_device_status(cypress_device_capture_recv_data, ret);
			wait_specific_cypress_device_buffer_free(capture_data, chosen_buffer);
			chosen_buffer->in_use = true;
			return;
		}
		wait_specific_cypress_device_buffer_free(capture_data, chosen_buffer);
	} while((*chosen_buffer->recalibration_request) && capture_data->status.connected && capture_data->status.running);
	chosen_buffer->in_use = true;
}

static bool cyni_device_acquisition_loop(CaptureData* capture_data, CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyni_device_usb_device* usb_device_desc = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	uint32_t index = 0;
	int ret = capture_start(handlers, usb_device_desc);
	if (ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return false;
	}
	CypressSetMaxTransferSize(handlers, get_cy_usb_info(usb_device_desc), cyni_device_get_video_in_size(usb_device_desc->device_type));
	auto clock_last_reset = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++) {
		CypressNisetroDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		ret = cypress_device_read_frame_request(capture_data, chosen_buffer, index++);
		if(ret < 0) {
			chosen_buffer->in_use = false;
			error_cypress_device_status(cypress_device_capture_recv_data, ret);
			capture_error_print(true, capture_data, "Initial Reads: Failed");
			return false;
		}
	}

	StartCaptureDma(handlers, usb_device_desc);
	while (capture_data->status.connected && capture_data->status.running) {
		ret = get_cypress_device_status(cypress_device_capture_recv_data);
		if(ret < 0) {
			wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
			if((*cypress_device_capture_recv_data[0].errors_since_last_output) > MAX_ERRORS_ALLOWED) {
				capture_error_print(true, capture_data, "Disconnected: Read error");
				return false;
			}
			*cypress_device_capture_recv_data[0].recalibration_request = false;
			*cypress_device_capture_recv_data[0].scheduled_special_read = 0;
			*cypress_device_capture_recv_data[0].is_active_special_read = false;
			cypress_pipe_reset_bulk_in(handlers, get_cy_usb_info(usb_device_desc));
			reset_cypress_device_status(cypress_device_capture_recv_data);
			default_sleep(100);
		}
		CypressNisetroDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		if(chosen_buffer == NULL)
			error_cypress_device_status(cypress_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
		if(chosen_buffer && (*chosen_buffer->recalibration_request)) {
			*chosen_buffer->recalibration_request = false;
			recalibration_reads(handlers, usb_device_desc, cypress_device_capture_recv_data, chosen_buffer, index);
		}
		ret = cypress_device_read_frame_request(capture_data, chosen_buffer, index++);
		if(ret < 0) {
			chosen_buffer->in_use = false;
			error_cypress_device_status(cypress_device_capture_recv_data, ret);
			capture_error_print(true, capture_data, "Setup Read: Failed");
			return false;
		}
	}
	return true;
}

void cyni_device_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	bool is_done_thread;
	std::thread async_processing_thread;
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyni_device_usb_device* usb_device_desc = (const cyni_device_usb_device*)capture_data->status.device.descriptor;

	uint32_t last_index = -1;
	int status = 0;
	std::vector<cy_async_callback_data*> cb_queue;
	int queue_elems = 0;
	size_t scheduled_special_read = 0;
	uint32_t active_special_read_index = 0;
	bool is_active_special_read = false;
	bool one_transfer_active = false;
	bool recalibration_request = false;
	int errors_since_last_output = 0;
	int consecutive_output_to_thread = 0;
	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_NISETRO_CYPRESS_BUFFERS);
	SharedConsumerMutex is_transfer_done_shared_mutex(NUM_NISETRO_CYPRESS_BUFFERS);
	SharedConsumerMutex is_transfer_data_ready_shared_mutex(NUM_NISETRO_CYPRESS_BUFFERS);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	CypressNisetroDeviceCaptureReceivedData* cypress_device_capture_recv_data = new CypressNisetroDeviceCaptureReceivedData[NUM_NISETRO_CYPRESS_BUFFERS];
	for(int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++) {
		cypress_device_capture_recv_data[i].in_use = false;
		cypress_device_capture_recv_data[i].index = i;
		cypress_device_capture_recv_data[i].capture_data = capture_data;
		cypress_device_capture_recv_data[i].scheduled_special_read = &scheduled_special_read;
		cypress_device_capture_recv_data[i].active_special_read_index = &active_special_read_index;
		cypress_device_capture_recv_data[i].is_active_special_read = &is_active_special_read;
		cypress_device_capture_recv_data[i].last_index = &last_index;
		cypress_device_capture_recv_data[i].errors_since_last_output = &errors_since_last_output;
		cypress_device_capture_recv_data[i].recalibration_request = &recalibration_request;
		cypress_device_capture_recv_data[i].consecutive_output_to_thread = &consecutive_output_to_thread;
		cypress_device_capture_recv_data[i].clock_start = &clock_start;
		cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex = &is_buffer_free_shared_mutex;
		cypress_device_capture_recv_data[i].status = &status;
		cypress_device_capture_recv_data[i].cb_data.actual_user_data = &cypress_device_capture_recv_data[i];
		cypress_device_capture_recv_data[i].cb_data.transfer_data = NULL;
		cypress_device_capture_recv_data[i].cb_data.is_transfer_done_mutex = &is_transfer_done_shared_mutex;
		cypress_device_capture_recv_data[i].cb_data.internal_index = i;
		cypress_device_capture_recv_data[i].cb_data.is_transfer_data_ready_mutex = &is_transfer_data_ready_shared_mutex;
		cypress_device_capture_recv_data[i].cb_data.in_use_ptr = &cypress_device_capture_recv_data[i].in_use;
		cypress_device_capture_recv_data[i].cb_data.error_function = exported_error_cypress_device_status;
		cb_queue.push_back(&cypress_device_capture_recv_data[i].cb_data);
	}
	CypressSetupCypressDeviceAsyncThread(handlers, cb_queue, &async_processing_thread, &is_done_thread);
	bool proper_return = cyni_device_acquisition_loop(capture_data, cypress_device_capture_recv_data);
	wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
	CypressEndCypressDeviceAsyncThread(handlers, cb_queue, &async_processing_thread, &is_done_thread);
	delete []cypress_device_capture_recv_data;

	if(proper_return)
		capture_end(handlers, usb_device_desc);
}

void usb_cyni_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	for(int i = 0; i < NUM_NISETRO_CYPRESS_BUFFERS; i++)
		capture_data->data_buffers.ReleaseWriterBuffer(i, false);
	cypress_nisetro_connection_end((cy_device_device_handlers*)capture_data->handle, (const cyni_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}
void usb_cyni_device_init() {
	return usb_init();
}

void usb_cyni_device_close() {
	usb_close();
}


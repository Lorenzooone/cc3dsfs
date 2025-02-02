#include "devicecapture.hpp"
#include "cypress_nisetro_driver_comms.hpp"
#include "cypress_nisetro_libusb_comms.hpp"
#include "cypress_nisetro_communications.hpp"
#include "cypress_nisetro_acquisition.hpp"
#include "cypress_nisetro_acquisition_general.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>

#define SERIAL_NUMBER_SIZE (0x100 + 1)
#define MAX_TIME_WAIT 1.0
#define MAX_ERRORS_ALLOWED 100
#define NUM_CONSECUTIVE_NEEDED_OUTPUT 6

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status);

static cyni_device_device_handlers* usb_find_by_serial_number(const cyni_device_usb_device* usb_device_desc, std::string wanted_serial_number, CaptureDevice* new_device) {
	cyni_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(usb_device_desc, wanted_serial_number, curr_serial_extra_id, new_device);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_find_by_serial_number(usb_device_desc, wanted_serial_number, curr_serial_extra_id, new_device);
	return final_handlers;
}

static int usb_find_free_fw_id(const cyni_device_usb_device* usb_device_desc) {
	int curr_serial_extra_id = 0;
	const int num_free_fw_ids = 0x100;
	bool found[num_free_fw_ids];
	for(int i = 0; i < num_free_fw_ids; i++)
		found[i] = false;
	cypress_libusb_find_used_serial(usb_device_desc, found, num_free_fw_ids, curr_serial_extra_id);
	cypress_driver_find_used_serial(usb_device_desc, found, num_free_fw_ids, curr_serial_extra_id);

	for(int i = 0; i < num_free_fw_ids; i++)
		if(!found[i])
			return i;
	return 0;
}

static cyni_device_device_handlers* usb_reconnect(const cyni_device_usb_device* usb_device_desc, CaptureDevice* device) {
	cyni_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(usb_device_desc, device->serial_number, curr_serial_extra_id, NULL);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_serial_reconnection(device);
	return final_handlers;
}

std::string get_serial(const cyni_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if(serial != "")
		return serial;
	if(usb_device_desc->has_bcd_device_serial)
		return std::to_string(bcd_device & 0xFF);
	return std::to_string((curr_serial_extra_id++) + 1);
}

static void cypress_nisetro_connection_end(cyni_device_device_handlers* handlers, const cyni_device_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		cypress_libusb_end_connection(handlers, device_desc, interface_claimed);
	else
		cypress_nisetro_driver_end_connection(handlers);
	delete handlers;
}

CaptureDevice cypress_create_device(const cyni_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	return CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, path, CAPTURE_CONN_CYPRESS_NISETRO, (void*)usb_device_desc, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS, usb_device_desc->video_data_type);
}

CaptureDevice cypress_create_device(const cyni_device_usb_device* usb_device_desc, std::string serial) {
	return CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_CYPRESS_NISETRO, (void*)usb_device_desc, false, false, false, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS, usb_device_desc->video_data_type);;
}

void cypress_insert_device(std::vector<CaptureDevice>& devices_list, const cyni_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id, std::string path) {
	devices_list.push_back(cypress_create_device(usb_device_desc, get_serial(usb_device_desc, serial, bcd_device, curr_serial_extra_id), path));
}

void cypress_insert_device(std::vector<CaptureDevice>& devices_list, const cyni_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	devices_list.push_back(cypress_create_device(usb_device_desc, get_serial(usb_device_desc, serial, bcd_device, curr_serial_extra_id)));
}

void list_devices_cyni_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	const size_t num_cyni_device_desc = GetNumCyNiDeviceDesc();
	int* curr_serial_extra_id_cyni_device = new int[num_cyni_device_desc];
	bool* no_access_elems = new bool[num_cyni_device_desc];
	bool* not_supported_elems = new bool[num_cyni_device_desc];
	for (int i = 0; i < num_cyni_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_cyni_device[i] = 0;
	}
	cypress_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_cyni_device, num_cyni_device_desc);

	bool any_not_supported = false;
	for(int i = 0; i < num_cyni_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(int i = 0; i < num_cyni_device_desc; i++)
		if(no_access_elems[i]) {
			const cyni_device_usb_device* usb_device = GetCyNiDeviceDesc(i);
			no_access_list.emplace_back(usb_device->vid, usb_device->pid);
		}
	if(any_not_supported)
		cypress_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_cyni_device, num_cyni_device_desc);

	delete[] curr_serial_extra_id_cyni_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

bool cyni_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)device->descriptor;
	cyni_device_device_handlers* handlers = usb_reconnect(usb_device_info, device);
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

void cypress_output_to_thread(CaptureData* capture_data, CaptureReceived* capture_buf, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t read_size, size_t* scheduled_special_read, bool* recalibration_request) {
	// Output to the other threads...
	int offset = find_first_vsync_byte(capture_buf, read_size);
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	if(offset) {
		if(offset % usb_device_info->max_usb_packet_size)
			*recalibration_request = true;
		else
			*scheduled_special_read = offset;
		return;
	}
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - (*clock_start);
	*clock_start = curr_time;
	capture_data->data_buffers.WriteToBuffer(capture_buf, read_size, diff.count(), &capture_data->status.device, curr_capture_type);
	if (capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

int cypress_device_read_frame_and_output(CaptureData* capture_data, CaptureReceived* capture_buf, CaptureScreensType curr_capture_type, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_start, size_t* scheduled_special_read, bool* recalibration_request) {
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	size_t read_size = cyni_device_get_video_in_size(usb_device_info->device_type);
	if(*scheduled_special_read)
		read_size = *scheduled_special_read;
	int ret = ReadFrame((cyni_device_device_handlers*)capture_data->handle, (uint8_t*)capture_buf, read_size, usb_device_info);
	if (ret < 0)
		return ret;
	if(*scheduled_special_read) {
		*scheduled_special_read = 0;
		return ret;
	}
	cypress_output_to_thread(capture_data, capture_buf, curr_capture_type, &clock_start, read_size, scheduled_special_read, recalibration_request);
	return ret;
}

int cypress_device_read_frame_request(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data, CaptureScreensType curr_capture_type, uint32_t index) {
	if(cypress_device_capture_recv_data == NULL)
		return LIBUSB_SUCCESS;
	if((*cypress_device_capture_recv_data->status) < 0) {
		cypress_device_capture_recv_data->in_use = false;
		return LIBUSB_SUCCESS;
	}
	const cyni_device_usb_device* usb_device_info = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	cypress_device_capture_recv_data->index = index;
	cypress_device_capture_recv_data->curr_capture_type = curr_capture_type;
	cypress_device_capture_recv_data->cb_data.function = cypress_device_read_frame_cb;
	size_t read_size = cyni_device_get_video_in_size(usb_device_info->device_type);
	if(*cypress_device_capture_recv_data->scheduled_special_read) {
		read_size = *cypress_device_capture_recv_data->scheduled_special_read;
		*cypress_device_capture_recv_data->active_special_read_index = index;
		*cypress_device_capture_recv_data->is_active_special_read = true;
		*cypress_device_capture_recv_data->scheduled_special_read = 0;
	}
	return ReadFrameAsync((cyni_device_device_handlers*)capture_data->handle, (uint8_t*)&cypress_device_capture_recv_data->buffer, read_size, usb_device_info, &cypress_device_capture_recv_data->cb_data);
}

static void end_cypress_device_read_frame_cb(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cypress_device_capture_recv_data->in_use = false;
	cypress_device_capture_recv_data->is_buffer_free_shared_mutex->specific_unlock(cypress_device_capture_recv_data->cb_data.internal_index);
}

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status) {
	CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data = (CypressDeviceCaptureReceivedData*)user_data;
	if((*cypress_device_capture_recv_data->status) < 0)
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data);
	if(transfer_status != LIBUSB_TRANSFER_COMPLETED) {
		int error = LIBUSB_ERROR_OTHER;
		if(transfer_status == LIBUSB_TRANSFER_TIMED_OUT)
			error = LIBUSB_ERROR_TIMEOUT;
		else {
			*cypress_device_capture_recv_data->errors_since_last_output += 1;
			*cypress_device_capture_recv_data->consecutive_output_to_thread = 0;
		}
		*cypress_device_capture_recv_data->status = error;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data);
	}

	if(((int32_t)(cypress_device_capture_recv_data->index - (*cypress_device_capture_recv_data->last_index))) <= 0) {
		//*cypress_device_capture_recv_data->status = LIBUSB_ERROR_INTERRUPTED;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data);
	}
	*cypress_device_capture_recv_data->consecutive_output_to_thread += 1;
	if((*cypress_device_capture_recv_data->consecutive_output_to_thread) > NUM_CONSECUTIVE_NEEDED_OUTPUT)
		*cypress_device_capture_recv_data->errors_since_last_output = 0;
	*cypress_device_capture_recv_data->last_index = cypress_device_capture_recv_data->index;

	// Realign, with multiple of max_usb_packet_size
	if((*cypress_device_capture_recv_data->is_active_special_read) && (((int32_t)(cypress_device_capture_recv_data->index - (*cypress_device_capture_recv_data->active_special_read_index))) >= 0)) {
		*cypress_device_capture_recv_data->is_active_special_read = false;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data);
	}

	if((*cypress_device_capture_recv_data->scheduled_special_read) || (*cypress_device_capture_recv_data->is_active_special_read) || (*cypress_device_capture_recv_data->recalibration_request))
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data);

	cypress_output_to_thread(cypress_device_capture_recv_data->capture_data, &cypress_device_capture_recv_data->buffer, cypress_device_capture_recv_data->curr_capture_type, cypress_device_capture_recv_data->clock_start, transfer_length, cypress_device_capture_recv_data->scheduled_special_read, cypress_device_capture_recv_data->recalibration_request);
	end_cypress_device_read_frame_cb(cypress_device_capture_recv_data);
}

int cypress_device_get_num_free_buffers(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	int num_free = 0;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!cypress_device_capture_recv_data[i].in_use)
			num_free += 1;
	return num_free;
}

static void close_all_reads_error(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed) {
	cyni_device_device_handlers* handlers = (cyni_device_device_handlers*)capture_data->handle;
	const cyni_device_usb_device* usb_device_desc = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0) {
		if(!async_read_closed) {
			if(handlers->usb_handle) {
				for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
					CloseAsyncRead(handlers, usb_device_desc, &cypress_device_capture_recv_data[i].cb_data);
			}
			else
				CloseAsyncRead(handlers, usb_device_desc, &cypress_device_capture_recv_data[0].cb_data);
			async_read_closed = true;
		}
	}
}

static bool has_too_much_time_passed(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - start_time;
	return diff.count() > MAX_TIME_WAIT;
}

static void error_too_much_time_passed(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed, const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	if(has_too_much_time_passed(start_time)) {
		error_cypress_device_status(cypress_device_capture_recv_data, -1);
		close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	}
}

void wait_all_cypress_device_buffers_free(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	if(cypress_device_capture_recv_data == NULL)
		return;
	cyni_device_device_handlers* handlers = (cyni_device_device_handlers*)capture_data->handle;
	bool async_read_closed = false;
	close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		while(cypress_device_capture_recv_data[i].in_use) {
			error_too_much_time_passed(capture_data, cypress_device_capture_recv_data, async_read_closed, start_time);
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
		}
}

void wait_one_cypress_device_buffer_free(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cyni_device_device_handlers* handlers = (cyni_device_device_handlers*)capture_data->handle;
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
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

void wait_specific_cypress_device_buffer_free(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cyni_device_device_handlers* handlers = (cyni_device_device_handlers*)capture_data->handle;
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

bool cypress_device_are_buffers_all_free(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return cypress_device_get_num_free_buffers(cypress_device_capture_recv_data) == NUM_CAPTURE_RECEIVED_DATA_BUFFERS;
}

CypressDeviceCaptureReceivedData* cypress_device_get_free_buffer(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	wait_one_cypress_device_buffer_free(capture_data, cypress_device_capture_recv_data);
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
		return NULL;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!cypress_device_capture_recv_data[i].in_use) {
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_try_lock(i);
			cypress_device_capture_recv_data[i].in_use = true;
			return &cypress_device_capture_recv_data[i];
		}
	return NULL;
}

int get_cypress_device_status(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return *cypress_device_capture_recv_data[0].status;
}

void error_cypress_device_status(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val) {
	if((error_val == 0) || (get_cypress_device_status(cypress_device_capture_recv_data) == 0))
		*cypress_device_capture_recv_data[0].status = error_val;
}

void reset_cypress_device_status(CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	error_cypress_device_status(cypress_device_capture_recv_data, 0);
}

void recalibration_reads(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data, CypressDeviceCaptureReceivedData* chosen_buffer, uint32_t &index, CaptureScreensType curr_capture_type) {
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
		CypressDeviceCaptureReceivedData* next_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		default_sleep();
		*chosen_buffer->recalibration_request = false;
		chosen_buffer->in_use = true;
		int ret = cypress_device_read_frame_request(capture_data, chosen_buffer, curr_capture_type, index++);
		if(ret < 0) {
			error_cypress_device_status(cypress_device_capture_recv_data, ret);
			return;
		}
		ret = cypress_device_read_frame_request(capture_data, next_buffer, curr_capture_type, index++);
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

static bool cyni_device_acquisition_loop(CaptureData* capture_data, CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cyni_device_device_handlers* handlers = (cyni_device_device_handlers*)capture_data->handle;
	const cyni_device_usb_device* usb_device_desc = (const cyni_device_usb_device*)capture_data->status.device.descriptor;
	uint32_t index = 0;
	CaptureScreensType curr_capture_type = capture_data->status.capture_type;
	CaptureSpeedsType curr_capture_speed = capture_data->status.capture_speed;
	int ret = capture_start(handlers, usb_device_desc);
	if (ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return false;
	}
	SetMaxTransferSize(handlers, usb_device_desc, cyni_device_get_video_in_size(usb_device_desc->device_type));
	auto clock_last_reset = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
		CypressDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		ret = cypress_device_read_frame_request(capture_data, chosen_buffer, curr_capture_type, index++);
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
			pipe_reset_bulk_in(handlers, usb_device_desc);
			reset_cypress_device_status(cypress_device_capture_recv_data);
			default_sleep(100);
		}
		CypressDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		if(chosen_buffer == NULL)
			error_cypress_device_status(cypress_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
		if(chosen_buffer && (*chosen_buffer->recalibration_request)) {
			*chosen_buffer->recalibration_request = false;
			recalibration_reads(handlers, usb_device_desc, cypress_device_capture_recv_data, chosen_buffer, index, curr_capture_type);
		}
		ret = cypress_device_read_frame_request(capture_data, chosen_buffer, curr_capture_type, index++);
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
	ConsumerMutex has_data_been_processed;
	std::thread async_processing_thread;
	cyni_device_device_handlers* handlers = (cyni_device_device_handlers*)capture_data->handle;
	const cyni_device_usb_device* usb_device_desc = (const cyni_device_usb_device*)capture_data->status.device.descriptor;

	uint32_t last_index = -1;
	int status = 0;
	cyni_async_callback_data* cb_queue[NUM_CAPTURE_RECEIVED_DATA_BUFFERS];
	int queue_elems = 0;
	size_t scheduled_special_read = 0;
	uint32_t active_special_read_index = 0;
	bool is_active_special_read = false;
	bool one_transfer_active = false;
	bool recalibration_request = false;
	int errors_since_last_output = 0;
	int consecutive_output_to_thread = 0;
	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	SharedConsumerMutex is_transfer_done_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	SharedConsumerMutex is_transfer_data_ready_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	CypressDeviceCaptureReceivedData* cypress_device_capture_recv_data = new CypressDeviceCaptureReceivedData[NUM_CAPTURE_RECEIVED_DATA_BUFFERS];
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
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
	}
	SetupCypressDeviceAsyncThread(handlers, cypress_device_capture_recv_data, &async_processing_thread, &is_done_thread, &has_data_been_processed);
	bool proper_return = cyni_device_acquisition_loop(capture_data, cypress_device_capture_recv_data);
	wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
	EndCypressDeviceAsyncThread(handlers, cypress_device_capture_recv_data, &async_processing_thread, &is_done_thread, &has_data_been_processed);
	delete []cypress_device_capture_recv_data;

	if(proper_return)
		capture_end(handlers, usb_device_desc);
}

void usb_cyni_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	cypress_nisetro_connection_end((cyni_device_device_handlers*)capture_data->handle, (const cyni_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}
void usb_cyni_device_init() {
	return usb_init();
}

void usb_cyni_device_close() {
	usb_close();
}


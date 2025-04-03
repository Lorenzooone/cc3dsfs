#include "devicecapture.hpp"
#include "cypress_shared_driver_comms.hpp"
#include "cypress_shared_libusb_comms.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_optimize_new_3ds_communications.hpp"
#include "cypress_optimize_new_3ds_acquisition.hpp"
#include "cypress_optimize_new_3ds_acquisition_general.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>

// This code was developed by exclusively looking at Wireshark USB packet
// captures to learn the USB device's protocol.
// As such, it was developed in a clean room environment, to provide
// compatibility of the USB device with more (and more modern) hardware.
// Such an action is allowed under EU law.
// No reverse engineering of the original software was done to create this.

#define NUM_CAPTURE_RECEIVED_DATA_BUFFERS NUM_CONCURRENT_DATA_BUFFER_WRITERS

// The driver only seems to support up to 4 concurrent reads. Not more...
#ifdef NUM_CAPTURE_RECEIVED_DATA_BUFFERS
#if NUM_CAPTURE_RECEIVED_DATA_BUFFERS > 4
#define NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS 4
#else
#define NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS NUM_CAPTURE_RECEIVED_DATA_BUFFERS
#endif
#endif

#define MAX_TIME_WAIT 1.0

#define OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER CYPRESS_WINDOWS_OPTIMIZE_NEW_USB_DRIVER

struct CypressOptimizeNew3DSDeviceCaptureReceivedData {
	volatile bool in_use;
	uint32_t index;
	SharedConsumerMutex* is_buffer_free_shared_mutex;
	int* status;
	uint32_t* last_index;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	cy_async_callback_data cb_data;
};

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status);
static int get_cypress_device_status(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data);
static void error_cypress_device_status(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val);

static cy_device_device_handlers* usb_find_by_serial_number(const cyopn_device_usb_device* usb_device_desc, std::string wanted_serial_number, CaptureDevice* new_device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device);

	if(final_handlers == NULL)
		final_handlers = cypress_driver_find_by_serial_number(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device, OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER);
	return final_handlers;
}

static int usb_find_free_fw_id(const cyopn_device_usb_device* usb_device_desc) {
	int curr_serial_extra_id = 0;
	const int num_free_fw_ids = 0x100;
	bool found[num_free_fw_ids];
	for(int i = 0; i < num_free_fw_ids; i++)
		found[i] = false;
	cypress_libusb_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id);
	cypress_driver_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id, OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER);

	for(int i = 0; i < num_free_fw_ids; i++)
		if(!found[i])
			return i;
	return 0;
}

static cy_device_device_handlers* usb_reconnect(const cyopn_device_usb_device* usb_device_desc, CaptureDevice* device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), device->serial_number, curr_serial_extra_id, NULL);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_serial_reconnection(device);
	return final_handlers;
}

static std::string _cypress_optimize_new_3ds_get_serial(const cyopn_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if((!usb_device_desc->has_bcd_device_serial) && (serial != ""))
		return serial;
	if(usb_device_desc->has_bcd_device_serial)
		return std::to_string(bcd_device & 0xFF);
	return std::to_string((curr_serial_extra_id++) + 1);
}

std::string cypress_optimize_new_3ds_get_serial(const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if(usb_device_desc == NULL)
		return "";
	return _cypress_optimize_new_3ds_get_serial((const cyopn_device_usb_device*)usb_device_desc, serial, bcd_device, curr_serial_extra_id);
}

static CaptureDevice _cypress_optimize_new_3ds_create_device(const cyopn_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	return CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_CYPRESS_NEW_OPTIMIZE, (void*)usb_device_desc, true, true, true, HEIGHT_3DS, TOP_WIDTH_3DS + BOT_WIDTH_3DS, HEIGHT_3DS, (TOP_WIDTH_3DS * 2) + BOT_WIDTH_3DS, OPTIMIZE_NEW_3DS_AUDIO_BUFFER_MAX_SIZE, 90, BOT_WIDTH_3DS, 0, BOT_WIDTH_3DS + TOP_WIDTH_3DS, 0, 0, 0, false, VIDEO_DATA_RGB16, 0x200, path);
}

CaptureDevice cypress_optimize_new_3ds_create_device(const void* usb_device_desc, std::string serial, std::string path) {
	if(usb_device_desc == NULL)
		return CaptureDevice();
	return _cypress_optimize_new_3ds_create_device((const cyopn_device_usb_device*)usb_device_desc, serial, path);
}

static void cypress_optimize_new_3ds_connection_end(cy_device_device_handlers* handlers, const cyopn_device_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		cypress_libusb_end_connection(handlers, get_cy_usb_info(device_desc), interface_claimed);
	else
		cypress_driver_end_connection(handlers);
	delete handlers;
}

void list_devices_cyopn_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	const size_t num_cyopn_device_desc = GetNumCyOpNDeviceDesc();
	int* curr_serial_extra_id_cyopn_device = new int[num_cyopn_device_desc];
	bool* no_access_elems = new bool[num_cyopn_device_desc];
	bool* not_supported_elems = new bool[num_cyopn_device_desc];
	std::vector<const cy_device_usb_device*> usb_devices_to_check;
	for (int i = 0; i < num_cyopn_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_cyopn_device[i] = 0;
		const cyopn_device_usb_device* curr_device_desc = GetCyOpNDeviceDesc(i);
		usb_devices_to_check.push_back(get_cy_usb_info(curr_device_desc));
	}
	cypress_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_cyopn_device, usb_devices_to_check);

	bool any_not_supported = false;
	for(int i = 0; i < num_cyopn_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(int i = 0; i < num_cyopn_device_desc; i++)
		if(no_access_elems[i])
			no_access_list.emplace_back(usb_devices_to_check[i]->vid, usb_devices_to_check[i]->pid);
	if(any_not_supported)
		cypress_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_cyopn_device, usb_devices_to_check, OPTIMIZE_NEW_3DS_CYPRESS_USB_WINDOWS_DRIVER);

	delete[] curr_serial_extra_id_cyopn_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

bool cyopn_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const cyopn_device_usb_device* usb_device_info = (const cyopn_device_usb_device*)device->descriptor;
	cy_device_device_handlers* handlers = usb_reconnect(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	if(has_to_load_firmware(usb_device_info)) {
		const cyopn_device_usb_device* next_usb_device_info = GetNextDeviceDesc(usb_device_info);
		int free_fw_id = usb_find_free_fw_id(next_usb_device_info);
		int ret = load_firmware(handlers, usb_device_info, free_fw_id);
		if(!ret) {
			capture_error_print(true, capture_data, "Firmware load error");
			return false;
		}
		cypress_optimize_new_3ds_connection_end(handlers, usb_device_info);
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

uint64_t cyopn_device_get_video_in_size(bool is_3d, InputVideoDataType video_data_type) {
	bool is_rgb888 = video_data_type == VIDEO_DATA_RGB;
	if(is_rgb888) {
		if(is_3d)
			return sizeof(USB888New3DSOptimizeCaptureReceived_3D);
		return sizeof(USB888New3DSOptimizeCaptureReceived);
	}
	if(is_3d)
		return sizeof(USB565New3DSOptimizeCaptureReceived_3D);
	return sizeof(USB565New3DSOptimizeCaptureReceived);
}


uint64_t cyopn_device_get_video_in_size(CaptureStatus* status, bool is_3d, InputVideoDataType video_data_type) {
	return cyopn_device_get_video_in_size(is_3d, video_data_type);
}


uint64_t cyopn_device_get_video_in_size(CaptureData* capture_data, bool is_3d, InputVideoDataType video_data_type) {
	return cyopn_device_get_video_in_size(&capture_data->status, is_3d, video_data_type);
}

static int cypress_device_read_frame_request(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t index, bool is_3d, InputVideoDataType video_data_type) {
	if(cypress_device_capture_recv_data == NULL)
		return LIBUSB_SUCCESS;
	if((*cypress_device_capture_recv_data->status) < 0) {
		cypress_device_capture_recv_data->in_use = false;
		return LIBUSB_SUCCESS;
	}
	const cyopn_device_usb_device* usb_device_info = (const cyopn_device_usb_device*)capture_data->status.device.descriptor;
	cypress_device_capture_recv_data->index = index;
	cypress_device_capture_recv_data->cb_data.function = cypress_device_read_frame_cb;
	size_t read_size = cyopn_device_get_video_in_size(is_3d, video_data_type);
	CaptureDataSingleBuffer* data_buf = capture_data->data_buffers.GetWriterBuffer(cypress_device_capture_recv_data->cb_data.internal_index);
	uint8_t* buffer = (uint8_t*)&data_buf->capture_buf;
	return ReadFrameAsync((cy_device_device_handlers*)capture_data->handle, buffer, read_size, usb_device_info, &cypress_device_capture_recv_data->cb_data);
}

static void end_cypress_device_read_frame_cb(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool early_release) {
	if(early_release)
		cypress_device_capture_recv_data->capture_data->data_buffers.ReleaseWriterBuffer(cypress_device_capture_recv_data->cb_data.internal_index, false);
	cypress_device_capture_recv_data->in_use = false;
	cypress_device_capture_recv_data->is_buffer_free_shared_mutex->specific_unlock(cypress_device_capture_recv_data->cb_data.internal_index);
}

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status) {
	CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data = (CypressOptimizeNew3DSDeviceCaptureReceivedData*)user_data;
	if((*cypress_device_capture_recv_data->status) < 0)
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	if(transfer_status != LIBUSB_TRANSFER_COMPLETED) {
		int error = LIBUSB_ERROR_OTHER;
		if(transfer_status == LIBUSB_TRANSFER_TIMED_OUT)
			error = LIBUSB_ERROR_TIMEOUT;
		*cypress_device_capture_recv_data->status = error;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	}
	return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
}

static int cypress_device_get_num_free_buffers(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	int num_free = 0;
	for(int i = 0; i < NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS; i++)
		if(!cypress_device_capture_recv_data[i].in_use)
			num_free += 1;
	return num_free;
}

static void close_all_reads_error(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyopn_device_usb_device* usb_device_desc = (const cyopn_device_usb_device*)capture_data->status.device.descriptor;
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0) {
		if(!async_read_closed) {
			if(handlers->usb_handle) {
				for (int i = 0; i < NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS; i++)
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

static void error_too_much_time_passed(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed, const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	if(has_too_much_time_passed(start_time)) {
		error_cypress_device_status(cypress_device_capture_recv_data, -1);
		close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	}
}

static void wait_all_cypress_device_buffers_free(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	if(cypress_device_capture_recv_data == NULL)
		return;
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	bool async_read_closed = false;
	close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS; i++)
		while(cypress_device_capture_recv_data[i].in_use) {
			error_too_much_time_passed(capture_data, cypress_device_capture_recv_data, async_read_closed, start_time);
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
		}
}

static void wait_one_cypress_device_buffer_free(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		for(int i = 0; i < NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS; i++) {
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

static void wait_specific_cypress_device_buffer_free(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
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

static bool cypress_device_are_buffers_all_free(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return cypress_device_get_num_free_buffers(cypress_device_capture_recv_data) == NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS;
}

static CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_get_free_buffer(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	wait_one_cypress_device_buffer_free(capture_data, cypress_device_capture_recv_data);
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
		return NULL;
	for(int i = 0; i < NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS; i++)
		if(!cypress_device_capture_recv_data[i].in_use) {
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_try_lock(i);
			cypress_device_capture_recv_data[i].in_use = true;
			return &cypress_device_capture_recv_data[i];
		}
	return NULL;
}

static int get_cypress_device_status(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return *cypress_device_capture_recv_data[0].status;
}

static void error_cypress_device_status(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val) {
	if((error_val == 0) || (get_cypress_device_status(cypress_device_capture_recv_data) == 0))
		*cypress_device_capture_recv_data[0].status = error_val;
}

static void exported_error_cypress_device_status(void* data, int error_val) {
	if(data == NULL)
		return;
	return error_cypress_device_status((CypressOptimizeNew3DSDeviceCaptureReceivedData*)data, error_val);
}

static void reset_cypress_device_status(CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	error_cypress_device_status(cypress_device_capture_recv_data, 0);
}

static bool cyopn_device_acquisition_loop(CaptureData* capture_data, CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyopn_device_usb_device* usb_device_desc = (const cyopn_device_usb_device*)capture_data->status.device.descriptor;
	InputVideoDataType stored_video_data_type = capture_data->status.device.video_data_type;
	bool stored_is_3d = false;
	uint32_t index = 0;
	int ret = capture_start(handlers, usb_device_desc, true, stored_video_data_type == VIDEO_DATA_RGB);
	if (ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return false;
	}
	CypressSetMaxTransferSize(handlers, get_cy_usb_info(usb_device_desc), cyopn_device_get_video_in_size(stored_is_3d, stored_video_data_type));
	auto clock_last_reset = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS; i++) {
		CypressOptimizeNew3DSDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		ret = cypress_device_read_frame_request(capture_data, chosen_buffer, index++, stored_is_3d, stored_video_data_type);
		if(ret < 0) {
			chosen_buffer->in_use = false;
			error_cypress_device_status(cypress_device_capture_recv_data, ret);
			capture_error_print(true, capture_data, "Initial Reads: Failed");
			return false;
		}
	}

	StartCaptureDma(handlers, usb_device_desc, stored_video_data_type == VIDEO_DATA_RGB);
	while (capture_data->status.connected && capture_data->status.running) {
		ret = get_cypress_device_status(cypress_device_capture_recv_data);
		if(ret < 0) {
			wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
			capture_error_print(true, capture_data, "Disconnected: Read error");
			return false;
		}
		CypressOptimizeNew3DSDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
		if(chosen_buffer == NULL)
			error_cypress_device_status(cypress_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
		ret = cypress_device_read_frame_request(capture_data, chosen_buffer, index++, stored_is_3d, stored_video_data_type);
		if(ret < 0) {
			chosen_buffer->in_use = false;
			error_cypress_device_status(cypress_device_capture_recv_data, ret);
			capture_error_print(true, capture_data, "Setup Read: Failed");
			return false;
		}
	}
	return true;
}

void cyopn_device_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	bool is_done_thread;
	std::thread async_processing_thread;
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyopn_device_usb_device* usb_device_desc = (const cyopn_device_usb_device*)capture_data->status.device.descriptor;

	uint32_t last_index = -1;
	int status = 0;
	std::vector<cy_async_callback_data*> cb_queue;
	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS);
	SharedConsumerMutex is_transfer_done_shared_mutex(NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS);
	SharedConsumerMutex is_transfer_data_ready_shared_mutex(NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	CypressOptimizeNew3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data = new CypressOptimizeNew3DSDeviceCaptureReceivedData[NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS];
	for(int i = 0; i < NUM_OPTIMIZE_NEW_3DS_CYPRESS_BUFFERS; i++) {
		cypress_device_capture_recv_data[i].in_use = false;
		cypress_device_capture_recv_data[i].index = i;
		cypress_device_capture_recv_data[i].capture_data = capture_data;
		cypress_device_capture_recv_data[i].last_index = &last_index;
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
	bool proper_return = cyopn_device_acquisition_loop(capture_data, cypress_device_capture_recv_data);
	wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
	CypressEndCypressDeviceAsyncThread(handlers, cb_queue, &async_processing_thread, &is_done_thread);
	delete []cypress_device_capture_recv_data;

	if(proper_return)
		capture_end(handlers, usb_device_desc);
}

void usb_cyopn_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;

	cypress_optimize_new_3ds_connection_end((cy_device_device_handlers*)capture_data->handle, (const cyopn_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}
void usb_cyopn_device_init() {
	return usb_init();
}

void usb_cyopn_device_close() {
	usb_close();
}


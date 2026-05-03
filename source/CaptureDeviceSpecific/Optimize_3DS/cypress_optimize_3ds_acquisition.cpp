#include "devicecapture.hpp"
#include "cypress_shared_driver_comms.hpp"
#include "cypress_shared_libusb_comms.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_optimize_3ds_communications.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"
#include "cypress_optimize_3ds_acquisition_general.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <chrono>
#include <cstring>
#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include <fstream>
#include <sstream>

// This code was developed by exclusively looking at Wireshark USB packet
// captures to learn the USB device's protocol.
// As such, it was developed in a clean room environment, to provide
// compatibility of the USB device with more (and more modern) hardware.
// Such an action is allowed under EU law.
// No reverse engineering of the original software was done to create this.

#define SYNCH_VALUE_OPTIMIZE 0xCC33
#define SYNCH_VALUE_OPTIMIZE_OLD_FW_565 0xF800
#define SYNCH_VALUE_OPTIMIZE_OLD_FW_888 0xFF00

#define TOTAL_WANTED_CONCURRENTLY_RUNNING_BUFFER_SIZE 0x100000
#define SINGLE_RING_BUFFER_SLICE_SIZE 0x4000
#define NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS (TOTAL_WANTED_CONCURRENTLY_RUNNING_BUFFER_SIZE / SINGLE_RING_BUFFER_SLICE_SIZE)

#define NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS (3 * NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS)

#define OPTIMIZE_3DS_TOTAL_BUFFERS_SIZE (NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS * SINGLE_RING_BUFFER_SLICE_SIZE)

#define MAX_TIME_WAIT 1.0
#define CAPTURE_START_AGAIN_NO_KEY_TIME 260.0
#define CAPTURE_START_FAILING_TIME_SINCE_LAST_PARAM_EDIT 180.0

#define OPTIMIZE_3DS_CYPRESS_USB_WINDOWS_DRIVER CYPRESS_WINDOWS_OPTIMIZE_USB_DRIVER

#define OPTIMIZE_RGB888_FORMAT VIDEO_DATA_RGB
#define OPTIMIZE_RGB565_FORMAT VIDEO_DATA_RGB16

struct CypressOptimize3DSDeviceCaptureReceivedData {
	CypressOptimize3DSDeviceCaptureReceivedData* array_ptr;
	volatile bool in_use;
	volatile bool to_process;
	uint32_t index;
	SharedConsumerMutex* is_buffer_free_shared_mutex;
	int* status;
	uint32_t* last_index;
	uint8_t *ring_slice_buffer_arr;
	bool* is_ring_buffer_slice_data_in_use_arr;
	bool* is_ring_buffer_slice_data_ready_arr;
	int ring_buffer_slice_index;
	int* last_ring_buffer_slice_allocated;
	int* first_usable_ring_buffer_slice_index;
	bool* is_synchronized;
	size_t* last_used_ring_buffer_slice_pos;
	int* buffer_ring_slice_to_array_ptr;
	InputVideoDataType* stored_video_data_type;
	bool* stored_is_3d;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	cy_async_callback_data cb_data;
};

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status);
static int get_cypress_device_status(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data);
static void error_cypress_device_status(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val);
static std::string read_key_for_device_id_from_file(uint64_t device_id, bool is_new_device);
static void add_key_for_device_id_to_file(uint64_t device_id, std::string key, bool is_new_device);
static void read_old_firmware_config_from_file(uint64_t device_id, CaptureOptimizeOldFirmwareConfig* out_full, CaptureOptimizeOldFirmwareConfig* out_limited);
static void write_old_firmware_config_to_file(uint64_t device_id, CaptureOptimizeOldFirmwareConfig* to_save_full, CaptureOptimizeOldFirmwareConfig* to_save_limited);

static cy_device_device_handlers* usb_find_by_serial_number(const cyop_device_usb_device* usb_device_desc, std::string wanted_serial_number, CaptureDevice* new_device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device);

	if(final_handlers == NULL)
		final_handlers = cypress_driver_find_by_serial_number(get_cy_usb_info(usb_device_desc), wanted_serial_number, curr_serial_extra_id, new_device, OPTIMIZE_3DS_CYPRESS_USB_WINDOWS_DRIVER);
	return final_handlers;
}

static int usb_find_free_fw_id(const cyop_device_usb_device* usb_device_desc) {
	int curr_serial_extra_id = 0;
	const int num_free_fw_ids = 0x100;
	bool found[num_free_fw_ids];
	for(int i = 0; i < num_free_fw_ids; i++)
		found[i] = false;
	cypress_libusb_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id);
	cypress_driver_find_used_serial(get_cy_usb_info(usb_device_desc), found, num_free_fw_ids, curr_serial_extra_id, OPTIMIZE_3DS_CYPRESS_USB_WINDOWS_DRIVER);

	for(int i = 0; i < num_free_fw_ids; i++)
		if(!found[i])
			return i;
	return 0;
}

static cy_device_device_handlers* usb_reconnect(const cyop_device_usb_device* usb_device_desc, CaptureDevice* device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), device->serial_number, curr_serial_extra_id, NULL);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_serial_reconnection(device);
	return final_handlers;
}

static std::string process_serial_string(const cyop_device_usb_device* usb_device_desc, std::string out_serial) {
	std::string preamble = "";
	if((usb_device_desc != NULL) && usb_device_desc->has_bcd_device_serial)
		preamble = "USB ";
	return preamble + out_serial;
}

static std::string _cypress_optimize_3ds_get_serial(const cyop_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if((!usb_device_desc->has_bcd_device_serial) && (serial != ""))
		return serial;
	if(usb_device_desc->has_bcd_device_serial)
		return std::to_string(bcd_device & 0xFF);
	return std::to_string((curr_serial_extra_id++) + 1);
}

std::string cypress_optimize_3ds_get_serial(cy_device_device_handlers* handlers, const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if(usb_device_desc == NULL)
		return "";
	const cyop_device_usb_device* in_usb_device_desc = (const cyop_device_usb_device*)usb_device_desc;
	return process_serial_string(in_usb_device_desc, _cypress_optimize_3ds_get_serial(in_usb_device_desc, serial, bcd_device, curr_serial_extra_id));
}

static CaptureDevice _cypress_optimize_3ds_create_device(const cyop_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	return CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_CYPRESS_OPTIMIZE, (void*)usb_device_desc, true, true, true, HEIGHT_3DS, TOP_WIDTH_3DS + BOT_WIDTH_3DS, HEIGHT_3DS, (TOP_WIDTH_3DS * 2) + BOT_WIDTH_3DS, OPTIMIZE_3DS_AUDIO_BUFFER_MAX_SIZE * 8, 90, BOT_WIDTH_3DS, 0, BOT_WIDTH_3DS + TOP_WIDTH_3DS, 0, 0, 0, false, OPTIMIZE_RGB565_FORMAT, 0x200, path);
}

CaptureDevice cypress_optimize_3ds_create_device(const void* usb_device_desc, std::string serial, std::string path) {
	if(usb_device_desc == NULL)
		return CaptureDevice();
	return _cypress_optimize_3ds_create_device((const cyop_device_usb_device*)usb_device_desc, serial, path);
}

static void cypress_optimize_3ds_connection_end(cy_device_device_handlers* handlers, const cyop_device_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		cypress_libusb_end_connection(handlers, get_cy_usb_info(device_desc), interface_claimed);
	else
		cypress_driver_end_connection(handlers);
	delete handlers;
}

void list_devices_cyop_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan) {
	const size_t num_cyop_device_desc = GetNumCyOpDeviceDesc();
	int* curr_serial_extra_id_cyop_device = new int[num_cyop_device_desc];
	bool* no_access_elems = new bool[num_cyop_device_desc];
	bool* not_supported_elems = new bool[num_cyop_device_desc];
	std::vector<const cy_device_usb_device*> usb_devices_to_check;
	for (size_t i = 0; i < num_cyop_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_cyop_device[i] = 0;
		const cyop_device_usb_device* curr_device_desc = GetCyOpDeviceDesc((int)i);
		if(devices_allowed_scan[curr_device_desc->index_in_allowed_scan])
			usb_devices_to_check.push_back(get_cy_usb_info(curr_device_desc));
	}
	cypress_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_cyop_device, usb_devices_to_check);

	bool any_not_supported = false;
	for(size_t i = 0; i < num_cyop_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(size_t i = 0; i < num_cyop_device_desc; i++)
		if(no_access_elems[i])
			no_access_list.emplace_back(usb_devices_to_check[i]->vid, usb_devices_to_check[i]->pid);
	if(any_not_supported)
		cypress_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_cyop_device, usb_devices_to_check, OPTIMIZE_3DS_CYPRESS_USB_WINDOWS_DRIVER);

	delete[] curr_serial_extra_id_cyop_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

bool cyop_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device, void* info) {
	const cyop_device_usb_device* usb_device_info = (const cyop_device_usb_device*)device->descriptor;
	cy_device_device_handlers* handlers = usb_reconnect(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	if(has_to_load_firmware(usb_device_info)) {
		const cyop_device_usb_device* next_usb_device_info = GetNextDeviceDesc(usb_device_info);
		int free_fw_id = usb_find_free_fw_id(next_usb_device_info);
		int ret = load_firmware(handlers, usb_device_info, free_fw_id);
		if(!ret) {
			capture_error_print(true, capture_data, "Firmware load error");
			return false;
		}
		cypress_optimize_3ds_connection_end(handlers, usb_device_info);
		std::string new_serial_number = process_serial_string(next_usb_device_info, std::to_string(free_fw_id));
		CaptureDevice new_device;
		setup_reconnection_device(info);
		for(int i = 0; i < 100; i++) {
			default_sleep(100);
			handlers = usb_find_by_serial_number(next_usb_device_info, new_serial_number, &new_device);
			if(handlers != NULL)
				break;
			// Do periodic polling so OSs don't think the software is locked up
			if(!wait_reconnection_device(info))
				break;
		}
		end_reconnection_device(info);
		if(handlers == NULL) {
			capture_error_print(true, capture_data, "Device reconnection error");
			return false;
		}
		*device = new_device;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

uint64_t cyop_device_get_video_in_size(const cyop_device_usb_device* usb_device_info, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type) {
	bool is_rgb888 = video_data_type == OPTIMIZE_RGB888_FORMAT;
	if(usb_device_info->is_old_firmware) {
		if(is_rgb888)
			return sizeof(USB8883DSOptimizeOldFirmwareCaptureReceived);
		return sizeof(USB5653DSOptimizeOldFirmwareCaptureReceived);
	}
	if(is_rgb888) {
		if(should_be_3d) {
			if(is_3d)
				return sizeof(USB8883DSOptimizeCaptureReceived_3D);
			return sizeof(USB8883DSOptimizeCaptureReceived_3D_Forced2DSingleScreen);
		}
		return sizeof(USB8883DSOptimizeCaptureReceived);
	}
	if(is_3d)
		return sizeof(USB5653DSOptimizeCaptureReceived_3D);
	return sizeof(USB5653DSOptimizeCaptureReceived);
}

uint64_t cyop_device_get_video_in_size_extra_header(const cyop_device_usb_device* usb_device_info, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type) {
	bool is_rgb888 = video_data_type == OPTIMIZE_RGB888_FORMAT;
	if(usb_device_info->is_old_firmware) {
		if(is_rgb888)
			return sizeof(USB8883DSOptimizeOldFirmwareCaptureReceived);
		return sizeof(USB5653DSOptimizeOldFirmwareCaptureReceived);
	}
	if(is_rgb888) {
		if(should_be_3d) {
			if(is_3d)
				return sizeof(USB8883DSOptimizeCaptureReceived_3D);
			return sizeof(USB8883DSOptimizeCaptureReceived_3D_Forced2DSingleScreen);
		}
		return sizeof(USB8883DSOptimizeCaptureReceivedExtraHeader);
	}
	if(is_3d)
		return sizeof(USB5653DSOptimizeCaptureReceived_3D);
	return sizeof(USB5653DSOptimizeCaptureReceivedExtraHeader);
}


uint64_t cyop_device_get_video_in_size(CaptureStatus* status, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type) {
	return cyop_device_get_video_in_size((const cyop_device_usb_device*)status->device.descriptor, is_3d, should_be_3d, video_data_type);
}


uint64_t cyop_device_get_video_in_size(CaptureData* capture_data, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type) {
	return cyop_device_get_video_in_size(&capture_data->status, is_3d, should_be_3d, video_data_type);
}

bool is_device_optimize_3ds(CaptureDevice* device) {
	if(device == NULL)
		return false;
	if(device->cc_type != CAPTURE_CONN_CYPRESS_OPTIMIZE)
		return false;
	return true;
}

static bool does_device_match_type(CaptureDevice* device, cypress_optimize_device_type type_searched_for) {
	if(!is_device_optimize_3ds(device))
		return false;
	const cyop_device_usb_device* usb_device_info = (const cyop_device_usb_device*)device->descriptor;
	if(usb_device_info == NULL)
		return false;
	return usb_device_info->device_type == type_searched_for;
}

bool is_device_optimize_o3ds(CaptureDevice* device) {
	return does_device_match_type(device, CYPRESS_OPTIMIZE_OLD_3DS_INSTANTIATED_DEVICE);
}

bool is_device_optimize_o2ds(CaptureDevice* device) {
	const cyop_device_usb_device* usb_device_info = (const cyop_device_usb_device*)device->descriptor;
	return usb_device_info->is_o2ds;
}

bool is_device_optimize_old_fw(CaptureDevice* device) {
	const cyop_device_usb_device* usb_device_info = (const cyop_device_usb_device*)device->descriptor;
	return usb_device_info->is_old_firmware;
}

bool is_device_optimize_n3ds(CaptureDevice* device) {
	return does_device_match_type(device, CYPRESS_OPTIMIZE_NEW_3DS_INSTANTIATED_DEVICE);
}

static InputVideoDataType extract_wanted_input_video_data_type(CaptureStatus* capture_status) {
	const cyop_device_usb_device* usb_device_info = (const cyop_device_usb_device*)capture_status->device.descriptor;
	if(usb_device_info->is_o2ds)
		return capture_status->device_specific_status.optimize_status.request_low_bw_format_old_2ds ? OPTIMIZE_RGB565_FORMAT : OPTIMIZE_RGB888_FORMAT;
	return capture_status->device_specific_status.optimize_status.request_low_bw_format ? OPTIMIZE_RGB565_FORMAT : OPTIMIZE_RGB888_FORMAT;
}

static InputVideoDataType extract_wanted_input_video_data_type(CaptureData* capture_data) {
	return extract_wanted_input_video_data_type(&capture_data->status);
}

static bool key_missing_capture_start_again_check_time(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - start_time;
	return diff.count() > CAPTURE_START_AGAIN_NO_KEY_TIME;
}

static bool param_change_capture_allow_issues_time(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - start_time;
	return diff.count() <= CAPTURE_START_FAILING_TIME_SINCE_LAST_PARAM_EDIT;
}

static int cypress_device_read_frame_request(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t index, bool is_3d, InputVideoDataType video_data_type) {
	if(cypress_device_capture_recv_data == NULL)
		return LIBUSB_SUCCESS;
	if((*cypress_device_capture_recv_data->status) < 0)
		return LIBUSB_SUCCESS;
	const cyop_device_usb_device* usb_device_info = (const cyop_device_usb_device*)capture_data->status.device.descriptor;
	cypress_device_capture_recv_data->index = index;
	cypress_device_capture_recv_data->cb_data.function = cypress_device_read_frame_cb;
	//size_t read_size = cyop_device_get_video_in_size(usb_device_info, is_3d, is_3d, video_data_type);
	size_t read_size = SINGLE_RING_BUFFER_SLICE_SIZE;
	int new_buffer_slice_index = ((*cypress_device_capture_recv_data->last_ring_buffer_slice_allocated) + 1) % NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS;
	uint8_t* buffer = &cypress_device_capture_recv_data->ring_slice_buffer_arr[new_buffer_slice_index * SINGLE_RING_BUFFER_SLICE_SIZE];
	cypress_device_capture_recv_data->is_ring_buffer_slice_data_ready_arr[new_buffer_slice_index] = false;
	cypress_device_capture_recv_data->is_ring_buffer_slice_data_in_use_arr[new_buffer_slice_index] = true;
	cypress_device_capture_recv_data->buffer_ring_slice_to_array_ptr[new_buffer_slice_index] = cypress_device_capture_recv_data->cb_data.internal_index;
	cypress_device_capture_recv_data->ring_buffer_slice_index = new_buffer_slice_index;
	*cypress_device_capture_recv_data->last_ring_buffer_slice_allocated = new_buffer_slice_index;
	cypress_device_capture_recv_data->is_buffer_free_shared_mutex->specific_try_lock(cypress_device_capture_recv_data->cb_data.internal_index);
	cypress_device_capture_recv_data->in_use = true;
	cypress_device_capture_recv_data->to_process = true;
	return ReadFrameAsync((cy_device_device_handlers*)capture_data->handle, buffer, (int)read_size, usb_device_info, &cypress_device_capture_recv_data->cb_data);
}

static void unlock_buffer_directly(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data_real, bool reset_slice_data_arr_info) {
	int slice_index = cypress_device_capture_recv_data_real->ring_buffer_slice_index;
	if(reset_slice_data_arr_info) {
		cypress_device_capture_recv_data_real->is_ring_buffer_slice_data_ready_arr[slice_index] = false;
		cypress_device_capture_recv_data_real->is_ring_buffer_slice_data_in_use_arr[slice_index] = false;
	}
	cypress_device_capture_recv_data_real->buffer_ring_slice_to_array_ptr[slice_index] = -1;
	cypress_device_capture_recv_data_real->in_use = false;
	if(!cypress_device_capture_recv_data_real->to_process)
		cypress_device_capture_recv_data_real->is_buffer_free_shared_mutex->specific_unlock(cypress_device_capture_recv_data_real->cb_data.internal_index);
}

static void unlock_buffer_slice_index(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, size_t slice_index, bool reset_slice_data_arr_info) {
	int index = cypress_device_capture_recv_data->buffer_ring_slice_to_array_ptr[slice_index];
	if(index == -1) {
		if(reset_slice_data_arr_info) {
			cypress_device_capture_recv_data->is_ring_buffer_slice_data_ready_arr[slice_index] = false;
			cypress_device_capture_recv_data->is_ring_buffer_slice_data_in_use_arr[slice_index] = false;
		}
		return;
	}
	unlock_buffer_directly(&cypress_device_capture_recv_data->array_ptr[index], reset_slice_data_arr_info);
}

static void end_cypress_device_read_frame_cb(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool to_unlock) {
	if(to_unlock)
		unlock_buffer_directly(cypress_device_capture_recv_data, true);
	cypress_device_capture_recv_data->to_process = false;
	if(!cypress_device_capture_recv_data->in_use)
		cypress_device_capture_recv_data->is_buffer_free_shared_mutex->specific_unlock(cypress_device_capture_recv_data->cb_data.internal_index);
}

static USB3DSOptimizeColumnInfo convert_to_column_info(uint16_t value) {
	// The macos compiler requires this... :/
	USB3DSOptimizeColumnInfo column_info;
	column_info.has_extra_header_data_2d_only = (value >> 15) & 1;
	column_info.buffer_num = (value >> 14) & 1;
	column_info.unk = (value >> 10) & 0xF;
	column_info.column_index = value & 0x3FF;
	return column_info;
}

static USB3DSOptimizeOldFirmwareColumnInfo convert_to_column_info_old_fw(uint16_t value) {
	// The macos compiler requires this... :/
	USB3DSOptimizeOldFirmwareColumnInfo column_info;
	column_info.is_first_column = (value >> 15) & 1;
	column_info.extra_column_data = (value >> 13) & 3;
	column_info.column_index = (value >> 4) & 0x1FF;
	column_info.fixed = value & 0x0F;
	return column_info;
}

static bool get_expect_extra_header_in_buffer(const cyop_device_usb_device* usb_device_desc, uint16_t value) {
	if(usb_device_desc->is_old_firmware)
		return false;
	return convert_to_column_info(value).has_extra_header_data_2d_only;
}

static bool get_is_data_2d_only(const cyop_device_usb_device* usb_device_desc, uint16_t value) {
	if(usb_device_desc->is_o2ds)
		return true;
	return convert_to_column_info(value).has_extra_header_data_2d_only;
}

static bool is_old_fw_column_magic_valid(USB3DSOptimizeOldFirmwareColumnInfo old_fw_column_info, uint16_t magic, uint16_t expected_magic_value) {
	if(old_fw_column_info.fixed != 5)
		return false;
	if((old_fw_column_info.column_index == 1) && (magic != 0))
		return false;
	if((old_fw_column_info.column_index != 1) && (magic != expected_magic_value))
		return false;
	if((old_fw_column_info.column_index == 0) && (old_fw_column_info.is_first_column == 0))
		return false;
	if((old_fw_column_info.column_index != 0) && (old_fw_column_info.is_first_column != 0))
		return false;
	return true;
}

static bool get_is_pos_column_synch_in_buffer(const cyop_device_usb_device* usb_device_desc, bool is_rgb888, uint8_t* buffer, size_t pos_to_check, uint16_t column_index) {
	if(usb_device_desc->is_old_firmware) {
		USB3DSOptimizeOldFirmwareColumnInfo old_fw_column_info = convert_to_column_info_old_fw(read_le16(buffer + pos_to_check + 8));
		uint16_t magic = read_le16(buffer + pos_to_check);
		if(is_rgb888) {
			magic ^= 0xA5A5; // For some reason, all bytes are xorred with 0xA5 for Old FW RGB888...
			old_fw_column_info = convert_to_column_info_old_fw(read_le16(buffer + pos_to_check + 2) ^ 0xA5A5);
			// extra_column_data is 1 for rgb888 on 2ds...? Would make sense, as it is 2d only...
			if(!is_old_fw_column_magic_valid(old_fw_column_info, magic, SYNCH_VALUE_OPTIMIZE_OLD_FW_888))
				return false;
			return old_fw_column_info.column_index == column_index;
		}
		// extra_column_data is 0 for rgb565...?
		if(!is_old_fw_column_magic_valid(old_fw_column_info, magic, SYNCH_VALUE_OPTIMIZE_OLD_FW_565))
			return false;
		return old_fw_column_info.column_index == column_index;
	}
	if(read_le16(buffer + pos_to_check) != SYNCH_VALUE_OPTIMIZE)
		return false;
	return convert_to_column_info(read_le16(buffer + pos_to_check + 2)).column_index == column_index;
}

static bool get_is_pos_column_synch_in_buffer(const cyop_device_usb_device* usb_device_desc, bool is_rgb888, uint8_t* buffer, uint16_t column_index) {
	return get_is_pos_column_synch_in_buffer(usb_device_desc, is_rgb888, buffer, 0, column_index);
}

static bool get_is_pos_first_synch_in_buffer(const cyop_device_usb_device* usb_device_desc, bool is_rgb888, uint8_t* buffer, size_t pos_to_check) {
	return get_is_pos_column_synch_in_buffer(usb_device_desc, is_rgb888, buffer, pos_to_check, 0);
}

static bool get_is_pos_first_synch_in_buffer(const cyop_device_usb_device* usb_device_desc, InputVideoDataType video_data_type, uint8_t* buffer, size_t pos_to_check) {
	return get_is_pos_first_synch_in_buffer(usb_device_desc, video_data_type == OPTIMIZE_RGB888_FORMAT, buffer, pos_to_check);
}

static bool get_is_buffer_rgb565_fully_synced(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer) {
	USB5653DSOptimizeCaptureReceived* real_buffer = &buffer->cypress_optimize_received_565;
	USB5653DSOptimizeCaptureReceivedExtraHeader* real_buffer_special = &buffer->cypress_optimize_received_565_extra_header;
	bool is_special = get_expect_extra_header_in_buffer(usb_device_desc, read_le16((uint8_t*)&real_buffer->columns_data[0].header_sound.header_info.column_info));
	for(int i = 0; i < TOP_WIDTH_3DS; i++)
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, false, (uint8_t*)&real_buffer->columns_data[i].header_sound, i))
			return false;
	if(is_special && (!get_is_pos_column_synch_in_buffer(usb_device_desc, false, (uint8_t*)&real_buffer_special->columns_data[TOP_WIDTH_3DS].header_sound, TOP_WIDTH_3DS)))
		return false;
	return true;
}

static bool get_is_buffer_rgb565_3d_fully_synced(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer) {
	USB5653DSOptimizeCaptureReceived_3D* real_buffer = &buffer->cypress_optimize_received_565_3d;
	for(int i = 0; i < TOP_WIDTH_3DS; i++) {
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, false, (uint8_t*)&real_buffer->columns_data[i].top_r_screen_column.header_sound, i * 2))
			return false;
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, false, (uint8_t*)&real_buffer->columns_data[i].bot_top_l_screens_column.header_sound, (i * 2) + 1))
			return false;
	}
	if(!get_is_pos_column_synch_in_buffer(usb_device_desc, false, (uint8_t*)&real_buffer->bottom_only_column.header_sound, TOP_WIDTH_3DS * 2))
			return false;
	return true;
}

static bool get_is_buffer_rgb565_old_firmware_fully_synced(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer) {
	USB5653DSOptimizeOldFirmwareCaptureReceived* real_buffer = &buffer->cypress_optimize_old_firmware_received_565;
	for(int i = 0; i < TOP_WIDTH_3DS + 1; i++)
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, false, (uint8_t*)&real_buffer->columns_data[i].header, i)) {
			return false;
		}
	return true;
}

static bool get_is_buffer_rgb888_fully_synced(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer) {
	USB8883DSOptimizeCaptureReceived* real_buffer = &buffer->cypress_optimize_received_888;
	USB8883DSOptimizeCaptureReceivedExtraHeader* real_buffer_special = &buffer->cypress_optimize_received_888_extra_header;
	bool is_special = get_expect_extra_header_in_buffer(usb_device_desc, read_le16((uint8_t*)&real_buffer->columns_data[0].header_sound.header_info.column_info));
	for(int i = 0; i < TOP_WIDTH_3DS; i++)
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, true, (uint8_t*)&real_buffer->columns_data[i].header_sound, i))
			return false;
	if(is_special && (!get_is_pos_column_synch_in_buffer(usb_device_desc, true, (uint8_t*)&real_buffer_special->columns_data[TOP_WIDTH_3DS].header_sound, TOP_WIDTH_3DS)))
		return false;
	return true;
}

static bool get_is_buffer_rgb888_3d_fully_synced(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer) {
	USB8883DSOptimizeCaptureReceived_3D* real_buffer = &buffer->cypress_optimize_received_888_3d;
	for(int i = 0; i < TOP_WIDTH_3DS; i++) {
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, true, (uint8_t*)&real_buffer->columns_data[i][0].header_sound, i * 2))
			return false;
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, true, (uint8_t*)&real_buffer->columns_data[i][1].header_sound, (i * 2) + 1))
			return false;
	}
	if(!get_is_pos_column_synch_in_buffer(usb_device_desc, true, (uint8_t*)&real_buffer->bottom_only_column.header_sound, TOP_WIDTH_3DS * 2))
		return false;
	return true;
}

static bool get_is_buffer_rgb888_3d_2d_fully_synced(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer) {
	USB8883DSOptimizeCaptureReceived_3D_Forced2DSingleScreen* real_buffer = &buffer->cypress_optimize_received_888_3d_2d;
	for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++) {
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, true, (uint8_t*)&real_buffer->columns_data[i].header_sound, i))
			return false;
	}
	return true;
}

static bool get_is_buffer_rgb888_old_firmware_fully_synced(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer) {
	USB8883DSOptimizeOldFirmwareCaptureReceived* real_buffer = &buffer->cypress_optimize_old_firmware_received_888;
	for(int i = 0; i < TOP_WIDTH_3DS + 1; i++)
		if(!get_is_pos_column_synch_in_buffer(usb_device_desc, true, (uint8_t*)&real_buffer->columns_data[i].header_sound, i))
			return false;
	return true;
}

static bool get_is_buffer_fully_synched(const cyop_device_usb_device* usb_device_desc, CaptureReceived* buffer, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type) {
	bool is_rgb888 = video_data_type == OPTIMIZE_RGB888_FORMAT;
	if(usb_device_desc->is_old_firmware) {
		if(is_rgb888)
			return get_is_buffer_rgb888_old_firmware_fully_synced(usb_device_desc, buffer);
		return get_is_buffer_rgb565_old_firmware_fully_synced(usb_device_desc, buffer);
	}
	if(!is_rgb888) {
		if(!is_3d)
			return get_is_buffer_rgb565_fully_synced(usb_device_desc, buffer);
		return get_is_buffer_rgb565_3d_fully_synced(usb_device_desc, buffer);
	}
	if(is_3d)
		return get_is_buffer_rgb888_3d_fully_synced(usb_device_desc, buffer);
	if(should_be_3d)
		return get_is_buffer_rgb888_3d_2d_fully_synced(usb_device_desc, buffer);
	return get_is_buffer_rgb888_fully_synced(usb_device_desc, buffer);
}

static size_t get_pos_first_synch_in_buffer(const cyop_device_usb_device* usb_device_desc, InputVideoDataType video_data_type, uint8_t* buffer, size_t start_pos) {
	for(size_t i = (start_pos / 2); i < (SINGLE_RING_BUFFER_SLICE_SIZE / 2); i++) {
		if(get_is_pos_first_synch_in_buffer(usb_device_desc, video_data_type, buffer, i * 2))
			return i * 2;
	}
	return SINGLE_RING_BUFFER_SLICE_SIZE + 1;
}

static int get_num_consecutive_ready_ring_buffer_slices(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int start_index) {
	for(int i = 0; i < NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS; i++) {
		int index_check = (start_index + i) % NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS;
		if(!cypress_device_capture_recv_data->is_ring_buffer_slice_data_ready_arr[index_check])
			return i;
	}
	// Should never happen
	return NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS;
}

static void unlock_buffers_from_x(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int pos_to_unlock, int num_buffers_to_unlock, bool reset_slice_data_arr_info) {
	for(int i = 0; i < num_buffers_to_unlock; i++)
		unlock_buffer_slice_index(cypress_device_capture_recv_data, (pos_to_unlock + i) % NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS, reset_slice_data_arr_info);
}

static void copy_slice_data_to_buffer(uint8_t* dst, uint8_t *src, size_t start_slice_index, size_t start_slice_pos, size_t copy_size) {
	size_t start_byte = ((start_slice_index * SINGLE_RING_BUFFER_SLICE_SIZE) + start_slice_pos) % OPTIMIZE_3DS_TOTAL_BUFFERS_SIZE;
	if((start_byte + copy_size) <= OPTIMIZE_3DS_TOTAL_BUFFERS_SIZE)
		memcpy(dst, src + start_byte, copy_size);
	else {
		size_t upper_read_size = OPTIMIZE_3DS_TOTAL_BUFFERS_SIZE - start_byte;
		size_t start_read_size = copy_size - upper_read_size;
		memcpy(dst, src + start_byte, upper_read_size);
		memcpy(dst + upper_read_size, src, start_read_size);
	}
}

static void process_data_buffer(const cyop_device_usb_device* usb_device_desc, InputVideoDataType video_data_type, uint64_t* data, size_t size) {
	if(!usb_device_desc->is_old_firmware)
		return;
	bool is_rgb888 = video_data_type == OPTIMIZE_RGB888_FORMAT;
	if(!is_rgb888)
		return;
	// Annoying, but needed to simplify code elsewhere...
	for(size_t i = 0; i < (size / sizeof(uint64_t)); i++)
		data[i] ^= 0xA5A5A5A5A5A5A5A5;
}

static void cypress_output_to_thread(CaptureData* capture_data, uint8_t *buffer_arr, size_t start_slice_index, size_t start_slice_pos, int internal_index, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t read_size, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type) {
	// Output to the other threads...
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)capture_data->status.device.descriptor;
	CaptureDataSingleBuffer* data_buf = capture_data->data_buffers.GetWriterBuffer(internal_index);
	copy_slice_data_to_buffer((uint8_t*)&data_buf->capture_buf, buffer_arr, start_slice_index, start_slice_pos, read_size);
	if(!get_is_buffer_fully_synched(usb_device_desc, &data_buf->capture_buf, is_3d, should_be_3d, video_data_type)) {
		capture_data->data_buffers.ReleaseWriterBuffer(internal_index, false);
		return;
	}
	process_data_buffer(usb_device_desc, video_data_type, (uint64_t*)&data_buf->capture_buf, read_size);
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - (*clock_start);
	*clock_start = curr_time;
	capture_data->data_buffers.WriteToBuffer(NULL, read_size, diff.count(), &capture_data->status.device, internal_index, is_3d, should_be_3d);
	if (capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static bool cypress_device_read_frame_not_synchronized(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int &error) {
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)cypress_device_capture_recv_data->capture_data->status.device.descriptor;
	InputVideoDataType video_data_type = *cypress_device_capture_recv_data->stored_video_data_type;
	volatile int first_slice_to_check = *cypress_device_capture_recv_data->first_usable_ring_buffer_slice_index;
	volatile int original_first_slice_to_check = first_slice_to_check;
	volatile int first_slice_pos_to_check = (int)*cypress_device_capture_recv_data->last_used_ring_buffer_slice_pos;
	bool found = false;
	// Determine which buffer is the first which needs to still be checked
	for(int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		int index_to_check = (first_slice_to_check + i) % NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS;
		if(cypress_device_capture_recv_data->is_ring_buffer_slice_data_in_use_arr[index_to_check]) {
			first_slice_to_check = index_to_check;
			found = true;
			break;
		}
	}
	// Too many have been checked... Just fail.
	if(!found) {
		error = LIBUSB_ERROR_OTHER;
		return false;
	}
	for(int i = 0; i < NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS; i++) {
		int check_index = (first_slice_to_check + i) % NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS;
		// Search for the synch values. If they're not there, free the buffer.
		// If they are there, enque the buffer and switch mode.
		// Checks all available buffers which have data (but in order).
		if(cypress_device_capture_recv_data->is_ring_buffer_slice_data_ready_arr[check_index]) {
			size_t start_result = 0;
			// Increase by two to prevent looping...
			if(check_index == original_first_slice_to_check)
				start_result = first_slice_pos_to_check + 2;
			size_t result = get_pos_first_synch_in_buffer(usb_device_desc, video_data_type, &cypress_device_capture_recv_data->ring_slice_buffer_arr[check_index * SINGLE_RING_BUFFER_SLICE_SIZE], start_result);
			if(result >= SINGLE_RING_BUFFER_SLICE_SIZE) {
				unlock_buffer_slice_index(cypress_device_capture_recv_data, check_index, true);
			}
			else {
				*cypress_device_capture_recv_data->first_usable_ring_buffer_slice_index = check_index;
				*cypress_device_capture_recv_data->last_used_ring_buffer_slice_pos = result;
				*cypress_device_capture_recv_data->is_synchronized = true;
				break;
			}
		}
		else
			break;
	}
	return true;
}

static size_t get_header_size_optimize_packet(const cyop_device_usb_device* usb_device_desc, InputVideoDataType video_data_type) {
	if(!usb_device_desc->is_old_firmware)
		return sizeof(USB3DSOptimizeHeaderData);
	bool is_rgb888 = video_data_type == OPTIMIZE_RGB888_FORMAT;
	if(is_rgb888)
		return sizeof(USB8883DSOptimizeOldFirmwareHeaderData);
	return sizeof(USB5653DSOptimizeOldFirmwareHeaderData);
}

static void cypress_device_read_frame_synchronized(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)cypress_device_capture_recv_data->capture_data->status.device.descriptor;
	volatile int read_slice_index = *cypress_device_capture_recv_data->first_usable_ring_buffer_slice_index;
	volatile size_t read_slice_pos = *cypress_device_capture_recv_data->last_used_ring_buffer_slice_pos;
	int num_consecutive_ready_buffers = get_num_consecutive_ready_ring_buffer_slices(cypress_device_capture_recv_data, read_slice_index);
	bool should_be_3d_data = *cypress_device_capture_recv_data->stored_is_3d;
	if(usb_device_desc->is_o2ds)
		should_be_3d_data = false;
	bool is_3d_data = should_be_3d_data;
	InputVideoDataType video_data_type = *cypress_device_capture_recv_data->stored_video_data_type;
	size_t video_in_size = (size_t)cyop_device_get_video_in_size(usb_device_desc, is_3d_data, should_be_3d_data, video_data_type);
	size_t curr_consecutive_available_bytes = (num_consecutive_ready_buffers * SINGLE_RING_BUFFER_SLICE_SIZE) - read_slice_pos;
	size_t header_packet_size = get_header_size_optimize_packet(usb_device_desc, video_data_type);
	if(curr_consecutive_available_bytes >= header_packet_size) {
		uint8_t tmp_buffer[sizeof(USB5653DSOptimizeOldFirmwareHeaderData)];
		copy_slice_data_to_buffer(tmp_buffer, cypress_device_capture_recv_data->ring_slice_buffer_arr, read_slice_index, read_slice_pos, header_packet_size);
		if(!get_is_pos_first_synch_in_buffer(usb_device_desc, video_data_type, tmp_buffer, 0)) {
			*cypress_device_capture_recv_data->is_synchronized = false;
			return;
		}
		else {
			if(get_expect_extra_header_in_buffer(usb_device_desc, read_le16(tmp_buffer, 1)))
				video_in_size = (size_t)cyop_device_get_video_in_size_extra_header(usb_device_desc, is_3d_data, should_be_3d_data, video_data_type);
			if(get_is_data_2d_only(usb_device_desc, read_le16(tmp_buffer, 1))) {
				is_3d_data = false;
				video_in_size = (size_t)cyop_device_get_video_in_size_extra_header(usb_device_desc, is_3d_data, should_be_3d_data, video_data_type);
			}
		}
	}
	if(curr_consecutive_available_bytes >= video_in_size) {
		// Enough data. Time to do output...
		cypress_output_to_thread(cypress_device_capture_recv_data->capture_data, cypress_device_capture_recv_data->ring_slice_buffer_arr, read_slice_index, read_slice_pos, 0, cypress_device_capture_recv_data->clock_start, video_in_size, is_3d_data, should_be_3d_data, video_data_type);
		// Keep the ring buffer going.
		size_t raw_new_pos = read_slice_pos + video_in_size;
		int new_slice_index = (int)(read_slice_index + (raw_new_pos / SINGLE_RING_BUFFER_SLICE_SIZE));
		size_t new_slice_pos = raw_new_pos % SINGLE_RING_BUFFER_SLICE_SIZE;
		new_slice_index %= NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS;
		// Fully unlock used buffers
		unlock_buffers_from_x(cypress_device_capture_recv_data, read_slice_index, (new_slice_index + NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS - read_slice_index) % NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS, true);
		// Update the position of the data
		*cypress_device_capture_recv_data->first_usable_ring_buffer_slice_index = new_slice_index;
		*cypress_device_capture_recv_data->last_used_ring_buffer_slice_pos = new_slice_pos;
	}
	// Partially unlock buffers already with data.
	// Unlocks the transfers to be used for more.
	// This is possible under the assumption that there are significantly
	// more buffers than transfers!!!
	// Without said assumption, you'd risk the two stepping on each other.
	read_slice_index = *cypress_device_capture_recv_data->first_usable_ring_buffer_slice_index;
	num_consecutive_ready_buffers = get_num_consecutive_ready_ring_buffer_slices(cypress_device_capture_recv_data, read_slice_index);
	unlock_buffers_from_x(cypress_device_capture_recv_data, read_slice_index, num_consecutive_ready_buffers, false);
}

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status) {
	CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data = (CypressOptimize3DSDeviceCaptureReceivedData*)user_data;
	if((*cypress_device_capture_recv_data->status) < 0)
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	if((transfer_status != LIBUSB_TRANSFER_COMPLETED) || (transfer_length < SINGLE_RING_BUFFER_SLICE_SIZE)) {
		int error = LIBUSB_ERROR_OTHER;
		if(transfer_status == LIBUSB_TRANSFER_TIMED_OUT)
			error = LIBUSB_ERROR_TIMEOUT;
		*cypress_device_capture_recv_data->status = error;
		return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
	}
	cypress_device_capture_recv_data->is_ring_buffer_slice_data_ready_arr[cypress_device_capture_recv_data->ring_buffer_slice_index] = true;
	// Not synchronized case
	bool to_free = false;
	bool continue_looping = true;
	int remaining_iters_before_force_exit = 4;
	volatile bool read_is_synchronized = *cypress_device_capture_recv_data->is_synchronized;
	while(continue_looping) {
		if(read_is_synchronized)
			cypress_device_read_frame_synchronized(cypress_device_capture_recv_data);
		else {
			int error = 0;
			bool retval = cypress_device_read_frame_not_synchronized(cypress_device_capture_recv_data, error);
			if(!retval) {
				*cypress_device_capture_recv_data->status = error;
				return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, true);
			}
		}
		volatile bool new_read_is_synchronized = *cypress_device_capture_recv_data->is_synchronized;
		remaining_iters_before_force_exit--;
		if((new_read_is_synchronized == read_is_synchronized) || (remaining_iters_before_force_exit == 0))
			continue_looping = false;
		read_is_synchronized = new_read_is_synchronized;
	}
	return end_cypress_device_read_frame_cb(cypress_device_capture_recv_data, to_free);
}

static void close_all_reads_error(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)capture_data->status.device.descriptor;
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0) {
		if(!async_read_closed) {
			if(handlers->usb_handle) {
				for (int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++)
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

static void error_too_much_time_passed(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed, const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	if(has_too_much_time_passed(start_time)) {
		error_cypress_device_status(cypress_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
		close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	}
}

static void unlock_buffer_in_error_case(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int status) {
	if((status < 0) && (!cypress_device_capture_recv_data->to_process)) {
		unlock_buffer_directly(cypress_device_capture_recv_data, true);
	}
}

static bool is_buffer_still_in_use(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return cypress_device_capture_recv_data->in_use || cypress_device_capture_recv_data->to_process;
}

static void wait_all_cypress_device_buffers_free(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	if(cypress_device_capture_recv_data == NULL)
		return;
	bool async_read_closed = false;
	close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++)
		while(is_buffer_still_in_use(&cypress_device_capture_recv_data[i])) {
			error_too_much_time_passed(capture_data, cypress_device_capture_recv_data, async_read_closed, start_time);
			unlock_buffer_in_error_case(&cypress_device_capture_recv_data[i], get_cypress_device_status(cypress_device_capture_recv_data));
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
		}
}

static void wait_one_cypress_device_buffer_free(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		for(int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
			if(!is_buffer_still_in_use(&cypress_device_capture_recv_data[i]))
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

static CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_get_free_buffer(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	wait_one_cypress_device_buffer_free(capture_data, cypress_device_capture_recv_data);
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
		return NULL;
	for(int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++)
		if(!is_buffer_still_in_use(&cypress_device_capture_recv_data[i])) {
			return &cypress_device_capture_recv_data[i];
		}
	return NULL;
}

static int get_cypress_device_status(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return *cypress_device_capture_recv_data[0].status;
}

static void error_cypress_device_status(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val) {
	if((error_val == 0) || (get_cypress_device_status(cypress_device_capture_recv_data) == 0))
		*cypress_device_capture_recv_data[0].status = error_val;
}

static void exported_error_cypress_device_status(void* data, int error_val) {
	if(data == NULL)
		return;
	return error_cypress_device_status((CypressOptimize3DSDeviceCaptureReceivedData*)data, error_val);
}

static bool get_buffer_and_schedule_read(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t &index, InputVideoDataType &stored_video_data_type, bool &stored_is_3d, const std::string error_to_print) {
	CypressOptimize3DSDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
	if(chosen_buffer == NULL)
		error_cypress_device_status(cypress_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
	int ret = cypress_device_read_frame_request(capture_data, chosen_buffer, index++, stored_is_3d, stored_video_data_type);
	if(ret < 0) {
		chosen_buffer->in_use = false;
		cypress_device_capture_recv_data->to_process = false;
		error_cypress_device_status(cypress_device_capture_recv_data, ret);
		if(error_to_print != "")
			capture_error_print(true, capture_data, error_to_print);
		return false;
	}
	return true;
}

static bool schedule_all_reads(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t &index, InputVideoDataType &stored_video_data_type, bool &stored_is_3d, const std::string error_to_print) {
	for(int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		if(!get_buffer_and_schedule_read(capture_data, cypress_device_capture_recv_data, index, stored_video_data_type, stored_is_3d, error_to_print))
			return false;
	}
	return true;
}

static int end_capture_from_capture_data(CaptureData* capture_data) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)capture_data->status.device.descriptor;
	return capture_end(handlers, usb_device_desc);
}

static void reset_buffer_processing_data(CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	*cypress_device_capture_recv_data[0].last_ring_buffer_slice_allocated = -1;
	*cypress_device_capture_recv_data[0].first_usable_ring_buffer_slice_index = 0;
	*cypress_device_capture_recv_data[0].last_used_ring_buffer_slice_pos = 0;
	*cypress_device_capture_recv_data[0].is_synchronized = false;
	*cypress_device_capture_recv_data[0].last_index = -1;
	for(int i = 0; i < NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS; i++) {
		cypress_device_capture_recv_data[0].is_ring_buffer_slice_data_ready_arr[i] = false;
		cypress_device_capture_recv_data[0].is_ring_buffer_slice_data_in_use_arr[i] = false;
		cypress_device_capture_recv_data[0].buffer_ring_slice_to_array_ptr[i] = -1;
	}
	for(int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		cypress_device_capture_recv_data[i].in_use = false;
		cypress_device_capture_recv_data[i].to_process = false;
		cypress_device_capture_recv_data[i].index = i;
		cypress_device_capture_recv_data[i].ring_buffer_slice_index = -1;
	}
}

static bool are_old_fw_config_cases_different(CaptureOptimizeOldFirmwareConfigCase &first, CaptureOptimizeOldFirmwareConfigCase &second) {
	return (first.low_screen_clock != second.low_screen_clock) || (first.top_screen_clock != second.top_screen_clock) || (first.top_screen_data != second.top_screen_data) || (first.top_screen_sync != second.top_screen_sync);
}

static bool are_old_fw_configs_different(CaptureOptimizeOldFirmwareConfig &first, CaptureOptimizeOldFirmwareConfig &second) {
	return are_old_fw_config_cases_different(first.mode_2d_only, second.mode_2d_only) || are_old_fw_config_cases_different(first.mode_regular, second.mode_regular);
}

static bool are_there_changes_present_to_old_fw_config(CaptureData* capture_data, InputVideoDataType &stored_video_data_type, CaptureOptimizeOldFirmwareConfig &stored_old_fw_cfg_full, CaptureOptimizeOldFirmwareConfig &stored_old_fw_cfg_limited) {
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)capture_data->status.device.descriptor;
	if(!usb_device_desc->is_old_firmware)
		return false;

	if(stored_video_data_type == OPTIMIZE_RGB888_FORMAT)
		return are_old_fw_configs_different(capture_data->status.device_specific_status.optimize_status.old_fw_config_full_bw_format, stored_old_fw_cfg_full);

	return are_old_fw_configs_different(capture_data->status.device_specific_status.optimize_status.old_fw_config_low_bw_format, stored_old_fw_cfg_limited);
}

static int restart_captures_cc_reads(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint64_t device_id, uint32_t &index, InputVideoDataType &stored_video_data_type, bool &stored_is_3d, bool could_use_3d, CaptureOptimizeOldFirmwareConfig &stored_old_fw_cfg_full, CaptureOptimizeOldFirmwareConfig &stored_old_fw_cfg_limited, bool force_capture_start, std::string read_key, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_capture_start, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_param_edit) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)capture_data->status.device.descriptor;
	int retval = end_capture_from_capture_data(capture_data);
	if(retval < 0)
		return retval;
	error_cypress_device_status(cypress_device_capture_recv_data, 0);
	default_sleep(100);

	InputVideoDataType wanted_input_video_data_type = extract_wanted_input_video_data_type(capture_data);
	bool set_max_again = false;
	if((wanted_input_video_data_type != stored_video_data_type) || force_capture_start){
		set_max_again = true;
		stored_video_data_type = wanted_input_video_data_type;
		uint64_t device_id_dummy;
		std::string read_key_dummy;
		retval = capture_start(handlers, usb_device_desc, false, stored_video_data_type == OPTIMIZE_RGB888_FORMAT, device_id_dummy, read_key_dummy);
		clock_last_capture_start = std::chrono::high_resolution_clock::now();
		clock_last_param_edit = std::chrono::high_resolution_clock::now(); // Changing the format changes the loaded parameters...
		capture_data->status.device.video_data_type = stored_video_data_type;
		if(retval < 0)
			return retval;
	}

	bool is_new_3d = could_use_3d && get_3d_enabled(&capture_data->status);
	if(is_new_3d != stored_is_3d) {
		set_max_again = true;
		stored_is_3d = is_new_3d;
	}

	if(set_max_again)
		CypressSetMaxTransferSize(handlers, get_cy_usb_info(usb_device_desc), (size_t)cyop_device_get_video_in_size_extra_header(usb_device_desc, stored_is_3d, stored_is_3d, stored_video_data_type));

	bool present_new_old_fw_config = are_there_changes_present_to_old_fw_config(capture_data, stored_video_data_type, stored_old_fw_cfg_full, stored_old_fw_cfg_limited);
	if(present_new_old_fw_config) {
		stored_old_fw_cfg_full = capture_data->status.device_specific_status.optimize_status.old_fw_config_full_bw_format;
		stored_old_fw_cfg_limited = capture_data->status.device_specific_status.optimize_status.old_fw_config_low_bw_format;
		write_old_firmware_config_to_file(device_id, &stored_old_fw_cfg_full, &stored_old_fw_cfg_limited);
		clock_last_param_edit = std::chrono::high_resolution_clock::now(); 
	}

	index = 0;
	reset_buffer_processing_data(cypress_device_capture_recv_data);

	if(!schedule_all_reads(capture_data, cypress_device_capture_recv_data, index, stored_video_data_type, stored_is_3d, ""))
		return -1;
	capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	CaptureOptimizeOldFirmwareConfig* to_use_old_fw_cfg = stored_video_data_type == OPTIMIZE_RGB888_FORMAT ? &stored_old_fw_cfg_full : &stored_old_fw_cfg_limited;
	StartCaptureDma(handlers, usb_device_desc, stored_video_data_type == OPTIMIZE_RGB888_FORMAT, stored_is_3d, to_use_old_fw_cfg, read_key);
	return 0;
}

static void cyop_key_missing_warning_message(CaptureData* capture_data) {
	capture_warning_print(capture_data, "Key missing!\nClose to Timeout!\nReconfigured FPGA!", "Key missing! Close to Timeout! Reconfigured FPGA!");
}

static void cyop_key_check_and_interrogate_file(CaptureStatus* capture_status, const cyop_device_usb_device* usb_device_desc, uint64_t device_id, bool &is_key_valid, std::string &read_key) {
	if(is_key_valid)
		return;

	// This can only happen during the first connection
	is_key_valid = check_key_matches_device_id(device_id, read_key, usb_device_desc);

	// This also can only happen during the first connection
	if(is_key_valid)
		add_key_for_device_id_to_file(device_id, read_key, usb_device_desc);
	else {
		read_key = read_key_for_device_id_from_file(device_id, usb_device_desc->is_new_device);
		is_key_valid = check_key_matches_device_id(device_id, read_key, usb_device_desc->is_new_device);
	}

	capture_status->device.device_id = device_id;
	capture_status->device.key = read_key;
	capture_status->device_specific_status.optimize_status.key_updated = false;
}

static bool cyop_device_acquisition_loop(CaptureData* capture_data, CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data, InputVideoDataType &stored_video_data_type, bool &stored_is_3d, bool could_use_3d) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)capture_data->status.device.descriptor;
	uint32_t index = 0;
	uint64_t device_id = 0;
	std::string read_key = "";
	bool force_capture_start_key = false;
	CaptureOptimizeOldFirmwareConfig stored_old_fw_cfg_full = {};
	CaptureOptimizeOldFirmwareConfig stored_old_fw_cfg_limited = {};
	int ret = capture_start(handlers, usb_device_desc, true, stored_video_data_type == OPTIMIZE_RGB888_FORMAT, device_id, read_key);
	capture_data->status.title_check_id += 1;
	capture_data->status.device_specific_status.optimize_status.key_updated = false;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_capture_start = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_param_edit = std::chrono::high_resolution_clock::now();

	bool is_key_valid = false;
	cyop_key_check_and_interrogate_file(&capture_data->status, usb_device_desc, device_id, is_key_valid, read_key);

	if(usb_device_desc->is_old_firmware)
		read_old_firmware_config_from_file(device_id, &stored_old_fw_cfg_full, &stored_old_fw_cfg_limited);
	capture_data->status.device_specific_status.optimize_status.old_fw_config_full_bw_format = stored_old_fw_cfg_full;
	capture_data->status.device_specific_status.optimize_status.old_fw_config_low_bw_format = stored_old_fw_cfg_limited;

	capture_data->status.device.video_data_type = stored_video_data_type;

	if (ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return false;
	}

	CypressSetMaxTransferSize(handlers, get_cy_usb_info(usb_device_desc), (size_t)cyop_device_get_video_in_size_extra_header(usb_device_desc, stored_is_3d, stored_is_3d, stored_video_data_type));
	if(!schedule_all_reads(capture_data, cypress_device_capture_recv_data, index, stored_video_data_type, stored_is_3d, "Initial Reads: Failed"))
		return false;

	CaptureOptimizeOldFirmwareConfig* to_use_old_fw_cfg = stored_video_data_type == OPTIMIZE_RGB888_FORMAT ? &stored_old_fw_cfg_full : &stored_old_fw_cfg_limited;
	StartCaptureDma(handlers, usb_device_desc, stored_video_data_type == OPTIMIZE_RGB888_FORMAT, stored_is_3d, to_use_old_fw_cfg, read_key);
	while (capture_data->status.connected && capture_data->status.running) {
		ret = get_cypress_device_status(cypress_device_capture_recv_data);

		if(ret < 0) {
			int cause_error = ret;
			capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM + NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS;
			wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
			bool has_recovered = false;
			if((cause_error == LIBUSB_ERROR_TIMEOUT) || ((cause_error == LIBUSB_ERROR_OTHER) && usb_device_desc->is_old_firmware && param_change_capture_allow_issues_time(clock_last_param_edit))) {
				force_capture_start_key = (!check_key_matches_device_id(device_id, read_key, usb_device_desc)) && key_missing_capture_start_again_check_time(clock_last_capture_start);
				cyop_key_check_and_interrogate_file(&capture_data->status, usb_device_desc, device_id, is_key_valid, read_key);
				int timeout_ret = restart_captures_cc_reads(capture_data, cypress_device_capture_recv_data, device_id, index, stored_video_data_type, stored_is_3d, could_use_3d, stored_old_fw_cfg_full, stored_old_fw_cfg_limited, force_capture_start_key, read_key, clock_last_capture_start, clock_last_param_edit);
				if(timeout_ret >= 0) {
					has_recovered = true;
					if(force_capture_start_key && (!is_key_valid))
						cyop_key_missing_warning_message(capture_data);
				}
			}
			if(!has_recovered) {
				capture_error_print(true, capture_data, "Disconnected: Read error");
				return false;
			}
		}

		bool is_new_3d = could_use_3d && get_3d_enabled(&capture_data->status);
		bool present_new_old_fw_config = are_there_changes_present_to_old_fw_config(capture_data, stored_video_data_type, stored_old_fw_cfg_full, stored_old_fw_cfg_limited);
		InputVideoDataType wanted_input_video_data_type = extract_wanted_input_video_data_type(capture_data);
		force_capture_start_key = (!is_key_valid) && (!check_key_matches_device_id(device_id, read_key, usb_device_desc)) && key_missing_capture_start_again_check_time(clock_last_capture_start);
		if((wanted_input_video_data_type != stored_video_data_type) || (is_new_3d != stored_is_3d) || force_capture_start_key || capture_data->status.device_specific_status.optimize_status.key_updated || present_new_old_fw_config) {
			capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM + NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS;
			wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
			cyop_key_check_and_interrogate_file(&capture_data->status, usb_device_desc, device_id, is_key_valid, read_key);
			ret = restart_captures_cc_reads(capture_data, cypress_device_capture_recv_data, device_id, index, stored_video_data_type, stored_is_3d, could_use_3d, stored_old_fw_cfg_full, stored_old_fw_cfg_limited, force_capture_start_key, read_key, clock_last_capture_start, clock_last_param_edit);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Disconnected: Update mode error");
				return false;
			}
			if(force_capture_start_key && (!is_key_valid))
				cyop_key_missing_warning_message(capture_data);
		}
		if(!get_buffer_and_schedule_read(capture_data, cypress_device_capture_recv_data, index, stored_video_data_type, stored_is_3d, "Setup Read: Failed"))
			return false;
	}
	return true;
}

void cyop_device_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	bool is_done_thread;
	std::thread async_processing_thread;
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cyop_device_usb_device* usb_device_desc = (const cyop_device_usb_device*)capture_data->status.device.descriptor;

	uint32_t last_index = -1;
	int status = 0;
	InputVideoDataType stored_video_data_type = extract_wanted_input_video_data_type(capture_data);
	bool could_use_3d = get_3d_enabled(&capture_data->status, true);
	bool stored_is_3d = could_use_3d && get_3d_enabled(&capture_data->status);

	uint8_t *ring_slice_buffer = new uint8_t[OPTIMIZE_3DS_TOTAL_BUFFERS_SIZE];
	bool *is_ring_buffer_slice_data_ready = new bool[NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS];
	bool *is_ring_buffer_slice_data_in_use = new bool[NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS];
	int *buffer_ring_slice_to_array = new int[NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS];
	for(int i = 0; i < NUM_TOTAL_OPTIMIZE_3DS_CYPRESS_BUFFERS; i++) {
		is_ring_buffer_slice_data_ready[i] = false;
		is_ring_buffer_slice_data_in_use[i] = false;
		buffer_ring_slice_to_array[i] = -1;
	}
	int first_usable_ring_buffer_slice_index = 0;
	bool is_synchronized = false;
	size_t last_used_ring_buffer_slice_pos = 0;
	int last_ring_buffer_slice_allocated = -1;

	std::vector<cy_async_callback_data*> cb_queue;
	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS);
	SharedConsumerMutex is_transfer_done_shared_mutex(NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS);
	SharedConsumerMutex is_transfer_data_ready_shared_mutex(NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	CypressOptimize3DSDeviceCaptureReceivedData* cypress_device_capture_recv_data = new CypressOptimize3DSDeviceCaptureReceivedData[NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS];
	for(int i = 0; i < NUM_OPTIMIZE_3DS_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		cypress_device_capture_recv_data[i].array_ptr = cypress_device_capture_recv_data;
		cypress_device_capture_recv_data[i].in_use = false;
		cypress_device_capture_recv_data[i].to_process = false;
		cypress_device_capture_recv_data[i].index = i;
		cypress_device_capture_recv_data[i].ring_buffer_slice_index = -1;
		cypress_device_capture_recv_data[i].stored_video_data_type = &stored_video_data_type;
		cypress_device_capture_recv_data[i].stored_is_3d = &stored_is_3d;
		cypress_device_capture_recv_data[i].ring_slice_buffer_arr = ring_slice_buffer;
		cypress_device_capture_recv_data[i].is_ring_buffer_slice_data_in_use_arr = is_ring_buffer_slice_data_in_use;
		cypress_device_capture_recv_data[i].is_ring_buffer_slice_data_ready_arr = is_ring_buffer_slice_data_ready;
		cypress_device_capture_recv_data[i].last_ring_buffer_slice_allocated = &last_ring_buffer_slice_allocated;
		cypress_device_capture_recv_data[i].first_usable_ring_buffer_slice_index = &first_usable_ring_buffer_slice_index;
		cypress_device_capture_recv_data[i].is_synchronized = &is_synchronized;
		cypress_device_capture_recv_data[i].last_used_ring_buffer_slice_pos = &last_used_ring_buffer_slice_pos;
		cypress_device_capture_recv_data[i].buffer_ring_slice_to_array_ptr = buffer_ring_slice_to_array;
		cypress_device_capture_recv_data[i].capture_data = capture_data;
		cypress_device_capture_recv_data[i].last_index = &last_index;
		cypress_device_capture_recv_data[i].clock_start = &clock_start;
		cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex = &is_buffer_free_shared_mutex;
		cypress_device_capture_recv_data[i].status = &status;
		cypress_device_capture_recv_data[i].cb_data.actual_user_data = &cypress_device_capture_recv_data[i];
		cypress_device_capture_recv_data[i].cb_data.transfer_data = NULL;
		cypress_device_capture_recv_data[i].cb_data.is_transfer_done_mutex = &is_transfer_done_shared_mutex;
		cypress_device_capture_recv_data[i].cb_data.internal_index = i;
		cypress_device_capture_recv_data[i].cb_data.is_data_ready = false;
		cypress_device_capture_recv_data[i].cb_data.is_transfer_data_ready_mutex = &is_transfer_data_ready_shared_mutex;
		cypress_device_capture_recv_data[i].cb_data.in_use_ptr = &cypress_device_capture_recv_data[i].to_process;
		cypress_device_capture_recv_data[i].cb_data.error_function = exported_error_cypress_device_status;
		cb_queue.push_back(&cypress_device_capture_recv_data[i].cb_data);
	}
	CypressSetupCypressDeviceAsyncThread(handlers, cb_queue, &async_processing_thread, &is_done_thread);
	bool proper_return = cyop_device_acquisition_loop(capture_data, cypress_device_capture_recv_data, stored_video_data_type, stored_is_3d, could_use_3d);
	wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
	CypressEndCypressDeviceAsyncThread(handlers, cb_queue, &async_processing_thread, &is_done_thread);
	delete []cypress_device_capture_recv_data;
	delete []ring_slice_buffer;
	delete []is_ring_buffer_slice_data_ready;
	delete []is_ring_buffer_slice_data_in_use;
	delete []buffer_ring_slice_to_array;

	if(proper_return)
		capture_end(handlers, usb_device_desc);
}

void usb_cyop_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;

	cypress_optimize_3ds_connection_end((cy_device_device_handlers*)capture_data->handle, (const cyop_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}

static std::string get_old_fw_config_file_path(uint64_t device_id) {
	std::string path = get_base_path_device_specific_configs();

	path += "optimize_old_fw_" + to_hex_u60(device_id) + ".txt";
	return path;
}

static std::string write_old_firmware_config_case_string(std::string base, CaptureOptimizeOldFirmwareConfigCase* config_case) {
	std::string out_str = "";

	out_str += base + "_low_screen_clk" + "=" + std::to_string(config_case->low_screen_clock) + "\n";
	out_str += base + "_top_screen_clk" + "=" + std::to_string(config_case->top_screen_clock) + "\n";
	out_str += base + "_top_screen_data" + "=" + std::to_string(config_case->top_screen_data) + "\n";
	out_str += base + "_top_screen_sync" + "=" + std::to_string(config_case->top_screen_sync) + "\n";

	return out_str;
}

static bool read_old_firmware_config_case_string(std::string key, std::string value, std::string base, CaptureOptimizeOldFirmwareConfigCase* config_case) {

	if(key == (base + "_low_screen_clk")) {
		config_case->low_screen_clock = std::stoi(value);
		return true;
	}

	if(key == (base + "_top_screen_clk")) {
		config_case->top_screen_clock = std::stoi(value);
		return true;
	}

	if(key == (base + "_top_screen_data")) {
		config_case->top_screen_data = std::stoi(value);
		return true;
	}

	if(key == (base + "_top_screen_sync")) {
		config_case->top_screen_sync = std::stoi(value);
		return true;
	}

	return false;
}

static void read_old_firmware_config_from_file(uint64_t device_id, CaptureOptimizeOldFirmwareConfig* out_full, CaptureOptimizeOldFirmwareConfig* out_limited) {
	std::ifstream file(get_old_fw_config_file_path(device_id));
	std::string line;

	*out_full = {};
	*out_limited = {};
	sanitize_capture_optimize_old_fw_config(out_full);
	sanitize_capture_optimize_old_fw_config(out_limited);

	if((!file) || (!file.is_open()) || (!file.good()))
		return;

	try {
		while(std::getline(file, line)) {
			std::istringstream kvp(line);
			std::string key;

			if(std::getline(kvp, key, '=')) {
				std::string value;

				if(std::getline(kvp, value)) {
					if(read_old_firmware_config_case_string(key, value, "full_2d_mode", &out_full->mode_2d_only))
						continue;

					if(read_old_firmware_config_case_string(key, value, "full_regular_mode", &out_full->mode_regular))
						continue;

					if(read_old_firmware_config_case_string(key, value, "limited_2d_mode", &out_limited->mode_2d_only))
						continue;

					if(read_old_firmware_config_case_string(key, value, "limited_regular_mode", &out_limited->mode_regular))
						continue;
				}
			}
		}
	}
	catch(...) {
	}

	sanitize_capture_optimize_old_fw_config(out_full);
	sanitize_capture_optimize_old_fw_config(out_limited);
}

static void write_old_firmware_config_to_file(uint64_t device_id, CaptureOptimizeOldFirmwareConfig* to_save_full, CaptureOptimizeOldFirmwareConfig* to_save_limited) {
	std::ofstream file(get_old_fw_config_file_path(device_id));
	if(!file.good())
		return;

	file << write_old_firmware_config_case_string("full_2d_mode", &to_save_full->mode_2d_only);
	file << write_old_firmware_config_case_string("full_regular_mode", &to_save_full->mode_regular);

	file << write_old_firmware_config_case_string("limited_2d_mode", &to_save_limited->mode_2d_only);
	file << write_old_firmware_config_case_string("limited_regular_mode", &to_save_limited->mode_regular);

	file.close();
}

bool cyop_is_key_for_device(CaptureDevice* device, std::string key) {
	return check_key_matches_device_id(device->device_id, key, (const cyop_device_usb_device*)device->descriptor);
}

static std::string get_keys_file_path(bool is_new_device) {
	std::string path = get_base_path_keys();

	if(is_new_device)
		path += "optimize_new_3ds.txt";
	else
		path += "optimize_old_3ds.txt";
	return path;
}

static std::string read_key_for_device_id_from_file(uint64_t device_id, bool is_new_device) {
	std::ifstream file(get_keys_file_path(is_new_device));
	std::string line;
	std::string out_key = "";

	if((!file) || (!file.is_open()) || (!file.good()))
		return out_key;

	try {
		while(std::getline(file, line)) {
			std::istringstream kvp(line);
			std::string value;
			if(!std::getline(kvp, value))
				continue;
			if(check_key_matches_device_id(device_id, value, is_new_device)) {
				out_key = value;
				break;
			}
		}
	}
	catch(...) {
		out_key = "";
	}

	file.close();
	return out_key;
}

static void add_key_for_device_id_to_file(uint64_t device_id, std::string key, bool is_new_device) {
	// This is fine for a limited amount of keys.
	// It should be optimized if many keys need to be stored...
	if(key == "")
		return;
	if(read_key_for_device_id_from_file(device_id, is_new_device) == key)
		return;

	std::ofstream file(get_keys_file_path(is_new_device), std::ios_base::app | std::ios_base::out);
	file << key << std::endl;
	file.close();
}

KeySaveError add_key_to_file(std::string key, bool is_for_new_3ds) {
	uint64_t device_id = get_device_id_from_key(key, is_for_new_3ds);
	if(device_id == 0)
		return KEY_INVALID;
	if(read_key_for_device_id_from_file(device_id, is_for_new_3ds) == key)
		return KEY_ALREADY_PRESENT;
	add_key_for_device_id_to_file(device_id, key, is_for_new_3ds);
	return KEY_SAVED;
}

bool cyop_is_key_for_device_id(CaptureDevice* device, std::string key) {
	if(device == NULL)
		return false;
	if(device->cc_type != CAPTURE_CONN_CYPRESS_OPTIMIZE)
		return false;
	if(device->device_id == 0)
		return false;
	if(key == "")
		return false;
	return check_key_matches_device_id(device->device_id, key, (const cyop_device_usb_device*)device->descriptor);
}

void usb_cyop_device_init() {
	return usb_init();
}

void usb_cyop_device_close() {
	usb_close();
}


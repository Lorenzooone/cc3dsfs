#include "devicecapture.hpp"
#include "cypress_shared_driver_comms.hpp"
#include "cypress_shared_libusb_comms.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_partner_ctr_communications.hpp"
#include "cypress_partner_ctr_acquisition.hpp"
#include "cypress_partner_ctr_acquisition_general.hpp"
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

#define RESET_TIMEOUT 4.0
#define BATTERY_TIMEOUT 5.0
#define SLEEP_RESET_TIME_MS 2000

#define SYNCH_VALUE_PARTNER_CTR PARTNER_CTR_CAPTURE_BASE_COMMAND

#define TOTAL_WANTED_CONCURRENTLY_RUNNING_BUFFER_SIZE 0x100000
#define SINGLE_RING_BUFFER_SLICE_SIZE 0x10000
#define NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS (TOTAL_WANTED_CONCURRENTLY_RUNNING_BUFFER_SIZE / SINGLE_RING_BUFFER_SLICE_SIZE)

#define NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS (3 * NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS)

#define PARTNER_CTR_TOTAL_BUFFERS_SIZE (NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS * SINGLE_RING_BUFFER_SLICE_SIZE)

#define ERROR_CTR_SCREEN_SEARCH_NOT_ENOUGH_DATA ((size_t)-1)
#define ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED ((size_t)-2)

#define MAX_TIME_WAIT 1.0

#define PARTNER_CTR_CYPRESS_USB_WINDOWS_DRIVER CYPRESS_WINDOWS_DEFAULT_USB_DRIVER

struct CypressPartnerCTRDeviceCaptureReceivedData {
	CypressPartnerCTRDeviceCaptureReceivedData* array_ptr;
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
	bool* stored_is_3d;
	CaptureData* capture_data;
	std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start;
	cy_async_callback_data cb_data;
};

static void cypress_device_read_frame_cb(void* user_data, int transfer_length, int transfer_status);
static int get_cypress_device_status(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data);
static void error_cypress_device_status(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val);

static cy_device_device_handlers* usb_reconnect(const cypart_device_usb_device* usb_device_desc, CaptureDevice* device) {
	cy_device_device_handlers* final_handlers = NULL;
	int curr_serial_extra_id = 0;
	final_handlers = cypress_libusb_serial_reconnection(get_cy_usb_info(usb_device_desc), device->serial_number, curr_serial_extra_id, NULL);

	if (final_handlers == NULL)
		final_handlers = cypress_driver_serial_reconnection(device);
	return final_handlers;
}

static std::string _cypress_partner_ctr_get_serial(cy_device_device_handlers* handlers, const cypart_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	serial = read_serial_ctr_capture(handlers, usb_device_desc);
	if(serial != "")
		return serial;
	return std::to_string((curr_serial_extra_id++) + 1);
}

std::string cypress_partner_ctr_get_serial(cy_device_device_handlers* handlers, const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	if(usb_device_desc == NULL)
		return "";
	if(handlers == NULL)
		return "";
	const cypart_device_usb_device* in_usb_device_desc = (const cypart_device_usb_device*)usb_device_desc;
	return _cypress_partner_ctr_get_serial(handlers, in_usb_device_desc, serial, bcd_device, curr_serial_extra_id);
}

static CaptureDevice _cypress_partner_ctr_create_device(const cypart_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	CaptureDevice out_device = CaptureDevice(serial, usb_device_desc->name, usb_device_desc->long_name, CAPTURE_CONN_PARTNER_CTR, (void*)usb_device_desc, true, true, true, TOP_WIDTH_3DS, HEIGHT_3DS * 2, TOP_WIDTH_3DS, HEIGHT_3DS * 3, N3DSXL_SAMPLES_IN, 180, 0, HEIGHT_3DS, 0, 2 * HEIGHT_3DS, 0, 0, false, usb_device_desc->video_data_type, 0x200, path);
	out_device.is_horizontally_flipped = true;
	out_device.continuous_3d_screens = false;
	out_device.sample_rate = SAMPLE_RATE_48K;
	return out_device;
}

CaptureDevice cypress_partner_ctr_create_device(const void* usb_device_desc, std::string serial, std::string path) {
	if(usb_device_desc == NULL)
		return CaptureDevice();
	return _cypress_partner_ctr_create_device((const cypart_device_usb_device*)usb_device_desc, serial, path);
}

static void cypress_partner_ctr_connection_end(cy_device_device_handlers* handlers, const cypart_device_usb_device *device_desc, bool interface_claimed = true) {
	if (handlers == NULL)
		return;
	if (handlers->usb_handle)
		cypress_libusb_end_connection(handlers, get_cy_usb_info(device_desc), interface_claimed);
	else
		cypress_driver_end_connection(handlers);
	delete handlers;
}

void list_devices_cypart_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan) {
	const size_t num_cypart_device_desc = GetNumCyPartnerCTRDeviceDesc();
	int* curr_serial_extra_id_cypart_device = new int[num_cypart_device_desc];
	bool* no_access_elems = new bool[num_cypart_device_desc];
	bool* not_supported_elems = new bool[num_cypart_device_desc];
	std::vector<const cy_device_usb_device*> usb_devices_to_check;
	for (size_t i = 0; i < num_cypart_device_desc; i++) {
		no_access_elems[i] = false;
		not_supported_elems[i] = false;
		curr_serial_extra_id_cypart_device[i] = 0;
		const cypart_device_usb_device* curr_device_desc = GetCyPartnerCTRDeviceDesc((int)i);
		if(devices_allowed_scan[curr_device_desc->index_in_allowed_scan])
			usb_devices_to_check.push_back(get_cy_usb_info(curr_device_desc));
	}
	cypress_libusb_list_devices(devices_list, no_access_elems, not_supported_elems, curr_serial_extra_id_cypart_device, usb_devices_to_check);

	bool any_not_supported = false;
	for(size_t i = 0; i < num_cypart_device_desc; i++)
		any_not_supported |= not_supported_elems[i];
	for(size_t i = 0; i < num_cypart_device_desc; i++)
		if(no_access_elems[i])
			no_access_list.emplace_back(usb_devices_to_check[i]->vid, usb_devices_to_check[i]->pid);
	if(any_not_supported)
		cypress_driver_list_devices(devices_list, not_supported_elems, curr_serial_extra_id_cypart_device, usb_devices_to_check, PARTNER_CTR_CYPRESS_USB_WINDOWS_DRIVER);

	delete[] curr_serial_extra_id_cypart_device;
	delete[] no_access_elems;
	delete[] not_supported_elems;
}

bool cypart_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	const cypart_device_usb_device* usb_device_info = (const cypart_device_usb_device*)device->descriptor;
	cy_device_device_handlers* handlers = usb_reconnect(usb_device_info, device);
	if(handlers == NULL) {
		capture_error_print(true, capture_data, "Device not found");
		return false;
	}
	capture_data->handle = (void*)handlers;

	return true;
}

static uint64_t cypart_device_get_video_in_size(bool is_3d) {
	if(is_3d)
		return ((2 * TOP_WIDTH_3DS * HEIGHT_3DS) + (BOT_WIDTH_3DS * HEIGHT_3DS)) * sizeof(VideoPixelBGR);
	return ((TOP_WIDTH_3DS * HEIGHT_3DS) + (BOT_WIDTH_3DS * HEIGHT_3DS)) * sizeof(VideoPixelBGR);
}

uint64_t cypart_device_get_video_in_size(CaptureStatus* status, bool is_3d) {
	return cypart_device_get_video_in_size(is_3d);
}

uint64_t cypart_device_get_video_in_size(CaptureData* capture_data, bool is_3d) {
	return cypart_device_get_video_in_size(&capture_data->status, is_3d);
}

static int cypress_device_read_frame_request(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t index, bool is_3d) {
	if(cypress_device_capture_recv_data == NULL)
		return LIBUSB_SUCCESS;
	if((*cypress_device_capture_recv_data->status) < 0)
		return LIBUSB_SUCCESS;
	const cypart_device_usb_device* usb_device_info = (const cypart_device_usb_device*)capture_data->status.device.descriptor;
	cypress_device_capture_recv_data->index = index;
	cypress_device_capture_recv_data->cb_data.function = cypress_device_read_frame_cb;
	//size_t read_size = cypart_device_get_video_in_size(is_3d);
	size_t read_size = SINGLE_RING_BUFFER_SLICE_SIZE;
	int new_buffer_slice_index = ((*cypress_device_capture_recv_data->last_ring_buffer_slice_allocated) + 1) % NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS;
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

static void unlock_buffer_directly(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data_real, bool reset_slice_data_arr_info) {
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

static void unlock_buffer_slice_index(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, size_t slice_index, bool reset_slice_data_arr_info) {
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

static void end_cypress_device_read_frame_cb(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool to_unlock) {
	if(to_unlock)
		unlock_buffer_directly(cypress_device_capture_recv_data, true);
	cypress_device_capture_recv_data->to_process = false;
	if(!cypress_device_capture_recv_data->in_use)
		cypress_device_capture_recv_data->is_buffer_free_shared_mutex->specific_unlock(cypress_device_capture_recv_data->cb_data.internal_index);
}

static PartnerCTRCaptureCommand read_partner_ctr_base_command(uint8_t* data) {
	PartnerCTRCaptureCommand out_cmd;
	// Important: Ensure the data after the buffers for a single frame
	// is stopped by wrong magic value...
	out_cmd.magic = read_le16(data, 0);
	out_cmd.command = read_le16(data, 1);
	out_cmd.payload_size = read_le32(data, 1);
	return out_cmd;
}

static size_t get_partner_ctr_size_command_header(PartnerCTRCaptureCommand read_command) {
	switch (read_command.command) {
		case PARTNER_CTR_CAPTURE_COMMAND_INPUT:
			return sizeof(PartnerCTRCaptureCommandHeader0F);
		case PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN:
			return sizeof(PartnerCTRCaptureCommandHeaderCxScreen);
		case PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN:
			return sizeof(PartnerCTRCaptureCommandHeaderCxScreen);
		case PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN:
			return sizeof(PartnerCTRCaptureCommandHeaderCxScreen);
		case PARTNER_CTR_CAPTURE_COMMAND_AUDIO:
			return sizeof(PartnerCTRCaptureCommandHeaderC7);
		default:
			ActualConsoleOutTextError("Partner CTR Capture: Unknown command found! " + std::to_string(read_command.command));
			return sizeof(PartnerCTRCaptureCommandHeader0F);
	}
}

static bool get_is_pos_synch_in_buffer(uint8_t* buffer, size_t pos_to_check) {
	PartnerCTRCaptureCommand base_command = read_partner_ctr_base_command(buffer + pos_to_check);
	return base_command.magic == SYNCH_VALUE_PARTNER_CTR;
}

static bool get_is_pos_first_synch_in_buffer(uint8_t* buffer, size_t pos_to_check) {
	PartnerCTRCaptureCommand base_command = read_partner_ctr_base_command(buffer + pos_to_check);
	return (base_command.magic == SYNCH_VALUE_PARTNER_CTR) && (base_command.command == PARTNER_CTR_CAPTURE_COMMAND_INPUT);
}

static size_t get_pos_first_synch_in_buffer(uint8_t* buffer, int start_pos) {
	for(int i = start_pos; i < (SINGLE_RING_BUFFER_SLICE_SIZE / 2); i++) {
		if(get_is_pos_first_synch_in_buffer(buffer, i * 2))
			return i * 2;
	}
	return SINGLE_RING_BUFFER_SLICE_SIZE + 1;
}

static int get_num_consecutive_ready_ring_buffer_slices(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, int start_index) {
	for(int i = 0; i < NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS; i++) {
		int index_check = (start_index + i) % NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS;
		if(!cypress_device_capture_recv_data->is_ring_buffer_slice_data_ready_arr[index_check])
			return i;
	}
	// Should never happen
	return NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS;
}

static void unlock_buffers_from_x(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, int pos_to_unlock, int num_buffers_to_unlock, bool reset_slice_data_arr_info) {
	for(int i = 0; i < num_buffers_to_unlock; i++)
		unlock_buffer_slice_index(cypress_device_capture_recv_data, (pos_to_unlock + i) % NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS, reset_slice_data_arr_info);
}

static void copy_slice_data_to_buffer(uint8_t* dst, uint8_t *src, size_t start_slice_index, size_t start_slice_pos, size_t copy_size) {
	size_t start_byte = ((start_slice_index * SINGLE_RING_BUFFER_SLICE_SIZE) + start_slice_pos) % PARTNER_CTR_TOTAL_BUFFERS_SIZE;
	if((start_byte + copy_size) <= PARTNER_CTR_TOTAL_BUFFERS_SIZE)
		memcpy(dst, src + start_byte, copy_size);
	else {
		size_t upper_read_size = PARTNER_CTR_TOTAL_BUFFERS_SIZE - start_byte;
		size_t start_read_size = copy_size - upper_read_size;
		memcpy(dst, src + start_byte, upper_read_size);
		memcpy(dst + upper_read_size, src, start_read_size);
	}
}

static void cypress_output_to_thread(CaptureData* capture_data, uint8_t *buffer_arr, size_t start_slice_index, size_t start_slice_pos, int internal_index, std::chrono::time_point<std::chrono::high_resolution_clock>* clock_start, size_t read_size, bool is_3d, bool should_be_3d) {
	// Output to the other threads...
	CaptureDataSingleBuffer* data_buf = capture_data->data_buffers.GetWriterBuffer(internal_index);
	copy_slice_data_to_buffer((uint8_t*)&data_buf->capture_buf, buffer_arr, start_slice_index, start_slice_pos, read_size);
	// Ensure the buffer is ended by non-valid data...
	write_le16(((uint8_t*)&data_buf->capture_buf) + read_size, 0xFFFF);
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - (*clock_start);
	*clock_start = curr_time;
	capture_data->data_buffers.WriteToBuffer(NULL, read_size, diff.count(), &capture_data->status.device, internal_index, is_3d, should_be_3d);
	if (capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static bool cypress_device_read_frame_not_synchronized(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, int &error) {
	volatile int first_slice_to_check = *cypress_device_capture_recv_data->first_usable_ring_buffer_slice_index;
	volatile int first_slice_pos_to_check = *cypress_device_capture_recv_data->last_used_ring_buffer_slice_pos;
	bool found = false;
	// Determine which buffer is the first which needs to still be checked
	for(int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		int index_to_check = (first_slice_to_check + i) % NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS;
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
	for(int i = 0; i < NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS; i++) {
		int check_index = (first_slice_to_check + i) % NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS;
		// Search for the synch values. If they're not there, free the buffer.
		// If they are there, enque the buffer and switch mode.
		// Checks all available buffers which have data (but in order).
		if(cypress_device_capture_recv_data->is_ring_buffer_slice_data_ready_arr[check_index]) {
			size_t start_result = 0;
			if(check_index == first_slice_to_check)
				start_result = first_slice_pos_to_check + 2;
			size_t result = get_pos_first_synch_in_buffer(&cypress_device_capture_recv_data->ring_slice_buffer_arr[check_index * SINGLE_RING_BUFFER_SLICE_SIZE], start_result);
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

static PartnerCTRCaptureCommand get_command_partner_ctr(uint8_t* data, size_t slice_index, size_t start_pos, size_t curr_pos) {
	uint8_t tmp_buffer[sizeof(PartnerCTRCaptureCommand)];
	copy_slice_data_to_buffer(tmp_buffer, data, slice_index, start_pos + curr_pos, sizeof(PartnerCTRCaptureCommand));

	return read_partner_ctr_base_command(tmp_buffer);
}

static PartnerCTRCaptureCommandHeaderCxScreen get_command_screen_partner_ctr(uint8_t* data, size_t slice_index, size_t start_pos, size_t curr_pos) {
	uint8_t tmp_buffer[sizeof(PartnerCTRCaptureCommandHeaderCxScreen)];
	copy_slice_data_to_buffer(tmp_buffer, data, slice_index, start_pos + curr_pos, sizeof(PartnerCTRCaptureCommandHeaderCxScreen));

	PartnerCTRCaptureCommandHeaderCxScreen out_cmd;

	out_cmd.command = read_partner_ctr_base_command(tmp_buffer);
	out_cmd.index_kind = tmp_buffer[sizeof(PartnerCTRCaptureCommand)];
	out_cmd.index = read_le16(tmp_buffer + sizeof(PartnerCTRCaptureCommand) + 1);
	out_cmd.unk = tmp_buffer[sizeof(PartnerCTRCaptureCommand) + 3];
	for(int i = 0; i < 2; i++)
		out_cmd.unk2[i] = read_le16(tmp_buffer + sizeof(PartnerCTRCaptureCommand) + 4, i);

	return out_cmd;
}

static size_t get_pos_next_command_partner_ctr(uint8_t* data, size_t slice_index, size_t start_pos, size_t curr_pos) {
	uint8_t tmp_buffer[sizeof(PartnerCTRCaptureCommand)];
	copy_slice_data_to_buffer(tmp_buffer, data, slice_index, start_pos + curr_pos, sizeof(PartnerCTRCaptureCommand));

	PartnerCTRCaptureCommand read_command = read_partner_ctr_base_command(tmp_buffer);
	return curr_pos + get_partner_ctr_size_command_header(read_command) + read_command.payload_size;
}

static size_t find_pos_partner_ctr_x_screen(uint8_t* data, size_t slice_index, size_t start_pos, size_t curr_pos, size_t available_bytes) {
	if((curr_pos + sizeof(PartnerCTRCaptureCommand)) > available_bytes)
		return ERROR_CTR_SCREEN_SEARCH_NOT_ENOUGH_DATA;

	PartnerCTRCaptureCommand read_command = get_command_partner_ctr(data, slice_index, start_pos, curr_pos);
	if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
		return ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED;
	if((read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN))
		return curr_pos;

	while(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_AUDIO) {
		curr_pos = get_pos_next_command_partner_ctr(data, slice_index, start_pos, curr_pos);

		if((curr_pos + sizeof(PartnerCTRCaptureCommand)) > available_bytes)
			return ERROR_CTR_SCREEN_SEARCH_NOT_ENOUGH_DATA;

		read_command = get_command_partner_ctr(data, slice_index, start_pos, curr_pos);
		if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
			return ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED;
	}

	if((read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN))
		return curr_pos;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_INPUT)
		curr_pos = get_pos_next_command_partner_ctr(data, slice_index, start_pos, curr_pos);

	if((curr_pos + sizeof(PartnerCTRCaptureCommand)) > available_bytes)
		return ERROR_CTR_SCREEN_SEARCH_NOT_ENOUGH_DATA;
	read_command = get_command_partner_ctr(data, slice_index, start_pos, curr_pos);
	if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
		return ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED;

	if((read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN))
		return curr_pos;
	ActualConsoleOutTextError("Partner CTR Capture: Unknown command found! " + std::to_string(read_command.command));
	return ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED;
}

// Package together a frame
static bool is_valid_frame_partner_ctr(uint8_t* data, size_t slice_index, size_t start_pos, size_t available_bytes, bool &enabled_3d, size_t &out_end_pos, bool &synchronized) {
	bool has_top = false;
	bool has_top_second = false;
	bool has_bottom = false;
	size_t first_screen_pos = 0;
	size_t second_screen_pos = 0;
	size_t third_screen_pos = 0;
	size_t top_screen_pos = 0;
	PartnerCTRCaptureCommand top_command;
	out_end_pos = 0;

	first_screen_pos = find_pos_partner_ctr_x_screen(data, slice_index, start_pos, 0, available_bytes);
	if(first_screen_pos == ERROR_CTR_SCREEN_SEARCH_NOT_ENOUGH_DATA)
		return false;
	if(first_screen_pos == ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED) {
		synchronized = false;
		return false;
	}
	out_end_pos = get_pos_next_command_partner_ctr(data, slice_index, start_pos, first_screen_pos);

	PartnerCTRCaptureCommand read_command = get_command_partner_ctr(data, slice_index, start_pos, first_screen_pos);
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) {
		has_top = true;
		top_screen_pos = first_screen_pos;
	}
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN)
		has_bottom = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN)
		has_top_second = true;

	second_screen_pos = find_pos_partner_ctr_x_screen(data, slice_index, start_pos, out_end_pos, available_bytes);
	if(second_screen_pos == ERROR_CTR_SCREEN_SEARCH_NOT_ENOUGH_DATA)
		return false;
	if(second_screen_pos == ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED) {
		synchronized = false;
		return false;
	}
	out_end_pos = get_pos_next_command_partner_ctr(data, slice_index, start_pos, second_screen_pos);

	read_command = get_command_partner_ctr(data, slice_index, start_pos, second_screen_pos);
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) {
		has_top = true;
		top_screen_pos = second_screen_pos;
	}
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN)
		has_bottom = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN)
		has_top_second = true;

	if(enabled_3d && has_top) {
		if(out_end_pos > available_bytes)
			return false;

		PartnerCTRCaptureCommandHeaderCxScreen top_screen_command = get_command_screen_partner_ctr(data, slice_index, start_pos, top_screen_pos);
		// Does this frame support 3D?
		if((top_screen_command.index_kind == PARTNER_CTR_CAPTURE_SCREEN_INDEX_KIND_2D_TOP) && (!has_top_second))
			enabled_3d = false;
	}

	if(!enabled_3d) {
		if(!(has_top && has_bottom)) {
			synchronized = false;
			return false;
		}
		if(out_end_pos > available_bytes)
			return false;

		return true;
	}

	third_screen_pos = find_pos_partner_ctr_x_screen(data, slice_index, start_pos, out_end_pos, available_bytes);
	if(third_screen_pos == ERROR_CTR_SCREEN_SEARCH_NOT_ENOUGH_DATA)
		return false;
	if(third_screen_pos == ERROR_CTR_SCREEN_SEARCH_NOT_SYNCHRONIZED) {
		synchronized = false;
		return false;
	}
	out_end_pos = get_pos_next_command_partner_ctr(data, slice_index, start_pos, third_screen_pos);

	read_command = get_command_partner_ctr(data, slice_index, start_pos, third_screen_pos);
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) {
		has_top = true;
		top_screen_pos = third_screen_pos;
	}
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN)
		has_bottom = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN)
		has_top_second = true;

	if(!(has_top && has_bottom && has_top_second)) {
		synchronized = false;
		return false;
	}
	if(out_end_pos > available_bytes)
		return false;

	return true;
}

// Package together a frame by calling is_valid_frame_partner_ctr,
// then try adding to the end of it audio data, if present and read...
static bool is_valid_frame_partner_ctr_attempt_add_sound(uint8_t* data, size_t slice_index, size_t start_pos, size_t available_bytes, bool &enabled_3d, size_t &out_end_pos, bool &synchronized) {
	bool is_valid = is_valid_frame_partner_ctr(data, slice_index, start_pos, available_bytes, enabled_3d, out_end_pos, synchronized);
	if(!is_valid)
		return false;

	if((out_end_pos + sizeof(PartnerCTRCaptureCommand)) > available_bytes)
		return true;

	PartnerCTRCaptureCommand read_command = get_command_partner_ctr(data, slice_index, start_pos, out_end_pos);
	if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
		return true;

	if(read_command.command != PARTNER_CTR_CAPTURE_COMMAND_AUDIO)
		return true;

	size_t tentative_pos = get_pos_next_command_partner_ctr(data, slice_index, start_pos, out_end_pos);

	if(tentative_pos > available_bytes)
		return true;

	out_end_pos = tentative_pos;
	return true;
}

static void cypress_device_read_frame_synchronized(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	volatile int read_slice_index = *cypress_device_capture_recv_data->first_usable_ring_buffer_slice_index;
	volatile size_t read_slice_pos = *cypress_device_capture_recv_data->last_used_ring_buffer_slice_pos;
	int num_consecutive_ready_buffers = get_num_consecutive_ready_ring_buffer_slices(cypress_device_capture_recv_data, read_slice_index);
	bool should_be_3d_data = *cypress_device_capture_recv_data->stored_is_3d;
	bool is_3d_data = should_be_3d_data;
	size_t final_data_size = 0;
	bool synchronized = true;
	size_t curr_consecutive_available_bytes = (num_consecutive_ready_buffers * SINGLE_RING_BUFFER_SLICE_SIZE) - read_slice_pos;
	bool is_data_ready = is_valid_frame_partner_ctr_attempt_add_sound(cypress_device_capture_recv_data->ring_slice_buffer_arr, read_slice_index, read_slice_pos, curr_consecutive_available_bytes, is_3d_data, final_data_size, synchronized);
	if((!is_data_ready) && (!synchronized)) {
		*cypress_device_capture_recv_data->is_synchronized = false;
		return;
	}
	if(is_data_ready) {
		// Enough data. Time to do output...
		cypress_output_to_thread(cypress_device_capture_recv_data->capture_data, cypress_device_capture_recv_data->ring_slice_buffer_arr, read_slice_index, read_slice_pos, 0, cypress_device_capture_recv_data->clock_start, final_data_size, is_3d_data, should_be_3d_data);
		// Keep the ring buffer going.
		size_t raw_new_pos = read_slice_pos + final_data_size;
		int new_slice_index = (int)(read_slice_index + (raw_new_pos / SINGLE_RING_BUFFER_SLICE_SIZE));
		size_t new_slice_pos = raw_new_pos % SINGLE_RING_BUFFER_SLICE_SIZE;
		new_slice_index %= NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS;
		// Fully unlock used buffers
		unlock_buffers_from_x(cypress_device_capture_recv_data, read_slice_index, (new_slice_index + NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS - read_slice_index) % NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS, true);
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
	CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data = (CypressPartnerCTRDeviceCaptureReceivedData*)user_data;
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

static void close_all_reads_error(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cypart_device_usb_device* usb_device_desc = (const cypart_device_usb_device*)capture_data->status.device.descriptor;
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0) {
		if(!async_read_closed) {
			if(handlers->usb_handle) {
				for (int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++)
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

static void error_too_much_time_passed(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &async_read_closed, const std::chrono::time_point<std::chrono::high_resolution_clock> &start_time) {
	if(has_too_much_time_passed(start_time)) {
		error_cypress_device_status(cypress_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
		close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	}
}

static void unlock_buffer_in_error_case(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, int status) {
	if((status < 0) && (!cypress_device_capture_recv_data->to_process)) {
		unlock_buffer_directly(cypress_device_capture_recv_data, true);
	}
}

static bool is_buffer_still_in_use(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return cypress_device_capture_recv_data->in_use || cypress_device_capture_recv_data->to_process;
}

static void wait_all_cypress_device_buffers_free(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	if(cypress_device_capture_recv_data == NULL)
		return;
	bool async_read_closed = false;
	close_all_reads_error(capture_data, cypress_device_capture_recv_data, async_read_closed);
	const auto start_time = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++)
		while(is_buffer_still_in_use(&cypress_device_capture_recv_data[i])) {
			error_too_much_time_passed(capture_data, cypress_device_capture_recv_data, async_read_closed, start_time);
			unlock_buffer_in_error_case(&cypress_device_capture_recv_data[i], get_cypress_device_status(cypress_device_capture_recv_data));
			cypress_device_capture_recv_data[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
		}
}

static void wait_one_cypress_device_buffer_free(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	bool done = false;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(!done) {
		for(int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
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

static CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_get_free_buffer(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	wait_one_cypress_device_buffer_free(capture_data, cypress_device_capture_recv_data);
	if(get_cypress_device_status(cypress_device_capture_recv_data) < 0)
		return NULL;
	for(int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++)
		if(!is_buffer_still_in_use(&cypress_device_capture_recv_data[i])) {
			return &cypress_device_capture_recv_data[i];
		}
	return NULL;
}

static int get_cypress_device_status(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	return *cypress_device_capture_recv_data[0].status;
}

static void error_cypress_device_status(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, int error_val) {
	if((error_val == 0) || (get_cypress_device_status(cypress_device_capture_recv_data) == 0))
		*cypress_device_capture_recv_data[0].status = error_val;
}

static void exported_error_cypress_device_status(void* data, int error_val) {
	if(data == NULL)
		return;
	return error_cypress_device_status((CypressPartnerCTRDeviceCaptureReceivedData*)data, error_val);
}

static bool get_buffer_and_schedule_read(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t &index, bool &stored_is_3d, const std::string error_to_print) {
	CypressPartnerCTRDeviceCaptureReceivedData* chosen_buffer = cypress_device_get_free_buffer(capture_data, cypress_device_capture_recv_data);
	if(chosen_buffer == NULL)
		error_cypress_device_status(cypress_device_capture_recv_data, LIBUSB_ERROR_TIMEOUT);
	int ret = cypress_device_read_frame_request(capture_data, chosen_buffer, index++, stored_is_3d);
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

static bool schedule_all_reads(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t &index, bool &stored_is_3d, const std::string error_to_print) {
	for(int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		if(!get_buffer_and_schedule_read(capture_data, cypress_device_capture_recv_data, index, stored_is_3d, error_to_print))
			return false;
	}
	return true;
}

static int end_capture_from_capture_data(CaptureData* capture_data) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cypart_device_usb_device* usb_device_desc = (const cypart_device_usb_device*)capture_data->status.device.descriptor;
	return capture_end(handlers, usb_device_desc);
}

static void reset_buffer_processing_data(CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data) {
	*cypress_device_capture_recv_data[0].last_ring_buffer_slice_allocated = -1;
	*cypress_device_capture_recv_data[0].first_usable_ring_buffer_slice_index = 0;
	*cypress_device_capture_recv_data[0].last_used_ring_buffer_slice_pos = 0;
	*cypress_device_capture_recv_data[0].is_synchronized = false;
	*cypress_device_capture_recv_data[0].last_index = -1;
	for(int i = 0; i < NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS; i++) {
		cypress_device_capture_recv_data[0].is_ring_buffer_slice_data_ready_arr[i] = false;
		cypress_device_capture_recv_data[0].is_ring_buffer_slice_data_in_use_arr[i] = false;
		cypress_device_capture_recv_data[0].buffer_ring_slice_to_array_ptr[i] = -1;
	}
	for(int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		cypress_device_capture_recv_data[i].in_use = false;
		cypress_device_capture_recv_data[i].to_process = false;
		cypress_device_capture_recv_data[i].index = i;
		cypress_device_capture_recv_data[i].ring_buffer_slice_index = -1;
	}
}

static int restart_captures_cc_reads(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, uint32_t &index, bool &stored_is_3d, bool could_use_3d, bool force_capture_start, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_capture_start) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cypart_device_usb_device* usb_device_desc = (const cypart_device_usb_device*)capture_data->status.device.descriptor;
	int retval = end_capture_from_capture_data(capture_data);
	if(retval < 0)
		return retval;
	error_cypress_device_status(cypress_device_capture_recv_data, 0);
	default_sleep(100);

	bool set_max_again = false;
	if(force_capture_start){
		std::string read_serial_dummy = "";
		retval = capture_start(handlers, usb_device_desc, read_serial_dummy);
		clock_last_capture_start = std::chrono::high_resolution_clock::now();
		if(retval < 0)
			return retval;
	}

	bool is_new_3d = could_use_3d && get_3d_enabled(&capture_data->status);
	if(is_new_3d != stored_is_3d) {
		set_max_again = true;
		stored_is_3d = is_new_3d;
	}

	index = 0;
	reset_buffer_processing_data(cypress_device_capture_recv_data);

	capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	StartCaptureDma(handlers, usb_device_desc, stored_is_3d);

	if(!schedule_all_reads(capture_data, cypress_device_capture_recv_data, index, stored_is_3d, ""))
		return -1;

	return 0;
}

static bool has_battery_stuff_changed(CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_battery_set, int &curr_battery_percentage, bool &curr_ac_adapter_connected, bool &curr_ac_adapter_charging) {
	int loaded_battery_percentage = capture_data->status.partner_ctr_battery_percentage;
	bool loaded_ac_adapter_connected = capture_data->status.partner_ctr_ac_adapter_connected;
	bool loaded_ac_adapter_charging = capture_data->status.partner_ctr_ac_adapter_charging;

	const auto curr_time_battery = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff_battery = curr_time_battery - clock_last_battery_set;

	return !(((curr_battery_percentage == loaded_battery_percentage) || (diff_battery.count() <= BATTERY_TIMEOUT)) && (curr_ac_adapter_connected == loaded_ac_adapter_connected) && (curr_ac_adapter_charging == loaded_ac_adapter_charging));
}

static bool hardware_reset_requested(CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_reset) {
	bool reset_hardware = capture_data->status.reset_hardware;
	capture_data->status.reset_hardware = false;

	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - clock_last_reset;
	// Do not reset too fast... In general.
	return reset_hardware && (diff.count() > RESET_TIMEOUT);
}

static int CaptureBatteryHandleHardware(CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_battery_set, int &curr_battery_percentage, bool &curr_ac_adapter_connected, bool &curr_ac_adapter_charging, bool force = false) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cypart_device_usb_device* usb_device_desc = (const cypart_device_usb_device*)capture_data->status.device.descriptor;
	int loaded_battery_percentage = capture_data->status.partner_ctr_battery_percentage;
	bool loaded_ac_adapter_connected = capture_data->status.partner_ctr_ac_adapter_connected;
	bool loaded_ac_adapter_charging = capture_data->status.partner_ctr_ac_adapter_charging;

	const auto curr_time_battery = std::chrono::high_resolution_clock::now();

	if(force || (curr_battery_percentage != loaded_battery_percentage)) {
		int ret = set_battery_level(handlers, usb_device_desc, loaded_battery_percentage);
		if(ret < 0) {
			capture_error_print(true, capture_data, "Battery Set: Failed");
			return ret;
		}
		clock_last_battery_set = curr_time_battery;
		curr_battery_percentage = loaded_battery_percentage;
	}

	if(force || (curr_ac_adapter_connected != loaded_ac_adapter_connected) || (curr_ac_adapter_charging != loaded_ac_adapter_charging)) {
		int ret = set_charge_kind(handlers, usb_device_desc, loaded_ac_adapter_connected, loaded_ac_adapter_charging);
		if(ret < 0) {
			capture_error_print(true, capture_data, "AC Adapter Set: Failed");
			return ret;
		}
		curr_ac_adapter_connected = loaded_ac_adapter_connected;
		curr_ac_adapter_charging = loaded_ac_adapter_charging;
	}

	return 0;
}

static int CaptureResetHardware(CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &clock_last_reset) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cypart_device_usb_device* usb_device_desc = (const cypart_device_usb_device*)capture_data->status.device.descriptor;

	const auto curr_time = std::chrono::high_resolution_clock::now();
	clock_last_reset = curr_time;

	int ret = hardware_reset(handlers, usb_device_desc);
	if(ret < 0)
		return ret;
	default_sleep(SLEEP_RESET_TIME_MS);
	return ret;
}

static bool cypart_device_acquisition_loop(CaptureData* capture_data, CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data, bool &stored_is_3d, bool could_use_3d) {
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cypart_device_usb_device* usb_device_desc = (const cypart_device_usb_device*)capture_data->status.device.descriptor;
	uint32_t index = 0;
	uint64_t device_id = 0;
	int curr_battery_percentage = capture_data->status.partner_ctr_battery_percentage;
	bool curr_ac_adapter_connected = capture_data->status.partner_ctr_ac_adapter_connected;
	bool curr_ac_adapter_charging = capture_data->status.partner_ctr_ac_adapter_charging;
	std::string read_serial = "";
	bool force_capture_start_key = false;
	int ret = capture_start(handlers, usb_device_desc, read_serial);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_capture_start = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_reset = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_last_battery_set = std::chrono::high_resolution_clock::now();

	if (ret < 0) {
		capture_error_print(true, capture_data, "Capture Start: Failed");
		return false;
	}

	ret = CaptureBatteryHandleHardware(capture_data, clock_last_battery_set, curr_battery_percentage, curr_ac_adapter_connected, curr_ac_adapter_charging, true);

	if (ret < 0)
		return false;

	CypressSetMaxTransferSize(handlers, get_cy_usb_info(usb_device_desc), 0x80000);

	StartCaptureDma(handlers, usb_device_desc, stored_is_3d);
	if(!schedule_all_reads(capture_data, cypress_device_capture_recv_data, index, stored_is_3d, "Initial Reads: Failed"))
		return false;

	while (capture_data->status.connected && capture_data->status.running) {
		ret = get_cypress_device_status(cypress_device_capture_recv_data);
		if(ret < 0) {
			int cause_error = ret;
			capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM + NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS;
			wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);
			bool has_recovered = false;
			if(cause_error == LIBUSB_ERROR_TIMEOUT) {
				int timeout_ret = restart_captures_cc_reads(capture_data, cypress_device_capture_recv_data, index, stored_is_3d, could_use_3d, true, clock_last_capture_start);
				if(timeout_ret >= 0)
					has_recovered = true;
			}
			if(!has_recovered) {
				capture_error_print(true, capture_data, "Disconnected: Read error");
				return false;
			}
		}
		bool is_new_3d = could_use_3d && get_3d_enabled(&capture_data->status);
		bool updated_battery = has_battery_stuff_changed(capture_data, clock_last_battery_set, curr_battery_percentage, curr_ac_adapter_connected, curr_ac_adapter_charging);
		bool requested_reset = hardware_reset_requested(capture_data, clock_last_reset);
		if((is_new_3d != stored_is_3d) || updated_battery || requested_reset) {
			capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM + NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS;
			wait_all_cypress_device_buffers_free(capture_data, cypress_device_capture_recv_data);

			if(updated_battery) {
				ret = CaptureBatteryHandleHardware(capture_data, clock_last_battery_set, curr_battery_percentage, curr_ac_adapter_connected, curr_ac_adapter_charging);
				if(ret < 0)
					return false;
			}

			if(requested_reset) {
				ret = CaptureResetHardware(capture_data, clock_last_reset);
				if(ret < 0) {
					capture_error_print(true, capture_data, "Hardware Reset: Failed");
					return false;
				}
			}

			ret = restart_captures_cc_reads(capture_data, cypress_device_capture_recv_data, index, stored_is_3d, could_use_3d, false, clock_last_capture_start);
			if(ret < 0) {
				capture_error_print(true, capture_data, "Disconnected: Update mode error");
				return false;
			}
		}
		if(!get_buffer_and_schedule_read(capture_data, cypress_device_capture_recv_data, index, stored_is_3d, "Setup Read: Failed"))
			return false;
	}
	return true;
}

void cypart_device_acquisition_main_loop(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	bool is_done_thread;
	std::thread async_processing_thread;
	cy_device_device_handlers* handlers = (cy_device_device_handlers*)capture_data->handle;
	const cypart_device_usb_device* usb_device_desc = (const cypart_device_usb_device*)capture_data->status.device.descriptor;

	uint32_t last_index = -1;
	int status = 0;
	bool could_use_3d = get_3d_enabled(&capture_data->status, true);
	bool stored_is_3d = could_use_3d && get_3d_enabled(&capture_data->status);

	uint8_t *ring_slice_buffer = new uint8_t[PARTNER_CTR_TOTAL_BUFFERS_SIZE];
	bool *is_ring_buffer_slice_data_ready = new bool[NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS];
	bool *is_ring_buffer_slice_data_in_use = new bool[NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS];
	int *buffer_ring_slice_to_array = new int[NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS];
	for(int i = 0; i < NUM_TOTAL_PARTNER_CTR_CYPRESS_BUFFERS; i++) {
		is_ring_buffer_slice_data_ready[i] = false;
		is_ring_buffer_slice_data_in_use[i] = false;
		buffer_ring_slice_to_array[i] = -1;
	}
	int first_usable_ring_buffer_slice_index = 0;
	bool is_synchronized = false;
	size_t last_used_ring_buffer_slice_pos = 0;
	int last_ring_buffer_slice_allocated = -1;

	std::vector<cy_async_callback_data*> cb_queue;
	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS);
	SharedConsumerMutex is_transfer_done_shared_mutex(NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS);
	SharedConsumerMutex is_transfer_data_ready_shared_mutex(NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS);
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	CypressPartnerCTRDeviceCaptureReceivedData* cypress_device_capture_recv_data = new CypressPartnerCTRDeviceCaptureReceivedData[NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS];
	for(int i = 0; i < NUM_PARTNER_CTR_CYPRESS_CONCURRENTLY_RUNNING_BUFFERS; i++) {
		cypress_device_capture_recv_data[i].array_ptr = cypress_device_capture_recv_data;
		cypress_device_capture_recv_data[i].in_use = false;
		cypress_device_capture_recv_data[i].to_process = false;
		cypress_device_capture_recv_data[i].index = i;
		cypress_device_capture_recv_data[i].ring_buffer_slice_index = -1;
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
	bool proper_return = cypart_device_acquisition_loop(capture_data, cypress_device_capture_recv_data, stored_is_3d, could_use_3d);
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

void usb_cypart_device_acquisition_cleanup(CaptureData* capture_data) {
	if(!usb_is_initialized())
		return;
	cypress_partner_ctr_connection_end((cy_device_device_handlers*)capture_data->handle, (const cypart_device_usb_device*)capture_data->status.device.descriptor);
	capture_data->handle = NULL;
}

bool is_device_partner_ctr(CaptureDevice* device) {
	if(device == NULL)
		return false;
	if(device->cc_type != CAPTURE_CONN_PARTNER_CTR)
		return false;
	return true;
}

void usb_cypart_device_init() {
	return usb_init();
}

void usb_cypart_device_close() {
	usb_close();
}


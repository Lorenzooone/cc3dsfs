#include "dscapture_libftdi2.hpp"
#include "dscapture_ftd2_general.hpp"
#include "dscapture_ftd2_compatibility.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"

#include <ftdi.h>

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define FT_FAILED(x) ((x) != FT_OK)

#define MANUFACTURER_SIZE 128
#define DESCRIPTION_SIZE 128
#define SERIAL_NUMBER_SIZE 128

#define ENABLE_AUDIO true

#define FTDI_VID 0x0403
#define FT232H_PID 0x6014

#define EXPECTED_IGNORED_HALFWORDS 1

#define IGNORE_FIRST_FEW_FRAMES_SYNC (NUM_CAPTURE_RECEIVED_DATA_BUFFERS * 2)
#define RESYNC_TIMEOUT 0.050

struct vid_pid_descriptor {
	int vid;
	int pid;
};

static const vid_pid_descriptor base_device = {
	.vid = FTDI_VID, .pid = FT232H_PID
};

static const vid_pid_descriptor* accepted_devices[] = {
&base_device,
};

static const vid_pid_descriptor* get_device_descriptor(int vid, int pid) {
	for(int i = 0; i < sizeof(accepted_devices) / sizeof(*accepted_devices); i++)
		if((vid == accepted_devices[i]->vid) && (pid == accepted_devices[i]->pid))
			return accepted_devices[i];
	return NULL;
}

void libftdi_init() {
}

void libftdi_end() {
}

static void libftdi_usb_thread_function(bool* usb_thread_run, libusb_context *usb_ctx) {
	if(!usb_is_initialized())
		return;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 300000;
	while(*usb_thread_run)
		libusb_handle_events_timeout_completed(usb_ctx, &tv, NULL);
}

static void libftdi_start_thread(std::thread* thread_ptr, bool* usb_thread_run, libusb_context *usb_ctx) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = true;
	*thread_ptr = std::thread(libftdi_usb_thread_function, usb_thread_run, usb_ctx);
}

static void libftdi_close_thread(std::thread* thread_ptr, bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = false;
	thread_ptr->join();
}

static int check_single_device_valid_libftdi(ftdi_context *handle, libusb_device* dev, char* description, char* SerialNumber, const vid_pid_descriptor** curr_descriptor) {
	char manufacturer[MANUFACTURER_SIZE];
	libusb_device_handle *dev_handle = NULL;
	int retval = libusb_open(dev, &dev_handle);
	if(retval || (dev_handle == NULL))
		return retval;
	retval = libusb_claim_interface(dev_handle, handle->interface);
	if(retval == LIBUSB_SUCCESS)
		libusb_release_interface(dev_handle, handle->interface);
	libusb_close(dev_handle);
	if(retval < 0)
		return retval;
	libusb_device_descriptor desc = {0};
	retval = libusb_get_device_descriptor(dev, &desc);
	*curr_descriptor = NULL;
	if(retval >= 0)
		*curr_descriptor = get_device_descriptor(desc.idVendor, desc.idProduct);
	if((retval < 0) || ((retval = ftdi_usb_get_strings(handle, dev, manufacturer, MANUFACTURER_SIZE, description, DESCRIPTION_SIZE, SerialNumber, SERIAL_NUMBER_SIZE)) < 0))
		return retval;
	if((desc.bcdUSB < 0x0200) || ((*curr_descriptor) == NULL))
		return LIBUSB_ERROR_OTHER;
	return LIBUSB_SUCCESS;
}

void list_devices_libftdi(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	ftdi_device_list *devlist, *curdev;
	ftdi_context *handle = ftdi_new();
	char description[DESCRIPTION_SIZE], SerialNumber[SERIAL_NUMBER_SIZE];
	int debug_multiplier = 1;
	bool insert_anyway = false;
	bool perm_error = false;
	const vid_pid_descriptor* curr_descriptor;

	int num_devices = ftdi_usb_find_all(handle, &devlist, 0, 0);
	if(num_devices < 0) {
		ftdi_free(handle);
		return;
	}
	curdev = devlist;
	while(curdev != NULL)
	{
		int retval = check_single_device_valid_libftdi(handle, curdev->dev, description, SerialNumber, &curr_descriptor);
		if(retval < 0) {
			if(retval == LIBUSB_ERROR_ACCESS)
				perm_error = true;
			curdev = curdev->next;
			continue;
		}
		std::string serial_number = std::string(SerialNumber);
		bool is_already_inserted = false;
		for(int j = 0; j < devices_list.size(); j++) {
			if((devices_list[j].cc_type == CAPTURE_CONN_FTD2) && (devices_list[j].serial_number == serial_number)) {
				is_already_inserted = true;
				break;
			}
		}
		if(is_already_inserted && (!insert_anyway)) {
			curdev = curdev->next;
			continue;
		}
		for(int j = 0; j < get_num_ftd2_device_types(); j++) {
			if(description == get_ftd2_fw_desc(j)) {
				for(int u = 0; u < debug_multiplier; u++)
					devices_list.emplace_back(serial_number, "DS.2", "DS.2.l565", CAPTURE_CONN_FTD2, (void*)curr_descriptor, false, false, ENABLE_AUDIO, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, get_max_samples(false), 0, 0, 0, 0, HEIGHT_DS, VIDEO_DATA_RGB16, get_ftd2_fw_index(j), false, std::string(description));
				break;
			}
		}
		curdev = curdev->next;
	}
	ftdi_list_free(&devlist);
	ftdi_free(handle);
	if(perm_error)
		no_access_list.emplace_back("libftdi");
}

int libftdi_reset(void* handle) {
	return ftdi_usb_reset((ftdi_context*)handle);
}

int libftdi_set_latency_timer(void* handle, unsigned char latency) {
	return ftdi_set_latency_timer((ftdi_context*)handle, latency);
}

int libftdi_setflowctrl(void* handle, int flowctrl, unsigned char xon, unsigned char xoff) {
	if((flowctrl == SIO_DISABLE_FLOW_CTRL) || (flowctrl == SIO_RTS_CTS_HS) || (flowctrl == SIO_DTR_DSR_HS))
		return ftdi_setflowctrl((ftdi_context*)handle, flowctrl);
	return ftdi_setflowctrl_xonxoff((ftdi_context*)handle, xon, xoff);
}

int libftdi_set_bitmode(void* handle, unsigned char bitmask, unsigned char mode) {
	return ftdi_set_bitmode((ftdi_context*)handle, bitmask, mode);
}

int libftdi_purge(void* handle, bool do_read, bool do_write) {
	if(do_read && do_write)
		return ftdi_tcioflush((ftdi_context*)handle);
	if(do_read)
		return ftdi_tciflush((ftdi_context*)handle);
	if(do_write)
		return ftdi_tcoflush((ftdi_context*)handle);
	return 0;
}

int libftdi_read_eeprom(void* handle, int eeprom_addr, int *eeprom_val) {
	unsigned short val = 0;
	int ret = ftdi_read_eeprom_location((ftdi_context*)handle, eeprom_addr, &val);
	*eeprom_val = val;
	return ret;
}

int libftdi_set_chars(void* handle, unsigned char eventch, unsigned char event_enable, unsigned char errorch, unsigned char error_enable) {
	int ret = ftdi_set_event_char((ftdi_context*)handle, eventch, event_enable);
	if(ret < 0)
		return ret;
	return ftdi_set_error_char((ftdi_context*)handle, errorch, error_enable);
}

int libftdi_set_usb_chunksizes(void* handle, unsigned int chunksize_in, unsigned int chunksize_out) {
	int ret = ftdi_read_data_set_chunksize((ftdi_context*)handle, chunksize_in);
	if(ret < 0)
		return ret;
	return ftdi_write_data_set_chunksize((ftdi_context*)handle, chunksize_out);
}

void libftdi_set_timeouts(void* handle, int timeout_in_ms, int timeout_out_ms) {
	ftdi_context* in_handle = (ftdi_context*)handle;
	in_handle->usb_read_timeout = timeout_in_ms;
	in_handle->usb_write_timeout = timeout_out_ms;
}

int libftdi_write(void* handle, const uint8_t* data, size_t size, size_t* bytesOut) {
	*bytesOut = ftdi_write_data((ftdi_context*)handle, data, size);
	if((*bytesOut) >= 0)
		return 0;
	return *bytesOut;
}

int libftdi_read(void* handle, uint8_t* data, size_t size, size_t* bytesIn) {
	*bytesIn = ftdi_read_data((ftdi_context*)handle, data, size);
	if((*bytesIn) >= 0)
		return 0;
	return *bytesIn;
}

int get_libftdi_read_queue_size(void* handle, size_t* bytesIn) {
	uint8_t buffer[64];
	*bytesIn = 0;
	ftdi_context* in_handle = (ftdi_context*)handle;
	int timeout_in_ms = in_handle->usb_read_timeout;
	in_handle->usb_read_timeout = 0;
	size_t curr_bytes_in = 0;
	bool done = false;
	int retval = 0;
	while(!done) {
		retval = libftdi_read(handle, buffer, 64, &curr_bytes_in);
		if(retval <= 0)
			done = true;
		else
			*bytesIn += curr_bytes_in;
	}
	in_handle->usb_read_timeout = timeout_in_ms;
	return LIBUSB_SUCCESS;
}

int libftdi_open_serial(CaptureDevice* device, void** handle) {
	ftdi_device_list *devlist, *curdev;
	ftdi_context *curr_handle = ftdi_new();
	char description[DESCRIPTION_SIZE], SerialNumber[SERIAL_NUMBER_SIZE];
	int debug_multiplier = 1;
	bool insert_anyway = false;
	bool perm_error = false;
	const vid_pid_descriptor* curr_descriptor;
	int ret = LIBUSB_ERROR_OTHER;

	int num_devices = ftdi_usb_find_all(curr_handle, &devlist, 0, 0);
	if(num_devices < 0) {
		ftdi_free(curr_handle);
		return LIBUSB_ERROR_OTHER;
	}
	curdev = devlist;
	while(curdev != NULL)
	{
		int retval = check_single_device_valid_libftdi(curr_handle, curdev->dev, description, SerialNumber, &curr_descriptor);
		if(retval < 0) {
			curdev = curdev->next;
			continue;
		}
		if(curr_descriptor != ((const vid_pid_descriptor*)device->descriptor)) {
			curdev = curdev->next;
			continue;
		}
		std::string serial_number = std::string(SerialNumber);
		std::string desc = std::string(description);
		if((serial_number != device->serial_number) || (desc != device->path)) {
			curdev = curdev->next;
			continue;
		}
		ret = ftdi_usb_open_dev(curr_handle, curdev->dev);
		if(ret >= 0)
			*handle = (void*)curr_handle;
		curdev = NULL;
	}
	ftdi_list_free(&devlist);
	if(ret < 0)
		ftdi_free(curr_handle);
	return ret;
}

int libftdi_close(void* handle) {
	ftdi_usb_close((ftdi_context*)handle);
	ftdi_free((ftdi_context*)handle);
	return LIBUSB_SUCCESS;
}

void libftdi_cancel_callback(ftd2_async_callback_data* cb_data) {
	cb_data->transfer_data_access.lock();
	if(cb_data->transfer_data)
		libusb_cancel_transfer((libusb_transfer*)cb_data->transfer_data);
	cb_data->transfer_data_access.unlock();
}

static void STDCALL libftdi_read_callback(libusb_transfer* transfer) {
	ftd2_async_callback_data* cb_data = (ftd2_async_callback_data*)transfer->user_data;
	FTD2CaptureReceivedData* user_data = (FTD2CaptureReceivedData*)cb_data->actual_user_data;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = NULL;
	cb_data->is_transfer_done_mutex->specific_unlock(cb_data->internal_index);
	cb_data->transfer_data_access.unlock();
	cb_data->function((void*)user_data, transfer->actual_length, transfer->status);
}

// Read from bulk
static void libftdi_schedule_read(ftd2_async_callback_data* cb_data, uint8_t* buffer_raw, int length) {
	const int max_packet_size = MAX_PACKET_SIZE_USB2;
	libusb_transfer *transfer_in = libusb_alloc_transfer(0);
	if(!transfer_in)
		return;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = transfer_in;
	cb_data->is_transfer_done_mutex->specific_try_lock(cb_data->internal_index);
	length += ((length + (max_packet_size - FTD2_INTRA_PACKET_HEADER_SIZE) - 1) / (max_packet_size - FTD2_INTRA_PACKET_HEADER_SIZE)) * FTD2_INTRA_PACKET_HEADER_SIZE;
	cb_data->requested_length = length;
	ftdi_context* in_handle = (ftdi_context*)cb_data->handle;
	libusb_fill_bulk_transfer(transfer_in, in_handle->usb_dev, in_handle->out_ep, buffer_raw, length, libftdi_read_callback, (void*)cb_data, in_handle->usb_read_timeout * NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	transfer_in->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
	libusb_submit_transfer(transfer_in);
	cb_data->transfer_data_access.unlock();
}

static size_t libftdi_get_actual_length(const int max_packet_size, size_t length, size_t header_packet_size) {
	// Remove the small headers every 512 bytes...
	// The "- header_packet_size" instead of "-1" covers for partial header transfers...
	int num_iters = (length + max_packet_size - header_packet_size) / max_packet_size;
	if(num_iters > 0)
		length -= (num_iters * header_packet_size);
	else
		length = 0;
	return length;
}

static void libftdi_copy_buffer_to_target(uint8_t* buffer_written, uint8_t* buffer_target, const int max_packet_size, size_t length, size_t header_packet_size) {
	// Remove the small headers every 512 bytes...
	// The "- header_packet_size" instead of "-1" covers for partial header transfers...
	int num_iters = (length + max_packet_size - header_packet_size) / max_packet_size;
	if(num_iters <= 0)
		return;

	length -= (num_iters * header_packet_size);
	for(int i = 0; i < num_iters; i++) {
		int rem_size = length - ((max_packet_size - header_packet_size) * i);
		if(rem_size > ((int)(max_packet_size - header_packet_size)))
			rem_size = max_packet_size - header_packet_size;
		if(rem_size <= 0)
			break;
		memcpy(buffer_target + ((max_packet_size - header_packet_size) * i), buffer_written + header_packet_size + (max_packet_size * i), rem_size);
	}
}

// Read from bulk
static int libftdi_direct_read(void* handle, uint8_t* buffer_raw, uint8_t* buffer_normal, size_t length, size_t* transferred) {
	const int max_packet_size = MAX_PACKET_SIZE_USB2;
	length += ((length + (max_packet_size - FTD2_INTRA_PACKET_HEADER_SIZE) - 1) / (max_packet_size - FTD2_INTRA_PACKET_HEADER_SIZE)) * FTD2_INTRA_PACKET_HEADER_SIZE;
	ftdi_context* in_handle = (ftdi_context*)handle;
	*transferred = 0;
	int internal_transferred = 0;
	int retval = libusb_bulk_transfer(in_handle->usb_dev, in_handle->out_ep, buffer_raw, length, &internal_transferred, in_handle->usb_read_timeout);
	if(retval < 0)
		return retval;
	libftdi_copy_buffer_to_target(buffer_raw, buffer_normal, max_packet_size, internal_transferred, FTD2_INTRA_PACKET_HEADER_SIZE);
	*transferred = libftdi_get_actual_length(max_packet_size, internal_transferred, FTD2_INTRA_PACKET_HEADER_SIZE);
	return LIBUSB_SUCCESS;
}

static int libftdi_full_read(void* handle, uint8_t* buffer_raw, uint8_t* buffer_normal, size_t length, double timeout) {
	size_t total_transferred = 0;
	const auto start_time = std::chrono::high_resolution_clock::now();
	while(total_transferred < length) {
		size_t received = 0;
		int retval = libftdi_direct_read(handle, buffer_raw, buffer_normal, length - total_transferred, &received);
		if(ftd2_is_error(retval, true))
			return retval;
		total_transferred += received;
		if(received == 0)
			break;
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - start_time;
		if(diff.count() > timeout)
			return LIBUSB_ERROR_TIMEOUT;
	}
	return LIBUSB_SUCCESS;
}

static void libftdi_copy_buffer_to_target_and_skip(uint8_t* buffer_written, uint8_t* buffer_target, const int max_packet_size, size_t length, size_t header_packet_size, size_t ignored_bytes) {
	// This could be made faster for small "ignored_bytes", however this scales well...
	// Plus, most of the time is used up by the memcpy routine, so...

	// Remove the small headers every 512 bytes...
	// The "- header_packet_size" instead of "-1" covers for partial header transfers...
	int num_iters = (length + max_packet_size - header_packet_size) / max_packet_size;
	if(num_iters <= 0)
		return;

	size_t inner_length = length - (num_iters * header_packet_size);
	size_t fully_ignored_iters = ignored_bytes / (max_packet_size - header_packet_size);
	size_t partially_ignored_iters = (ignored_bytes + (max_packet_size - header_packet_size) - 1) / (max_packet_size - header_packet_size);
	num_iters -= fully_ignored_iters;
	if(num_iters <= 0)
		return;

	buffer_written += fully_ignored_iters * max_packet_size;
	// Skip inside a packet, since it's misaligned
	if(partially_ignored_iters != fully_ignored_iters) {
		size_t offset_bytes = ignored_bytes % (max_packet_size - header_packet_size);
		int rem_size = inner_length - ((max_packet_size - header_packet_size) * fully_ignored_iters);
		if(rem_size > ((int)((max_packet_size - header_packet_size))))
			rem_size = max_packet_size - header_packet_size;
		rem_size -= offset_bytes;
		if(rem_size > 0) {
			memcpy(buffer_target, buffer_written + header_packet_size + offset_bytes, rem_size);
			buffer_written += max_packet_size;
			buffer_target += rem_size;
		}
	}
	if(length <= (max_packet_size * partially_ignored_iters))
		return;
	libftdi_copy_buffer_to_target(buffer_written, buffer_target, max_packet_size, length - (max_packet_size * partially_ignored_iters), header_packet_size);
}

static int get_libftdi_status(FTD2CaptureReceivedData* received_data_buffers) {
	return *received_data_buffers[0].status;
}

static void reset_libftdi_status(FTD2CaptureReceivedData* received_data_buffers) {
	*received_data_buffers[0].status = 0;
}

static void error_libftdi_status(FTD2CaptureReceivedData* received_data_buffers, int error) {
	*received_data_buffers[0].status = error;
}

static int libftdi_get_num_free_buffers(FTD2CaptureReceivedData* received_data_buffers) {
	int num_free = 0;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!received_data_buffers[i].in_use)
			num_free += 1;
	return num_free;
}

static void wait_all_libftdi_transfers_done(FTD2CaptureReceivedData* received_data_buffers) {
	if (received_data_buffers == NULL)
		return;
	if (*received_data_buffers[0].status < 0) {
		for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
			libftdi_cancel_callback(&received_data_buffers[i].cb_data);
	}
	for (int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
		void* transfer_data;
		do {
			received_data_buffers[i].cb_data.transfer_data_access.lock();
			transfer_data = received_data_buffers[i].cb_data.transfer_data;
			received_data_buffers[i].cb_data.transfer_data_access.unlock();
			if(transfer_data)
				received_data_buffers[i].cb_data.is_transfer_done_mutex->specific_timed_lock(i);
		} while(transfer_data);
	}
}

static void wait_all_libftdi_buffers_free(FTD2CaptureReceivedData* received_data_buffers) {
	if(received_data_buffers == NULL)
		return;
	if(*received_data_buffers[0].status < 0) {
		for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
			libftdi_cancel_callback(&received_data_buffers[i].cb_data);
	}
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		while(received_data_buffers[i].in_use)
			received_data_buffers[i].is_buffer_free_shared_mutex->specific_timed_lock(i);
}

static void wait_one_libftdi_buffer_free(FTD2CaptureReceivedData* received_data_buffers) {
	bool done = false;
	while(!done) {
		for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
			if(!received_data_buffers[i].in_use)
				done = true;
		}
		if(!done) {
			if(*received_data_buffers[0].status < 0)
				return;
			int dummy = 0;
			received_data_buffers[0].is_buffer_free_shared_mutex->general_timed_lock(&dummy);
		}
	}
}

static bool libftdi_are_buffers_all_free(FTD2CaptureReceivedData* received_data_buffers) {
	return libftdi_get_num_free_buffers(received_data_buffers) == NUM_CAPTURE_RECEIVED_DATA_BUFFERS;
}

static FTD2CaptureReceivedData* libftdi_get_free_buffer(FTD2CaptureReceivedData* received_data_buffers) {
	wait_one_libftdi_buffer_free(received_data_buffers);
	if(*received_data_buffers[0].status < 0)
		return NULL;
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		if(!received_data_buffers[i].in_use) {
			received_data_buffers[i].is_buffer_free_shared_mutex->specific_try_lock(i);
			received_data_buffers[i].in_use = true;
			return &received_data_buffers[i];
		}
	return NULL;
}

static void libftdi_start_read(FTD2CaptureReceivedData* received_data_buffer, int index, size_t size) {
	if(received_data_buffer == NULL)
		return;
	CaptureDataSingleBuffer* full_data_buf = received_data_buffer->capture_data->data_buffers.GetWriterBuffer(received_data_buffer->cb_data.internal_index);
	CaptureReceived* data_buffer = &full_data_buf->capture_buf;
	received_data_buffer->buffer_raw = (uint8_t*)&data_buffer->ftd2_received_old_ds_normal_plus_raw.raw_data;
	received_data_buffer->buffer_target = (uint32_t*)data_buffer;
	received_data_buffer->index = index;
	libftdi_schedule_read(&received_data_buffer->cb_data, received_data_buffer->buffer_raw, size);
}

static void end_libftdi_read_frame_cb(FTD2CaptureReceivedData* received_data_buffer, bool has_succeded) {
	if(!has_succeded)
		received_data_buffer->capture_data->data_buffers.ReleaseWriterBuffer(received_data_buffer->cb_data.internal_index, false);
	received_data_buffer->in_use = false;
	received_data_buffer->is_buffer_free_shared_mutex->specific_unlock(received_data_buffer->cb_data.internal_index);
}

static size_t libftdi_copy_buffer_to_target_and_skip_synch(uint8_t* in_buffer, uint32_t* out_buffer, int read_length, size_t* sync_offset) {
	// This is because the actual data seems to always start with a SYNCH
	size_t ignored_halfwords = 0;
	uint16_t* in_u16 = (uint16_t*)in_buffer;
	size_t real_length = libftdi_get_actual_length(MAX_PACKET_SIZE_USB2, read_length, FTD2_INTRA_PACKET_HEADER_SIZE);
	while((ignored_halfwords < (real_length / 2)) && (in_u16[ignored_halfwords + 1 + (ignored_halfwords / (MAX_PACKET_SIZE_USB2 / 2))] == FTD2_OLDDS_SYNCH_VALUES))
		ignored_halfwords++;
	size_t copy_offset = ignored_halfwords * 2;
	libftdi_copy_buffer_to_target_and_skip(in_buffer, (uint8_t*)out_buffer, MAX_PACKET_SIZE_USB2, read_length, FTD2_INTRA_PACKET_HEADER_SIZE, copy_offset);
	if((copy_offset == 0) || (copy_offset >= (MAX_PACKET_SIZE_USB2 - FTD2_INTRA_PACKET_HEADER_SIZE))) {
		size_t internal_sync_offset = 0;
		bool is_synced = synchronization_check((uint16_t*)out_buffer, real_length, NULL, &internal_sync_offset);
		if(!is_synced) {
			*sync_offset = (internal_sync_offset / (MAX_PACKET_SIZE_USB2 - FTD2_INTRA_PACKET_HEADER_SIZE)) * (MAX_PACKET_SIZE_USB2 - FTD2_INTRA_PACKET_HEADER_SIZE);
			return 0;
		}
		else
			*sync_offset = 0;
	}
	else
		*sync_offset = 0;
	uint16_t* out_u16 = (uint16_t*)out_buffer;
	for(int i = 0; i < ignored_halfwords; i++)
		out_u16[(real_length / 2) - ignored_halfwords + i] = FTD2_OLDDS_SYNCH_VALUES;
	return remove_synch_from_final_length(out_buffer, real_length);
}

static void output_to_thread(CaptureData* capture_data, int internal_index, uint8_t* buffer_raw, uint32_t* buffer_target, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time, int read_length, size_t* sync_offset) {
	// For some reason, there is usually one.
	// Though make this generic enough
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	// Copy data to buffer, with special memcpy which accounts for ftd2 header data and skips synch bytes
	size_t real_length = libftdi_copy_buffer_to_target_and_skip_synch(buffer_raw, buffer_target, read_length, sync_offset);
	capture_data->data_buffers.WriteToBuffer(NULL, real_length, diff.count(), &capture_data->status.device, internal_index);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static void libftdi_capture_process_data(void* in_user_data, int transfer_length, int transfer_status) {
	// Note: sometimes the data returned has length 0...
	// It's because the code is too fast...
	FTD2CaptureReceivedData* user_data = (FTD2CaptureReceivedData*)in_user_data;
	if((*user_data->status) < 0)
		return end_libftdi_read_frame_cb(user_data, false);
	if(transfer_status != LIBUSB_TRANSFER_COMPLETED) {
		*user_data->status = LIBUSB_ERROR_OTHER;
		return end_libftdi_read_frame_cb(user_data, false);
	}
	if(transfer_length < user_data->cb_data.requested_length)
		return end_libftdi_read_frame_cb(user_data, false);
	if(((int32_t)(user_data->index - (*user_data->last_used_index))) <= 0) {
		//*user_data->status = LIBUSB_ERROR_INTERRUPTED;
		return end_libftdi_read_frame_cb(user_data, false);
	}
	*user_data->last_used_index = user_data->index;

	// For some reason, saving the raw buffer and then passing it
	// like this is way faster than loading it.
	// Even if it's still needed to get the writer buffer...
	output_to_thread(user_data->capture_data, user_data->cb_data.internal_index, user_data->buffer_raw, user_data->buffer_target, *user_data->clock_start, transfer_length, user_data->curr_offset);
	end_libftdi_read_frame_cb(user_data, true);
}

static void resync_offset(FTD2CaptureReceivedData* received_data_buffers, uint32_t &index, size_t full_size) {
	size_t wanted_offset = *received_data_buffers[0].curr_offset;
	if(wanted_offset == 0)
		return;
	wait_all_libftdi_buffers_free(received_data_buffers);
	if(get_libftdi_status(received_data_buffers) != 0)
		return;
	wanted_offset = *received_data_buffers[0].curr_offset;
	if(wanted_offset == 0)
		return;
	*received_data_buffers[0].curr_offset = 0;
	#if defined(__APPLE__) || defined(_WIN32)
	// Literally throw a die... Seems to work!
	default_sleep(1);
	return;
	#endif
	FTD2OldDSCaptureReceivedRaw* buffer_raw = new FTD2OldDSCaptureReceivedRaw;
	FTD2OldDSCaptureReceived* buffer = new FTD2OldDSCaptureReceived;
	CaptureData* capture_data = received_data_buffers[0].capture_data;
	bool is_synced = false;
	size_t chosen_transfer_size = (MAX_PACKET_SIZE_USB2 - FTD2_INTRA_PACKET_HEADER_SIZE) * 4;
	while((!is_synced) && (capture_data->status.connected && capture_data->status.running)) {
		int retval = libftdi_full_read(received_data_buffers[0].cb_data.handle, (uint8_t*)buffer_raw, (uint8_t*)buffer, chosen_transfer_size, RESYNC_TIMEOUT);
		if(ftd2_is_error(retval, true)) {
			//error_libftdi_status(received_data_buffers, retval);
			delete buffer_raw;
			delete buffer;
			return;
		}
		size_t internal_sync_offset = 0;
		is_synced = synchronization_check((uint16_t*)buffer, chosen_transfer_size, NULL, &internal_sync_offset, true);
		if((!is_synced) && (internal_sync_offset < chosen_transfer_size))
			is_synced = true;
		else
			is_synced = false;
	}
	int retval = libftdi_full_read(received_data_buffers[0].cb_data.handle, (uint8_t*)buffer_raw, (uint8_t*)buffer, full_size - chosen_transfer_size, RESYNC_TIMEOUT);
	if(ftd2_is_error(retval, true)) {
		error_libftdi_status(received_data_buffers, retval);
		delete buffer_raw;
		delete buffer;
		return;
	}
	capture_data->status.cooldown_curr_in = IGNORE_FIRST_FEW_FRAMES_SYNC;
	delete buffer_raw;
	delete buffer;
}

void ftd2_capture_main_loop_libftdi(CaptureData* capture_data) {
	const bool is_libftdi = true;
	bool is_done = false;
	int inner_curr_in = 0;
	int retval = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	FTD2CaptureReceivedData* received_data_buffers = new FTD2CaptureReceivedData[NUM_CAPTURE_RECEIVED_DATA_BUFFERS];
	int curr_data_buffer = 0;
	int next_data_buffer = 0;
	int status = 0;
	uint32_t last_used_index = -1;
	uint32_t index = 0;
	size_t curr_offset = 0;
	const size_t full_size = get_capture_size(capture_data->status.device.is_rgb_888);
	size_t bytesIn;
	bool usb_thread_run = false;
	std::thread processing_thread;
	libftdi_start_thread(&processing_thread, &usb_thread_run, ((ftdi_context*)capture_data->handle)->usb_ctx);

	SharedConsumerMutex is_buffer_free_shared_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	SharedConsumerMutex is_transfer_done_mutex(NUM_CAPTURE_RECEIVED_DATA_BUFFERS);
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++) {
		received_data_buffers[i].actual_length = 0;
		received_data_buffers[i].is_data_ready = false;
		received_data_buffers[i].in_use = false;
		received_data_buffers[i].is_buffer_free_shared_mutex = &is_buffer_free_shared_mutex;
		received_data_buffers[i].status = &status;
		received_data_buffers[i].index = 0;
		received_data_buffers[i].curr_offset = &curr_offset;
		received_data_buffers[i].last_used_index = &last_used_index;
		received_data_buffers[i].capture_data = capture_data;
		received_data_buffers[i].clock_start = &clock_start;
		received_data_buffers[i].cb_data.function = libftdi_capture_process_data;
		received_data_buffers[i].cb_data.actual_user_data = &received_data_buffers[i];
		received_data_buffers[i].cb_data.transfer_data = NULL;
		received_data_buffers[i].cb_data.handle = capture_data->handle;
		received_data_buffers[i].cb_data.is_transfer_done_mutex = &is_transfer_done_mutex;
		received_data_buffers[i].cb_data.internal_index = i;
		received_data_buffers[i].cb_data.requested_length = 0;
	}

	if(!enable_capture(capture_data->handle, is_libftdi)) {
		capture_error_print(true, capture_data, "Capture enable error");
		is_done = true;
	}

	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		libftdi_start_read(libftdi_get_free_buffer(received_data_buffers), index++, full_size);


	while(capture_data->status.connected && capture_data->status.running) {
		if(get_libftdi_status(received_data_buffers) != 0) {
			capture_error_print(true, capture_data, "Disconnected: Read error");
			is_done = true;
		}
		if(is_done)
			break;
		libftdi_start_read(libftdi_get_free_buffer(received_data_buffers), index++, full_size);
		resync_offset(received_data_buffers, index, full_size);
	}
	wait_all_libftdi_buffers_free(received_data_buffers);
	libftdi_close_thread(&processing_thread, &usb_thread_run);
	delete []received_data_buffers;
}

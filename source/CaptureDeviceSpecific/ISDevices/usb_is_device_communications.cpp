#include "frontend.hpp"
#include "usb_is_device_communications.hpp"
#include "usb_is_device_libusb.hpp"
#include "usb_is_device_is_driver.hpp"
#include "is_twl_cap_init_seed_table.h"
#include "is_twl_cap_crc32_table.h"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

// Code based off of Gericom's sample code. Distributed under the MIT License. Copyright (c) 2024 Gericom
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#define IS_TWL_SERIAL_NUMBER_SIZE 8

#define IS_NITRO_USB_PACKET_LIMIT 0x2000
#define IS_TWL_USB_PACKET_LIMIT 0x200000

#define REG_USB_DMA_CONTROL_2 0x0C000028
#define REG_USB_BIU_CONTROL_2 0x0C0000A4

#define DEFAULT_MAX_LENGTH_IS_TWL_VIDEO (5 * sizeof(ISTWLCaptureVideoInternalReceived))
#define DEFAULT_MAX_LENGTH_IS_TWL_AUDIO (5 * 5 * sizeof(ISTWLCaptureAudioReceived))

enum is_nitro_packet_emulator_dir {
	IS_NITRO_PACKET_EMU_DIR_WRITE = 0x10,
	IS_NITRO_PACKET_EMU_DIR_READ  = 0x11
};

enum is_nitro_packet_capture_dir {
	IS_NITRO_PACKET_CAP_DIR_WRITE = 0x00,
	IS_NITRO_PACKET_CAP_DIR_READ  = 0x01
};

enum is_twl_packet_capture_dir {
	IS_TWL_PACKET_CAP_DIR_WRITE = 0x00,
	IS_TWL_PACKET_CAP_DIR_READ  = 0x01,
	IS_TWL_PACKET_CAP_DIR_WRITE_READ  = 0x02
};

enum is_device_packet_type {
	IS_NITRO_PACKET_TYPE_COMMAND = 0,
	IS_NITRO_PACKET_TYPE_EMU_MEMORY = 1,
	IS_NITRO_EMULATOR_PACKET_TYPE_AGB_SRAM = 2,
	IS_NITRO_CAPTURE_PACKET_TYPE_DMA_CONTROL = 2,
	IS_NITRO_PACKET_TYPE_CAPTURE = 3,
	IS_NITRO_PACKET_TYPE_AGB_CART_ROM = 4,
	IS_NITRO_PACKET_TYPE_AGB_BUS2 = 5,
	IS_TWL_PACKET_TYPE_FRAME = 7,
};

enum is_nitro_emulator_command {
	IS_NITRO_EMU_CMD_GET_SERIAL = 0x13,
	IS_NITRO_EMU_CMD_READ_NEC_MEM = 0x17,
	IS_NITRO_EMU_CMD_WRITE_NEC_MEM = 0x24,
	IS_NITRO_EMU_CMD_SET_READ_NEC_MEM = 0x25,
	IS_NITRO_EMU_CMD_FULL_HARDWARE_RESET = 0x81,
	IS_NITRO_EMU_CMD_CPU_RESET = 0x8A,
	IS_NITRO_EMU_CMD_AD = 0xAD,
};

enum is_nitro_capture_command {
	IS_NITRO_CAP_CMD_ENABLE_CAP = 0x00,
	IS_NITRO_CAP_CMD_GET_SERIAL = 0x03,
	IS_NITRO_CAP_CMD_SET_FWD_RATE = 0x12,
	IS_NITRO_CAP_CMD_SET_FWD_MODE = 0x13,
	IS_NITRO_CAP_CMD_SET_FWD_COLOURS = 0x14,
	IS_NITRO_CAP_CMD_SET_FWD_FRAMES = 0x15,
	IS_NITRO_CAP_CMD_GET_DEBUG_STATE = 0x18,
	IS_NITRO_CAP_CMD_SET_RESTART_FULL = 0x18,
	IS_NITRO_CAP_CMD_GET_LID_STATE = 0x19,
	IS_NITRO_CAP_CMD_SET_FWD_RESTART = 0x1C,
	IS_NITRO_CAP_CMD_SET_RESET_CPU_ON = 0x21,
	IS_NITRO_CAP_CMD_SET_RESET_CPU_OFF = 0x22,
};

enum is_twl_capture_command {
	IS_TWL_CAP_CMD_GET_CAP = 0x00,
	IS_TWL_CAP_CMD_GET_ENC_DEC_SEEDS = 0x04,
	IS_TWL_CAP_CMD_GET_SERIAL = 0x13,
	IS_TWL_CAP_CMD_UNLOCK_COMMS = 0x1C,
	IS_TWL_CAP_CMD_GENERIC_INFO = 0x80,
	IS_TWL_CAP_CMD_MNTR = 0x81,
	IS_TWL_CAP_CMD_POWER_ON_OFF = 0x82,
	IS_TWL_CAP_CMD_ASK_CAPTURE_INFORMATION = 0x83,
	IS_TWL_CAP_CMD_SCTG = 0x84,
	IS_TWL_CAP_CMD_TCZC = 0x85,
	IS_TWL_CAP_CMD_STOP_FRAME_READS = 0x88,
	IS_TWL_CAP_CMD_SET_LAST_FRAME_INFORMATION = 0x8B,
	IS_TWL_CAP_CMD_ACTS = 0x8C,
	IS_TWL_CAP_CMD_EPAC = 0x92,
	IS_TWL_CAP_CMD_POST_EPAC = 0x93,
};

enum is_nitro_emulator_forward_bits {
	IS_NITRO_EMULATOR_FORWARD_ENABLE_BIT = 0,
	IS_NITRO_EMULATOR_FORWARD_COUNTER_RESTART_BIT = 1,
};

enum is_nitro_capture_forward_bits {
	IS_NITRO_CAPTURE_FORWARD_COUNTER_RESTART_BIT = 0,
};

#pragma pack(push, 1)
struct PACKED is_nitro_nec_packet_header {
	uint8_t command;
	uint8_t unit_size;
	uint16_t count;
	uint32_t address;
};

struct PACKED is_device_packet_header {
	uint16_t command;
	uint8_t direction;
	uint8_t type;
	uint32_t address;
	uint32_t length;
	uint32_t padding;
};

struct PACKED is_device_rw_packet_header {
	uint16_t command;
	uint8_t direction;
	uint8_t type;
	uint32_t address;
	uint32_t length_w;
	uint32_t length_r;
};

struct PACKED is_twl_ask_capture_information_packet {
	uint32_t unk[2];
	uint32_t max_length_video;
	uint32_t unk2[2];
	uint32_t max_length_audio;
	uint32_t unk3[0x10];
};
// These two are probably the same struct, with different fields used for
// different packets...
// I say probably, that's why I keep them separate.
struct PACKED is_twl_capture_information_packet {
	uint32_t available_video_length;
	uint32_t video_start_address;
	uint32_t unk;
	uint32_t available_audio_length;
	uint32_t audio_start_address;
	uint32_t unk2;
	uint32_t unk3[0x10];
};
#pragma pack(pop)

static const is_device_usb_device usb_is_nitro_emu_rare_desc = {
.name = "ISNEr", .long_name = "IS Nitro Emulator(R)",
.vid = 0x0f6e, .pid = 0x0400,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep_in = 2 | LIBUSB_ENDPOINT_IN, .ep_out = 1 | LIBUSB_ENDPOINT_OUT,
.write_pipe = "Pipe00", .read_pipe = "Pipe01",
.product_id = 2, .manufacturer_id = 1, .device_type = IS_NITRO_EMULATOR_DEVICE,
.video_data_type = VIDEO_DATA_BGR, .max_usb_packet_size = IS_NITRO_USB_PACKET_LIMIT,
.do_pipe_clear_reset = true, .audio_enabled = false, .max_audio_samples_size = 0
};

static const is_device_usb_device usb_is_nitro_emu_common_desc = {
.name = "ISNE", .long_name = "IS Nitro Emulator",
.vid = 0x0f6e, .pid = 0x0404,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep_in = 2 | LIBUSB_ENDPOINT_IN, .ep_out = 1 | LIBUSB_ENDPOINT_OUT,
.write_pipe = "Pipe00", .read_pipe = "Pipe01",
.product_id = 2, .manufacturer_id = 1, .device_type = IS_NITRO_EMULATOR_DEVICE,
.video_data_type = VIDEO_DATA_BGR, .max_usb_packet_size = IS_NITRO_USB_PACKET_LIMIT,
.do_pipe_clear_reset = true, .audio_enabled = false, .max_audio_samples_size = 0
};

static const is_device_usb_device usb_is_nitro_cap_desc = {
.name = "ISNC", .long_name = "IS Nitro Capture",
.vid = 0x0f6e, .pid = 0x0403,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep_in = 2 | LIBUSB_ENDPOINT_IN, .ep_out = 1 | LIBUSB_ENDPOINT_OUT,
.write_pipe = "Pipe00", .read_pipe = "Pipe01",
.product_id = 2, .manufacturer_id = 1, .device_type = IS_NITRO_CAPTURE_DEVICE,
.video_data_type = VIDEO_DATA_BGR, .max_usb_packet_size = IS_NITRO_USB_PACKET_LIMIT,
.do_pipe_clear_reset = true, .audio_enabled = false, .max_audio_samples_size = 0
};

static const is_device_usb_device usb_is_twl_cap_desc = {
.name = "ISTCD", .long_name = "IS TWL Capture (Dev)",
.vid = 0x0f6e, .pid = 0x0501,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep_in = 6 | LIBUSB_ENDPOINT_IN, .ep_out = 2 | LIBUSB_ENDPOINT_OUT,
.write_pipe = "Pipe01", .read_pipe = "Pipe03",
.product_id = 2, .manufacturer_id = 1, .device_type = IS_TWL_CAPTURE_DEVICE,
.video_data_type = VIDEO_DATA_RGB, .max_usb_packet_size = IS_TWL_USB_PACKET_LIMIT,
.do_pipe_clear_reset = false, .audio_enabled = true, .max_audio_samples_size = sizeof(ISTWLCaptureSoundData) * TWL_CAPTURE_MAX_SAMPLES_CHUNK_NUM
};

static const is_device_usb_device usb_is_twl_cap_desc_2 = {
.name = "ISTCR", .long_name = "IS TWL Capture (Ret)",
.vid = 0x0f6e, .pid = 0x0502,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep_in = 6 | LIBUSB_ENDPOINT_IN, .ep_out = 2 | LIBUSB_ENDPOINT_OUT,
.write_pipe = "Pipe01", .read_pipe = "Pipe03",
.product_id = 2, .manufacturer_id = 1, .device_type = IS_TWL_CAPTURE_DEVICE,
.video_data_type = VIDEO_DATA_RGB, .max_usb_packet_size = IS_TWL_USB_PACKET_LIMIT,
.do_pipe_clear_reset = false, .audio_enabled = true, .max_audio_samples_size = sizeof(ISTWLCaptureSoundData) * TWL_CAPTURE_MAX_SAMPLES_CHUNK_NUM
};

static const is_device_usb_device* all_usb_is_device_devices_desc[] = {
	&usb_is_nitro_emu_rare_desc,
	&usb_is_nitro_emu_common_desc,
	&usb_is_nitro_cap_desc,
	&usb_is_twl_cap_desc,
	&usb_is_twl_cap_desc_2,
};

int GetNumISDeviceDesc() {
	return sizeof(all_usb_is_device_devices_desc) / sizeof(all_usb_is_device_devices_desc[0]);
}

const is_device_usb_device* GetISDeviceDesc(int index) {
	if((index < 0) || (index >= GetNumISDeviceDesc()))
		index = 0;
	return all_usb_is_device_devices_desc[index];
}

static void fix_endianness_header(is_device_packet_header* header) {
	header->command = to_le(header->command);
	header->address = to_le(header->address);
	header->length = to_le(header->length);
}

static void fix_endianness_header(is_device_rw_packet_header* header) {
	header->command = to_le(header->command);
	header->address = to_le(header->address);
	header->length_w = to_le(header->length_w);
	header->length_r = to_le(header->length_r);
}

static void fix_endianness_header(is_nitro_nec_packet_header* header) {
	header->count = to_le(header->count);
	header->address = to_le(header->address);
}

static uint32_t get_crc32_data_comm(uint8_t* data, size_t size) {
	uint32_t value = 0xFFFFFFFF;
	for(int i = 0; i < size; i++) {
		uint8_t inner_value = (value >> 24) ^ data[i];
		value <<= 8;
		value ^= read_le32(is_twl_cap_crc32_table, inner_value);
	}
	return ~value;
}

static void apply_enc_dec_action_value(uint32_t action_value, uint32_t rotating_value, uint8_t* data, size_t size_u16) {
	int increment = 0;
	bool is_rot_odd = (rotating_value & 1) == 1;
	uint16_t first_val = 0;
	uint16_t last_val = 0;
	if((action_value & 0xF000) == 0x1000) {
		if(is_rot_odd)
			increment = 0xFFFF6494;
		increment += 0xA597;
	}
	else if((action_value & 0xF000) == 0x2000) {
		if(is_rot_odd)
			increment = 0x9B6C;
		increment += 0x5A69;
	}
	switch((action_value & 0xFF) - 1) {
		case 0:
			for(int j = 0; j < size_u16; j++)
				write_be16(data, read_le16(data, j), j);
			break;
		case 1:
			for(int j = 0; j < size_u16; j++) {
				write_le16(data, read_le16(data, j) ^ rotating_value, j);
				rotating_value += increment;
			}
			break;
		case 2:
			for(int j = 0; j < size_u16; j++) {
				write_le16(data, read_le16(data, j) + rotating_value, j);
				rotating_value += increment;
			}
			break;
		case 3:
			for(int j = 0; j < size_u16; j++) {
				write_le16(data, read_le16(data, j) - rotating_value, j);
				rotating_value += increment;
			}
			break;
		case 4:
			for(int j = 0; j < (size_u16 / 2); j++) {
				first_val = read_le16(data, j);
				write_le16(data, read_le16(data, size_u16 - 1 - j), j);
				write_le16(data, first_val, size_u16 - 1 - j);
			}
			break;
		case 5:
			first_val = read_le16(data);
			for(int j = 0; j < (size_u16 - 1); j++)
				write_le16(data, read_le16(data, j + 1), j);
			write_le16(data, first_val, size_u16 - 1);
			break;
		case 6:
			last_val = read_le16(data, size_u16 - 1);
			for(int j = 0; j < (size_u16 - 1); j++)
				write_le16(data, read_le16(data, size_u16 - 1 - j - 1), size_u16 - 1 - j);
			write_le16(data, last_val);
			break;
		default:
			break;
	}
}

static void enc_dec_table_add_usage(is_device_twl_enc_dec_table* table) {
	table->rotating_value = rotate_bits_right(table->rotating_value);
	table->num_iters_before_refresh -= 1;
	if(table->num_iters_before_refresh > 0)
		return;
	table->num_iters_before_refresh = table->num_iters_full;
	uint16_t base_table_value = table->action_values[0];
	for(int i = 0; i < table->num_values - 1; i++)
		table->action_values[i] = table->action_values[i + 1];
	table->action_values[table->num_values - 1] = base_table_value;
}

static void table_print(uint8_t* data, size_t size) {
	/*
	for(size_t i = 0; i < size; i++) {
		printf("%02x ", data[i]);
		if((i % 0x10) == 0xF)
			printf("\n");
	}
	printf("\n");
	*/
}

static void enc_dec_table_apply_to_data(is_device_twl_enc_dec_table* table, uint8_t* data, size_t size, bool is_enc) {
	table_print(data, size);
	for(int i = 0; i < table->num_values; i++) {
		int action_value = table->action_values[i];
		if(!is_enc)
			action_value = table->action_values[table->num_values - 1 - i];
		apply_enc_dec_action_value(action_value, table->rotating_value, data, size / 2);
		table_print(data, size);
	}
	enc_dec_table_add_usage(table);
}

// Write to bulk endpoint.  Returns libusb error code
static int bulk_out(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t *buf, int length, int *transferred) {
	if(handlers->usb_handle)
		return is_device_libusb_bulk_out(handlers, usb_device_desc, buf, length, transferred);
	return is_driver_bulk_out(handlers, usb_device_desc, buf, length, transferred);
}

// Read from bulk endpoint.  Returns libusb error code
static int bulk_in(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t *buf, int length, int *transferred) {
	if(handlers->usb_handle)
		return is_device_libusb_bulk_in(handlers, usb_device_desc, buf, length, transferred);
	return is_driver_bulk_in(handlers, usb_device_desc, buf, length, transferred);
}

// Read from ctrl endpoint.  Returns libusb error code
static int ctrl_in(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t *buf, int length, uint8_t request, uint16_t index, int *transferred) {
	if(handlers->usb_handle)
		return is_device_libusb_ctrl_in(handlers, usb_device_desc, buf, length, request, index, transferred);
	return is_driver_ctrl_in(handlers, buf, length, request, index, transferred);
}

// Read from bulk endpoint.  Returns libusb error code
static void bulk_in_async(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t *buf, int length, isd_async_callback_data* cb_data) {
	if(handlers->usb_handle)
		return is_device_libusb_async_in_start(handlers, usb_device_desc, buf, length, cb_data);
	is_drive_async_in_start(handlers, usb_device_desc, buf, length, cb_data);
}

// Prepare ctrl_in pipe, if needed...
static bool prepare_ctrl_in(is_device_device_handlers* handlers) {
	if(handlers->usb_handle)
		return true;
	return is_driver_prepare_ctrl_in_handle(handlers);
}

// Close ctrl_in pipe, if needed...
static void close_ctrl_in(is_device_device_handlers* handlers) {
	if(handlers->usb_handle)
		return;
	is_driver_close_ctrl_in_handle(handlers);
}

static int return_and_delete(uint8_t* ptr, int value) {
	delete []ptr;
	return value;
}

static uint8_t get_packet_direction(is_device_type device_type, size_t transfer_size_r, size_t transfer_size_w) {
	if((transfer_size_r != 0) && (transfer_size_w != 0)) {
		switch(device_type) {
			case IS_TWL_CAPTURE_DEVICE:
				return IS_TWL_PACKET_CAP_DIR_WRITE_READ;
			default:
				return 0;
		}
	}
	else if(transfer_size_r != 0) {
		switch(device_type) {
			case IS_NITRO_EMULATOR_DEVICE:
				return IS_NITRO_PACKET_EMU_DIR_READ;
			case IS_NITRO_CAPTURE_DEVICE:
				return IS_NITRO_PACKET_CAP_DIR_READ;
			case IS_TWL_CAPTURE_DEVICE:
				return IS_TWL_PACKET_CAP_DIR_READ;
			default:
				return 0;
		}
	}
	else {
		switch(device_type) {
			case IS_NITRO_EMULATOR_DEVICE:
				return IS_NITRO_PACKET_EMU_DIR_WRITE;
			case IS_NITRO_CAPTURE_DEVICE:
				return IS_NITRO_PACKET_CAP_DIR_WRITE;
			case IS_TWL_CAPTURE_DEVICE:
				return IS_TWL_PACKET_CAP_DIR_WRITE;
			default:
				return 0;
		}
	}
}

static uint8_t is_packet_direction_read(is_device_type device_type, uint8_t packet_direction) {
	switch(device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return packet_direction == IS_NITRO_PACKET_EMU_DIR_READ;
		case IS_NITRO_CAPTURE_DEVICE:
			return packet_direction == IS_NITRO_PACKET_CAP_DIR_READ;
		case IS_TWL_CAPTURE_DEVICE:
			return (packet_direction == IS_TWL_PACKET_CAP_DIR_WRITE_READ) || (packet_direction == IS_TWL_PACKET_CAP_DIR_READ);
		default:
			return false;
	}
}

static uint8_t is_packet_direction_write(is_device_type device_type, uint8_t packet_direction) {
	switch(device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return packet_direction == IS_NITRO_PACKET_EMU_DIR_WRITE;
		case IS_NITRO_CAPTURE_DEVICE:
			return packet_direction == IS_NITRO_PACKET_CAP_DIR_WRITE;
		case IS_TWL_CAPTURE_DEVICE:
			return (packet_direction == IS_TWL_PACKET_CAP_DIR_WRITE_READ) || (packet_direction == IS_TWL_PACKET_CAP_DIR_WRITE);
		default:
			return false;
	}
}

static int SendWritePacket(is_device_device_handlers* handlers, uint16_t command, is_device_packet_type type, uint32_t address, uint8_t* buf, int length, const is_device_usb_device* device_desc, bool expect_result = true) {
	is_device_packet_header header;
	bool append_mode = true;
	if(device_desc->device_type == IS_NITRO_CAPTURE_DEVICE)
		append_mode = false;
	int single_packet_covered_size = device_desc->max_usb_packet_size - sizeof(header);
	if(!append_mode)
		single_packet_covered_size = device_desc->max_usb_packet_size;
	int num_iters = (length + single_packet_covered_size - 1) / single_packet_covered_size;
	if(!num_iters)
		num_iters = 1;
	size_t max_size_packet = length;
	if(length > single_packet_covered_size)
		max_size_packet = single_packet_covered_size;
	uint8_t* single_usb_packet = new uint8_t[max_size_packet + sizeof(header)];
	for(int i = 0; i < num_iters; i++) {
		int transfer_size = length - (i * single_packet_covered_size);
		if(transfer_size > single_packet_covered_size)
			transfer_size = single_packet_covered_size;

		uint8_t packet_direction = get_packet_direction(device_desc->device_type, 0, transfer_size);
		header.command = command;
		header.direction = packet_direction;
		header.type = type;
		header.address = address;
		header.length = transfer_size;
		header.padding = 0;
		fix_endianness_header(&header);
		int ret = 0;
		int num_bytes = 0;
		for(int j = 0; j < sizeof(is_device_packet_header); j++)
			single_usb_packet[j] = ((uint8_t*)&header)[j];
		if(append_mode && (buf != NULL)) {
			for(int j = 0; j < transfer_size; j++)
				single_usb_packet[sizeof(is_device_packet_header) + j] = buf[(i * single_packet_covered_size) + j];
			ret = bulk_out(handlers, device_desc, single_usb_packet, transfer_size + sizeof(is_device_packet_header), &num_bytes);
		}
		else {
			int header_bytes = 0;
			ret = bulk_out(handlers, device_desc, single_usb_packet, sizeof(is_device_packet_header), &header_bytes);
			if(ret < 0)
				return return_and_delete(single_usb_packet, ret);
			if(header_bytes != sizeof(is_device_packet_header))
				return return_and_delete(single_usb_packet, LIBUSB_ERROR_INTERRUPTED);
			if((buf != NULL) && (transfer_size > 0))
				ret = bulk_out(handlers, device_desc, &buf[(i * single_packet_covered_size)], transfer_size, &num_bytes);
			num_bytes += header_bytes;
		}
		if(ret < 0)
			return return_and_delete(single_usb_packet, ret);
		if(num_bytes != (transfer_size + sizeof(is_device_packet_header)))
			return return_and_delete(single_usb_packet, LIBUSB_ERROR_INTERRUPTED);
	}
	if((device_desc->device_type == IS_NITRO_CAPTURE_DEVICE) && expect_result) {
		uint8_t status[16];
		int status_bytes = 0;
		int ret = bulk_in(handlers, device_desc, status, sizeof(status), &status_bytes);
		if(ret < 0)
			return return_and_delete(single_usb_packet, ret);
		if(status_bytes != sizeof(status))
			return return_and_delete(single_usb_packet, LIBUSB_ERROR_INTERRUPTED);
		if(status[0] != 1)
			return return_and_delete(single_usb_packet, LIBUSB_ERROR_OTHER);
	}
	return return_and_delete(single_usb_packet, LIBUSB_SUCCESS);
}

static int SendReadPacket(is_device_device_handlers* handlers, uint16_t command, is_device_packet_type type, uint32_t address, uint8_t* buf, int length, const is_device_usb_device* device_desc, bool expect_result = true) {
	is_device_packet_header header;
	int single_packet_covered_size = device_desc->max_usb_packet_size;
	int num_iters = (length + single_packet_covered_size - 1) / single_packet_covered_size;
	if(num_iters == 0)
		num_iters = 1;
	for(int i = 0; i < num_iters; i++) {
		int transfer_size = length - (i * single_packet_covered_size);
		if(transfer_size > single_packet_covered_size)
			transfer_size = single_packet_covered_size;

		uint8_t packet_direction = get_packet_direction(device_desc->device_type, transfer_size, 0);
		header.command = command;
		header.direction = packet_direction;
		header.type = type;
		header.address = address;
		header.length = transfer_size;
		header.padding = 0;
		fix_endianness_header(&header);
		int num_bytes = 0;
		int ret = bulk_out(handlers, device_desc, (uint8_t*)&header, sizeof(is_device_packet_header), &num_bytes);
		if(ret < 0)
			return ret;
		if(num_bytes != sizeof(is_device_packet_header))
			return LIBUSB_ERROR_INTERRUPTED;
		if(buf != NULL) {
			ret = bulk_in(handlers, device_desc, buf + (i * single_packet_covered_size), transfer_size, &num_bytes);
			if(ret < 0)
				return ret;
			if(num_bytes != transfer_size)
				return LIBUSB_ERROR_INTERRUPTED;
		}
	}
	if((device_desc->device_type == IS_NITRO_CAPTURE_DEVICE) && expect_result) {
		uint8_t status[16];
		int status_bytes = 0;
		int ret = bulk_in(handlers, device_desc, status, sizeof(status), &status_bytes);
		if(ret < 0)
			return ret;
		if(status_bytes != sizeof(status))
			return LIBUSB_ERROR_INTERRUPTED;
		if(status[0] != 1)
			return LIBUSB_ERROR_OTHER;
	}
	return LIBUSB_SUCCESS;
}

static int SendReadWritePacket(is_device_device_handlers* handlers, uint16_t command, is_device_packet_type type, uint32_t address, uint8_t* buf_w, int length_w, uint8_t* buf_r, int length_r, const is_device_usb_device* device_desc) {
	if((device_desc->device_type == IS_NITRO_EMULATOR_DEVICE) || (device_desc->device_type == IS_NITRO_CAPTURE_DEVICE))
		return LIBUSB_SUCCESS;
	is_device_rw_packet_header header;
	int single_packet_covered_size_w = device_desc->max_usb_packet_size - sizeof(header);
	int num_iters_w = (length_w + single_packet_covered_size_w - 1) / single_packet_covered_size_w;
	if(!num_iters_w)
		num_iters_w = 1;
	int single_packet_covered_size_r = device_desc->max_usb_packet_size;
	int num_iters_r = (length_r + single_packet_covered_size_r - 1) / single_packet_covered_size_r;
	if(!num_iters_r)
		num_iters_r = 1;
	int num_iters = num_iters_w;
	if(num_iters_r > num_iters)
		num_iters = num_iters_r;
	size_t max_size_packet_w = length_w;
	if(length_w > single_packet_covered_size_w)
		max_size_packet_w = single_packet_covered_size_w;
	uint8_t* single_usb_packet = new uint8_t[max_size_packet_w + sizeof(header)];
	for(int i = 0; i < num_iters; i++) {
		int transfer_size_w = length_w - (i * single_packet_covered_size_w);
		if(transfer_size_w > single_packet_covered_size_w)
			transfer_size_w = single_packet_covered_size_w;
		int transfer_size_r = length_r - (i * single_packet_covered_size_r);
		if(transfer_size_r > single_packet_covered_size_r)
			transfer_size_r = single_packet_covered_size_r;

		uint8_t packet_direction = get_packet_direction(device_desc->device_type, transfer_size_r, transfer_size_w);
		header.command = command;
		header.direction = packet_direction;
		header.type = type;
		header.address = address;
		header.length_w = transfer_size_w;
		header.length_r = transfer_size_r;
		fix_endianness_header(&header);
		int ret = 0;
		int num_bytes = 0;
		if(is_packet_direction_write(device_desc->device_type, packet_direction)) {
			for(int j = 0; j < sizeof(is_device_rw_packet_header); j++)
				single_usb_packet[j] = ((uint8_t*)&header)[j];
			if(buf_w != NULL) {
				for(int j = 0; j < transfer_size_w; j++)
					single_usb_packet[sizeof(is_device_rw_packet_header) + j] = buf_w[(i * single_packet_covered_size_w) + j];
				ret = bulk_out(handlers, device_desc, single_usb_packet, transfer_size_w + sizeof(is_device_rw_packet_header), &num_bytes);
			}
			else {
				ret = bulk_out(handlers, device_desc, single_usb_packet, sizeof(is_device_rw_packet_header), &num_bytes);
				transfer_size_w = 0;
			}
			if(ret < 0)
				return return_and_delete(single_usb_packet, ret);
			if(num_bytes != (transfer_size_w + sizeof(is_device_rw_packet_header)))
				return return_and_delete(single_usb_packet, LIBUSB_ERROR_INTERRUPTED);
		}
		else {
			header.length_r = 0;
			header.length_w = transfer_size_r;
			ret = bulk_out(handlers, device_desc, (uint8_t*)&header, sizeof(is_device_rw_packet_header), &num_bytes);
			if(ret < 0)
				return return_and_delete(single_usb_packet, ret);
			if(num_bytes != sizeof(is_device_packet_header))
				return return_and_delete(single_usb_packet, LIBUSB_ERROR_INTERRUPTED);
		}
		num_bytes = 0;
		if(is_packet_direction_read(device_desc->device_type, packet_direction)) {
			if(buf_r != NULL) {
				ret = bulk_in(handlers, device_desc, buf_r + (i * single_packet_covered_size_r), transfer_size_r, &num_bytes);
				if(ret < 0)
					return return_and_delete(single_usb_packet, ret);
				if(num_bytes != transfer_size_r)
					return return_and_delete(single_usb_packet, LIBUSB_ERROR_INTERRUPTED);
			}
		}
	}
	return return_and_delete(single_usb_packet, LIBUSB_SUCCESS);
}

int SendReadCommand(is_device_device_handlers* handlers, uint16_t command, uint8_t* buf, int length, const is_device_usb_device* device_desc) {
	return SendReadPacket(handlers, command, IS_NITRO_PACKET_TYPE_COMMAND, 0, buf, length, device_desc);
}

int SendWriteCommand(is_device_device_handlers* handlers, uint16_t command, uint8_t* buf, int length, const is_device_usb_device* device_desc) {
	return SendWritePacket(handlers, command, IS_NITRO_PACKET_TYPE_COMMAND, 0, buf, length, device_desc);
}

int SendReadWriteCommand(is_device_device_handlers* handlers, uint16_t command, uint8_t* out_buf, int out_length, uint8_t* in_buf, int in_length, const is_device_usb_device* device_desc) {
	return SendReadWritePacket(handlers, command, IS_NITRO_PACKET_TYPE_COMMAND, 0, out_buf, out_length, in_buf, in_length, device_desc);
}

int SendReadCommandU32(is_device_device_handlers* handlers, uint16_t command, uint32_t* out, const is_device_usb_device* device_desc) {
	uint32_t buffer;
	int ret = SendReadCommand(handlers, command, (uint8_t*)&buffer, sizeof(uint32_t), device_desc);
	if(ret < 0)
		return ret;
	*out = from_le(buffer);
	return 0;
}

int SendWriteCommandU32(is_device_device_handlers* handlers, uint16_t command, uint32_t value, const is_device_usb_device* device_desc) {
	uint32_t buffer = to_le(value);
	return SendWriteCommand(handlers, command, (uint8_t*)&buffer, sizeof(uint32_t), device_desc);
}

static int AuthCtrlIn(is_device_device_handlers* handlers, const is_device_usb_device* device_desc) {
	int ret = 0;
	bool b_ret = true;
	int num_bytes = 0;
	uint8_t buffer[2];
	uint16_t auth_val = 0;
	switch(device_desc->device_type) {
		case IS_TWL_CAPTURE_DEVICE:
			b_ret = prepare_ctrl_in(handlers);
			if(!b_ret)
				return LIBUSB_ERROR_INTERRUPTED;
			ret = ctrl_in(handlers, device_desc, buffer, 2, 0xDF, 0, &num_bytes);
			if(ret < 0) {
				close_ctrl_in(handlers);
				return ret;
			}
			if(num_bytes < 2) {
				close_ctrl_in(handlers);
				return LIBUSB_ERROR_INTERRUPTED;
			}
			auth_val = read_le16(buffer) - 0x644C;
			ret = ctrl_in(handlers, device_desc, buffer, 1, 0xE3, ((auth_val >> 3) & 0x1C92) ^ auth_val, &num_bytes);
			close_ctrl_in(handlers);
			if(ret < 0)
				return ret;
			if((num_bytes < 1) || (buffer[0] != 0))
				return LIBUSB_ERROR_INTERRUPTED;
			return LIBUSB_SUCCESS;
		default:
			return 0;
	}
}

int GetDeviceSerial(is_device_device_handlers* handlers, uint8_t* buf, const is_device_usb_device* device_desc) {
	int ret = 0;
	uint32_t value = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return SendReadCommand(handlers, IS_NITRO_EMU_CMD_GET_SERIAL, buf, IS_DEVICE_REAL_SERIAL_NUMBER_SIZE, device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			return SendReadCommand(handlers, IS_NITRO_CAP_CMD_GET_SERIAL, buf, IS_DEVICE_REAL_SERIAL_NUMBER_SIZE, device_desc);
		case IS_TWL_CAPTURE_DEVICE:
			ret = AuthCtrlIn(handlers, device_desc);
			if(ret < 0)
				return ret;
			ret = SendReadCommandU32(handlers, IS_TWL_CAP_CMD_UNLOCK_COMMS, &value, device_desc);
			if(ret < 0)
				return ret;
			ret = SendReadCommand(handlers, IS_TWL_CAP_CMD_GET_SERIAL, buf, IS_TWL_SERIAL_NUMBER_SIZE, device_desc);
			buf[IS_TWL_SERIAL_NUMBER_SIZE] = '\0';
			return ret;
		default:
			return 0;
	}
}

int ReadNecMem(is_device_device_handlers* handlers, uint32_t address, uint8_t unit_size, uint8_t* buf, int count, const is_device_usb_device* device_desc) {
	is_nitro_nec_packet_header header;
	header.command = IS_NITRO_EMU_CMD_SET_READ_NEC_MEM;
	header.unit_size = unit_size;
	header.count = count;
	header.address = address;
	fix_endianness_header(&header);
	int ret = SendWriteCommand(handlers, header.command, (uint8_t*)&header, sizeof(is_nitro_nec_packet_header), device_desc);
	if(ret < 0)
		return ret;
	return SendReadCommand(handlers, IS_NITRO_EMU_CMD_READ_NEC_MEM, buf, count * unit_size, device_desc);
}

int ReadNecMemU16(is_device_device_handlers* handlers, uint32_t address, uint16_t* out, const is_device_usb_device* device_desc) {
	uint16_t buffer;
	int ret = ReadNecMem(handlers, address, 2, (uint8_t*)&buffer, 1, device_desc);
	if(ret < 0)
		return ret;
	*out = from_le(buffer);
	return 0;
}

int ReadNecMemU32(is_device_device_handlers* handlers, uint32_t address, uint32_t* out, const is_device_usb_device* device_desc) {
	uint32_t buffer;
	int ret = ReadNecMem(handlers, address, 2, (uint8_t*)&buffer, 2, device_desc);
	if(ret < 0)
		return ret;
	*out = from_le(buffer);
	return 0;
}

int WriteNecMem(is_device_device_handlers* handlers, uint32_t address, uint8_t unit_size, uint8_t* buf, int count, const is_device_usb_device* device_desc) {
	uint8_t* buffer = new uint8_t[(count * unit_size) + sizeof(is_nitro_nec_packet_header)];
	is_nitro_nec_packet_header header;
	header.command = IS_NITRO_EMU_CMD_WRITE_NEC_MEM;
	header.unit_size = unit_size;
	header.count = count;
	header.address = address;
	fix_endianness_header(&header);
	for(int i = 0; i < sizeof(is_nitro_nec_packet_header); i++)
		buffer[i] = ((uint8_t*)&header)[i];
	for(int i = 0; i < count * unit_size; i++)
		buffer[i + sizeof(is_nitro_nec_packet_header)] = buf[i];
	int ret = SendWriteCommand(handlers, header.command, buffer, (count * unit_size) + sizeof(is_nitro_nec_packet_header), device_desc);
	delete []buffer;
	return ret;
}

int WriteNecMemU16(is_device_device_handlers* handlers, uint32_t address, uint16_t value, const is_device_usb_device* device_desc) {
	uint16_t buffer = to_le(value);
	return WriteNecMem(handlers, address, 2, (uint8_t*)&buffer, 1, device_desc);
}

int WriteNecMemU32(is_device_device_handlers* handlers, uint32_t address, uint32_t value, const is_device_usb_device* device_desc) {
	uint32_t buffer = to_le(value);
	return WriteNecMem(handlers, address, 2, (uint8_t*)&buffer, 2, device_desc);
}

int DisableLca2(is_device_device_handlers* handlers, const is_device_usb_device* device_desc) {
	if(device_desc->device_type != IS_NITRO_EMULATOR_DEVICE)
		return LIBUSB_SUCCESS;
	int ret = WriteNecMemU16(handlers, 0x00805180, 0, device_desc);
	if(ret < 0)
		return ret;
	ret = WriteNecMemU16(handlers, 0x0F84000A, 1, device_desc);
	if(ret < 0)
		return ret;
	//SleepBetweenTransfers(handlers, 2000);
	return WriteNecMemU16(handlers, 0x0F84000A, 0, device_desc);
}

static bool is_string_present_and_same(const std::string text, uint8_t* buffer, size_t text_pos = 0) {
	if(text == "")
		return true;
	// Strings are pain.
	// For some reason, under MSVC comparing strings doesn't work,
	// but comparing the .c_str() representation does...
	// Too bad that same method doesn't work when using GCC... :/
	// Solution? Write the string to a buffer, and compare the two...
	uint8_t* str_check = new uint8_t[text.size()];
	write_string(str_check, text);
	bool is_different = false;
	for(size_t i = 0; i < text.size(); i++) {
		if(str_check[i] != buffer[i + text_pos]) {
			is_different = true;
			break;
		}
	}
	delete []str_check;
	return !is_different;
}

static int data_dec_and_check(is_device_twl_enc_dec_table* dec_table, uint8_t* buffer, size_t total_size, const std::string text = "", size_t text_pos = 0) {
	enc_dec_table_apply_to_data(dec_table, buffer, total_size, false);
	uint32_t crc32 = get_crc32_data_comm(buffer, total_size - 4);
	uint32_t read_crc32 = read_le32(buffer, (total_size - 4) / 4);
	if((crc32 != read_crc32) || (!is_string_present_and_same(text, buffer, text_pos)))
		return LIBUSB_ERROR_INTERRUPTED;
	return LIBUSB_SUCCESS;
}

static void data_crc32_and_enc(is_device_twl_enc_dec_table* enc_table, uint8_t* buffer, size_t total_size, const std::string text = "", size_t text_pos = 0) {
	if(text != "")
		write_string(buffer + 4 + text_pos, text);
	uint32_t crc32 = get_crc32_data_comm(buffer + 4, total_size - 4);
	write_le32(buffer, crc32);
	enc_dec_table_apply_to_data(enc_table, buffer, total_size, true);
}

int StartUsbCaptureDma(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, bool enable_sound_capture, is_device_twl_enc_dec_table* enc_table, is_device_twl_enc_dec_table* dec_table) {
	int ret = 0;
	uint8_t buffer[0xB4];
	uint32_t enabled_value = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			ret = WriteNecMemU16(handlers, REG_USB_DMA_CONTROL_2, 2, device_desc);
			if(ret < 0)
				return ret;
			return WriteNecMemU16(handlers, REG_USB_BIU_CONTROL_2, 1, device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			return SendReadPacket(handlers, IS_NITRO_CAP_CMD_ENABLE_CAP, IS_NITRO_CAPTURE_PACKET_TYPE_DMA_CONTROL, 0, NULL, 1, device_desc, false);
		case IS_TWL_CAPTURE_DEVICE:
			if((enc_table == NULL) || (dec_table == NULL))
				return LIBUSB_ERROR_INTERRUPTED;
			ret = SendWriteCommand(handlers, IS_TWL_CAP_CMD_ACTS, NULL, 0, device_desc);
			if(ret < 0)
				return ret;
			memset(buffer, 0, 0xB4);
			write_le16(buffer, 0xB0, 2 + 2);
			write_le16(buffer, 6, 3 + 2);
			data_crc32_and_enc(enc_table, buffer, 0xB4, "MNTR", 8);
			ret = SendReadWriteCommand(handlers, IS_TWL_CAP_CMD_MNTR, buffer, 0xB4, buffer, 0xB4, device_desc);
			if(ret < 0)
				return ret;
			ret = data_dec_and_check(dec_table, buffer, 0xB4, "MNTR", 8);
			if(ret < 0)
				return ret;
			ret = SendReadCommand(handlers, IS_TWL_CAP_CMD_EPAC, buffer, 8, device_desc);
			if(ret < 0)
				return ret;
			ret = data_dec_and_check(dec_table, buffer, 8, "EPAC");
			if(ret < 0)
				return ret;
			ret = SendReadCommand(handlers, IS_TWL_CAP_CMD_POST_EPAC, buffer, 8, device_desc);
			if(ret < 0)
				return ret;
			ret = data_dec_and_check(dec_table, buffer, 8);
			if(ret < 0)
				return ret;
			if(read_le32(buffer) != 0)
				return LIBUSB_ERROR_INTERRUPTED;
			memset(buffer, 0, 0x10);
			data_crc32_and_enc(enc_table, buffer, 0x10, "TCZC");
			ret = SendWriteCommand(handlers, IS_TWL_CAP_CMD_TCZC, buffer, 0x10, device_desc);
			if(ret < 0)
				return ret;
			ret = SendReadCommand(handlers, IS_TWL_CAP_CMD_SCTG, buffer, 0x10, device_desc);
			if(ret < 0)
				return ret;
			ret = data_dec_and_check(dec_table, buffer, 0x10, "SCTG");
			if(ret < 0)
				return ret;
			memset(buffer, 0, 0x10);
			enabled_value = 1;
			if(enable_sound_capture)
				enabled_value |= 0x10;
			write_le32(buffer, enabled_value, 2);
			// FPS. Seems to do nothing? Set to 60 / multiplier,
			// but it has no effect...
			write_le32(buffer, 60, 3);
			data_crc32_and_enc(enc_table, buffer, 0x10, "ACTS");
			ret = SendReadWriteCommand(handlers, IS_TWL_CAP_CMD_ACTS, buffer, 0x10, buffer, 8, device_desc);
			if(ret < 0)
				return ret;
			ret = data_dec_and_check(dec_table, buffer, 8);
			if(ret < 0)
				return ret;
			if(read_le32(buffer) != 0)
				return LIBUSB_ERROR_INTERRUPTED;
			return ret;
		default:
			return 0;
	}
}

int StopUsbCaptureDma(is_device_device_handlers* handlers, const is_device_usb_device* device_desc) {
	int ret = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			ret = WriteNecMemU16(handlers, REG_USB_DMA_CONTROL_2, 0, device_desc);
			if(ret < 0)
				return ret;
			return WriteNecMemU16(handlers, REG_USB_BIU_CONTROL_2, 0, device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			return SendReadPacket(handlers, IS_NITRO_CAP_CMD_ENABLE_CAP, IS_NITRO_CAPTURE_PACKET_TYPE_DMA_CONTROL, 0, NULL, 0, device_desc);
		case IS_TWL_CAPTURE_DEVICE:
			return SendWriteCommand(handlers, IS_TWL_CAP_CMD_STOP_FRAME_READS, NULL, 0, device_desc);
		default:
			return 0;
	}
}

int SetForwardFrameCount(is_device_device_handlers* handlers, uint16_t count, const is_device_usb_device* device_desc) {
	if(!count)
		return LIBUSB_ERROR_INTERRUPTED;
	count -= 1;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return WriteNecMemU32(handlers, 0x0800000C, (count >> 8) | ((count & 0xFF) << 16), device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			return SendWriteCommandU32(handlers, IS_NITRO_CAP_CMD_SET_FWD_FRAMES, count, device_desc);
		default:
			return 0;
	}
}

int SetForwardFramePermanent(is_device_device_handlers* handlers, const is_device_usb_device* device_desc) {
	return SetForwardFrameCount(handlers, 0x4001, device_desc);
}

int GetFrameCounter(is_device_device_handlers* handlers, uint16_t* out, const is_device_usb_device* device_desc) {
	int ret = 0;
	uint32_t counter = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			ReadNecMemU32(handlers, 0x08000028, &counter, device_desc);
			if(ret < 0)
				return ret;
			*out = (counter & 0xFF) | ((counter & 0xFF0000) >> 8);
			return ret;
		case IS_NITRO_CAPTURE_DEVICE:
			*out = 0;
			return LIBUSB_SUCCESS;
		default:
			return 0;
	}
}

int UpdateFrameForwardConfig(is_device_device_handlers* handlers, is_device_forward_config_values_colors colors, is_device_forward_config_values_screens screens, is_device_forward_config_values_rate rate, const is_device_usb_device* device_desc) {
	int ret = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return WriteNecMemU16(handlers, 0x0800000A, ((colors & 1) << 4) | ((screens & 3) << 2) | ((rate & 3) << 0), device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			ret = SendWriteCommandU32(handlers, IS_NITRO_CAP_CMD_SET_FWD_RATE, rate & 3, device_desc);
			if(ret < 0)
				return ret;
			ret = SendWriteCommandU32(handlers, IS_NITRO_CAP_CMD_SET_FWD_MODE, screens & 3, device_desc);
			if(ret < 0)
				return ret;
			return SendWriteCommandU32(handlers, IS_NITRO_CAP_CMD_SET_FWD_COLOURS, colors & 1, device_desc);
		default:
			return 0;
	}
}

int UpdateFrameForwardEnable(is_device_device_handlers* handlers, bool enable, bool restart, const is_device_usb_device* device_desc) {
	uint32_t value = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			if(enable)
				value |= (1 << IS_NITRO_EMULATOR_FORWARD_ENABLE_BIT);
			if(restart)
				value |= (1 << IS_NITRO_EMULATOR_FORWARD_COUNTER_RESTART_BIT);
			return WriteNecMemU16(handlers, 0x08000008, value, device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			if(restart)
				value |= (1 << IS_NITRO_CAPTURE_FORWARD_COUNTER_RESTART_BIT);
			return SendWriteCommandU32(handlers, IS_NITRO_CAP_CMD_SET_FWD_RESTART, value, device_desc);
		default:
			return 0;
	}
}

int ReadLidState(is_device_device_handlers* handlers, bool* out, const is_device_usb_device* device_desc) {
	uint32_t flags = 0;
	int ret = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			ret = ReadNecMemU32(handlers, 0x08000000, &flags, device_desc);
			if(ret < 0)
				return ret;
			*out = (flags & 2) ? true : false;
			return ret;
		case IS_NITRO_CAPTURE_DEVICE:
			ret = SendReadCommandU32(handlers, IS_NITRO_CAP_CMD_GET_LID_STATE, &flags, device_desc);
			*out = (flags & 1) ? true : false;
			return ret;
		default:
			return 0;
	}
}

int ReadDebugButtonState(is_device_device_handlers* handlers, bool* out, const is_device_usb_device* device_desc) {
	uint32_t flags = 0;
	int ret = 0;
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			ret = ReadNecMemU32(handlers, 0x08000000, &flags, device_desc);
			if(ret < 0)
				return ret;
			*out = (flags & 1) ? true : false;
			return ret;
		case IS_NITRO_CAPTURE_DEVICE:
			ret = SendReadCommandU32(handlers, IS_NITRO_CAP_CMD_GET_DEBUG_STATE, &flags, device_desc);
			*out = (flags & 1) ? true : false;
			return ret;
		default:
			return 0;
	}
}

int ReadPowerButtonState(is_device_device_handlers* handlers, bool* out, const is_device_usb_device* device_desc) {
	return ReadDebugButtonState(handlers, out, device_desc);
}

static int ResetCPUEmulatorGeneral(is_device_device_handlers* handlers, bool on, const is_device_usb_device* device_desc) {
	uint8_t data[] = {IS_NITRO_EMU_CMD_CPU_RESET, 0, (uint8_t)(on ? 1 : 0), 0};
	return SendWriteCommand(handlers, IS_NITRO_EMU_CMD_CPU_RESET, data, sizeof(data), device_desc);
}

int ResetCPUStart(is_device_device_handlers* handlers, const is_device_usb_device* device_desc) {
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return ResetCPUEmulatorGeneral(handlers, true, device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			return SendWriteCommand(handlers, IS_NITRO_CAP_CMD_SET_RESET_CPU_ON, NULL, 0, device_desc);
		case IS_TWL_CAPTURE_DEVICE:
			return SendWriteCommandU32(handlers, IS_TWL_CAP_CMD_POWER_ON_OFF, 0x00070082, device_desc);
		default:
			return 0;
	}
}

int ResetCPUEnd(is_device_device_handlers* handlers, const is_device_usb_device* device_desc) {
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return ResetCPUEmulatorGeneral(handlers, false, device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			return SendWriteCommand(handlers, IS_NITRO_CAP_CMD_SET_RESET_CPU_OFF, NULL, 0, device_desc);
		case IS_TWL_CAPTURE_DEVICE:
			return SendWriteCommandU32(handlers, IS_TWL_CAP_CMD_POWER_ON_OFF, 0x00000082, device_desc);
		default:
			return 0;
	}
}

int ResetFullHardware(is_device_device_handlers* handlers, const is_device_usb_device* device_desc) {
	uint8_t isn_emu_rst_data[] = {IS_NITRO_EMU_CMD_FULL_HARDWARE_RESET, 0xF2};
	switch(device_desc->device_type) {
		case IS_NITRO_EMULATOR_DEVICE:
			return SendWriteCommand(handlers, IS_NITRO_EMU_CMD_FULL_HARDWARE_RESET, isn_emu_rst_data, sizeof(isn_emu_rst_data), device_desc);
		case IS_NITRO_CAPTURE_DEVICE:
			return SendWriteCommand(handlers, IS_NITRO_CAP_CMD_SET_RESTART_FULL, NULL, 0, device_desc);
		default:
			return 0;
	}
}

int AskFrameLengthPos(is_device_device_handlers* handlers, uint32_t* video_address, uint32_t* video_length, bool video_enabled, uint32_t* audio_address, uint32_t* audio_length, bool audio_enabled, const is_device_usb_device* device_desc) {
	if(device_desc->device_type != IS_TWL_CAPTURE_DEVICE)
		return 0;
	is_twl_ask_capture_information_packet out_packet;
	is_twl_capture_information_packet in_packet;
	memset(&out_packet, 0, sizeof(out_packet));
	if(video_enabled)
		out_packet.max_length_video = to_le((uint32_t)DEFAULT_MAX_LENGTH_IS_TWL_VIDEO);
	if(audio_enabled)
		out_packet.max_length_audio = to_le((uint32_t)DEFAULT_MAX_LENGTH_IS_TWL_AUDIO);
	int ret = SendReadWriteCommand(handlers, IS_TWL_CAP_CMD_ASK_CAPTURE_INFORMATION, (uint8_t*)&out_packet, sizeof(out_packet), (uint8_t*)&in_packet, sizeof(in_packet), device_desc);
	if(ret < 0)
		return ret;
	*video_length = from_le(in_packet.available_video_length);
	*video_address = from_le(in_packet.video_start_address);
	*audio_length = from_le(in_packet.available_audio_length);
	*audio_address = from_le(in_packet.audio_start_address);
	return ret;
}

int SetLastFrameInfo(is_device_device_handlers* handlers, uint32_t video_address, uint32_t video_length, uint32_t audio_address, uint32_t audio_length, const is_device_usb_device* device_desc) {
	if(device_desc->device_type != IS_TWL_CAPTURE_DEVICE)
		return 0;
	is_twl_capture_information_packet out_packet;
	memset(&out_packet, 0, sizeof(out_packet));
	out_packet.available_video_length = to_le(video_length);
	out_packet.video_start_address = to_le(video_address);
	out_packet.available_audio_length = to_le(audio_length);
	out_packet.audio_start_address = to_le(audio_address);
	return SendWriteCommand(handlers, IS_TWL_CAP_CMD_SET_LAST_FRAME_INFORMATION, (uint8_t*)&out_packet, sizeof(out_packet), device_desc);
}

int ReadFrame(is_device_device_handlers* handlers, uint8_t* buf, uint32_t address, uint32_t length, const is_device_usb_device* device_desc) {
	if(device_desc->device_type != IS_TWL_CAPTURE_DEVICE)
		return 0;
	if(length == 0)
		return 0;
	return SendReadPacket(handlers, IS_TWL_CAP_CMD_GET_CAP, IS_TWL_PACKET_TYPE_FRAME, address, buf, length, device_desc);
}

int ReadFrame(is_device_device_handlers* handlers, uint8_t* buf, int length, const is_device_usb_device* device_desc) {
	// Maybe making this async would be better for lower end hardware...
	int num_bytes = 0;
	if(length == 0)
		return 0;
	int ret = bulk_in(handlers, device_desc, buf, length, &num_bytes);
	if(num_bytes != length)
		return LIBUSB_ERROR_INTERRUPTED;
	return ret;
}

void ReadFrameAsync(is_device_device_handlers* handlers, uint8_t* buf, int length, const is_device_usb_device* device_desc, isd_async_callback_data* cb_data) {
	return bulk_in_async(handlers, device_desc, buf, length, cb_data);
}

void CloseAsyncRead(is_device_device_handlers* handlers, isd_async_callback_data* cb_data) {
	if(handlers->usb_handle)
		return is_device_libusb_cancell_callback(cb_data);
	return is_device_is_driver_cancel_callback(cb_data);
}

int ResetUSBDevice(is_device_device_handlers* handlers) {
	if(handlers->usb_handle)
		return is_device_libusb_reset_device(handlers);
	return is_device_is_driver_reset_device(handlers);
}

static void prepare_enc_dec_table_data(is_device_twl_enc_dec_table* table, uint32_t seed_1, uint32_t seed_2, uint8_t num_iters, uint8_t num_values, bool is_enc) {
	uint32_t rotating_value = seed_1 ^ seed_2 ^ 0x3CF0F03C;
	table->rotating_value = rotating_value;
	table->num_values = num_values;
	table->num_iters_full = num_iters;
	table->num_iters_before_refresh = num_iters;
	uint8_t buffer[8];
	uint8_t values_buffer[8];
	for(int i = 0; i < 7; i++)
		buffer[i] = i + 1;
	buffer[7] = 7;
	values_buffer[0] = 0;
	for(int i = 1; i < num_values; i++) {
		int divisor = (7 - (i - 1));
		int mod = rotating_value % divisor;
		values_buffer[i] = buffer[mod];
		for(int j = mod; j < 7; j++)
			buffer[j] = buffer[j + 1];
		rotating_value = rotate_bits_right(rotating_value);
	}
	int offset = 0;
	if(!is_enc)
		offset = 1;
	for(int i = 0; i < num_values; i++) 
		table->action_values[i] = read_le16(is_twl_cap_init_seed_table, (values_buffer[i] * 2) + offset);
}

int PrepareEncDecTable(is_device_device_handlers* handlers, is_device_twl_enc_dec_table* enc_table, is_device_twl_enc_dec_table* dec_table, const is_device_usb_device* device_desc) {
	int ret = 0;
	uint8_t buffer[0xC];
	uint32_t system_ms = ms_since_start();
	uint32_t incoming_seed = 0;
	switch(device_desc->device_type) {
		case IS_TWL_CAPTURE_DEVICE:
			ret = AuthCtrlIn(handlers, device_desc);
			if(ret < 0)
				return ret;
			write_le32(buffer, IS_TWL_CAP_CMD_GET_ENC_DEC_SEEDS);
			write_le32(buffer, system_ms, 1);
			write_le32(buffer, system_ms ^ 0x84327472, 2);
			ret = SendWriteCommand(handlers, IS_TWL_CAP_CMD_GET_ENC_DEC_SEEDS, buffer, sizeof(buffer), device_desc);
			if(ret < 0)
				return ret;
			ret = SendReadCommand(handlers, IS_TWL_CAP_CMD_GET_ENC_DEC_SEEDS, buffer, 4, device_desc);
			if(ret < 0)
				return ret;
			incoming_seed = read_le32(buffer);
			prepare_enc_dec_table_data(enc_table, system_ms, system_ms ^ 0x84327472 ^ 0xB0762D27, 0x10, 7, true);
			prepare_enc_dec_table_data(dec_table, ~system_ms, incoming_seed ^ 0xD13F1EBB, 0x40, 3, false);
			return ret;
		default:
			return 0;
	}
}

void SetupISDeviceAsyncThread(is_device_device_handlers* handlers, void* user_data, std::thread* thread_ptr, bool* keep_going, ConsumerMutex* is_data_ready) {
	if(handlers->usb_handle)
		return is_device_libusb_start_thread(thread_ptr, keep_going);
	return is_device_is_driver_start_thread(thread_ptr, keep_going, (ISDeviceCaptureReceivedData*)user_data, handlers, is_data_ready);
}

void EndISDeviceAsyncThread(is_device_device_handlers* handlers, void* user_data, std::thread* thread_ptr, bool* keep_going, ConsumerMutex* is_data_ready) {
	if(handlers->usb_handle)
		return is_device_libusb_close_thread(thread_ptr, keep_going);
	return is_device_is_driver_close_thread(thread_ptr, keep_going, (ISDeviceCaptureReceivedData*)user_data);
}

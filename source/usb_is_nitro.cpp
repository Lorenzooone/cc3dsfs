#include "frontend.hpp"
#include "usb_is_nitro.hpp"

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

#define USB_PACKET_LIMIT 0x2000

#define REG_USB_DMA_CONTROL_2 0x0C000028
#define REG_USB_BIU_CONTROL_2 0x0C0000A4

enum is_nitro_packet_emulator_dir {
	IS_NITRO_PACKET_EMU_DIR_WRITE = 0x10,
	IS_NITRO_PACKET_EMU_DIR_READ  = 0x11
};

enum is_nitro_packet_capture_dir {
	IS_NITRO_PACKET_CAP_DIR_WRITE = 0x00,
	IS_NITRO_PACKET_CAP_DIR_READ  = 0x01
};

enum is_nitro_packet_type {
	IS_NITRO_PACKET_TYPE_COMMAND = 0,
	IS_NITRO_PACKET_TYPE_EMU_MEMORY = 1,
	IS_NITRO_EMULATOR_PACKET_TYPE_AGB_SRAM = 2,
	IS_NITRO_CAPTURE_PACKET_TYPE_DMA_CONTROL = 2,
	IS_NITRO_PACKET_TYPE_CAPTURE = 3,
	IS_NITRO_PACKET_TYPE_AGB_CART_ROM = 4,
	IS_NITRO_PACKET_TYPE_AGB_BUS2 = 5
};

enum is_nitro_emulator_command {
	IS_NITRO_EMU_CMD_GET_SERIAL = 0x13,
	IS_NITRO_EMU_CMD_READ_NEC_MEM = 0x17,
	IS_NITRO_EMU_CMD_WRITE_NEC_MEM = 0x24,
	IS_NITRO_EMU_CMD_SET_READ_NEC_MEM = 0x25,
	IS_NITRO_EMU_CMD_AD = 0xAD,
};

enum is_nitro_capture_command {
	IS_NITRO_CAP_CMD_ENABLE_CAP = 0x00,
	IS_NITRO_CAP_CMD_GET_SERIAL = 0x03,
	IS_NITRO_CAP_CMD_SET_FWD_RATE = 0x12,
	IS_NITRO_CAP_CMD_SET_FWD_MODE = 0x13,
	IS_NITRO_CAP_CMD_SET_FWD_COLOURS = 0x14,
	IS_NITRO_CAP_CMD_SET_FWD_FRAMES = 0x15,
	IS_NITRO_CAP_CMD_SET_FWD_RESTART = 0x1C,
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

struct PACKED is_nitro_packet_header {
	uint16_t command;
	uint8_t direction;
	uint8_t type;
	uint32_t address;
	uint32_t length;
	uint32_t padding;
};
#pragma pack(pop)

const is_nitro_usb_device usb_is_nitro_emu_rare_desc = {
.name = "IS Nitro Emulator(R)",
.vid = 0x0f6e, .pid = 0x0400,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep2_in = 2 | LIBUSB_ENDPOINT_IN, .ep1_out = 1 | LIBUSB_ENDPOINT_OUT,
.product_id = 2, .manufacturer_id = 1, .is_capture = false
};

const is_nitro_usb_device usb_is_nitro_emu_common_desc = {
.name = "IS Nitro Emulator",
.vid = 0x0f6e, .pid = 0x0404,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep2_in = 2 | LIBUSB_ENDPOINT_IN, .ep1_out = 1 | LIBUSB_ENDPOINT_OUT,
.product_id = 2, .manufacturer_id = 1, .is_capture = false
};

const is_nitro_usb_device usb_is_nitro_cap_desc = {
.name = "IS Nitro Capture",
.vid = 0x0f6e, .pid = 0x0403,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep2_in = 2 | LIBUSB_ENDPOINT_IN, .ep1_out = 1 | LIBUSB_ENDPOINT_OUT,
.product_id = 2, .manufacturer_id = 1, .is_capture = true
};

const is_nitro_usb_device* GetISNitroDesc(is_nitro_possible_devices wanted_device_id) {
	switch(wanted_device_id) {
		case IS_NITRO_EMULATOR_COMMON_ID:
			return &usb_is_nitro_emu_common_desc;
		case IS_NITRO_EMULATOR_RARE_ID:
			return &usb_is_nitro_emu_rare_desc;
		case IS_NITRO_CAPTURE_ID:
			return &usb_is_nitro_cap_desc;
		default:
			return &usb_is_nitro_emu_common_desc;
	}
}

static void fix_endianness_header(is_nitro_packet_header* header) {
	header->command = to_le(header->command);
	header->address = to_le(header->address);
	header->length = to_le(header->length);
}

static void fix_endianness_header(is_nitro_nec_packet_header* header) {
	header->count = to_le(header->count);
	header->address = to_le(header->address);
}

// Write to bulk endpoint.  Returns libusb error code
static int bulk_out(libusb_device_handle *handle, const is_nitro_usb_device* usb_device_desc, uint8_t *buf, int length, int *transferred) {
	return libusb_bulk_transfer(handle, usb_device_desc->ep1_out, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Read from bulk endpoint.  Returns libusb error code
static int bulk_in(libusb_device_handle *handle, const is_nitro_usb_device* usb_device_desc, uint8_t *buf, int length, int *transferred) {
	return libusb_bulk_transfer(handle, usb_device_desc->ep2_in, buf, length, transferred, usb_device_desc->bulk_timeout);
}

static int SendWritePacket(libusb_device_handle *handle, uint16_t command, is_nitro_packet_type type, uint32_t address, uint8_t* buf, int length, const is_nitro_usb_device* device_desc, bool expect_result = true) {
	is_nitro_packet_header header;
	bool append_mode = true;
	if(device_desc->is_capture)
		append_mode = false;
	uint8_t single_usb_packet[USB_PACKET_LIMIT];
	int single_packet_covered_size = USB_PACKET_LIMIT - sizeof(header);
	if(!append_mode)
		single_packet_covered_size = USB_PACKET_LIMIT;
	int num_iters = (length + single_packet_covered_size - 1) / single_packet_covered_size;
	if(!num_iters)
		num_iters = 1;
	for(int i = 0; i < num_iters; i++) {
		int transfer_size = length - (i * single_packet_covered_size);
		if(transfer_size > single_packet_covered_size)
			transfer_size = single_packet_covered_size;

		uint8_t packet_direction = IS_NITRO_PACKET_EMU_DIR_WRITE;
		if(device_desc->is_capture)
			packet_direction = IS_NITRO_PACKET_CAP_DIR_WRITE;
			
		header.command = command;
		header.direction = packet_direction;
		header.type = type;
		header.address = address;
		header.length = transfer_size;
		header.padding = 0;
		fix_endianness_header(&header);
		int ret = 0;
		int num_bytes = 0;
		for(int j = 0; j < sizeof(is_nitro_packet_header); j++)
			single_usb_packet[j] = ((uint8_t*)&header)[j];
		if(append_mode && (buf != NULL)) {
			for(int j = 0; j < transfer_size; j++)
				single_usb_packet[sizeof(is_nitro_packet_header) + j] = buf[(i * single_packet_covered_size) + j];
			ret = bulk_out(handle, device_desc, single_usb_packet, transfer_size + sizeof(is_nitro_packet_header), &num_bytes);
		}
		else {
			int header_bytes = 0;
			ret = bulk_out(handle, device_desc, single_usb_packet, sizeof(is_nitro_packet_header), &header_bytes);
			if(ret < 0)
				return ret;
			if(header_bytes != sizeof(is_nitro_packet_header))
				return LIBUSB_ERROR_INTERRUPTED;
			if((buf != NULL) && (transfer_size > 0))
				ret = bulk_out(handle, device_desc, &buf[(i * single_packet_covered_size)], transfer_size, &num_bytes);
			num_bytes += header_bytes;
		}
		if(ret < 0)
			return ret;
		if(num_bytes != (transfer_size + sizeof(is_nitro_packet_header)))
			return LIBUSB_ERROR_INTERRUPTED;
	}
	if(device_desc->is_capture && expect_result) {
		uint8_t status[16];
		int status_bytes = 0;
		int ret = bulk_in(handle, device_desc, status, sizeof(status), &status_bytes);
		if(ret < 0)
			return ret;
		if(status_bytes != sizeof(status))
			return LIBUSB_ERROR_INTERRUPTED;
		if(status[0] != 1)
			return LIBUSB_ERROR_OTHER;
	}
	return LIBUSB_SUCCESS;
}

static int SendReadPacket(libusb_device_handle *handle, uint16_t command, is_nitro_packet_type type, uint32_t address, uint8_t* buf, int length, const is_nitro_usb_device* device_desc, bool expect_result = true) {
	is_nitro_packet_header header;
	int single_packet_covered_size = USB_PACKET_LIMIT;
	int num_iters = (length + single_packet_covered_size - 1) / single_packet_covered_size;
	if(num_iters == 0)
		num_iters = 1;
	for(int i = 0; i < num_iters; i++) {
		int transfer_size = length - (i * single_packet_covered_size);
		if(transfer_size > single_packet_covered_size)
			transfer_size = single_packet_covered_size;

		uint8_t packet_direction = IS_NITRO_PACKET_EMU_DIR_READ;
		if(device_desc->is_capture)
			packet_direction = IS_NITRO_PACKET_CAP_DIR_READ;

		header.command = command;
		header.direction = packet_direction;
		header.type = type;
		header.address = address;
		header.length = transfer_size;
		header.padding = 0;
		fix_endianness_header(&header);
		int num_bytes = 0;
		int ret = bulk_out(handle, device_desc, (uint8_t*)&header, sizeof(is_nitro_packet_header), &num_bytes);
		if(ret < 0)
			return ret;
		if(num_bytes != sizeof(is_nitro_packet_header))
			return LIBUSB_ERROR_INTERRUPTED;
		if(buf != NULL) {
			ret = bulk_in(handle, device_desc, buf + (i * single_packet_covered_size), transfer_size, &num_bytes);
			if(ret < 0)
				return ret;
			if(num_bytes != transfer_size)
				return LIBUSB_ERROR_INTERRUPTED;
		}
	}
	if(device_desc->is_capture && expect_result) {
		uint8_t status[16];
		int status_bytes = 0;
		int ret = bulk_in(handle, device_desc, status, sizeof(status), &status_bytes);
		if(ret < 0)
			return ret;
		if(status_bytes != sizeof(status))
			return LIBUSB_ERROR_INTERRUPTED;
		if(status[0] != 1)
			return LIBUSB_ERROR_OTHER;
	}
	return LIBUSB_SUCCESS;
}

int SendReadCommand(libusb_device_handle *handle, uint16_t command, uint8_t* buf, int length, const is_nitro_usb_device* device_desc) {
    return SendReadPacket(handle, command, IS_NITRO_PACKET_TYPE_COMMAND, 0, buf, length, device_desc);
}

int SendWriteCommand(libusb_device_handle *handle, uint16_t command, uint8_t* buf, int length, const is_nitro_usb_device* device_desc) {
    return SendWritePacket(handle, command, IS_NITRO_PACKET_TYPE_COMMAND, 0, buf, length, device_desc);
}

int SendReadCommandU32(libusb_device_handle *handle, uint16_t command, uint32_t* out, const is_nitro_usb_device* device_desc) {
	uint32_t buffer;
    int ret = SendReadCommand(handle, command, (uint8_t*)&buffer, sizeof(uint32_t), device_desc);
	if(ret < 0)
		return ret;
	*out = from_le(buffer);
	return 0;
}

int SendWriteCommandU32(libusb_device_handle *handle, uint16_t command, uint32_t value, const is_nitro_usb_device* device_desc) {
	uint32_t buffer = to_le(value);
    return SendWriteCommand(handle, command, (uint8_t*)&buffer, sizeof(uint32_t), device_desc);
}

int GetDeviceSerial(libusb_device_handle *handle, uint8_t* buf, const is_nitro_usb_device* device_desc) {
	if(device_desc->is_capture)
	    return SendReadCommand(handle, IS_NITRO_CAP_CMD_GET_SERIAL, buf, IS_NITRO_REAL_SERIAL_NUMBER_SIZE, device_desc);
    return SendReadCommand(handle, IS_NITRO_EMU_CMD_GET_SERIAL, buf, IS_NITRO_REAL_SERIAL_NUMBER_SIZE, device_desc);
}

int ReadNecMem(libusb_device_handle *handle, uint32_t address, uint8_t unit_size, uint8_t* buf, int count, const is_nitro_usb_device* device_desc) {
	is_nitro_nec_packet_header header;
	header.command = IS_NITRO_EMU_CMD_SET_READ_NEC_MEM;
	header.unit_size = unit_size;
	header.count = count;
	header.address = address;
	fix_endianness_header(&header);
	int ret = SendWriteCommand(handle, header.command, (uint8_t*)&header, sizeof(is_nitro_nec_packet_header), device_desc);
	if(ret < 0)
		return ret;
	return SendReadCommand(handle, IS_NITRO_EMU_CMD_READ_NEC_MEM, buf, count * unit_size, device_desc);
}

int ReadNecMemU16(libusb_device_handle *handle, uint32_t address, uint16_t* out, const is_nitro_usb_device* device_desc) {
	uint16_t buffer;
	int ret = ReadNecMem(handle, address, 2, (uint8_t*)&buffer, 1, device_desc);
	if(ret < 0)
		return ret;
	*out = from_le(buffer);
	return 0;
}

int ReadNecMemU32(libusb_device_handle *handle, uint32_t address, uint32_t* out, const is_nitro_usb_device* device_desc) {
	uint32_t buffer;
	int ret = ReadNecMem(handle, address, 2, (uint8_t*)&buffer, 2, device_desc);
	if(ret < 0)
		return ret;
	*out = from_le(buffer);
	return 0;
}

int WriteNecMem(libusb_device_handle *handle, uint32_t address, uint8_t unit_size, uint8_t* buf, int count, const is_nitro_usb_device* device_desc) {
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
	int ret = SendWriteCommand(handle, header.command, buffer, (count * unit_size) + sizeof(is_nitro_nec_packet_header), device_desc);
	delete []buffer;
	return ret;
}

int WriteNecMemU16(libusb_device_handle *handle, uint32_t address, uint16_t value, const is_nitro_usb_device* device_desc) {
	uint16_t buffer = to_le(value);
    return WriteNecMem(handle, address, 2, (uint8_t*)&buffer, 1, device_desc);
}

int WriteNecMemU32(libusb_device_handle *handle, uint32_t address, uint32_t value, const is_nitro_usb_device* device_desc) {
	uint32_t buffer = to_le(value);
    return WriteNecMem(handle, address, 2, (uint8_t*)&buffer, 2, device_desc);
}

int DisableLca2(libusb_device_handle *handle, const is_nitro_usb_device* device_desc) {
	if(device_desc->is_capture)
		return LIBUSB_SUCCESS;
	int ret = WriteNecMemU16(handle, 0x00805180, 0, device_desc);
	if(ret < 0)
		return ret;
	ret = WriteNecMemU16(handle, 0x0F84000A, 1, device_desc);
	if(ret < 0)
		return ret;
	//default_sleep(2000);
	return WriteNecMemU16(handle, 0x0F84000A, 0, device_desc);
}

int StartUsbCaptureDma(libusb_device_handle *handle, const is_nitro_usb_device* device_desc) {
	if(!device_desc->is_capture) {
		int ret = WriteNecMemU16(handle, REG_USB_DMA_CONTROL_2, 2, device_desc);
		if(ret < 0)
			return ret;
		return WriteNecMemU16(handle, REG_USB_BIU_CONTROL_2, 1, device_desc);
	}
    return SendReadPacket(handle, IS_NITRO_CAP_CMD_ENABLE_CAP, IS_NITRO_CAPTURE_PACKET_TYPE_DMA_CONTROL, 0, NULL, 1, device_desc, false);
}

int StopUsbCaptureDma(libusb_device_handle *handle, const is_nitro_usb_device* device_desc) {
	if(!device_desc->is_capture) {
		int ret = WriteNecMemU16(handle, REG_USB_DMA_CONTROL_2, 0, device_desc);
		if(ret < 0)
			return ret;
		return WriteNecMemU16(handle, REG_USB_BIU_CONTROL_2, 0, device_desc);
	}
    return SendReadPacket(handle, IS_NITRO_CAP_CMD_ENABLE_CAP, IS_NITRO_CAPTURE_PACKET_TYPE_DMA_CONTROL, 0, NULL, 0, device_desc);
}

int SetForwardFrameCount(libusb_device_handle *handle, uint16_t count, const is_nitro_usb_device* device_desc) {
	if(!count)
		return LIBUSB_ERROR_INTERRUPTED;
	count -= 1;
	if(!device_desc->is_capture)
		return WriteNecMemU32(handle, 0x0800000C, (count >> 8) | ((count & 0xFF) << 16), device_desc);
	return SendWriteCommandU32(handle, IS_NITRO_CAP_CMD_SET_FWD_FRAMES, count, device_desc);
}

int SetForwardFramePermanent(libusb_device_handle *handle, const is_nitro_usb_device* device_desc) {
	return SetForwardFrameCount(handle, 0x4001, device_desc);
}

int GetFrameCounter(libusb_device_handle *handle, uint16_t* out, const is_nitro_usb_device* device_desc) {
    uint32_t counter = 0;
    int ret = ReadNecMemU32(handle, 0x08000028, &counter, device_desc);
    if(ret < 0)
    	return ret;
    *out = (counter & 0xFF) | ((counter & 0xFF0000) >> 8);
    return ret;
}

int UpdateFrameForwardConfig(libusb_device_handle *handle, is_nitro_forward_config_values_colors colors, is_nitro_forward_config_values_screens screens, is_nitro_forward_config_values_rate rate, const is_nitro_usb_device* device_desc) {
	if(!device_desc->is_capture)
		return WriteNecMemU16(handle, 0x0800000A, ((colors & 1) << 4) | ((screens & 3) << 2) | ((rate & 3) << 0), device_desc);
	int ret = SendWriteCommandU32(handle, IS_NITRO_CAP_CMD_SET_FWD_COLOURS, colors & 1, device_desc);
	if(ret < 0)
		return ret;
	ret = SendWriteCommandU32(handle, IS_NITRO_CAP_CMD_SET_FWD_RATE, rate & 3, device_desc);
	if(ret < 0)
		return ret;
	return SendWriteCommandU32(handle, IS_NITRO_CAP_CMD_SET_FWD_MODE, screens & 3, device_desc);
}

int UpdateFrameForwardEnable(libusb_device_handle *handle, bool enable, bool restart, const is_nitro_usb_device* device_desc) {
	uint32_t value = 0;
	if(!device_desc->is_capture) {
		if(enable)
			value |= (1 << IS_NITRO_EMULATOR_FORWARD_ENABLE_BIT);
		if(restart)
			value |= (1 << IS_NITRO_EMULATOR_FORWARD_COUNTER_RESTART_BIT);
		return WriteNecMemU16(handle, 0x08000008, value, device_desc);
	}
	if(restart)
		value |= (1 << IS_NITRO_CAPTURE_FORWARD_COUNTER_RESTART_BIT);
	return SendWriteCommandU32(handle, IS_NITRO_CAP_CMD_SET_FWD_RESTART, value, device_desc);
}

int ReadFrame(libusb_device_handle *handle, uint8_t* buf, int length, const is_nitro_usb_device* device_desc) {
	// Maybe making this async would be better for lower end hardware...
	int num_bytes = 0;
	int ret = bulk_in(handle, device_desc, buf, length, &num_bytes);
	if(num_bytes != length)
		return LIBUSB_ERROR_INTERRUPTED;
	return ret;
}


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

enum is_nitro_packet_dir {
	IS_NITRO_PACKET_DIR_WRITE = 0x10,
	IS_NITRO_PACKET_DIR_READ  = 0x11
};

enum is_nitro_packet_type {
	IS_NITRO_PACKET_TYPE_COMMAND = 0,
	IS_NITRO_PACKET_TYPE_EMU_MEMORY = 1,
	IS_NITRO_PACKET_TYPE_AGB_SRAM = 2,
	IS_NITRO_PACKET_TYPE_CAPTURE = 3,
	IS_NITRO_PACKET_TYPE_AGB_CART_ROM = 4,
	IS_NITRO_PACKET_TYPE_AGB_BUS2 = 5
};

enum is_nitro_command {
	IS_NITRO_CMD_GET_SERIAL = 0x13,
	IS_NITRO_CMD_READ_NEC_MEM = 0x17,
	IS_NITRO_CMD_WRITE_NEC_MEM = 0x24,
	IS_NITRO_CMD_SET_READ_NEC_MEM = 0x25,
	IS_NITRO_CMD_AD = 0xAD,
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

const is_nitro_usb_device usb_is_nitro_desc = {
.vid = 0x0f6e, .pid = 0x0404,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep2_in = 2 | LIBUSB_ENDPOINT_IN, .ep1_out = 1 | LIBUSB_ENDPOINT_OUT,
.product_id = 2, .manufacturer_id = 1
};

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

static int SendWritePacket(libusb_device_handle *handle, uint16_t command, is_nitro_packet_type type, uint32_t address, uint8_t* buf, int length) {
	is_nitro_packet_header header;
	uint8_t single_usb_packet[USB_PACKET_LIMIT];
	int single_packet_covered_size = USB_PACKET_LIMIT - sizeof(header);
	int num_iters = (length + single_packet_covered_size - 1) / single_packet_covered_size;
	if(!num_iters)
		num_iters = 1;
	for(int i = 0; i < num_iters; i++) {
		int transfer_size = length - (i * single_packet_covered_size);
		if(transfer_size > single_packet_covered_size)
			transfer_size = single_packet_covered_size;

		header.command = command;
		header.direction = IS_NITRO_PACKET_DIR_WRITE;
		header.type = type;
		header.address = address;
		header.length = transfer_size;
		header.padding = 0;
		fix_endianness_header(&header);
		for(int j = 0; j < sizeof(is_nitro_packet_header); j++)
			single_usb_packet[j] = ((uint8_t*)&header)[j];
		for(int j = 0; j < transfer_size; j++)
			single_usb_packet[sizeof(is_nitro_packet_header) + j] = buf[(i * single_packet_covered_size) + j];
		int num_bytes = 0;
		int ret = bulk_out(handle, &usb_is_nitro_desc, single_usb_packet, transfer_size + sizeof(is_nitro_packet_header), &num_bytes);
		if(ret < 0)
			return ret;
		if(num_bytes != (transfer_size + sizeof(is_nitro_packet_header)))
			return LIBUSB_ERROR_INTERRUPTED;
	}
	return LIBUSB_SUCCESS;
}

static int SendReadPacket(libusb_device_handle *handle, uint16_t command, is_nitro_packet_type type, uint32_t address, uint8_t* buf, int length) {
	is_nitro_packet_header header;
	int single_packet_covered_size = USB_PACKET_LIMIT;
	int num_iters = (length + single_packet_covered_size - 1) / single_packet_covered_size;
	for(int i = 0; i < num_iters; i++) {
		int transfer_size = length - (i * single_packet_covered_size);
		if(transfer_size > single_packet_covered_size)
			transfer_size = single_packet_covered_size;

		header.command = command;
		header.direction = IS_NITRO_PACKET_DIR_READ;
		header.type = type;
		header.address = address;
		header.length = transfer_size;
		header.padding = 0;
		fix_endianness_header(&header);
		int num_bytes = 0;
		int ret = bulk_out(handle, &usb_is_nitro_desc, (uint8_t*)&header, sizeof(is_nitro_packet_header), &num_bytes);
		if(ret < 0)
			return ret;
		if(num_bytes != sizeof(is_nitro_packet_header))
			return LIBUSB_ERROR_INTERRUPTED;
		ret = bulk_in(handle, &usb_is_nitro_desc, buf + (i * single_packet_covered_size), transfer_size, &num_bytes);
		if(ret < 0)
			return ret;
		if(num_bytes != transfer_size)
			return LIBUSB_ERROR_INTERRUPTED;
	}
	return LIBUSB_SUCCESS;
}

int SendReadCommand(libusb_device_handle *handle, uint16_t command, uint8_t* buf, int length) {
    return SendReadPacket(handle, command, IS_NITRO_PACKET_TYPE_COMMAND, 0, buf, length);
}

int SendWriteCommand(libusb_device_handle *handle, uint16_t command, uint8_t* buf, int length) {
    return SendWritePacket(handle, command, IS_NITRO_PACKET_TYPE_COMMAND, 0, buf, length);
}

int GetDeviceSerial(libusb_device_handle *handle, uint8_t* buf) {
    return SendReadCommand(handle, IS_NITRO_CMD_GET_SERIAL, buf, IS_NITRO_REAL_SERIAL_NUMBER_SIZE);
}

int ReadNecMem(libusb_device_handle *handle, uint32_t address, uint8_t unit_size, uint8_t* buf, int count) {
	is_nitro_nec_packet_header header;
	header.command = IS_NITRO_CMD_SET_READ_NEC_MEM;
	header.unit_size = unit_size;
	header.count = count;
	header.address = address;
	fix_endianness_header(&header);
	int ret = SendWriteCommand(handle, header.command, (uint8_t*)&header, sizeof(is_nitro_nec_packet_header));
	if(ret < 0)
		return ret;
	return SendReadCommand(handle, IS_NITRO_CMD_READ_NEC_MEM, buf, count * unit_size);
}

int ReadNecMemU16(libusb_device_handle *handle, uint32_t address, uint16_t* out) {
	uint8_t buffer[2];
	int ret = ReadNecMem(handle, address, 2, buffer, 1);
	if(ret < 0)
		return ret;
	*out = buffer[0] | (buffer[1] << 8);
	return 0;
}

int ReadNecMemU32(libusb_device_handle *handle, uint32_t address, uint32_t* out) {
	uint8_t buffer[4];
	int ret = ReadNecMem(handle, address, 2, buffer, 2);
	if(ret < 0)
		return ret;
	*out = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
	return 0;
}

int WriteNecMem(libusb_device_handle *handle, uint32_t address, uint8_t unit_size, uint8_t* buf, int count) {
	uint8_t* buffer = new uint8_t[(count * unit_size) + sizeof(is_nitro_nec_packet_header)];
	is_nitro_nec_packet_header header;
	header.command = IS_NITRO_CMD_WRITE_NEC_MEM;
	header.unit_size = unit_size;
	header.count = count;
	header.address = address;
	fix_endianness_header(&header);
	for(int i = 0; i < sizeof(is_nitro_nec_packet_header); i++)
		buffer[i] = ((uint8_t*)&header)[i];
	for(int i = 0; i < count * unit_size; i++)
		buffer[i + sizeof(is_nitro_nec_packet_header)] = buf[i];
	int ret = SendWriteCommand(handle, header.command, buffer, (count * unit_size) + sizeof(is_nitro_nec_packet_header));
	delete []buffer;
	return ret;
}

int WriteNecMemU16(libusb_device_handle *handle, uint32_t address, uint16_t value) {
	uint8_t buffer[2];
	buffer[0] = value & 0xFF;
	buffer[1] = (value >> 8) & 0xFF;
    return WriteNecMem(handle, address, 2, buffer, 1);
}

int WriteNecMemU32(libusb_device_handle *handle, uint32_t address, uint32_t value) {
	uint8_t buffer[4];
	buffer[0] = value & 0xFF;
	buffer[1] = (value >> 8) & 0xFF;
	buffer[2] = (value >> 16) & 0xFF;
	buffer[3] = (value >> 24) & 0xFF;
    return WriteNecMem(handle, address, 2, buffer, 2);
}

int DisableLca2(libusb_device_handle *handle) {
	int ret = WriteNecMemU16(handle, 0x00805180, 0);
	if(ret < 0)
		return ret;
	ret = WriteNecMemU16(handle, 0xF84000A, 1);
	if(ret < 0)
		return ret;
	//default_sleep(2000);
	return WriteNecMemU16(handle, 0xF84000A, 0);
}

int StartUsbCaptureDma(libusb_device_handle *handle) {
	int ret = WriteNecMemU16(handle, REG_USB_DMA_CONTROL_2, 2);
	if(ret < 0)
		return ret;
	return WriteNecMemU16(handle, REG_USB_BIU_CONTROL_2, 1);
}

int StopUsbCaptureDma(libusb_device_handle *handle) {
	int ret = WriteNecMemU16(handle, REG_USB_DMA_CONTROL_2, 0);
	if(ret < 0)
		return ret;
	return WriteNecMemU16(handle, REG_USB_BIU_CONTROL_2, 0);
}

int SetForwardFrameCount(libusb_device_handle *handle, uint16_t count) {
	if(!count)
		return LIBUSB_ERROR_INTERRUPTED;
	count -= 1;
	return WriteNecMemU32(handle, 0x800000C, (count >> 8) | ((count & 0xFF) << 16));
}

int GetFrameCounter(libusb_device_handle *handle, uint16_t* out) {
    uint32_t counter = 0;
    int ret = ReadNecMemU32(handle, 0x08000028, &counter);
    if(ret < 0)
    	return ret;
    *out = (counter & 0xFF) | ((counter & 0xFF0000) >> 8);
    return ret;
}

int UpdateFrameForwardConfig(libusb_device_handle *handle, uint8_t value) {
	return WriteNecMemU16(handle, 0x0800000A, value);
}

int UpdateFrameForwardEnable(libusb_device_handle *handle, uint8_t value) {
	return WriteNecMemU16(handle, 0x08000008, value);
}

int ReadFrame(libusb_device_handle *handle, uint8_t* buf, int length) {
	// Maybe making this async would be better for lower end hardware...
	int num_bytes = 0;
	int ret = bulk_in(handle, &usb_is_nitro_desc, buf, length, &num_bytes);
	if(num_bytes != length)
		return LIBUSB_ERROR_INTERRUPTED;
	return ret;
}


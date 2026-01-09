#include "frontend.hpp"
#include "cypress_optimize_3ds_communications.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_optimize_3ds_acquisition_general.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"
#include "usb_generic.hpp"

#include "optimize_new_3ds_fw.h"
#include "optimize_new_3ds_565_fpga_pl.h"
#include "optimize_new_3ds_888_fpga_pl.h"

#include "optimize_old_3ds_fw.h"
#include "optimize_old_3ds_565_fpga_pl.h"
#include "optimize_old_3ds_888_fpga_pl.h"

#include "optimize_serial_key_to_byte_table.h"

// CRC32 table is a slightly edited crc32-adler.
// First entry all FFs instead of 00s.
#include "adler_crc32_table_sp.h"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>

// This code was developed by exclusively looking at Wireshark USB packet
// captures to learn the USB device's protocol.
// As such, it was developed in a clean room environment, to provide
// compatibility of the USB device with more (and more modern) hardware.
// Such an action is allowed under EU law.
// No reverse engineering of the original software was done to create this.

#define CYPRESS_BASE_USB_PACKET_LIMIT 64
#define CYPRESS_OPTIMIZE_NEW_3DS_USB_PACKET_LIMIT 512
#define CYPRESS_OPTIMIZE_OLD_3DS_USB_PACKET_LIMIT 512

#define OPTIMIZE_NEW_3DS_WANTED_VALUE_BASE 0xFE00
#define OPTIMIZE_OLD_3DS_WANTED_VALUE_BASE 0xFD00

#define OPTIMIZE_NUM_KEY_CHARS SERIAL_KEY_OPTIMIZE_NO_DASHES_SIZE
#define OPTIMIZE_NUM_KEY_CHARS_WITH_DASHES SERIAL_KEY_OPTIMIZE_WITH_DASHES_SIZE
#define OPTIMIZE_NUM_KEY_BYTES 12

#define OPTIMIZE_EEPROM_NEW_SIZE 0x80
#define OPTIMIZE_EEPROM_STRUCT_SIZE 0x79

const size_t key_bytes_to_serial_bytes_map[OPTIMIZE_NUM_KEY_BYTES] = {0, 7, 8, 9, 1, 4, 6, 3, 10, 2, 11, 5};

const uint8_t start_capture_setup_key_buffer_565_new[]    = { 0x61, 0x07, 0x00, 0x0F, 0x00, 0x3E, 0x00, 0xF8, 0x00, 0x38, 0x00, 0x00, 0x80, 0x00, 0x01, 0x00 };
const uint8_t start_capture_setup_key_buffer_565_3d_new[] = { 0x61, 0x07, 0x00, 0x0F, 0x00, 0x3E, 0x00, 0xF8, 0x00, 0x3C, 0x00, 0x00, 0x80, 0x00, 0x01, 0x00 };
const uint8_t start_capture_setup_key_buffer_888_new[]    = { 0x61, 0x07, 0x00, 0xA6, 0x00, 0x28, 0x00, 0x62, 0x00, 0x3A, 0x00, 0x00, 0x80, 0x00, 0x01, 0x00 };
const uint8_t start_capture_setup_key_buffer_888_3d_new[] = { 0x61, 0x07, 0x00, 0xA6, 0x00, 0x28, 0x00, 0x62, 0x00, 0x16, 0x00, 0x00, 0x80, 0x00, 0x01, 0x00 };
const uint8_t start_capture_setup_key_buffer_565_old[]    = { 0x61, 0x07, 0x00, 0x26, 0x00, 0x22, 0x00, 0x98, 0x00, 0x08, 0x00, 0x0E, 0x00, 0x00, 0x18, 0x00 };
const uint8_t start_capture_setup_key_buffer_565_3d_old[] = { 0x61, 0x07, 0x00, 0x26, 0x00, 0x22, 0x00, 0x98, 0x00, 0x08, 0x00, 0x0F, 0x00, 0x00, 0x18, 0x00 };
const uint8_t start_capture_setup_key_buffer_888_old[]    = { 0x61, 0x07, 0x00, 0xA6, 0x00, 0x20, 0x00, 0x98, 0x00, 0x08, 0x00, 0x0E, 0x00, 0x00, 0x18, 0x00 };
const uint8_t start_capture_setup_key_buffer_888_3d_old[] = { 0x61, 0x07, 0x00, 0xA6, 0x00, 0x20, 0x00, 0x98, 0x00, 0x08, 0x00, 0x05, 0x00, 0x00, 0x18, 0x00 };

static const cyop_device_usb_device cypress_optimize_new_3ds_generic_device = {
.name = "FX2LO->Opt. N3DS", .long_name = "EZ-USB FX2LPO -> Optimize New 3DS",
.device_type = CYPRESS_OPTIMIZE_NEW_3DS_BLANK_DEVICE,
.firmware_to_load = optimize_new_3ds_fw, .firmware_size = optimize_new_3ds_fw_len,
.fpga_pl_565 = NULL, .fpga_pl_565_size = 0,
.fpga_pl_888 = NULL, .fpga_pl_888_size = 0,
.is_new_device = true,
.next_device = CYPRESS_OPTIMIZE_NEW_3DS_INSTANTIATED_DEVICE,
.has_bcd_device_serial = false,
.index_in_allowed_scan = CC_OPTIMIZE_N3DS,
.usb_device_info = {
	.vid = 0x0752, .pid = 0x8613,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 0, .ep_ctrl_bulk_out = 0,
	.ep_bulk_in = 0,
	.max_usb_packet_size = CYPRESS_BASE_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = false,
	.alt_interface = 1,
	.full_data = &cypress_optimize_new_3ds_generic_device,
	.get_serial_requires_setup = false,
	.get_serial_fn = cypress_optimize_3ds_get_serial,
	.create_device_fn = cypress_optimize_3ds_create_device,
	.bcd_device_mask = 0x0000,
	.bcd_device_wanted_value = 0x0000
}
};

static const cyop_device_usb_device cypress_optimize_new_3ds_instantiated_device = {
.name = "Opt. N3DS", .long_name = "Optimize New 3DS",
.device_type = CYPRESS_OPTIMIZE_NEW_3DS_INSTANTIATED_DEVICE,
.firmware_to_load = NULL, .firmware_size = 0,
.fpga_pl_565 = optimize_new_3ds_565_fpga_pl, .fpga_pl_565_size = optimize_new_3ds_565_fpga_pl_len,
.fpga_pl_888 = optimize_new_3ds_888_fpga_pl, .fpga_pl_888_size = optimize_new_3ds_888_fpga_pl_len,
.is_new_device = true,
.next_device = CYPRESS_OPTIMIZE_NEW_3DS_INSTANTIATED_DEVICE,
.has_bcd_device_serial = true,
.index_in_allowed_scan = CC_OPTIMIZE_N3DS,
.usb_device_info = {
	.vid = 0x04B4, .pid = 0x1004,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 1 | LIBUSB_ENDPOINT_IN, .ep_ctrl_bulk_out = 1 | LIBUSB_ENDPOINT_OUT,
	.ep_bulk_in = 2 | LIBUSB_ENDPOINT_IN,
	.max_usb_packet_size = CYPRESS_OPTIMIZE_NEW_3DS_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = true,
	.alt_interface = 0,
	.full_data = &cypress_optimize_new_3ds_instantiated_device,
	.get_serial_requires_setup = false,
	.get_serial_fn = cypress_optimize_3ds_get_serial,
	.create_device_fn = cypress_optimize_3ds_create_device,
	.bcd_device_mask = 0xFF00,
	.bcd_device_wanted_value = OPTIMIZE_NEW_3DS_WANTED_VALUE_BASE
}
};

static const cyop_device_usb_device cypress_optimize_old_3ds_generic_device = {
.name = "FX2LP->Opt. O3DS", .long_name = "EZ-USB FX2LP -> Optimize Old 3DS",
.device_type = CYPRESS_OPTIMIZE_OLD_3DS_BLANK_DEVICE,
.firmware_to_load = optimize_old_3ds_fw, .firmware_size = optimize_old_3ds_fw_len,
.fpga_pl_565 = NULL, .fpga_pl_565_size = 0,
.fpga_pl_888 = NULL, .fpga_pl_888_size = 0,
.is_new_device = false,
.next_device = CYPRESS_OPTIMIZE_OLD_3DS_INSTANTIATED_DEVICE,
.has_bcd_device_serial = false,
.index_in_allowed_scan = CC_OPTIMIZE_O3DS,
.usb_device_info = {
	.vid = 0x04B4, .pid = 0x8613,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 0, .ep_ctrl_bulk_out = 0,
	.ep_bulk_in = 0,
	.max_usb_packet_size = CYPRESS_BASE_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = false,
	.alt_interface = 1,
	.full_data = &cypress_optimize_old_3ds_generic_device,
	.get_serial_requires_setup = false,
	.get_serial_fn = cypress_optimize_3ds_get_serial,
	.create_device_fn = cypress_optimize_3ds_create_device,
	.bcd_device_mask = 0x0000,
	.bcd_device_wanted_value = 0x0000
}
};

static const cyop_device_usb_device cypress_optimize_old_3ds_instantiated_device = {
.name = "Opt. O3DS", .long_name = "Optimize Old 3DS",
.device_type = CYPRESS_OPTIMIZE_OLD_3DS_INSTANTIATED_DEVICE,
.firmware_to_load = NULL, .firmware_size = 0,
.fpga_pl_565 = optimize_old_3ds_565_fpga_pl, .fpga_pl_565_size = optimize_old_3ds_565_fpga_pl_len,
.fpga_pl_888 = optimize_old_3ds_888_fpga_pl, .fpga_pl_888_size = optimize_old_3ds_888_fpga_pl_len,
.is_new_device = false,
.next_device = CYPRESS_OPTIMIZE_OLD_3DS_INSTANTIATED_DEVICE,
.has_bcd_device_serial = true,
.index_in_allowed_scan = CC_OPTIMIZE_O3DS,
.usb_device_info = {
	.vid = 0x04B4, .pid = 0x1004,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 1 | LIBUSB_ENDPOINT_IN, .ep_ctrl_bulk_out = 1 | LIBUSB_ENDPOINT_OUT,
	.ep_bulk_in = 2 | LIBUSB_ENDPOINT_IN,
	.max_usb_packet_size = CYPRESS_OPTIMIZE_OLD_3DS_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = true,
	.alt_interface = 0,
	.full_data = &cypress_optimize_old_3ds_instantiated_device,
	.get_serial_requires_setup = false,
	.get_serial_fn = cypress_optimize_3ds_get_serial,
	.create_device_fn = cypress_optimize_3ds_create_device,
	.bcd_device_mask = 0xFF00,
	.bcd_device_wanted_value = OPTIMIZE_OLD_3DS_WANTED_VALUE_BASE
}
};

static const cyop_device_usb_device* all_usb_cyop_device_devices_desc[] = {
	&cypress_optimize_new_3ds_generic_device,
	&cypress_optimize_new_3ds_instantiated_device,
	&cypress_optimize_old_3ds_generic_device,
	&cypress_optimize_old_3ds_instantiated_device,
};

static uint32_t calc_crc32_adler(uint8_t* data, size_t size) {
	uint32_t value = 0xFFFFFFFF;
	for(size_t i = 0; i < size; i++) {
		uint8_t inner_value = data[i] ^ value;
		value >>= 8;
		value ^= read_le32(adler_crc32_table_sp, inner_value);
	}
	return ~value;
}

const cy_device_usb_device* get_cy_usb_info(const cyop_device_usb_device* usb_device_desc) {
	return &usb_device_desc->usb_device_info;
}

int GetNumCyOpDeviceDesc() {
	return sizeof(all_usb_cyop_device_devices_desc) / sizeof(all_usb_cyop_device_devices_desc[0]);
}

const cyop_device_usb_device* GetCyOpDeviceDesc(int index) {
	if((index < 0) || (index >= GetNumCyOpDeviceDesc()))
		index = 0;
	return all_usb_cyop_device_devices_desc[index];
}

const cyop_device_usb_device* GetNextDeviceDesc(const cyop_device_usb_device* device) {
	for(int i = 0; i < GetNumCyOpDeviceDesc(); i++) {
		const cyop_device_usb_device* examined_device = GetCyOpDeviceDesc(i);
		if(examined_device->device_type == device->next_device)
			return examined_device;
	}
	return NULL;
}

bool has_to_load_firmware(const cyop_device_usb_device* device) {
	return !((device->firmware_to_load == NULL) || (device->firmware_size == 0));
}

static bool free_firmware_and_return(uint8_t* fw_data, bool retval) {
	delete []fw_data;
	return retval;
}

static bool key_to_key_bytes(uint8_t* in, uint8_t* out) {
	uint16_t value_converted = 0;
	size_t converted_pos = 0;
	size_t out_index = 0;

	for(size_t i = 0; i < OPTIMIZE_NUM_KEY_CHARS; i++) {
		uint8_t character = in[i];
		if(optimize_serial_key_to_byte_table[character] == 0xFF)
			return false;
		value_converted |= optimize_serial_key_to_byte_table[character] << converted_pos;
		converted_pos += 5;
		if(converted_pos >= 8) {
			out[out_index++] = value_converted & 0xFF;
			converted_pos -= 8;
			value_converted >>= 8;
		}
	}
	if(converted_pos > 0)
		out[out_index++] = value_converted & 0xFF;
	return true;
}

static bool key_bytes_to_serial_bytes(uint8_t* in, uint8_t* out, bool is_new_cc) {
	uint8_t xor_factor = 0xAB;
	if(is_new_cc)
		xor_factor = 0x5A;
	int table_accessing_base_value = 0x22;
	if(is_new_cc)
		table_accessing_base_value = -0x45;

	for(int i = 0; i < OPTIMIZE_NUM_KEY_BYTES; i++)
		out[key_bytes_to_serial_bytes_map[i]] = in[i];
	uint8_t checksum_byte = (out[0] ^ xor_factor) & 0xFE;
	out[0] = out[0] & 1;

	for(int i = 1; i < 8; i++) {
		uint32_t xor_val = read_le32(adler_crc32_table_sp, (checksum_byte + (i * table_accessing_base_value)) & 0xFF);
		write_le32(out + i, read_le32(out + i) ^ xor_val);
	}

	uint8_t sum = 0;
	// The last one is skipped... For whatever reason?
	for(int i = 0; i < OPTIMIZE_NUM_KEY_BYTES - 1; i++)
		sum += out[i];
	return (read_le32(adler_crc32_table_sp, sum) & 0xFE) == checksum_byte;
}

static bool key_to_data(std::string in_str, uint8_t* out, bool is_new_cc) {
	in_str.erase(std::remove(in_str.begin(), in_str.end(), '-'), in_str.end());
	in_str.erase(std::remove(in_str.begin(), in_str.end(), '&'), in_str.end());
	in_str.erase(std::remove(in_str.begin(), in_str.end(), ':'), in_str.end());

	if(in_str.length() != OPTIMIZE_NUM_KEY_CHARS)
		return false;

	uint8_t key_string_bytes[OPTIMIZE_NUM_KEY_CHARS];
	uint8_t key_bytes[OPTIMIZE_NUM_KEY_BYTES + 1]; // The last 4 bits of the key are ignored?!

	write_string(key_string_bytes, in_str);
	if(!key_to_key_bytes(key_string_bytes, key_bytes))
		return false;

	return key_bytes_to_serial_bytes(key_bytes, out, is_new_cc);
}

bool load_firmware(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint8_t patch_id) {
	if(!has_to_load_firmware(device))
		return true;
	const cyop_device_usb_device* next_device = GetNextDeviceDesc(device);
	if((next_device == NULL) || (next_device == device))
		return true;
	int transferred = 0;
	uint16_t base_patch_value = next_device->usb_device_info.bcd_device_wanted_value;
	uint8_t* fw_data = new uint8_t[device->firmware_size];
	memcpy(fw_data, device->firmware_to_load, device->firmware_size);
	int num_patches = read_le16(fw_data, 1);
	for(int i = 0; i < num_patches; i++) {
		int pos_patch = read_le16(fw_data, 2 + i);
		write_le16(fw_data + pos_patch, patch_id | base_patch_value);
	}
	uint8_t buffer[0x8000];
	buffer[0] = 1;
	int ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, 1, 0xA0, 0xE600, 0, &transferred);
	if(ret < 0)
		return free_firmware_and_return(fw_data, false);
	bool done = false;
	uint16_t fw_pos = read_le16(fw_data);
	while(!done) {
		int offset = read_le16(fw_data + fw_pos);
		int index = read_le16(fw_data + fw_pos, 1);
		uint32_t inside_len = read_le16(fw_data + fw_pos, 2);
		done = (inside_len & 0x8000) != 0;
		inside_len &= 0x7FFF;
		fw_pos += 6;
		if((inside_len + fw_pos) > device->firmware_size)
			return free_firmware_and_return(fw_data, false);
		memcpy(buffer, fw_data + fw_pos, inside_len);
		fw_pos += inside_len;
		ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, inside_len, 0xA0, offset, index, &transferred);
		if(ret < 0)
			return free_firmware_and_return(fw_data, false);
	}
	buffer[0] = 0;
	ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, 1, 0xA0, 0xE600, 0, &transferred);
	// This may fail due to the device reconfiguring itself...
	//if(ret < 0)
	//	return free_firmware_and_return(fw_data, false);
	return free_firmware_and_return(fw_data, true);
}

bool read_firmware(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint8_t* buffer_out, size_t read_size) {
	size_t transfer_max_size = 0x400;
	size_t num_transfers = (read_size + transfer_max_size - 1) / transfer_max_size;
	int transferred = 0;
	bool done = false;
	int ret = 0;
	for(size_t i = 0; i < num_transfers; i++) {
		size_t transfer_size = read_size - (transfer_max_size * i);
		if(transfer_size > transfer_max_size)
			transfer_size = transfer_max_size;
		size_t position = transfer_max_size * i;
		ret = cypress_ctrl_in_transfer(handlers, get_cy_usb_info(device), buffer_out + position, (int)transfer_size, 0xA0, (uint16_t)position, 0, &transferred);
		if(ret < 0)
			return false;
	}
	return true;
}

bool reset_cpu(cy_device_device_handlers* handlers, const cyop_device_usb_device* device) {
	int transferred = 0;
	uint8_t buffer[1];
	buffer[0] = 1;
	int ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, 1, 0xA0, 0xE600, 0, &transferred);
	if(ret < 0)
		return false;
	buffer[0] = 0;
	ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, 1, 0xA0, 0xE600, 0, &transferred);
	return ret >= 0;
}

static bool is_eeprom_data_valid(uint8_t* eeprom_data) {
	uint32_t crc = calc_crc32_adler(eeprom_data, OPTIMIZE_EEPROM_STRUCT_SIZE);
	return crc == read_le32(eeprom_data + 0x7C);
}

static std::string extract_key_from_eeprom(uint8_t* eeprom_data) {
	if(!is_eeprom_data_valid(eeprom_data))
		return "";
	if(eeprom_data[8] != 0x66) // Key ID byte
		return "";
	return read_string(eeprom_data + 0x10, OPTIMIZE_NUM_KEY_CHARS_WITH_DASHES);
}

static int read_device_eeprom(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint8_t* in_buffer, size_t read_size) {
	const size_t read_block_size = 0x10;
	const size_t num_reads = (read_size + read_block_size - 1) / read_block_size;
	int transferred = 0;
	int ret = 0;
	uint8_t first_out_buffer[] = { 0x38, 0x00, 0x10, 0x30 };
	for(size_t i = 0; i < num_reads; i++) {
		size_t single_read_size = read_size - (i * read_block_size);
		if(single_read_size > read_block_size)
			single_read_size = read_block_size;
		first_out_buffer[1] = (uint8_t)(i * read_block_size);
		ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), first_out_buffer, sizeof(first_out_buffer), &transferred);
		if(ret < 0)
			return ret;
		ret = cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), in_buffer + (i * read_block_size), (int)single_read_size, &transferred);
		if(ret < 0)
			return ret;
		if(transferred < ((int)single_read_size))
			return -1;
	}
	return ret;
}

static int write_device_eeprom(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint8_t* out_buffer, size_t write_size) {
	const size_t write_block_size = 0x10;
	const size_t num_writes = (write_size + write_block_size - 1) / write_block_size;
	int transferred = 0;
	int ret = 0;
	uint8_t out_post[] = { 0x39, 0x00, 0x10 };
	uint8_t out_pre[] = { 0x31 };
	uint8_t inside_out_buffer[sizeof(out_pre) + write_block_size + sizeof(out_post)];
	for(size_t i = 0; i < num_writes; i++) {
		size_t single_write_size = write_size - (i * write_block_size);
		if(single_write_size > write_block_size)
			single_write_size = write_block_size;
		out_post[1] = (uint8_t)(i * write_block_size);
		memcpy(inside_out_buffer, out_pre, sizeof(out_pre));
		memcpy(inside_out_buffer + sizeof(out_pre), out_buffer + ((int)(i * write_block_size)), single_write_size);
		memcpy(inside_out_buffer + sizeof(out_pre) + single_write_size, out_post, sizeof(out_post));
		size_t full_write_size = sizeof(out_pre) + single_write_size + sizeof(out_post);
		ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), inside_out_buffer, (int)full_write_size, &transferred);
		if(ret < 0)
			return ret;
		if(transferred < ((int)full_write_size))
			return -1;
	}
	return ret;
}

static uint64_t bytes_to_serial_device_id(uint8_t* in_buffer) {
	// From multiple dumps, this seems to be the algorithm.
	// There may be something missing.
	uint8_t in_transformed_buffer[sizeof(uint64_t)];

	for(size_t i = 0; i < (sizeof(in_transformed_buffer) - 1); i++)
		in_transformed_buffer[i] = (reverse_u8(in_buffer[sizeof(in_transformed_buffer) - 1 - i]) >> 7) | (reverse_u8(in_buffer[sizeof(in_transformed_buffer) - 1 - i - 1]) << 1);
	// Bugged?! Why doesn't it use in_buffer[7]'s first 7 bits?!
	in_transformed_buffer[sizeof(in_transformed_buffer) - 1] = (reverse_u8(in_buffer[0]) >> 7);

	return read_le64(in_transformed_buffer);
}

static int read_direct_serial_device_id(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint64_t &device_id) {
	const uint8_t first_out_buffer[] = { 0x60, 0x02, 0x30, 0xFF, 0x60, 0xD0, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xCB, 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF };
	uint8_t in_buffer[sizeof(uint64_t)];
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), first_out_buffer, sizeof(first_out_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t second_out_buffer[] = { 0x60, 0x02, 0x30, 0xFF, 0x60, 0xF1, 0x60, 0x01, 0x20, 0xFF, 0x61, 0x08, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x80, 0xFF, 0x60, 0x01, 0x01, 0xFF, 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), second_out_buffer, sizeof(second_out_buffer), &transferred);
	if(ret < 0)
		return ret;
	ret = cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), in_buffer, sizeof(in_buffer), &transferred);
	if(ret < 0)
		return ret;
	if(((size_t)transferred) < sizeof(in_buffer))
		return -1;

	device_id = bytes_to_serial_device_id(in_buffer);
	return ret;
}

static int read_cached_device_id(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint64_t &device_id, bool &is_full_0s) {
	const uint8_t out_buffer[] = { 0x70 };
	uint8_t in_buffer[0x10];
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), out_buffer, sizeof(out_buffer), &transferred);
	if(ret < 0)
		return ret;
	ret = cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), in_buffer, sizeof(in_buffer), &transferred);
	if(ret < 0)
		return ret;
	if(((size_t)transferred) < sizeof(in_buffer))
		return -1;
	device_id = read_le64(in_buffer);
	is_full_0s = true;
	for(size_t i = 0; i < sizeof(in_buffer); i++)
		if(in_buffer[i] != 0) {
			is_full_0s = false;
			break;
		}
	return ret;
}

static int read_first_unk_value(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint32_t &value) {
	const uint8_t out_buffer[] = { 0x60, 0x02, 0x30, 0xFF, 0x60, 0xC9, 0x60, 0x01, 0x20, 0xFF, 0x61, 0x04, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x80, 0xFF, 0x60, 0x01, 0x01, 0xFF };
	uint8_t in_buffer[sizeof(uint32_t)];
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), out_buffer, sizeof(out_buffer), &transferred);
	if(ret < 0)
		return ret;
	ret = cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), in_buffer, sizeof(in_buffer), &transferred);
	if(ret < 0)
		return ret;
	if(((size_t)transferred) < sizeof(in_buffer))
		return -1;
	value = read_le32(in_buffer);
	return ret;
}

static int start_command_send(cy_device_device_handlers* handlers, const cyop_device_usb_device* device) {
	int transferred = 0;
	const uint8_t mono_out_buffer [] = { 0x65 };
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), mono_out_buffer, sizeof(mono_out_buffer), &transferred);
	if(ret < 0)
		return ret;
	return ret;
}

static int read_device_id_serial(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint32_t &value32, uint64_t &device_id, bool is_first_load) {	
	const uint8_t first_buffer[] = { 0x64, 0x60, 0x01, 0xFF, 0xFF, 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF };
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), first_buffer, sizeof(first_buffer), &transferred);
	if(ret < 0)
		return ret;
	if(device->is_new_device) {
		ret = read_first_unk_value(handlers, device, value32);
		if(ret < 0)
			return ret;
		ret = read_first_unk_value(handlers, device, value32);
		if(ret < 0)
			return ret;
	}
	bool is_full_0s = false;
	ret = read_cached_device_id(handlers, device, device_id, is_full_0s);
	if(ret < 0)
		return ret;
	if(is_full_0s || is_first_load)
		ret = read_direct_serial_device_id(handlers, device, device_id);
	if(ret < 0)
		return ret;
	return ret;
}

static int fpga_pl_load(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint8_t* fpga_pl, size_t fpga_pl_size) {
	const uint8_t first_buffer[] = { 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xCB, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xC5, 0x66, 0x64, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xC5 };
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), first_buffer, sizeof(first_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t second_buffer[] = { 0x60, 0x01, 0x20, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), second_buffer, sizeof(second_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t third_buffer[] = { 0x60, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), third_buffer, sizeof(third_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t fourth_buffer[] = { 0x60, 0x01, 0x01, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), fourth_buffer, sizeof(fourth_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t fifth_buffer[] = { 0x60, 0x02, 0x30, 0xFF, 0x60, 0xC5 };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), fifth_buffer, sizeof(fifth_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t sixth_buffer[] = { 0x60, 0x01, 0x20, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), sixth_buffer, sizeof(sixth_buffer), &transferred);
	if(ret < 0)
		return ret;

	const size_t fpga_pl_transfer_total_size = 64;
	const size_t fpga_pl_transfer_header_size = 2;
	const size_t fpga_pl_transfer_piece_size = fpga_pl_transfer_total_size - fpga_pl_transfer_header_size;
	const size_t num_iters = (fpga_pl_size + fpga_pl_transfer_piece_size - 1) / fpga_pl_transfer_piece_size;
	uint8_t pl_buffer[fpga_pl_transfer_total_size];
	pl_buffer[0] = 0x60;
	pl_buffer[1] = 0x1F;
	for(size_t i = 0; i < num_iters; i++) {
		size_t remaining_size = fpga_pl_size - (fpga_pl_transfer_piece_size * i);
		if(remaining_size > fpga_pl_transfer_piece_size)
			remaining_size = fpga_pl_transfer_piece_size;
		memcpy(pl_buffer + fpga_pl_transfer_header_size, fpga_pl + (fpga_pl_transfer_piece_size * i), remaining_size);
		ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), pl_buffer, (int)(remaining_size + fpga_pl_transfer_header_size), &transferred);
		if(ret < 0)
			return ret;
	}

	const uint8_t seventh_buffer[] = { 0x60, 0x0B, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x80, 0x00 };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), seventh_buffer, sizeof(seventh_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t eight_buffer[] = { 0x60, 0x01, 0x01, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), eight_buffer, sizeof(eight_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t ninth_buffer[] = { 0x60, 0x02, 0x30, 0xFF, 0x60, 0xD6, 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xFF, 0x60, 0x01, 0x20, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), ninth_buffer, sizeof(ninth_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t tenth_buffer[] = { 0x60, 0x01, 0x80, 0x00 };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), tenth_buffer, sizeof(tenth_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t eleventh_buffer[] = { 0x60, 0x01, 0x01, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), eleventh_buffer, sizeof(eleventh_buffer), &transferred);
	if(ret < 0)
		return ret;
	return ret;
}

static int insert_device_id(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint64_t device_id) {
	const uint8_t first_buffer[] = { 0x60, 0x02, 0x30, 0xFF, 0x60, 0xCC, 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xFF, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xFF };
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), first_buffer, sizeof(first_buffer), &transferred);
	if(ret < 0)
		return ret;
	uint8_t device_id_buffer_new_3ds[] = {0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xC3, 0x00, 0x00, 0x02, 0x30, 0xFF, 0x60, 0x65 };
	uint8_t device_id_buffer_old_3ds[] = {0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x0B, 0x01, 0x00, 0x02, 0x30, 0xFF, 0x60, 0x65 };
	uint8_t* target_device_id_buffer = device_id_buffer_new_3ds;
	if(!device->is_new_device)
		target_device_id_buffer = device_id_buffer_old_3ds;
	write_le64(target_device_id_buffer + 1, device_id);
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), target_device_id_buffer, sizeof(device_id_buffer_new_3ds), &transferred);
	if(ret < 0)
		return ret;
	return ret;
}

static int final_capture_start_transfer(cy_device_device_handlers* handlers, const cyop_device_usb_device* device) {
	const uint8_t first_buffer[] = { 0x5B, 0x59, 0x03 };
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), first_buffer, sizeof(first_buffer), &transferred);
	if(ret < 0)
		return ret;
	return ret;
}

int capture_start(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, bool is_first_load, bool is_rgb888, uint64_t &device_id, std::string &read_key) {
	int ret = 0;
	uint32_t value32 = 0;
	if(is_first_load) {
		for(int i = 0; i < 3; i++) {
			cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
			cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device));
		}
		ret = read_device_id_serial(handlers, device, value32, device_id, true);
		if(ret < 0)
			return ret;
	}
	ret = start_command_send(handlers, device);
	if(ret < 0)
		return ret;
	if(device->is_new_device) {
		uint8_t eeprom_data[OPTIMIZE_EEPROM_NEW_SIZE];
		ret = read_device_eeprom(handlers, device, eeprom_data, OPTIMIZE_EEPROM_NEW_SIZE);
		if(ret < 0)
			return ret;
		read_key = extract_key_from_eeprom(eeprom_data);
	}
	cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
	cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device));
	ret = read_device_id_serial(handlers, device, value32, device_id, is_first_load);
	if(ret < 0)
		return ret;

	uint8_t* pl_to_load = device->fpga_pl_565;
	size_t pl_len = device->fpga_pl_565_size;
	if(is_rgb888) {
		pl_to_load = device->fpga_pl_888;
		pl_len = device->fpga_pl_888_size;
	}
	ret = fpga_pl_load(handlers, device, pl_to_load, pl_len);
	if(ret < 0)
		return ret;

	ret = insert_device_id(handlers, device, device_id);
	if(ret < 0)
		return ret;

	ret = final_capture_start_transfer(handlers, device);
	if(ret < 0)
		return ret;
	cypress_pipe_reset_bulk_in(handlers, get_cy_usb_info(device));
	return ret;
}

static const uint8_t* get_start_capture_setup_key_buffer(const cyop_device_usb_device* device, bool is_rgb888, bool is_3d) {
	if(device->is_new_device) {
		if(is_rgb888) {
			if(is_3d)
				return start_capture_setup_key_buffer_888_3d_new;
			return start_capture_setup_key_buffer_888_new;
		}
		if(is_3d)
			return start_capture_setup_key_buffer_565_3d_new;
		return start_capture_setup_key_buffer_565_new;
	}
	if(is_rgb888) {
		if(is_3d)
			return start_capture_setup_key_buffer_888_3d_old;
		return start_capture_setup_key_buffer_888_old;
	}
	if(is_3d)
		return start_capture_setup_key_buffer_565_3d_old;
	return start_capture_setup_key_buffer_565_old;
}

uint64_t get_device_id_from_key(std::string key, bool is_new_device) {
	uint8_t key_bytes[OPTIMIZE_NUM_KEY_BYTES];
	bool is_key_valid = key_to_data(key, key_bytes, is_new_device);
	if(!is_key_valid)
		return 0;
	return read_be64(key_bytes);
}

bool check_key_matches_device_id(uint64_t device_id, std::string key, bool is_new_device) {
	uint64_t device_id_read = get_device_id_from_key(key, is_new_device);
	if(device_id_read == 0)
		return false;
	return device_id == device_id_read;
}

bool check_key_matches_device_id(uint64_t device_id, std::string key, const cyop_device_usb_device* device) {
	if(device == NULL)
		return false;
	return check_key_matches_device_id(device_id, key, device->is_new_device);
}

bool check_key_valid(std::string key, bool is_new_device) {
	uint8_t key_bytes[OPTIMIZE_NUM_KEY_BYTES];
	bool is_key_valid = key_to_data(key, key_bytes, is_new_device);
	// Extra checks here, if I ever feel like it.
	return is_key_valid;
}

bool check_key_valid(std::string key, const cyop_device_usb_device* device) {
	if(device == NULL)
		return false;
	return check_key_valid(key, device->is_new_device);
}

static void populate_capture_setup_key_buffer(const cyop_device_usb_device* device, uint8_t* setup_key_buffer, std::string key) {
	if(key == "")
		return;

	uint8_t key_bytes[OPTIMIZE_NUM_KEY_BYTES];
	bool is_key_valid = key_to_data(key, key_bytes, device->is_new_device);

	if(!is_key_valid)
		return;

	if(device->is_new_device) {
		setup_key_buffer[11] = key_bytes[11];
		setup_key_buffer[13] = key_bytes[10];
		return;
	}
	setup_key_buffer[11] |= (key_bytes[11] & 0xF) << 4;
	setup_key_buffer[13] = ((key_bytes[10] & 0xF) << 4) | (key_bytes[11] >> 4);
	setup_key_buffer[15] = key_bytes[10] >> 4;
}

int StartCaptureDma(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, bool is_rgb888, bool is_3d, std::string key) {
	const uint8_t mono_buffer[] = { 0x40 };
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), mono_buffer, sizeof(mono_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t second_buffer[] = { 0x64, 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xC2, 0x60, 0x01, 0x20, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), second_buffer, sizeof(second_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t* base_setup_key_buffer = get_start_capture_setup_key_buffer(device, is_rgb888, is_3d);
	uint8_t setup_key_buffer[sizeof(start_capture_setup_key_buffer_888_new)];
	memcpy(setup_key_buffer, base_setup_key_buffer, sizeof(setup_key_buffer));
	populate_capture_setup_key_buffer(device, setup_key_buffer, key);
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), setup_key_buffer, sizeof(setup_key_buffer), &transferred);
	if(ret < 0)
		return ret;
	uint8_t in_buffer[7];
	ret = cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), in_buffer, sizeof(in_buffer), &transferred);
	if(ret < 0)
		return ret;
	ret = start_command_send(handlers, device);
	if(ret < 0)
		return ret;
	return ret;
}

int capture_end(cy_device_device_handlers* handlers, const cyop_device_usb_device* device) {
	int transferred = 0;
	uint8_t buffer[] = { 0x41 };
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), buffer, sizeof(buffer), &transferred);
	if(ret < 0)
		return ret;

	cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
	cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device));
	return ret;
}

int ReadFrame(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cyop_device_usb_device* device_desc) {
	// Maybe making this async would be better for lower end hardware...
	int num_bytes = 0;
	int ret = cypress_bulk_in_transfer(handlers, get_cy_usb_info(device_desc), buf, length, &num_bytes);
	if(num_bytes != length)
		return LIBUSB_ERROR_INTERRUPTED;
	return ret;
}

int ReadFrameAsync(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cyop_device_usb_device* device_desc, cy_async_callback_data* cb_data) {
	return cypress_bulk_in_async(handlers, get_cy_usb_info(device_desc), buf, length, cb_data);
}

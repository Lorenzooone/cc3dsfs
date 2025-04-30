#include "frontend.hpp"
#include "cypress_optimize_3ds_communications.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_optimize_3ds_acquisition_general.hpp"
#include "usb_generic.hpp"

#include "optimize_new_3ds_fw.h"
#include "optimize_new_3ds_565_fpga_pl.h"
#include "optimize_new_3ds_888_fpga_pl.h"

#include "optimize_old_3ds_fw.h"
#include "optimize_old_3ds_565_fpga_pl.h"
#include "optimize_old_3ds_888_fpga_pl.h"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

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

const uint8_t start_capture_third_buffer_565_new[] = { 0x61, 0x07, 0x00, 0x0F, 0x00, 0x3E, 0x00, 0xF8, 0x00, 0x38, 0x00, 0x51, 0x80, 0x5C, 0x01, 0x00 };
const uint8_t start_capture_third_buffer_888_new[] = { 0x61, 0x07, 0x00, 0xA6, 0x00, 0x28, 0x00, 0x62, 0x00, 0x3A, 0x00, 0x51, 0x80, 0x5C, 0x01, 0x00 };
const uint8_t start_capture_third_buffer_565_old[] = { 0x61, 0x07, 0x00, 0x26, 0x00, 0x22, 0x00, 0x98, 0x00, 0x08, 0x00, 0xDE, 0x00, 0xF9, 0x18, 0x0F };
const uint8_t start_capture_third_buffer_888_old[] = { 0x61, 0x07, 0x00, 0xA6, 0x00, 0x20, 0x00, 0x98, 0x00, 0x08, 0x00, 0xDE, 0x00, 0xF9, 0x18, 0x0F };

static const cyop_device_usb_device cypress_optimize_new_3ds_generic_device = {
.name = "Optimize New 3DS", .long_name = "Optimize New 3DS",
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
	.get_serial_fn = cypress_optimize_3ds_get_serial,
	.create_device_fn = cypress_optimize_3ds_create_device,
	.bcd_device_mask = 0x0000,
	.bcd_device_wanted_value = 0x0000
}
};

static const cyop_device_usb_device cypress_optimize_new_3ds_instantiated_device = {
.name = "Optimize New 3DS", .long_name = "Optimize New 3DS",
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
	.get_serial_fn = cypress_optimize_3ds_get_serial,
	.create_device_fn = cypress_optimize_3ds_create_device,
	.bcd_device_mask = 0xFF00,
	.bcd_device_wanted_value = OPTIMIZE_NEW_3DS_WANTED_VALUE_BASE
}
};

static const cyop_device_usb_device cypress_optimize_old_3ds_generic_device = {
.name = "Optimize Old 3DS", .long_name = "Optimize Old 3DS",
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
	.get_serial_fn = cypress_optimize_3ds_get_serial,
	.create_device_fn = cypress_optimize_3ds_create_device,
	.bcd_device_mask = 0x0000,
	.bcd_device_wanted_value = 0x0000
}
};

static const cyop_device_usb_device cypress_optimize_old_3ds_instantiated_device = {
.name = "Optimize Old 3DS", .long_name = "Optimize Old 3DS",
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
	if(ret < 0)
		return free_firmware_and_return(fw_data, false);
	return free_firmware_and_return(fw_data, true);
}

static int read_device_info(cy_device_device_handlers* handlers, const cyop_device_usb_device* device) {
	const int read_block_size = 0x10;
	const int num_reads = 8;
	uint8_t in_buffer[read_block_size * num_reads];
	int transferred = 0;
	int ret = 0;
	uint8_t first_out_buffer[] = { 0x38, 0x00, 0x10, 0x30 };
	for(int i = 0; i < num_reads; i++) {
		first_out_buffer[1] = i * read_block_size;
		ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), first_out_buffer, sizeof(first_out_buffer), &transferred);
		if(ret < 0)
			return ret;
		ret = cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), in_buffer + (i * read_block_size), read_block_size, &transferred);
		if(ret < 0)
			return ret;
		if(transferred < read_block_size)
			return -1;
	}
	return ret;
}

static int read_second_device_id(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint64_t &second_device_id) {
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
	second_device_id = read_le64(in_buffer);
	return ret;
}

static int read_device_id(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint64_t &device_id, bool &is_full_0s) {
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

static int dual_first_unk_value_setup_read(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, uint32_t &value32, uint64_t &device_id, uint64_t &second_device_id) {
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
	ret = read_device_id(handlers, device, device_id, is_full_0s);
	if(ret < 0)
		return ret;
	if(is_full_0s)
		ret = read_second_device_id(handlers, device, second_device_id);
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

int capture_start(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, bool is_first_load, bool is_rgb888) {
	int ret = 0;
	uint32_t value32 = 0;
	uint64_t device_id = 0;
	uint64_t second_device_id = 0;
	if(is_first_load) {
		for(int i = 0; i < 3; i++) {
			cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
			cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device));
		}
		ret = dual_first_unk_value_setup_read(handlers, device, value32, device_id, second_device_id);
		if(ret < 0)
			return ret;
	}
	ret = start_command_send(handlers, device);
	if(ret < 0)
		return ret;
	if(device->is_new_device) {
		ret = read_device_info(handlers, device);
		if(ret < 0)
			return ret;
	}
	cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
	cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device));
	ret = dual_first_unk_value_setup_read(handlers, device, value32, device_id, second_device_id);
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

	// No clue about the algorithm to determine this...
	// Would need different examples to understand it.
	device_id = 0;
	ret = insert_device_id(handlers, device, device_id);
	if(ret < 0)
		return ret;

	ret = final_capture_start_transfer(handlers, device);
	if(ret < 0)
		return ret;
	cypress_pipe_reset_bulk_in(handlers, get_cy_usb_info(device));
	return ret;
}

static const uint8_t* get_start_capture_third_buffer(const cyop_device_usb_device* device, bool is_rgb888) {
	if(device->is_new_device) {
		if(is_rgb888)
			return start_capture_third_buffer_888_new;
		return start_capture_third_buffer_565_new;
	}
	if(is_rgb888)
		return start_capture_third_buffer_888_old;
	return start_capture_third_buffer_565_old;
}

int StartCaptureDma(cy_device_device_handlers* handlers, const cyop_device_usb_device* device, bool is_rgb888, bool is_3d) {
	const uint8_t mono_buffer[] = { 0x40 };
	int transferred = 0;
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), mono_buffer, sizeof(mono_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t second_buffer[] = { 0x64, 0x60, 0x02, 0x00, 0xFF, 0x00, 0xFF, 0x60, 0x02, 0x30, 0xFF, 0x60, 0xC2, 0x60, 0x01, 0x20, 0xFF };
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), second_buffer, sizeof(second_buffer), &transferred);
	if(ret < 0)
		return ret;
	const uint8_t* third_buffer = get_start_capture_third_buffer(device, is_rgb888);
	uint8_t real_third_buffer[sizeof(start_capture_third_buffer_888_new)];
	memcpy(real_third_buffer, third_buffer, sizeof(real_third_buffer));
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), real_third_buffer, sizeof(real_third_buffer), &transferred);
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

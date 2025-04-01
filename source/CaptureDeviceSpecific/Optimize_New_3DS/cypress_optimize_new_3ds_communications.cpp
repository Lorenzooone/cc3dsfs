#include "frontend.hpp"
#include "cypress_optimize_new_3ds_communications.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_optimize_new_3ds_acquisition_general.hpp"
#include "usb_generic.hpp"

#include "optimize_new_3ds_fw.h"
#include "optimize_new_3ds_565_fpga_pl.h"
#include "optimize_new_3ds_888_fpga_pl.h"

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

static const cyopn_device_usb_device cypress_optimize_new_3ds_generic_device = {
.name = "EZ-USB Optimize New 3DS", .long_name = "EZ-USB Optimize New 3DS",
.device_type = CYPRESS_OPTIMIZE_NEW_3DS_BLANK_DEVICE,
.firmware_to_load = optimize_new_3ds_fw, .firmware_size = optimize_new_3ds_fw_len,
.next_device = CYPRESS_OPTIMIZE_NEW_3DS_BLANK_DEVICE,
.has_bcd_device_serial = false,
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
	.get_serial_fn = cypress_optimize_new_3ds_get_serial,
	.create_device_fn = cypress_optimize_new_3ds_create_device
}
};

static const cyopn_device_usb_device* all_usb_cyopn_device_devices_desc[] = {
	&cypress_optimize_new_3ds_generic_device,
};

const cy_device_usb_device* get_cy_usb_info(const cyopn_device_usb_device* usb_device_desc) {
	return &usb_device_desc->usb_device_info;
}

int GetNumCyOpNDeviceDesc() {
	return sizeof(all_usb_cyopn_device_devices_desc) / sizeof(all_usb_cyopn_device_devices_desc[0]);
}

const cyopn_device_usb_device* GetCyOpNDeviceDesc(int index) {
	if((index < 0) || (index >= GetNumCyOpNDeviceDesc()))
		index = 0;
	return all_usb_cyopn_device_devices_desc[index];
}

const cyopn_device_usb_device* GetNextDeviceDesc(const cyopn_device_usb_device* device) {
	for(int i = 0; i < GetNumCyOpNDeviceDesc(); i++) {
		const cyopn_device_usb_device* examined_device = GetCyOpNDeviceDesc(i);
		if(examined_device->device_type == device->next_device)
			return examined_device;
	}
	return NULL;
}

bool has_to_load_firmware(const cyopn_device_usb_device* device) {
	return !((device->firmware_to_load == NULL) || (device->firmware_size == 0));
}

static bool free_firmware_and_return(uint8_t* fw_data, bool retval) {
	delete []fw_data;
	return retval;
}

bool load_firmware(cy_device_device_handlers* handlers, const cyopn_device_usb_device* device, uint8_t patch_id) {
	if(!has_to_load_firmware(device))
		return true;
	int transferred = 0;
	uint8_t* fw_data = new uint8_t[device->firmware_size];
	memcpy(fw_data, device->firmware_to_load, device->firmware_size);
	int num_patches = read_le16(fw_data, 1);
	for(int i = 0; i < num_patches; i++) {
		int pos_patch = read_le16(fw_data, 2 + i);
		write_le16(fw_data + pos_patch, patch_id | 0xFF00);
	}
	uint8_t buffer[0x8000];
	buffer[0] = 1;
	int ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, 1, 0xA0, 0xE600, 0, &transferred);
	if(ret < 0)
		return free_firmware_and_return(fw_data, false);
	bool done = false;
	int fw_pos = read_le16(fw_data);
	while(!done) {
		int offset = read_le16(fw_data + fw_pos);
		int index = read_le16(fw_data + fw_pos, 1);
		int inside_len = read_le16(fw_data + fw_pos, 2);
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

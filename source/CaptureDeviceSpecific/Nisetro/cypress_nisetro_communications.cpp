#include "frontend.hpp"
#include "cypress_nisetro_communications.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_nisetro_acquisition_general.hpp"
#include "nisetro_ds_fw.h"
#include "usb_generic.hpp"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define CYPRESS_BASE_USB_PACKET_LIMIT 64
#define CYPRESS_NISETRO_USB_PACKET_LIMIT 512

#define CMD_EP6IN_START     0x50
#define CMD_EP6IN_STOP      0x51
#define CMD_EP2OUT_START    0x52
#define CMD_EP2OUT_STOP     0x53
#define CMD_PORT_CFG        0x54
#define CMD_REG_READ        0x55
#define CMD_REG_WRITE       0x56
#define CMD_PORT_READ       0x57
#define CMD_PORT_WRITE      0x58
#define CMD_IFCONFIG        0x59
#define CMD_MODE_IDLE       0x5a

#define PIO_DROP    0x20
#define PIO_RESET   0x10
#define PIO_DIR     0x08

#define NISETRO_DS_WANTED_VALUE_BASE 0xFF00

static const cyni_device_usb_device cypress_fx2_generic_device = {
.name = "FX2LP -> NiseTro DS", .long_name = "EZ-USB FX2LP ->NiseTro DS(i)",
.device_type = CYPRESS_NISETRO_BLANK_DEVICE,
.video_data_type = VIDEO_DATA_RGB, 
.firmware_to_load = nisetro_ds_fw, .firmware_size = nisetro_ds_fw_len,
.next_device = CYPRESS_NISETRO_DS_DEVICE,
.has_bcd_device_serial = false,
.index_in_allowed_scan = CC_NISETRO_DS,
.usb_device_info = {
	.vid = 0x04B4, .pid = 0x8613,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 0, .ep_ctrl_bulk_out = 0,
	.ep_bulk_in = 0,
	.max_usb_packet_size = CYPRESS_BASE_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = false,
	.alt_interface = 1,
	.full_data = &cypress_fx2_generic_device,
	.get_serial_fn = cypress_nisetro_get_serial,
	.create_device_fn = cypress_nisetro_create_device,
	.bcd_device_mask = 0x0000,
	.bcd_device_wanted_value = 0x0000
}
};

static const cyni_device_usb_device cypress_fx2_nisetro_ds_device = {
.name = "Nisetro DS(i)", .long_name = "Nisetro DS(i)",
.device_type = CYPRESS_NISETRO_DS_DEVICE,
.video_data_type = VIDEO_DATA_RGB,
.firmware_to_load = NULL, .firmware_size = 0,
.next_device = CYPRESS_NISETRO_DS_DEVICE,
.has_bcd_device_serial = true,
.index_in_allowed_scan = CC_NISETRO_DS,
.usb_device_info = {
	.vid = 0x04B4, .pid = 0x1004,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 1 | LIBUSB_ENDPOINT_IN, .ep_ctrl_bulk_out = 1 | LIBUSB_ENDPOINT_OUT,
	.ep_bulk_in = 6 | LIBUSB_ENDPOINT_IN,
	.max_usb_packet_size = CYPRESS_NISETRO_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = true,
	.alt_interface = 0,
	.full_data = &cypress_fx2_nisetro_ds_device,
	.get_serial_fn = cypress_nisetro_get_serial,
	.create_device_fn = cypress_nisetro_create_device,
	.bcd_device_mask = 0xFF00,
	.bcd_device_wanted_value = NISETRO_DS_WANTED_VALUE_BASE
}
};

static const cyni_device_usb_device* all_usb_cyni_device_devices_desc[] = {
	&cypress_fx2_generic_device,
	&cypress_fx2_nisetro_ds_device,
};

const cy_device_usb_device* get_cy_usb_info(const cyni_device_usb_device* usb_device_desc) {
	return &usb_device_desc->usb_device_info;
}

int GetNumCyNiDeviceDesc() {
	return sizeof(all_usb_cyni_device_devices_desc) / sizeof(all_usb_cyni_device_devices_desc[0]);
}

const cyni_device_usb_device* GetCyNiDeviceDesc(int index) {
	if((index < 0) || (index >= GetNumCyNiDeviceDesc()))
		index = 0;
	return all_usb_cyni_device_devices_desc[index];
}

const cyni_device_usb_device* GetNextDeviceDesc(const cyni_device_usb_device* device) {
	for(int i = 0; i < GetNumCyNiDeviceDesc(); i++) {
		const cyni_device_usb_device* examined_device = GetCyNiDeviceDesc(i);
		if(examined_device->device_type == device->next_device)
			return examined_device;
	}
	return NULL;
}

bool has_to_load_firmware(const cyni_device_usb_device* device) {
	return !((device->firmware_to_load == NULL) || (device->firmware_size == 0));
}

static bool free_firmware_and_return(uint8_t* fw_data, bool retval) {
	delete []fw_data;
	return retval;
}

bool load_firmware(cy_device_device_handlers* handlers, const cyni_device_usb_device* device, uint8_t patch_id) {
	if(!has_to_load_firmware(device))
		return true;
	int transferred = 0;
	uint8_t* fw_data = new uint8_t[device->firmware_size];
	memcpy(fw_data, device->firmware_to_load, device->firmware_size);
	int num_patches = read_le16(fw_data, 1);
	for(int i = 0; i < num_patches; i++) {
		int pos_patch = read_le16(fw_data, 2 + i);
		write_le16(fw_data + pos_patch, patch_id | NISETRO_DS_WANTED_VALUE_BASE);
	}
	uint8_t buffer[0x8000];
	buffer[0] = 1;
	int ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, 1, 0xA0, 0xE600, 0, &transferred);
	if(ret < 0)
		return free_firmware_and_return(fw_data, false);
	bool done = false;
	uint16_t fw_pos = read_le16(fw_data);
	while(!done) {
		uint32_t inside_len = read_be16(fw_data + fw_pos);
		int offset = read_be16(fw_data + fw_pos, 1);
		done = (inside_len & 0x8000) != 0;
		inside_len &= 0x7FFF;
		fw_pos += 4;
		if((inside_len + fw_pos) > device->firmware_size)
			return free_firmware_and_return(fw_data, false);
		memcpy(buffer, fw_data + fw_pos, inside_len);
		fw_pos += inside_len;
		ret = cypress_ctrl_out_transfer(handlers, get_cy_usb_info(device), buffer, inside_len, 0xA0, offset, 0, &transferred);
		if(ret < 0)
			return free_firmware_and_return(fw_data, false);
	}
	return free_firmware_and_return(fw_data, true);
}

int capture_start(cy_device_device_handlers* handlers, const cyni_device_usb_device* device) {
	uint8_t buffer[] = { CMD_PORT_CFG, 0x07, PIO_RESET | PIO_DIR, CMD_MODE_IDLE, CMD_IFCONFIG, 0xE3, CMD_REG_WRITE, 0, 0x08, CMD_MODE_IDLE};
	int transferred = 0;
	cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
	cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device));
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), buffer, sizeof(buffer), &transferred);
	if(ret < 0)
		return ret;
	cypress_pipe_reset_bulk_in(handlers, get_cy_usb_info(device));
	return ret;
}

int StartCaptureDma(cy_device_device_handlers* handlers, const cyni_device_usb_device* device) {
	uint8_t buffer[] = { CMD_PORT_WRITE, PIO_RESET, CMD_EP6IN_START, CMD_PORT_WRITE, 0};
	int transferred = 0;
	return cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), buffer, sizeof(buffer), &transferred);
}

int capture_end(cy_device_device_handlers* handlers, const cyni_device_usb_device* device) {
	int transferred = 0;
	uint8_t buffer[] = { CMD_PORT_WRITE, 0, CMD_EP6IN_STOP, CMD_MODE_IDLE};
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), buffer, sizeof(buffer), &transferred);
	if(ret < 0)
		return ret;

	uint8_t buffer2[] = { CMD_REG_READ, 0, CMD_REG_READ, 1, CMD_REG_READ, 2, CMD_REG_READ, 3, CMD_REG_READ, 4, CMD_REG_READ, 5, CMD_REG_READ, 6, CMD_REG_READ, 7};
	ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), buffer2, sizeof(buffer2), &transferred);
	if(ret < 0)
		return ret;

	cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
	return cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), buffer2, 8, &transferred);
}

int ReadFrame(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cyni_device_usb_device* device_desc) {
	// Maybe making this async would be better for lower end hardware...
	int num_bytes = 0;
	int ret = cypress_bulk_in_transfer(handlers, get_cy_usb_info(device_desc), buf, length, &num_bytes);
	if(num_bytes != length)
		return LIBUSB_ERROR_INTERRUPTED;
	return ret;
}

int ReadFrameAsync(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cyni_device_usb_device* device_desc, cy_async_callback_data* cb_data) {
	return cypress_bulk_in_async(handlers, get_cy_usb_info(device_desc), buf, length, cb_data);
}

#include "frontend.hpp"
#include "cypress_partner_ctr_communications.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_partner_ctr_acquisition_general.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define CYPRESS_PARTNER_CTR_USB_PACKET_LIMIT 512

#define PARTNER_CTR_OUT_GET_SERIAL 0x5F

#define PARTNER_CTR_SERIAL_SIZE 8

static const cypart_device_usb_device cypress_partner_ctr_device = {
.name = "P.CTR-Cap", .long_name = "Partner CTR Capture",
.video_data_type = VIDEO_DATA_BGR,
.index_in_allowed_scan = CC_PARTNER_CTR,
.ep_ctrl_serial_io_pipe = 1,
.usb_device_info = {
	.vid = 0x0ED2, .pid = 0x0004,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 4 | LIBUSB_ENDPOINT_IN, .ep_ctrl_bulk_out = 2 | LIBUSB_ENDPOINT_OUT,
	.ep_bulk_in = 6 | LIBUSB_ENDPOINT_IN,
	.max_usb_packet_size = CYPRESS_PARTNER_CTR_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = true,
	.alt_interface = 0,
	.full_data = &cypress_partner_ctr_device,
	.get_serial_requires_setup = true,
	.get_serial_fn = cypress_partner_ctr_get_serial,
	.create_device_fn = cypress_partner_ctr_create_device,
	.bcd_device_mask = 0x0000,
	.bcd_device_wanted_value = 0x0000
}
};

static const cypart_device_usb_device cypress_partner_ctr2_device = {
.name = "P.CTR-Cap2", .long_name = "Partner CTR Capture 2",
.video_data_type = VIDEO_DATA_BGR,
.index_in_allowed_scan = CC_PARTNER_CTR,
.ep_ctrl_serial_io_pipe = 1,
.usb_device_info = {
	.vid = 0x0ED2, .pid = 0x000B,
	.default_config = 1, .default_interface = 0,
	.bulk_timeout = 500,
	.ep_ctrl_bulk_in = 4 | LIBUSB_ENDPOINT_IN, .ep_ctrl_bulk_out = 2 | LIBUSB_ENDPOINT_OUT,
	.ep_bulk_in = 6 | LIBUSB_ENDPOINT_IN,
	.max_usb_packet_size = CYPRESS_PARTNER_CTR_USB_PACKET_LIMIT,
	.do_pipe_clear_reset = true,
	.alt_interface = 0,
	.full_data = &cypress_partner_ctr2_device,
	.get_serial_requires_setup = true,
	.get_serial_fn = cypress_partner_ctr_get_serial,
	.create_device_fn = cypress_partner_ctr_create_device,
	.bcd_device_mask = 0x0000,
	.bcd_device_wanted_value = 0x0000
}
};

static const cypart_device_usb_device* all_usb_cypart_device_devices_desc[] = {
	&cypress_partner_ctr_device,
	&cypress_partner_ctr2_device,
};

const cy_device_usb_device* get_cy_usb_info(const cypart_device_usb_device* usb_device_desc) {
	return &usb_device_desc->usb_device_info;
}

int GetNumCyPartnerCTRDeviceDesc() {
	return sizeof(all_usb_cypart_device_devices_desc) / sizeof(all_usb_cypart_device_devices_desc[0]);
}

const cypart_device_usb_device* GetCyPartnerCTRDeviceDesc(int index) {
	if((index < 0) || (index >= GetNumCyPartnerCTRDeviceDesc()))
		index = 0;
	return all_usb_cypart_device_devices_desc[index];
}

std::string read_serial_ctr_capture(cy_device_device_handlers* handlers, const cypart_device_usb_device* device) {
	uint8_t buffer[] = { PARTNER_CTR_OUT_GET_SERIAL, 0};
	uint8_t buffer_in[PARTNER_CTR_SERIAL_SIZE];
	int transferred = 0;
	cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device), device->ep_ctrl_serial_io_pipe | LIBUSB_ENDPOINT_OUT);
	cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device), device->ep_ctrl_serial_io_pipe | LIBUSB_ENDPOINT_IN);
	int ret = cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), buffer, sizeof(buffer), &transferred, device->ep_ctrl_serial_io_pipe | LIBUSB_ENDPOINT_OUT);
	if(ret < 0)
		return "";
	ret = cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), buffer_in, sizeof(buffer_in), &transferred, device->ep_ctrl_serial_io_pipe | LIBUSB_ENDPOINT_IN);
	if(ret < 0)
		return "";
	if(transferred != sizeof(buffer_in))
		return "";

	uint16_t serial_part_1 = read_be16(buffer_in + 2);
	return std::to_string(serial_part_1) + "-" + read_string(buffer_in + 5, 3);
}

int capture_start(cy_device_device_handlers* handlers, const cypart_device_usb_device* device) {
	return 0;
}

int StartCaptureDma(cy_device_device_handlers* handlers, const cypart_device_usb_device* device) {
	return 0;
}

int capture_end(cy_device_device_handlers* handlers, const cypart_device_usb_device* device) {
	return 0;
}

int ReadFrame(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cypart_device_usb_device* device_desc) {
	return 0;
}

int ReadFrameAsync(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cypart_device_usb_device* device_desc, cy_async_callback_data* cb_data) {
	return 0;
}

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

static int send_partner_ctr_packet(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, const uint8_t* data, size_t size_data, int &transferred) {
	cypress_pipe_reset_ctrl_bulk_out(handlers, get_cy_usb_info(device));
	return cypress_ctrl_bulk_out_transfer(handlers, get_cy_usb_info(device), data, size_data, &transferred);
}

static int recv_partner_ctr_packet(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint8_t* data, size_t size_data, int &transferred) {
	cypress_pipe_reset_ctrl_bulk_in(handlers, get_cy_usb_info(device));
	return cypress_ctrl_bulk_in_transfer(handlers, get_cy_usb_info(device), data, size_data, &transferred);
}

// Name is tentative, based on pattern observed in wireshark packets...
static int write_to_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, const uint8_t* data, size_t data_size, uint8_t data_size_byte) {
	int transferred = 0;
	const size_t out_data_size = 6 + data_size;
	uint8_t* buffer_out = new uint8_t[out_data_size];

	buffer_out[0] = 0x43;
	buffer_out[1] = data_size_byte;
	write_le32(buffer_out + 2, address);
	memcpy(buffer_out + 6, data, data_size);

	int ret = send_partner_ctr_packet(handlers, device, buffer_out, out_data_size, transferred);
	delete []buffer_out;
	if(ret < 0)
		return ret;
	if(transferred < ((int)out_data_size))
		return -1;
	return ret;
}

// No packet seems to be sent with this pattern?! Would need to be checked
// to be sure...
static int write_u8_to_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint8_t value) {
	return write_to_address_partner_ctr(handlers, device, address, &value, sizeof(value), 1);
}

static int write_u16_to_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint16_t value) {
	uint8_t buffer[sizeof(value)];
	write_le16(buffer, value);
	return write_to_address_partner_ctr(handlers, device, address, buffer, sizeof(value), 0);
}

static int write_u32_to_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint32_t value) {
	uint8_t buffer[sizeof(value)];
	write_le32(buffer, value);
	return write_to_address_partner_ctr(handlers, device, address, buffer, sizeof(value), 2);
}

// No packet seems to be sent with this pattern?! Based on read packets...
static int write_u64_plus_to_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, const uint8_t* data, size_t data_size) {
	return write_to_address_partner_ctr(handlers, device, address, data, data_size, ((data_size >> 3) << 4) | 2);
}

// Name is tentative, based on pattern observed in wireshark packets...
static int read_from_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint8_t* data, size_t data_size, uint8_t data_size_byte) {
	int transferred = 0;
	const size_t out_data_size = 6;
	uint8_t buffer_out[out_data_size];

	buffer_out[0] = 0x42;
	buffer_out[1] = data_size_byte;
	write_le32(buffer_out + 2, address);

	int ret = send_partner_ctr_packet(handlers, device, buffer_out, out_data_size, transferred);
	if(ret < 0)
		return ret;
	if(transferred < ((int)out_data_size))
		return -1;

	ret = recv_partner_ctr_packet(handlers, device, data, data_size, transferred);
	if(ret < 0)
		return ret;
	if(transferred < ((int)data_size))
		return -1;
	return ret;
}

// No packet seems to be read with this pattern?! Would need to be checked
// to be sure...
static int read_u8_from_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint8_t &value) {
	return read_from_address_partner_ctr(handlers, device, address, &value, sizeof(value), 1);
}

static int read_u16_from_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint16_t &value) {
	uint8_t buffer[sizeof(value)];
	int ret = read_from_address_partner_ctr(handlers, device, address, buffer, sizeof(value), 0);
	value = read_le16(buffer);
	return ret;
}

static int read_u32_from_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint32_t &value) {
	uint8_t buffer[sizeof(value)];
	int ret = read_from_address_partner_ctr(handlers, device, address, buffer, sizeof(value), 2);
	value = read_le32(buffer);
	return ret;
}

static int read_u64_plus_from_address_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t address, uint8_t* data, size_t data_size) {
	return read_from_address_partner_ctr(handlers, device, address, data, data_size, ((data_size >> 3) << 4) | 2);
}

static int read_partner_ctr_status(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint32_t &status) {
	return read_u32_from_address_partner_ctr(handlers, device, 0xF0000000, status);
}

// Name is tentative, especially for this one...
static int read_fw_info_partner_ctr(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint8_t info_kind, uint32_t &value) {
	int transferred = 0;
	uint8_t buffer_out[4] = {0x20, 0x01, 0x00, 0x00};
	uint8_t buffer_in[4];

	buffer_out[3] = info_kind;

	int ret = send_partner_ctr_packet(handlers, device, buffer_out, 4, transferred);
	if(ret < 0)
		return ret;
	if(transferred < 4)
		return -1;

	ret = recv_partner_ctr_packet(handlers, device, buffer_in, 4, transferred);
	if(ret < 0)
		return ret;
	if(transferred < 4)
		return -1;
	value = read_le32(buffer_in);
	return ret;
}

// Seems to be some state, maybe...? Influenced by the writes, which seem to
// select stuff...?
static int capture_start_unknown_read_0x1002(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint16_t base_value, uint16_t second_value, uint16_t &unk16) {
	int ret = 0;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8001002, base_value);
	if(ret < 0)
		return ret;
	ret = read_u16_from_address_partner_ctr(handlers, device, 0xF8001001, unk16);
	if(ret < 0)
		return ret;
	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8001002, second_value);
	if(ret < 0)
		return ret;
	ret = read_u16_from_address_partner_ctr(handlers, device, 0xF8001001, unk16);
	if(ret < 0)
		return ret;
	ret = read_u16_from_address_partner_ctr(handlers, device, 0xF8001003, unk16);
	if(ret < 0)
		return ret;

	// Resets position...?
	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8001000, 0x001D);
	if(ret < 0)
		return ret;

	return ret;
}

static int capture_start_unknown_write_0x1000(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint16_t out_value_xd) {
	int ret = 0;
	uint16_t base_out_val = 0x001D | (out_value_xd & 0xFF00);

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8001000, base_out_val);
	if(ret < 0)
		return ret;
	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8001000, out_value_xd);
	if(ret < 0)
		return ret;

	return ret;
}

static int capture_start_unknown_write_and_read_0x1000_0x1002(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint16_t out_value_xd, uint16_t base_value, uint16_t second_value, uint16_t &unk16) {
	int ret = 0;

	ret = capture_start_unknown_write_0x1000(handlers, device, out_value_xd);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_read_0x1002(handlers, device, base_value, second_value, unk16);
	if(ret < 0)
		return ret;

	return ret;
}

static int capture_start_first_unknown_write_and_read_0x1000_0x1002(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, uint16_t out_value_xd, uint16_t &unk16) {
	int ret = 0;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, out_value_xd, 0x0000, 0x0000, unk16);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, out_value_xd, 0x000F, 0x0000, unk16);
	if(ret < 0)
		return ret;

	return ret;
}

// Various unknown reads and writes from registers...
int capture_start(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, std::string &serial) {
	int ret = 0;
	uint32_t status = 0;
	uint32_t unk32 = 0;
	uint16_t unk16 = 0;

	// Stop capture, if one already ongoing...
	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000010, 0x80000000);
	if(ret < 0)
		return ret;

	default_sleep(30);

	serial = read_serial_ctr_capture(handlers, device);

	ret = read_fw_info_partner_ctr(handlers, device, 0, unk32);
	if(ret < 0)
		return ret;
	// Do processing based on this...?
	ret = read_fw_info_partner_ctr(handlers, device, 0x0F, unk32);
	if(ret < 0)
		return ret;
	// Do processing based on this...?

	ret = capture_start_first_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, unk16);
	if(ret < 0)
		return ret;

	ret = capture_start_first_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x003D, unk16);
	if(ret < 0)
		return ret;

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF000000C, 0xFFFFFFFF);
	if(ret < 0)
		return ret;

	ret = read_fw_info_partner_ctr(handlers, device, 0, unk32);
	if(ret < 0)
		return ret;
	// Do processing based on this...?
	ret = read_fw_info_partner_ctr(handlers, device, 0x0F, unk32);
	if(ret < 0)
		return ret;

	ret = capture_start_first_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, unk16);
	if(ret < 0)
		return ret;

	ret = capture_start_first_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x003D, unk16);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, 0x0018, 0x000F, unk16);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, 0x0019, 0x0000, unk16);
	if(ret < 0)
		return ret;

	// Is this FF5D due to uninitialized data, or...?
	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0xFF5D, 0xFF18, 0x0004, unk16);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, 0x0019, 0x0000, unk16);
	if(ret < 0)
		return ret;

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF000000C, 0x00001000);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, 0x0028, 0x0000, unk16);
	if(ret < 0)
		return ret;

	ret = read_partner_ctr_status(handlers, device, status);
	if(ret < 0)
		return ret;

	// Is this 015D due to uninitialized data, or...?
	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x015D, 0x012A, 0x0000, unk16);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF8002010, unk32);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, 0x0018, 0x0004, unk16);
	if(ret < 0)
		return ret;

	ret = capture_start_unknown_write_and_read_0x1000_0x1002(handlers, device, 0x005D, 0x001A, 0x0064, unk16);
	if(ret < 0)
		return ret;

	return 0;
}

// Various unknown reads and writes from registers...
int StartCaptureDma(cy_device_device_handlers* handlers, const cypart_device_usb_device* device, bool is_3d) {
	int ret = 0;
	uint32_t status = 0;
	uint32_t unk32 = 0;
	uint16_t unk16 = 0;
	uint8_t buffer_in[16];

	// Stop capture, if one already ongoing...
	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000010, 0x80000000);
	if(ret < 0)
		return ret;

	default_sleep(30);

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000010, 0x00000004);
	if(ret < 0)
		return ret;

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000010, 0x00000008);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF8002010, unk32);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF8002010, unk32);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF800200C, unk32);
	if(ret < 0)
		return ret;

	default_sleep(50);

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF800200C, unk32);
	if(ret < 0)
		return ret;

	default_sleep(180);

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x0000);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x0001);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF800200C, unk32);
	if(ret < 0)
		return ret;

	ret = read_u64_plus_from_address_partner_ctr(handlers, device, 0x01F00000, buffer_in, 16);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x0000);
	if(ret < 0)
		return ret;

	ret = read_partner_ctr_status(handlers, device, status);
	if(ret < 0)
		return ret;
	// Act based on status...?

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000010, 0x00000008);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002005, 0x0000);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002005, 0x0000);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF8002010, unk32);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF8002010, unk32);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF800200C, unk32);
	if(ret < 0)
		return ret;

	default_sleep(50);

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF800200C, unk32);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x0078);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x007A);
	if(ret < 0)
		return ret;

	default_sleep(12);

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x7778);
	if(ret < 0)
		return ret;

	default_sleep(70);

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x0078);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x0079);
	if(ret < 0)
		return ret;

	ret = read_u32_from_address_partner_ctr(handlers, device, 0xF800200C, unk32);
	if(ret < 0)
		return ret;

	ret = read_u64_plus_from_address_partner_ctr(handlers, device, 0x02800000, buffer_in, 16);
	if(ret < 0)
		return ret;

	ret = write_u16_to_address_partner_ctr(handlers, device, 0xF8002004, 0x0078);
	if(ret < 0)
		return ret;

	ret = read_partner_ctr_status(handlers, device, status);
	if(ret < 0)
		return ret;
	// Act based on status...?

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000010, 0x00000008);
	if(ret < 0)
		return ret;

	default_sleep(180);

	return 0;
}

// Various unknown reads and writes from registers...
int capture_end(cy_device_device_handlers* handlers, const cypart_device_usb_device* device) {
	int ret = 0;

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000014, 0x00000003);
	if(ret < 0)
		return ret;

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000010, 0x00000001);
	if(ret < 0)
		return ret;

	default_sleep(80);

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF000000C, 0x00001000);
	if(ret < 0)
		return ret;

	default_sleep(210);

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000014, 0x00000004);
	if(ret < 0)
		return ret;

	ret = write_u32_to_address_partner_ctr(handlers, device, 0xF0000014, 0x00000008);
	if(ret < 0)
		return ret;

	return 0;
}

int ReadFrame(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cypart_device_usb_device* device_desc) {
	// Maybe making this async would be better for lower end hardware...
	int num_bytes = 0;
	cypress_pipe_reset_bulk_in(handlers, get_cy_usb_info(device_desc));
	int ret = cypress_bulk_in_transfer(handlers, get_cy_usb_info(device_desc), buf, length, &num_bytes);
	if(num_bytes != length)
		return LIBUSB_ERROR_INTERRUPTED;
	return ret;
}

int ReadFrameAsync(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cypart_device_usb_device* device_desc, cy_async_callback_data* cb_data) {
	return cypress_bulk_in_async(handlers, get_cy_usb_info(device_desc), buf, length, cb_data);
}

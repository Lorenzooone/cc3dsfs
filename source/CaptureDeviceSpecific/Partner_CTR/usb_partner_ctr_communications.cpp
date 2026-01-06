#include "frontend.hpp"
#include "usb_partner_ctr_communications.hpp"
#include "usb_partner_ctr_libusb.hpp"
#include "usb_partner_ctr_driver.hpp"

#include "usb_generic.hpp"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define DEFAULT_SOUND_PACKET_SIZE 0x0FE0
#define MAX_NUM_SOUND_PACKETS 4

static const partner_ctr_usb_device usb_partner_ctr_cap_desc = {
.name = "P.CTR-C", .long_name = "Partner CTR Capture",
.index_in_allowed_scan = CC_PARTNER_CTR,
.vid = 0x0ed2, .pid = 0x0004,
.default_config = 1, .default_interface = 0,
.bulk_timeout = 500,
.ep_in = 6 | LIBUSB_ENDPOINT_IN, .ep_out = 2 | LIBUSB_ENDPOINT_OUT,
.write_pipe = "Pipe01", .read_pipe = "Pipe03",
.product_id = 2, .manufacturer_id = 1,
.video_data_type = VIDEO_DATA_BGR,
.max_audio_samples_size = DEFAULT_SOUND_PACKET_SIZE * MAX_NUM_SOUND_PACKETS
};

static const partner_ctr_usb_device* all_usb_partner_ctr_devices_desc[] = {
	&usb_partner_ctr_cap_desc,
};

int GetNumPartnerCTRDeviceDesc() {
	return sizeof(all_usb_partner_ctr_devices_desc) / sizeof(all_usb_partner_ctr_devices_desc[0]);
}

const partner_ctr_usb_device* GetPartnerCTRDeviceDesc(int index) {
	if((index < 0) || (index >= GetNumPartnerCTRDeviceDesc()))
		index = 0;
	return all_usb_partner_ctr_devices_desc[index];
}

#ifndef __CYPRESS_OPTIMIZE_NEW_3DS_DEVICE_COMMUNICATIONS_HPP
#define __CYPRESS_OPTIMIZE_NEW_3DS_DEVICE_COMMUNICATIONS_HPP

#include <libusb.h>
#include <vector>
#include <fstream>
#include <thread>
#include "utils.hpp"
#include "capture_structs.hpp"
#include "cypress_shared_communications.hpp"

enum cypress_optimize_new_device_type {
	CYPRESS_OPTIMIZE_NEW_3DS_BLANK_DEVICE,
};

struct cyopn_device_usb_device {
	std::string name;
	std::string long_name;
	cypress_optimize_new_device_type device_type;
	uint8_t* firmware_to_load;
	size_t firmware_size;
	cypress_optimize_new_device_type next_device;
	bool has_bcd_device_serial;
	cy_device_usb_device usb_device_info;
};


int GetNumCyOpNDeviceDesc(void);
const cyopn_device_usb_device* GetCyOpNDeviceDesc(int index);
const cyopn_device_usb_device* GetNextDeviceDesc(const cyopn_device_usb_device* device);
const cy_device_usb_device* get_cy_usb_info(const cyopn_device_usb_device* usb_device_desc);
bool has_to_load_firmware(const cyopn_device_usb_device* device);
bool load_firmware(cy_device_device_handlers* handlers, const cyopn_device_usb_device* device, uint8_t patch_id);
int capture_start(cy_device_device_handlers* handlers, const cyopn_device_usb_device* device);
int StartCaptureDma(cy_device_device_handlers* handlers, const cyopn_device_usb_device* device);
int capture_end(cy_device_device_handlers* handlers, const cyopn_device_usb_device* device);
int ReadFrame(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cyopn_device_usb_device* device_desc);
int ReadFrameAsync(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cyopn_device_usb_device* device_desc, cy_async_callback_data* cb_data);

#endif

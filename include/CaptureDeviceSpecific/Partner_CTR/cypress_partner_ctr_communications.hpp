#ifndef __CYPRESS_PARTNER_CTR_DEVICE_COMMUNICATIONS_HPP
#define __CYPRESS_PARTNER_CTR_DEVICE_COMMUNICATIONS_HPP

#include <libusb.h>
#include <vector>
#include <fstream>
#include <thread>
#include "utils.hpp"
#include "capture_structs.hpp"
#include "cypress_shared_communications.hpp"

struct cypart_device_usb_device {
	std::string name;
	std::string long_name;
	InputVideoDataType video_data_type;
	PossibleCaptureDevices index_in_allowed_scan;
	int ep_ctrl_serial_io_pipe;
	cy_device_usb_device usb_device_info;
};

int GetNumCyPartnerCTRDeviceDesc(void);
const cypart_device_usb_device* GetCyPartnerCTRDeviceDesc(int index);
const cy_device_usb_device* get_cy_usb_info(const cypart_device_usb_device* usb_device_desc);
std::string read_serial_ctr_capture(cy_device_device_handlers* handlers, const cypart_device_usb_device* device);
int capture_start(cy_device_device_handlers* handlers, const cypart_device_usb_device* device);
int StartCaptureDma(cy_device_device_handlers* handlers, const cypart_device_usb_device* device);
int capture_end(cy_device_device_handlers* handlers, const cypart_device_usb_device* device);
int ReadFrame(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cypart_device_usb_device* device_desc);
int ReadFrameAsync(cy_device_device_handlers* handlers, uint8_t* buf, int length, const cypart_device_usb_device* device_desc, cy_async_callback_data* cb_data);

#endif

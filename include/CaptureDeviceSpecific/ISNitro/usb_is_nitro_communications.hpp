#ifndef __USB_IS_NITRO_COMMUNICATIONS_HPP
#define __USB_IS_NITRO_COMMUNICATIONS_HPP

#include <libusb.h>
#include <vector>
#include <fstream>
#include "utils.hpp"

#define IS_NITRO_REAL_SERIAL_NUMBER_SIZE 10

enum is_nitro_forward_config_values_colors {
	IS_NITRO_FORWARD_CONFIG_COLOR_RGB24 = 0,
};

enum is_nitro_forward_config_values_screens {
	IS_NITRO_FORWARD_CONFIG_MODE_BOTH = 0,
	IS_NITRO_FORWARD_CONFIG_MODE_TOP = 1,
	IS_NITRO_FORWARD_CONFIG_MODE_BOTTOM = 2,
};

enum is_nitro_forward_config_values_rate {
	IS_NITRO_FORWARD_CONFIG_RATE_FULL = 0,
	IS_NITRO_FORWARD_CONFIG_RATE_HALF = 1,
	IS_NITRO_FORWARD_CONFIG_RATE_THIRD = 2,
	IS_NITRO_FORWARD_CONFIG_RATE_QUARTER = 3,
};

struct is_nitro_usb_device {
	std::string name;
	int vid;
	int pid;
	int default_config;
	int default_interface;
	int bulk_timeout;
	int ep2_in;
	int ep1_out;
	int product_id;
	int manufacturer_id;
	bool is_capture;
};

struct is_nitro_device_handlers {
	libusb_device_handle* usb_handle;
	void* read_handle;
	void* write_handle;
	void* mutex;
};

int GetNumISNitroDesc(void);
const is_nitro_usb_device* GetISNitroDesc(int index);
int DisableLca2(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc);
int StartUsbCaptureDma(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc);
int StopUsbCaptureDma(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc);
int SetForwardFrameCount(is_nitro_device_handlers* handlers, uint16_t count, const is_nitro_usb_device* device_desc);
int SetForwardFramePermanent(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc);
int GetFrameCounter(is_nitro_device_handlers* handlers, uint16_t* out, const is_nitro_usb_device* device_desc);
int GetDeviceSerial(is_nitro_device_handlers* handlers, uint8_t* buf, const is_nitro_usb_device* device_desc);
int UpdateFrameForwardConfig(is_nitro_device_handlers* handlers, is_nitro_forward_config_values_colors colors, is_nitro_forward_config_values_screens screens, is_nitro_forward_config_values_rate rate, const is_nitro_usb_device* device_desc);
int UpdateFrameForwardEnable(is_nitro_device_handlers* handlers, bool enable, bool restart, const is_nitro_usb_device* device_desc);
int ReadLidState(is_nitro_device_handlers* handlers, bool* out, const is_nitro_usb_device* device_desc);
int ReadDebugButtonState(is_nitro_device_handlers* handlers, bool* out, const is_nitro_usb_device* device_desc);
int ReadPowerButtonState(is_nitro_device_handlers* handlers, bool* out, const is_nitro_usb_device* device_desc);
int ResetCPUStart(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc);
int ResetCPUEnd(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc);
int ResetFullHardware(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc);
int ReadFrame(is_nitro_device_handlers* handlers, uint8_t* buf, int length, const is_nitro_usb_device* device_desc);

#endif

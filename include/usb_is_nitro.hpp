#ifndef __USB_IS_NITRO_HPP
#define __USB_IS_NITRO_HPP

#include <libusb.h>
#include <vector>
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

enum is_nitro_possible_devices {
	IS_NITRO_EMULATOR_COMMON_ID,
	IS_NITRO_EMULATOR_RARE_ID,
	IS_NITRO_CAPTURE_ID,
};

#define FIRST_IS_NITRO_DEVICE_ID IS_NITRO_EMULATOR_COMMON_ID

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

const is_nitro_usb_device* GetISNitroDesc(is_nitro_possible_devices wanted_device_id);
int DisableLca2(libusb_device_handle *handle, const is_nitro_usb_device* device_desc);
int StartUsbCaptureDma(libusb_device_handle *handle, const is_nitro_usb_device* device_desc);
int StopUsbCaptureDma(libusb_device_handle *handle, const is_nitro_usb_device* device_desc);
int SetForwardFrameCount(libusb_device_handle *handle, uint16_t count, const is_nitro_usb_device* device_desc);
int SetForwardFramePermanent(libusb_device_handle *handle, const is_nitro_usb_device* device_desc);
int GetFrameCounter(libusb_device_handle *handle, uint16_t* out, const is_nitro_usb_device* device_desc);
int GetDeviceSerial(libusb_device_handle *handle, uint8_t* buf, const is_nitro_usb_device* device_desc);
int UpdateFrameForwardConfig(libusb_device_handle *handle, is_nitro_forward_config_values_colors colors, is_nitro_forward_config_values_screens screens, is_nitro_forward_config_values_rate rate, const is_nitro_usb_device* device_desc);
int UpdateFrameForwardEnable(libusb_device_handle *handle, bool enable, bool restart, const is_nitro_usb_device* device_desc);
int ReadFrame(libusb_device_handle *handle, uint8_t* buf, int length, const is_nitro_usb_device* device_desc);

#endif

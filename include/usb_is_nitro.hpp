#ifndef __USB_IS_NITRO_HPP
#define __USB_IS_NITRO_HPP

#include <libusb.h>
#include <vector>
#include "utils.hpp"

#define IS_NITRO_REAL_SERIAL_NUMBER_SIZE 10

enum is_nitro_forward_enable_values {
	IS_NITRO_FORWARD_ENABLE_DISABLE = 0,
	IS_NITRO_FORWARD_ENABLE_ENABLE = 1,
	IS_NITRO_FORWARD_ENABLE_RESTART = 2,
};

enum is_nitro_forward_config_values {
	IS_NITRO_FORWARD_CONFIG_COLOR_RGB24 = 0,
	IS_NITRO_FORWARD_CONFIG_MODE_BOTH = 0,
	IS_NITRO_FORWARD_CONFIG_MODE_TOP = 4,
	IS_NITRO_FORWARD_CONFIG_MODE_BOTTOM = 8,
	IS_NITRO_FORWARD_CONFIG_RATE_FULL = 0,
	IS_NITRO_FORWARD_CONFIG_RATE_HALF = 1,
	IS_NITRO_FORWARD_CONFIG_RATE_THIRD = 2,
	IS_NITRO_FORWARD_CONFIG_RATE_QUARTER = 3,
};

struct is_nitro_usb_device {
	int vid;
	int pid;
	int default_config;
	int default_interface;
	int bulk_timeout;
	int ep2_in;
	int ep1_out;
	int product_id;
	int manufacturer_id;
};

extern const is_nitro_usb_device usb_is_nitro_desc;

/*
int SendReadCommand(libusb_device_handle *handle, uint16_t command, uint8_t* buf, int length);
int SendWriteCommand(libusb_device_handle *handle, uint16_t command, uint8_t* buf, int length);
int ReadNecMem(libusb_device_handle *handle, uint32_t address, uint8_t unit_size, uint8_t* buf, int count);
int ReadNecMemU16(libusb_device_handle *handle, uint32_t address, uint16_t* out);
int ReadNecMemU32(libusb_device_handle *handle, uint32_t address, uint32_t* out);
int WriteNecMem(libusb_device_handle *handle, uint32_t address, uint8_t unit_size, uint8_t* buf, int count);
int WriteNecMemU16(libusb_device_handle *handle, uint32_t address, uint16_t value);
int WriteNecMemU32(libusb_device_handle *handle, uint32_t address, uint32_t value);
*/

int DisableLca2(libusb_device_handle *handle);
int StartUsbCaptureDma(libusb_device_handle *handle);
int StopUsbCaptureDma(libusb_device_handle *handle);
int SetForwardFrameCount(libusb_device_handle *handle, uint16_t count);
int GetFrameCounter(libusb_device_handle *handle, uint16_t* out);
int GetDeviceSerial(libusb_device_handle *handle, uint8_t* buf);
int UpdateFrameForwardConfig(libusb_device_handle *handle, uint8_t value);
int UpdateFrameForwardEnable(libusb_device_handle *handle, uint8_t value);
int ReadFrame(libusb_device_handle *handle, uint8_t* buf, int length);

#endif

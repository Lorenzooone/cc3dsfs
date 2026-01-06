#ifndef __USB_PARTNER_CTR_COMMUNICATIONS_HPP
#define __USB_PARTNER_CTR_COMMUNICATIONS_HPP

#include <libusb.h>
#include <vector>
#include <fstream>
#include <thread>
#include "utils.hpp"
#include "capture_structs.hpp"

#define PARTNER_CTR_REAL_SERIAL_NUMBER_SIZE 8

struct partner_ctr_usb_device {
	std::string name;
	std::string long_name;
	PossibleCaptureDevices index_in_allowed_scan;
	int vid;
	int pid;
	int default_config;
	int default_interface;
	int bulk_timeout;
	int ep_in;
	int ep_out;
	std::string write_pipe;
	std::string read_pipe;
	int product_id;
	int manufacturer_id;
	InputVideoDataType video_data_type;
	size_t max_audio_samples_size;
};

struct partner_ctr_device_handlers {
	libusb_device_handle* usb_handle;
	void* read_handle;
	void* write_handle;
	void* ctrl_in_handle;
	void* mutex;
	std::string path;
};

int GetNumPartnerCTRDeviceDesc(void);
const partner_ctr_usb_device* GetPartnerCTRDeviceDesc(int index);

#endif

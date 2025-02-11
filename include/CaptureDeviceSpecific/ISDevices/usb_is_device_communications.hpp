#ifndef __USB_IS_DEVICE_COMMUNICATIONS_HPP
#define __USB_IS_DEVICE_COMMUNICATIONS_HPP

#include <libusb.h>
#include <vector>
#include <fstream>
#include <thread>
#include "utils.hpp"
#include "capture_structs.hpp"

#define IS_DEVICE_REAL_SERIAL_NUMBER_SIZE 10

enum is_device_type {
	IS_NITRO_EMULATOR_DEVICE,
	IS_NITRO_CAPTURE_DEVICE,
	IS_TWL_CAPTURE_DEVICE,
};

enum is_device_forward_config_values_colors {
	IS_DEVICE_FORWARD_CONFIG_COLOR_RGB24 = 0,
};

enum is_device_forward_config_values_screens {
	IS_DEVICE_FORWARD_CONFIG_MODE_BOTH = 0,
	IS_DEVICE_FORWARD_CONFIG_MODE_TOP = 1,
	IS_DEVICE_FORWARD_CONFIG_MODE_BOTTOM = 2,
};

enum is_device_forward_config_values_rate {
	IS_DEVICE_FORWARD_CONFIG_RATE_FULL = 0,
	IS_DEVICE_FORWARD_CONFIG_RATE_HALF = 1,
	IS_DEVICE_FORWARD_CONFIG_RATE_THIRD = 2,
	IS_DEVICE_FORWARD_CONFIG_RATE_QUARTER = 3,
};

typedef void (*isd_async_callback_function)(void* user_data, int transfer_length, int transfer_status);

struct isd_async_callback_data {
	isd_async_callback_function function;
	void* actual_user_data;
	void* transfer_data;
	void* handle;
	void* base_transfer_data;
	std::mutex transfer_data_access;
	SharedConsumerMutex* is_transfer_done_mutex;
	SharedConsumerMutex* is_transfer_data_ready_mutex;
	size_t requested_length;
	uint8_t* buffer;
	size_t actual_length;
	int status_value;
	bool is_data_ready;
	int internal_index;
};

struct is_device_usb_device {
	std::string name;
	std::string long_name;
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
	is_device_type device_type;
	InputVideoDataType video_data_type;
	size_t max_usb_packet_size;
	bool do_pipe_clear_reset;
	bool audio_enabled;
	size_t max_audio_samples_size;
};

struct is_device_device_handlers {
	libusb_device_handle* usb_handle;
	void* read_handle;
	void* write_handle;
	void* ctrl_in_handle;
	void* mutex;
	std::string path;
};

struct is_device_twl_enc_dec_table {
	uint32_t rotating_value;
	uint8_t num_values;
	uint8_t num_iters_full;
	uint8_t num_iters_before_refresh;
	uint16_t action_values[8];
};

int GetNumISDeviceDesc(void);
const is_device_usb_device* GetISDeviceDesc(int index);
int DisableLca2(is_device_device_handlers* handlers, const is_device_usb_device* device_desc);
int StartUsbCaptureDma(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, bool enable_sound_capture = false, is_device_twl_enc_dec_table* enc_table = NULL, is_device_twl_enc_dec_table* dec_table = NULL);
int StopUsbCaptureDma(is_device_device_handlers* handlers, const is_device_usb_device* device_desc);
int SetForwardFrameCount(is_device_device_handlers* handlers, uint16_t count, const is_device_usb_device* device_desc);
int SetForwardFramePermanent(is_device_device_handlers* handlers, const is_device_usb_device* device_desc);
int GetFrameCounter(is_device_device_handlers* handlers, uint16_t* out, const is_device_usb_device* device_desc);
int GetDeviceSerial(is_device_device_handlers* handlers, uint8_t* buf, const is_device_usb_device* device_desc);
int UpdateFrameForwardConfig(is_device_device_handlers* handlers, is_device_forward_config_values_colors colors, is_device_forward_config_values_screens screens, is_device_forward_config_values_rate rate, const is_device_usb_device* device_desc);
int UpdateFrameForwardEnable(is_device_device_handlers* handlers, bool enable, bool restart, const is_device_usb_device* device_desc);
int ReadLidState(is_device_device_handlers* handlers, bool* out, const is_device_usb_device* device_desc);
int ReadDebugButtonState(is_device_device_handlers* handlers, bool* out, const is_device_usb_device* device_desc);
int ReadPowerButtonState(is_device_device_handlers* handlers, bool* out, const is_device_usb_device* device_desc);
int ResetCPUStart(is_device_device_handlers* handlers, const is_device_usb_device* device_desc);
int ResetCPUEnd(is_device_device_handlers* handlers, const is_device_usb_device* device_desc);
int ResetFullHardware(is_device_device_handlers* handlers, const is_device_usb_device* device_desc);
int SetBatteryPercentage(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, int percentage);
int SetACAdapterConnected(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, bool connected);
int GetBatteryPercentageValues(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, int* percentage_one, int* percentage_two);
int GetACAdapterConnectedValues(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, bool* connected_one, bool* connected_two);
int AskFrameLengthPos(is_device_device_handlers* handlers, uint32_t* video_address, uint32_t* video_length, bool video_enabled, uint32_t* audio_address, uint32_t* audio_length, bool audio_enabled, const is_device_usb_device* device_desc);
int SetLastFrameInfo(is_device_device_handlers* handlers, uint32_t video_address, uint32_t video_length, uint32_t audio_address, uint32_t audio_length, const is_device_usb_device* device_desc);
int ReadFrame(is_device_device_handlers* handlers, uint8_t* buf, uint32_t address, uint32_t length, const is_device_usb_device* device_desc);
int ReadFrame(is_device_device_handlers* handlers, uint8_t* buf, int length, const is_device_usb_device* device_desc);
void ReadFrameAsync(is_device_device_handlers* handlers, uint8_t* buf, int length, const is_device_usb_device* device_desc, isd_async_callback_data* cb_data);
void CloseAsyncRead(is_device_device_handlers* handlers, isd_async_callback_data* cb_data);
int ResetUSBDevice(is_device_device_handlers* handlers);
int PrepareEncDecTable(is_device_device_handlers* handlers, is_device_twl_enc_dec_table* enc_table, is_device_twl_enc_dec_table* dec_table, const is_device_usb_device* device_desc);

void SetupISDeviceAsyncThread(is_device_device_handlers* handlers, void* user_data, std::thread* thread_ptr, bool* keep_going, ConsumerMutex* is_data_ready);
void EndISDeviceAsyncThread(is_device_device_handlers* handlers, void* user_data, std::thread* thread_ptr, bool* keep_going, ConsumerMutex* is_data_ready);

#endif

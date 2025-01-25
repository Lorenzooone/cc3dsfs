#ifndef __CYPRESS_NISETRO_DEVICE_COMMUNICATIONS_HPP
#define __CYPRESS_NISETRO_DEVICE_COMMUNICATIONS_HPP

#include <libusb.h>
#include <vector>
#include <fstream>
#include <thread>
#include "utils.hpp"
#include "capture_structs.hpp"

enum cypress_nisetro_device_type {
	CYPRESS_NISETRO_BLANK_DEVICE,
	CYPRESS_NISETRO_DS_DEVICE,
};

typedef void (*cyni_async_callback_function)(void* user_data, int transfer_length, int transfer_status);

struct cyni_async_callback_data {
	cyni_async_callback_function function;
	void* actual_user_data;
	void* transfer_data;
	void* handle;
	void* base_transfer_data;
	std::mutex transfer_data_access;
	SharedConsumerMutex* is_transfer_done_mutex;
	SharedConsumerMutex* is_transfer_data_ready_mutex;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_request;
	float timeout_s;
	size_t requested_length;
	uint8_t* buffer;
	size_t actual_length;
	int status_value;
	int internal_index;
	bool is_data_ready;
};

struct cyni_device_usb_device {
	std::string name;
	std::string long_name;
	int vid;
	int pid;
	int default_config;
	int default_interface;
	int bulk_timeout;
	int ep_ctrl_bulk_in;
	int ep_ctrl_bulk_out;
	int ep_bulk_in;
	cypress_nisetro_device_type device_type;
	InputVideoDataType video_data_type;
	size_t max_usb_packet_size;
	bool do_pipe_clear_reset;
	uint8_t* firmware_to_load;
	size_t firmware_size;
	cypress_nisetro_device_type next_device;
	bool has_bcd_device_serial;
	int alt_interface;
};

struct cyni_device_device_handlers {
	libusb_device_handle* usb_handle;
	void* read_handle;
	void* write_handle;
	void* mutex;
	std::string path;
};

int GetNumCyNiDeviceDesc(void);
const cyni_device_usb_device* GetCyNiDeviceDesc(int index);const cyni_device_usb_device* GetNextDeviceDesc(const cyni_device_usb_device* device);
const cyni_device_usb_device* GetNextDeviceDesc(const cyni_device_usb_device* device);
bool has_to_load_firmware(const cyni_device_usb_device* device);
bool load_firmware(cyni_device_device_handlers* handlers, const cyni_device_usb_device* device, uint8_t patch_id);
int capture_start(cyni_device_device_handlers* handlers, const cyni_device_usb_device* device);
int StartCaptureDma(cyni_device_device_handlers* handlers, const cyni_device_usb_device* device);
int capture_end(cyni_device_device_handlers* handlers, const cyni_device_usb_device* device);
int ReadFrame(cyni_device_device_handlers* handlers, uint8_t* buf, int length, const cyni_device_usb_device* device_desc);
int ReadFrameAsync(cyni_device_device_handlers* handlers, uint8_t* buf, int length, const cyni_device_usb_device* device_desc, cyni_async_callback_data* cb_data);
void CloseAsyncRead(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, cyni_async_callback_data* cb_data);
void SetupCypressDeviceAsyncThread(cyni_device_device_handlers* handlers, void* user_data, std::thread* thread_ptr, bool* keep_going, ConsumerMutex* is_data_ready);
void EndCypressDeviceAsyncThread(cyni_device_device_handlers* handlers, void* user_data, std::thread* thread_ptr, bool* keep_going, ConsumerMutex* is_data_ready);
void SetMaxTransferSize(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, size_t new_max_transfer_size);

#endif

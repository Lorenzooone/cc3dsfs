#ifndef __CYPRESS_SHARED_DEVICE_COMMUNICATIONS_HPP
#define __CYPRESS_SHARED_DEVICE_COMMUNICATIONS_HPP

#include <libusb.h>
#include <vector>
#include <fstream>
#include <thread>
#include "utils.hpp"
#include "capture_structs.hpp"

typedef void (*cy_async_callback_function)(void* user_data, int transfer_length, int transfer_status);
typedef void (*cy_error_function)(void* user_data, int error);
typedef CaptureDevice (*cy_create_device_function)(const void* usb_device_desc, std::string serial, std::string path);

struct cy_device_device_handlers {
	libusb_device_handle* usb_handle;
	void* read_handle;
	void* write_handle;
	void* mutex;
	std::string path;
};

typedef std::string (*cy_get_serial_function)(cy_device_device_handlers* handlers, const void* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id);

struct cy_async_callback_data {
	cy_async_callback_function function;
	cy_error_function error_function;
	volatile bool* in_use_ptr;
	void* actual_user_data;
	void* transfer_data;
	void* handle;
	void* extra_transfer_data;
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

struct cy_device_usb_device {
	int vid;
	int pid;
	int default_config;
	int default_interface;
	int bulk_timeout;
	int ep_ctrl_bulk_in;
	int ep_ctrl_bulk_out;
	int ep_bulk_in;
	size_t max_usb_packet_size;
	bool do_pipe_clear_reset;
	int alt_interface;
	const void* full_data;
	bool get_serial_requires_setup;
	cy_get_serial_function get_serial_fn;
	cy_create_device_function create_device_fn;
	uint16_t bcd_device_mask;
	uint16_t bcd_device_wanted_value;
};

// Helper methods
CaptureDevice cypress_create_device(const cy_device_usb_device* usb_device_desc, std::string serial, std::string path = "");
std::string cypress_get_serial(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id);
void cypress_insert_device(std::vector<CaptureDevice>& devices_list, const cy_device_usb_device* usb_device_desc, std::string real_serial, std::string path = "");

int cypress_ctrl_in_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred);
int cypress_ctrl_out_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred);
int cypress_bulk_in_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int cypress_bulk_in_async(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t *buf, int length, cy_async_callback_data* cb_data);
int cypress_ctrl_bulk_in_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred, int chosen_endpoint = -1);
int cypress_ctrl_bulk_out_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, const uint8_t* buf, int length, int* transferred, int chosen_endpoint = -1);
void cypress_pipe_reset_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc);
void cypress_pipe_reset_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, int chosen_endpoint = -1);
void cypress_pipe_reset_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, int chosen_endpoint = -1);
void CypressSetMaxTransferSize(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, size_t new_max_transfer_size);
void CypressCloseAsyncRead(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, cy_async_callback_data* cb_data);
void CypressSetupCypressDeviceAsyncThread(cy_device_device_handlers* handlers, std::vector<cy_async_callback_data*> &cb_data_vector, std::thread* thread_ptr, bool* keep_going);
void CypressEndCypressDeviceAsyncThread(cy_device_device_handlers* handlers, std::vector<cy_async_callback_data*> &cb_data_vector, std::thread* thread_ptr, bool* keep_going);

#endif

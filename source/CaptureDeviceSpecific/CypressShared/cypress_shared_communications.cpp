#include "frontend.hpp"
#include "cypress_shared_communications.hpp"
#include "cypress_shared_libusb_comms.hpp"
#include "cypress_shared_driver_comms.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

CaptureDevice cypress_create_device(const cy_device_usb_device* usb_device_desc, std::string serial, std::string path) {
	return usb_device_desc->create_device_fn(usb_device_desc->full_data, serial, path);
}

std::string cypress_get_serial(const cy_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id) {
	return usb_device_desc->get_serial_fn(usb_device_desc->full_data, serial, bcd_device, curr_serial_extra_id);
}

void cypress_insert_device(std::vector<CaptureDevice>& devices_list, const cy_device_usb_device* usb_device_desc, std::string serial, uint16_t bcd_device, int& curr_serial_extra_id, std::string path) {
	devices_list.push_back(cypress_create_device(usb_device_desc, cypress_get_serial(usb_device_desc, serial, bcd_device, curr_serial_extra_id), path));
}

int cypress_ctrl_in_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred) {
	if(handlers->usb_handle)
		return cypress_libusb_ctrl_in(handlers, usb_device_desc, buf, length, request, value, index, transferred);
	return cypress_driver_ctrl_transfer_in(handlers, buf, length, value, index, request, transferred);
}

int cypress_ctrl_out_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred) {
	if(handlers->usb_handle)
		return cypress_libusb_ctrl_out(handlers, usb_device_desc, buf, length, request, value, index, transferred);
	return cypress_driver_ctrl_transfer_out(handlers, buf, length, value, index, request, transferred);
}

int cypress_bulk_in_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	if(handlers->usb_handle)
		return cypress_libusb_bulk_in(handlers, usb_device_desc, buf, length, transferred);
	return cypress_driver_bulk_in(handlers, usb_device_desc, buf, length, transferred);
}

int cypress_bulk_in_async(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t *buf, int length, cy_async_callback_data* cb_data) {
	if(handlers->usb_handle)
		return cypress_libusb_async_in_start(handlers, usb_device_desc, buf, length, cb_data);
	return cypress_driver_async_in_start(handlers, usb_device_desc, buf, length, cb_data);
}

int cypress_ctrl_bulk_in_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	if(handlers->usb_handle)
		return cypress_libusb_ctrl_bulk_in(handlers, usb_device_desc, buf, length, transferred);
	return cypress_driver_ctrl_bulk_in(handlers, usb_device_desc, buf, length, transferred);
}

int cypress_ctrl_bulk_out_transfer(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, const uint8_t* buf, int length, int* transferred) {
	if(handlers->usb_handle)
		return cypress_libusb_ctrl_bulk_out(handlers, usb_device_desc, (uint8_t*)buf, length, transferred);
	return cypress_driver_ctrl_bulk_out(handlers, usb_device_desc, (uint8_t*)buf, length, transferred);
}

void cypress_pipe_reset_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	if(handlers->usb_handle)
		return cypress_libusb_pipe_reset_bulk_in(handlers, usb_device_desc);
	return cypress_driver_pipe_reset_bulk_in(handlers, usb_device_desc);
}

void cypress_pipe_reset_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	if(handlers->usb_handle)
		return cypress_libusb_pipe_reset_ctrl_bulk_in(handlers, usb_device_desc);
	return cypress_driver_pipe_reset_ctrl_bulk_in(handlers, usb_device_desc);
}

void cypress_pipe_reset_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	if(handlers->usb_handle)
		return cypress_libusb_pipe_reset_ctrl_bulk_out(handlers, usb_device_desc);
	return cypress_driver_pipe_reset_ctrl_bulk_out(handlers, usb_device_desc);
}

void CypressSetMaxTransferSize(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, size_t new_max_transfer_size) {
	if(handlers->usb_handle)
		return;
	cypress_driver_set_transfer_size_bulk_in(handlers, usb_device_desc, new_max_transfer_size);
}

void CypressCloseAsyncRead(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, cy_async_callback_data* cb_data) {
	if(handlers->usb_handle)
		return cypress_libusb_cancell_callback(cb_data);
	return cypress_driver_cancel_callback(handlers, usb_device_desc, cb_data);
}

void CypressSetupCypressDeviceAsyncThread(cy_device_device_handlers* handlers, std::vector<cy_async_callback_data*> &cb_data_vector, std::thread* thread_ptr, bool* keep_going) {
	if(handlers->usb_handle)
		return libusb_register_to_event_thread();
	return cypress_driver_start_thread(thread_ptr, keep_going, cb_data_vector, handlers);
}

void CypressEndCypressDeviceAsyncThread(cy_device_device_handlers* handlers, std::vector<cy_async_callback_data*> &cb_data_vector, std::thread* thread_ptr, bool* keep_going) {
	if(handlers->usb_handle)
		return libusb_unregister_from_event_thread();
	return cypress_driver_close_thread(thread_ptr, keep_going, cb_data_vector);
}

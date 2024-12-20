#include "usb_is_device_communications.hpp"
#include "usb_is_device_setup_general.hpp"
#include "usb_is_device_libusb.hpp"
#include "usb_generic.hpp"

#include <libusb.h>
#include <frontend.hpp>

// Code based off of Gericom's sample code. Distributed under the MIT License. Copyright (c) 2024 Gericom
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

static void is_device_usb_thread_function(bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 300000;
	while(*usb_thread_run)
		libusb_handle_events_timeout_completed(get_usb_ctx(), &tv, NULL);
}

void is_device_libusb_start_thread(std::thread* thread_ptr, bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = true;
	*thread_ptr = std::thread(is_device_usb_thread_function, usb_thread_run);
}

void is_device_libusb_close_thread(std::thread* thread_ptr, bool* usb_thread_run) {
	if(!usb_is_initialized())
		return;
	*usb_thread_run = false;
	thread_ptr->join();
}

static bool is_device_libusb_setup_connection(libusb_device_handle* handle, const is_device_usb_device* usb_device_desc) {
	if (libusb_set_configuration(handle, usb_device_desc->default_config) != LIBUSB_SUCCESS)
		return false;
	if(libusb_claim_interface(handle, usb_device_desc->default_interface) != LIBUSB_SUCCESS)
		return false;
	if(usb_device_desc->do_pipe_clear_reset) {
		if(libusb_clear_halt(handle, usb_device_desc->ep_out) != LIBUSB_SUCCESS)
			return false;
		if(libusb_clear_halt(handle, usb_device_desc->ep_in) != LIBUSB_SUCCESS)
			return false;
	}
	return true;
}

static int is_device_libusb_insert_device(std::vector<CaptureDevice> &devices_list, const is_device_usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return LIBUSB_ERROR_NOT_FOUND;
	if((usb_descriptor->iManufacturer != usb_device_desc->manufacturer_id) || (usb_descriptor->iProduct != usb_device_desc->product_id))
		return LIBUSB_ERROR_NOT_FOUND;
	int result = libusb_open(usb_device, &handle);
	if((result < 0) || (handle == NULL))
		return result;
	if(is_device_libusb_setup_connection(handle, usb_device_desc)) {
		is_device_device_handlers handlers;
		handlers.usb_handle = handle;
		is_device_insert_device(devices_list, &handlers, usb_device_desc, curr_serial_extra_id);
		libusb_release_interface(handle, usb_device_desc->default_interface);
	}
	libusb_close(handle);
	return result;
}

is_device_device_handlers* is_device_libusb_serial_reconnection(const is_device_usb_device* usb_device_desc, CaptureDevice* device, int &curr_serial_extra_id) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	is_device_device_handlers* final_handlers = NULL;

	for(int i = 0; i < num_devices; i++) {
		is_device_device_handlers handlers;
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if((usb_descriptor.idVendor != usb_device_desc->vid) || (usb_descriptor.idProduct != usb_device_desc->pid))
			continue;
		if((usb_descriptor.iManufacturer != usb_device_desc->manufacturer_id) || (usb_descriptor.iProduct != usb_device_desc->product_id))
			continue;
		result = libusb_open(usb_devices[i], &handlers.usb_handle);
		if(result || (handlers.usb_handle == NULL))
			continue;
		if (is_device_libusb_setup_connection(handlers.usb_handle, usb_device_desc)) {
			std::string device_serial_number = get_serial(usb_device_desc, &handlers, curr_serial_extra_id);
			if (device->serial_number == device_serial_number) {
				final_handlers = new is_device_device_handlers;
				final_handlers->usb_handle = handlers.usb_handle;
				break;
			}
			libusb_release_interface(handlers.usb_handle, usb_device_desc->default_interface);
		}
		libusb_close(handlers.usb_handle);
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);

	return final_handlers;
}

void is_device_libusb_end_connection(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, bool interface_claimed) {
	if (interface_claimed)
		libusb_release_interface(handlers->usb_handle, device_desc->default_interface);
	libusb_close(handlers->usb_handle);
	handlers->usb_handle = NULL;
}

void is_device_libusb_list_devices(std::vector<CaptureDevice> &devices_list, bool* no_access_elems, bool* not_supported_elems, int* curr_serial_extra_id_is_device, const size_t num_is_device_desc) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for (int j = 0; j < num_is_device_desc; j++) {
			result = is_device_libusb_insert_device(devices_list, GetISDeviceDesc(j), usb_devices[i], &usb_descriptor, curr_serial_extra_id_is_device[j]);
			if (result != LIBUSB_ERROR_NOT_FOUND) {
				if (result == LIBUSB_ERROR_ACCESS)
					no_access_elems[j] = true;
				if (result == LIBUSB_ERROR_NOT_SUPPORTED)
					not_supported_elems[j] = true;
				break;
			}
		}
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
}

// Write to bulk
int is_device_libusb_bulk_out(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer(handlers->usb_handle, usb_device_desc->ep_out, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Read from bulk
int is_device_libusb_bulk_in(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer(handlers->usb_handle, usb_device_desc->ep_in, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Read from ctrl_in
int is_device_libusb_ctrl_in(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t index, int* transferred) {
	int ret = libusb_control_transfer(handlers->usb_handle, 0xC0, request, 0, index, buf, length, usb_device_desc->bulk_timeout);
	if(ret >= 0)
		*transferred = ret;
	return ret;
}

void is_device_libusb_cancell_callback(isd_async_callback_data* cb_data) {
	cb_data->transfer_data_access.lock();
	if(cb_data->transfer_data)
		libusb_cancel_transfer((libusb_transfer*)cb_data->transfer_data);
	cb_data->transfer_data_access.unlock();
}

int is_device_libusb_reset_device(is_device_device_handlers* handlers) {
	return libusb_reset_device(handlers->usb_handle);
}

void STDCALL is_device_libusb_async_callback(libusb_transfer* transfer) {
	isd_async_callback_data* cb_data = (isd_async_callback_data*)transfer->user_data;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = NULL;
	cb_data->is_transfer_done_mutex->specific_unlock(cb_data->internal_index);
	cb_data->transfer_data_access.unlock();
	cb_data->function(cb_data->actual_user_data, transfer->actual_length, transfer->status);
}

// Read from bulk
void is_device_libusb_async_in_start(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, isd_async_callback_data* cb_data) {
	libusb_transfer *transfer_in = libusb_alloc_transfer(0);
	if(!transfer_in)
		return;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = transfer_in;
	cb_data->is_transfer_done_mutex->specific_try_lock(cb_data->internal_index);
	libusb_fill_bulk_transfer(transfer_in, handlers->usb_handle, usb_device_desc->ep_in, buf, length, is_device_libusb_async_callback, cb_data, usb_device_desc->bulk_timeout);
	transfer_in->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
	libusb_submit_transfer(transfer_in);
	cb_data->transfer_data_access.unlock();
}

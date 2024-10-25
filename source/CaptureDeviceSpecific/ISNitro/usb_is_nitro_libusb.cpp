#include "usb_is_nitro_communications.hpp"
#include "usb_is_nitro_setup_general.hpp"
#include "usb_is_nitro_libusb.hpp"
#include "usb_generic.hpp"

#include <libusb.h>

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

static bool is_nitro_libusb_setup_connection(libusb_device_handle* handle, const is_nitro_usb_device* usb_device_desc) {
	if (libusb_set_configuration(handle, usb_device_desc->default_config) != LIBUSB_SUCCESS)
		return false;
	if(libusb_claim_interface(handle, usb_device_desc->default_interface) != LIBUSB_SUCCESS)
		return false;
	if(libusb_clear_halt(handle, usb_device_desc->ep1_out) != LIBUSB_SUCCESS)
		return false;
	if(libusb_clear_halt(handle, usb_device_desc->ep2_in) != LIBUSB_SUCCESS)
		return false;
	return true;
}

static int is_nitro_libusb_insert_device(std::vector<CaptureDevice> &devices_list, const is_nitro_usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return LIBUSB_ERROR_NOT_FOUND;
	if((usb_descriptor->iManufacturer != usb_device_desc->manufacturer_id) || (usb_descriptor->iProduct != usb_device_desc->product_id))
		return LIBUSB_ERROR_NOT_FOUND;
	int result = libusb_open(usb_device, &handle);
	if((result < 0) || (handle == NULL))
		return result;
	if(is_nitro_libusb_setup_connection(handle, usb_device_desc)) {
		is_nitro_device_handlers handlers;
		handlers.usb_handle = handle;
		is_nitro_insert_device(devices_list, &handlers, usb_device_desc, curr_serial_extra_id);
		libusb_release_interface(handle, usb_device_desc->default_interface);
	}
	libusb_close(handle);
	return result;
}

is_nitro_device_handlers* is_nitro_libusb_serial_reconnection(const is_nitro_usb_device* usb_device_desc, CaptureDevice* device, int &curr_serial_extra_id) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	is_nitro_device_handlers* final_handlers = NULL;

	for(int i = 0; i < num_devices; i++) {
		is_nitro_device_handlers handlers;
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
		if (is_nitro_libusb_setup_connection(handlers.usb_handle, usb_device_desc)) {
			std::string device_serial_number = get_serial(usb_device_desc, &handlers, curr_serial_extra_id);
			if (device->serial_number == device_serial_number) {
				final_handlers = new is_nitro_device_handlers;
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

void is_nitro_libusb_end_connection(is_nitro_device_handlers* handlers, const is_nitro_usb_device* device_desc, bool interface_claimed) {
	if (interface_claimed)
		libusb_release_interface(handlers->usb_handle, device_desc->default_interface);
	libusb_close(handlers->usb_handle);
	handlers->usb_handle = NULL;
}

void is_nitro_libusb_list_devices(std::vector<CaptureDevice> &devices_list, bool* no_access_elems, bool* not_supported_elems, int* curr_serial_extra_id_is_nitro, const size_t num_is_nitro_desc) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for (int j = 0; j < num_is_nitro_desc; j++) {
			result = is_nitro_libusb_insert_device(devices_list, GetISNitroDesc(j), usb_devices[i], &usb_descriptor, curr_serial_extra_id_is_nitro[j]);
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
int is_nitro_libusb_bulk_out(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer(handlers->usb_handle, usb_device_desc->ep1_out, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Read from bulk
int is_nitro_libusb_bulk_in(is_nitro_device_handlers* handlers, const is_nitro_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer(handlers->usb_handle, usb_device_desc->ep2_in, buf, length, transferred, usb_device_desc->bulk_timeout);
}

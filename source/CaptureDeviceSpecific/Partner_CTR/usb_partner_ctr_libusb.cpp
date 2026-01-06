#include "usb_generic.hpp"

#include "usb_partner_ctr_setup_general.hpp"
#include "usb_partner_ctr_communications.hpp"
#include "usb_partner_ctr_libusb.hpp"

#include <libusb.h>
#include <frontend.hpp>

static bool partner_ctr_libusb_setup_connection(libusb_device_handle* handle, const partner_ctr_usb_device* usb_device_desc) {
	libusb_check_and_detach_kernel_driver(handle, usb_device_desc->default_interface);
	int result = libusb_check_and_set_configuration(handle, usb_device_desc->default_config);
	if(result != LIBUSB_SUCCESS)
		return false;
	libusb_check_and_detach_kernel_driver(handle, usb_device_desc->default_interface);
	if(libusb_claim_interface(handle, usb_device_desc->default_interface) != LIBUSB_SUCCESS)
		return false;
	if(libusb_clear_halt(handle, usb_device_desc->ep_out) != LIBUSB_SUCCESS)
		return false;
	if(libusb_clear_halt(handle, usb_device_desc->ep_in) != LIBUSB_SUCCESS)
		return false;
	return true;
}

static int partner_ctr_libusb_insert_device(std::vector<CaptureDevice> &devices_list, const partner_ctr_usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return LIBUSB_ERROR_NOT_FOUND;
	if((usb_descriptor->iManufacturer != usb_device_desc->manufacturer_id) || (usb_descriptor->iProduct != usb_device_desc->product_id))
		return LIBUSB_ERROR_NOT_FOUND;
	int result = libusb_open(usb_device, &handle);
	if((result < 0) || (handle == NULL))
		return result;
	if(partner_ctr_libusb_setup_connection(handle, usb_device_desc)) {
		partner_ctr_device_handlers handlers;
		handlers.usb_handle = handle;
		partner_ctr_insert_device(devices_list, &handlers, usb_device_desc, curr_serial_extra_id);
		libusb_release_interface(handle, usb_device_desc->default_interface);
	}
	libusb_close(handle);
	return result;
}

partner_ctr_device_handlers* partner_ctr_libusb_serial_reconnection(const partner_ctr_usb_device* usb_device_desc, CaptureDevice* device, int &curr_serial_extra_id) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	ssize_t num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	partner_ctr_device_handlers* final_handlers = NULL;

	for(ssize_t i = 0; i < num_devices; i++) {
		partner_ctr_device_handlers handlers;
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
		if (partner_ctr_libusb_setup_connection(handlers.usb_handle, usb_device_desc)) {
			std::string device_serial_number = get_serial(usb_device_desc, &handlers, curr_serial_extra_id);
			if (device->serial_number == device_serial_number) {
				final_handlers = new partner_ctr_device_handlers;
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

void partner_ctr_libusb_end_connection(partner_ctr_device_handlers* handlers, const partner_ctr_usb_device* device_desc, bool interface_claimed) {
	if (interface_claimed)
		libusb_release_interface(handlers->usb_handle, device_desc->default_interface);
	libusb_close(handlers->usb_handle);
	handlers->usb_handle = NULL;
}

void partner_ctr_libusb_list_devices(std::vector<CaptureDevice> &devices_list, bool* no_access_elems, bool* not_supported_elems, int* curr_serial_extra_id_partner_ctr, std::vector<const partner_ctr_usb_device*> &device_descriptions) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	ssize_t num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	for(ssize_t i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for (size_t j = 0; j < device_descriptions.size(); j++) {
			result = partner_ctr_libusb_insert_device(devices_list, device_descriptions[j], usb_devices[i], &usb_descriptor, curr_serial_extra_id_partner_ctr[j]);
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

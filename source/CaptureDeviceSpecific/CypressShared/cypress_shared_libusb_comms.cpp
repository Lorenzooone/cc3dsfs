#include "cypress_shared_libusb_comms.hpp"

#include <cstring>
#include <libusb.h>
#include "usb_generic.hpp"

// Read from ctrl_in
int cypress_libusb_ctrl_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred) {
	int ret = libusb_control_transfer(handlers->usb_handle, 0xC0, request, value, index, buf, length, usb_device_desc->bulk_timeout);
	if(ret >= 0)
		*transferred = ret;
	return ret;
}

// Write to ctrl_out
int cypress_libusb_ctrl_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred) {
	int ret = libusb_control_transfer(handlers->usb_handle, 0x40, request, value, index, buf, length, usb_device_desc->bulk_timeout);
	if(ret >= 0)
		*transferred = ret;
	return ret;
}

// Read from bulk
int cypress_libusb_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer(handlers->usb_handle, usb_device_desc->ep_bulk_in, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Read from ctrl bulk
int cypress_libusb_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer(handlers->usb_handle, usb_device_desc->ep_ctrl_bulk_in, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Write to ctrl bulk
int cypress_libusb_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred) {
	return libusb_bulk_transfer(handlers->usb_handle, usb_device_desc->ep_ctrl_bulk_out, buf, length, transferred, usb_device_desc->bulk_timeout);
}

void cypress_libusb_pipe_reset_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	libusb_clear_halt(handlers->usb_handle, usb_device_desc->ep_bulk_in);
}

void cypress_libusb_pipe_reset_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	libusb_clear_halt(handlers->usb_handle, usb_device_desc->ep_ctrl_bulk_in);
}

void cypress_libusb_pipe_reset_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	libusb_clear_halt(handlers->usb_handle, usb_device_desc->ep_ctrl_bulk_out);
}

void cypress_libusb_cancell_callback(cy_async_callback_data* cb_data) {
	cb_data->transfer_data_access.lock();
	if(cb_data->transfer_data)
		libusb_cancel_transfer((libusb_transfer*)cb_data->transfer_data);
	cb_data->transfer_data_access.unlock();
}

void STDCALL cypress_libusb_async_callback(libusb_transfer* transfer) {
	cy_async_callback_data* cb_data = (cy_async_callback_data*)transfer->user_data;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = NULL;
	cb_data->is_transfer_done_mutex->specific_unlock(cb_data->internal_index);
	cb_data->transfer_data_access.unlock();
	cb_data->function(cb_data->actual_user_data, transfer->actual_length, transfer->status);
}

// Read from bulk
int cypress_libusb_async_in_start(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, cy_async_callback_data* cb_data) {
	libusb_transfer *transfer_in = libusb_alloc_transfer(0);
	if(!transfer_in)
		return LIBUSB_ERROR_OTHER;
	cb_data->transfer_data_access.lock();
	cb_data->transfer_data = transfer_in;
	cb_data->is_transfer_done_mutex->specific_try_lock(cb_data->internal_index);
	libusb_fill_bulk_transfer(transfer_in, handlers->usb_handle, usb_device_desc->ep_bulk_in, buf, length, cypress_libusb_async_callback, cb_data, usb_device_desc->bulk_timeout);
	transfer_in->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
	int retval = libusb_submit_transfer(transfer_in);
	cb_data->transfer_data_access.unlock();
	return retval;
}

static bool cypress_libusb_setup_connection(libusb_device_handle* handle, const cy_device_usb_device* usb_device_desc, bool *claimed) {
	*claimed = false;
	libusb_check_and_detach_kernel_driver(handle, usb_device_desc->default_interface);
	int result = libusb_check_and_set_configuration(handle, usb_device_desc->default_config);
	if(result != LIBUSB_SUCCESS)
		return false;
	libusb_check_and_detach_kernel_driver(handle, usb_device_desc->default_interface);
	result = libusb_claim_interface(handle, usb_device_desc->default_interface);
	if(result != LIBUSB_SUCCESS)
		return false;
	*claimed = true;
	if(usb_device_desc->alt_interface != 0) {
		result = libusb_set_interface_alt_setting(handle, usb_device_desc->default_interface, usb_device_desc->alt_interface);
		if(result != LIBUSB_SUCCESS)
			return false;
	}
	return true;
}

static void read_strings(libusb_device_handle *handle, libusb_device_descriptor *usb_descriptor, char* manufacturer, char* product, char* serial) {
	manufacturer[0] = 0;
	product[0] = 0;
	serial[0] = 0;
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iManufacturer, (uint8_t*)manufacturer, 0x100);
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iProduct, (uint8_t*)product, 0x100);
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iSerialNumber, (uint8_t*)serial, 0x100);
	manufacturer[0xFF] = 0;
	product[0xFF] = 0;
	serial[0xFF] = 0;
}

static int cypress_libusb_insert_device(std::vector<CaptureDevice> &devices_list, const cy_device_usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return LIBUSB_ERROR_NOT_FOUND;
	int result = libusb_open(usb_device, &handle);
	if((result < 0) || (handle == NULL))
		return result;
	char manufacturer[0x100];
	char product[0x100];
	char serial[0x100];
	read_strings(handle, usb_descriptor, manufacturer, product, serial);
	std::string product_str = (std::string)product;
	if(usb_device_desc->filter_for_product && (usb_device_desc->wanted_product_str != product_str)) {
		libusb_close(handle);
		return LIBUSB_ERROR_NOT_FOUND;
	}
	bool claimed = false;
	bool result_setup = cypress_libusb_setup_connection(handle, usb_device_desc, &claimed);
	if(result_setup)
		cypress_insert_device(devices_list, usb_device_desc, (std::string)(serial), usb_descriptor->bcdDevice, curr_serial_extra_id);
	if(claimed)
		libusb_release_interface(handle, usb_device_desc->default_interface);
	libusb_close(handle);
	return result;
}

cy_device_device_handlers* cypress_libusb_serial_reconnection(const cy_device_usb_device* usb_device_desc, std::string wanted_serial_number, int &curr_serial_extra_id, CaptureDevice* new_device) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	cy_device_device_handlers* final_handlers = NULL;

	for(int i = 0; i < num_devices; i++) {
		cy_device_device_handlers handlers;
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if((usb_descriptor.idVendor != usb_device_desc->vid) || (usb_descriptor.idProduct != usb_device_desc->pid))
			continue;
		result = libusb_open(usb_devices[i], &handlers.usb_handle);
		if((result < 0) || (handlers.usb_handle == NULL))
			continue;
		char manufacturer[0x100];
		char product[0x100];
		char serial[0x100];
		read_strings(handlers.usb_handle, &usb_descriptor, manufacturer, product, serial);
		std::string product_str = (std::string)product;
		if(usb_device_desc->filter_for_product && (usb_device_desc->wanted_product_str != product_str)) {
			libusb_close(handlers.usb_handle);
			continue;
		}
		std::string device_serial_number = cypress_get_serial(usb_device_desc, (std::string)(serial), usb_descriptor.bcdDevice, curr_serial_extra_id);
		bool claimed = false;
		if((wanted_serial_number == device_serial_number) && (cypress_libusb_setup_connection(handlers.usb_handle, usb_device_desc, &claimed))) {
			final_handlers = new cy_device_device_handlers;
			final_handlers->usb_handle = handlers.usb_handle;
			if(new_device != NULL)
				*new_device = cypress_create_device(usb_device_desc, wanted_serial_number);
			break;
		}
		if(claimed)
			libusb_release_interface(handlers.usb_handle, usb_device_desc->default_interface);
		libusb_close(handlers.usb_handle);
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);

	return final_handlers;
}

void cypress_libusb_end_connection(cy_device_device_handlers* handlers, const cy_device_usb_device* device_desc, bool interface_claimed) {
	if (interface_claimed)
		libusb_release_interface(handlers->usb_handle, device_desc->default_interface);
	libusb_close(handlers->usb_handle);
	handlers->usb_handle = NULL;
}

void cypress_libusb_find_used_serial(const cy_device_usb_device* usb_device_desc, bool* found, size_t num_free_fw_ids, int &curr_serial_extra_id) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	cy_device_device_handlers* final_handlers = NULL;

	for(int i = 0; i < num_devices; i++) {
		cy_device_device_handlers handlers;
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if((usb_descriptor.idVendor != usb_device_desc->vid) || (usb_descriptor.idProduct != usb_device_desc->pid))
			continue;
		result = libusb_open(usb_devices[i], &handlers.usb_handle);
		if(result || (handlers.usb_handle == NULL))
			continue;
		char manufacturer[0x100];
		char product[0x100];
		char serial[0x100];
		read_strings(handlers.usb_handle, &usb_descriptor, manufacturer, product, serial);
		libusb_close(handlers.usb_handle);
		std::string product_str = (std::string)product;
		if(usb_device_desc->filter_for_product && (usb_device_desc->wanted_product_str != product_str))
			continue;
		std::string device_serial_number = cypress_get_serial(usb_device_desc, (std::string)(serial), usb_descriptor.bcdDevice, curr_serial_extra_id);
		try {
			int pos = std::stoi(device_serial_number);
			if((pos < 0) || (pos >= num_free_fw_ids))
				continue;
			found[pos] = true;
		}
		catch(...) {
			continue;
		}
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);

	return;
}

void cypress_libusb_list_devices(std::vector<CaptureDevice> &devices_list, bool* no_access_elems, bool* not_supported_elems, int *curr_serial_extra_id_cypress, std::vector<const cy_device_usb_device*> &device_descriptions) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	int num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	for(int i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for (int j = 0; j < device_descriptions.size(); j++) {
			result = cypress_libusb_insert_device(devices_list, device_descriptions[j], usb_devices[i], &usb_descriptor, curr_serial_extra_id_cypress[j]);
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

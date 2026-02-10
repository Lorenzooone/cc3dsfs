#include "cypress_shared_libusb_comms.hpp"

#include <cstring>
#include <libusb.h>
#include "usb_generic.hpp"

#define CYPRESS_EZ_FX2_VID 0x04B4
#define CYPRESS_EZ_FX2_PID 0x8613

#define CYPRESS_EZ_FX2_BAD_VID_INTRAORAL 0x0547
#define CYPRESS_EZ_FX2_BAD_PID_INTRAORAL 0x2001

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
int cypress_libusb_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred, int chosen_endpoint) {
	if(chosen_endpoint == -1)
		chosen_endpoint = usb_device_desc->ep_ctrl_bulk_in;
	return libusb_bulk_transfer(handlers->usb_handle, chosen_endpoint, buf, length, transferred, usb_device_desc->bulk_timeout);
}

// Write to ctrl bulk
int cypress_libusb_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred, int chosen_endpoint) {
	if(chosen_endpoint == -1)
		chosen_endpoint = usb_device_desc->ep_ctrl_bulk_out;
	return libusb_bulk_transfer(handlers->usb_handle, chosen_endpoint, buf, length, transferred, usb_device_desc->bulk_timeout);
}

void cypress_libusb_pipe_reset_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc) {
	libusb_clear_halt(handlers->usb_handle, usb_device_desc->ep_bulk_in);
}

void cypress_libusb_pipe_reset_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, int chosen_endpoint) {
	if(chosen_endpoint == -1)
		chosen_endpoint = usb_device_desc->ep_ctrl_bulk_in;
	libusb_clear_halt(handlers->usb_handle, chosen_endpoint);
}

void cypress_libusb_pipe_reset_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, int chosen_endpoint) {
	if(chosen_endpoint == -1)
		chosen_endpoint = usb_device_desc->ep_ctrl_bulk_out;
	libusb_clear_halt(handlers->usb_handle, chosen_endpoint);
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

static bool cypress_libusb_setup_connection(libusb_device_handle* handle, const cy_device_usb_device* usb_device_desc, bool *claimed, bool &executed_setup) {
	static bool last_result = false;
	if(executed_setup)
		return last_result;

	last_result = false;
	executed_setup = true;
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
	last_result = true;
	return true;
}

static void read_strings(libusb_device_handle *handle, libusb_device_descriptor *usb_descriptor, char* manufacturer, char* serial) {
	manufacturer[0] = 0;
	serial[0] = 0;
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iManufacturer, (uint8_t*)manufacturer, 0x100);
	libusb_get_string_descriptor_ascii(handle, usb_descriptor->iSerialNumber, (uint8_t*)serial, 0x100);
	manufacturer[0xFF] = 0;
	serial[0xFF] = 0;
}

static std::string read_real_serial(libusb_device_handle *handle, libusb_device_descriptor *usb_descriptor, const cy_device_usb_device* usb_device_desc, char* serial, int &curr_serial_extra_id, bool *claimed, bool &executed_setup) {
	if(handle == NULL)
		return "";

	if(usb_device_desc->get_serial_requires_setup && (!cypress_libusb_setup_connection(handle, usb_device_desc, claimed, executed_setup)))
		return "";

	cy_device_device_handlers handlers;
	handlers.usb_handle = handle;
	return cypress_get_serial(&handlers, usb_device_desc, (std::string)(serial), usb_descriptor->bcdDevice, curr_serial_extra_id);
}

static int cypress_libusb_insert_device(std::vector<CaptureDevice> &devices_list, const cy_device_usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor, int &curr_serial_extra_id) {
	libusb_device_handle *handle = NULL;
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return LIBUSB_ERROR_NOT_FOUND;
	uint16_t masked_wanted_bcd_device = usb_device_desc->bcd_device_mask & usb_device_desc->bcd_device_wanted_value;
	if(masked_wanted_bcd_device != (usb_descriptor->bcdDevice & usb_device_desc->bcd_device_mask))
		return LIBUSB_ERROR_NOT_FOUND;
	int result = libusb_open(usb_device, &handle);
	if((result < 0) || (handle == NULL))
		return result;
	char manufacturer[0x100];
	char serial[0x100];
	read_strings(handle, usb_descriptor, manufacturer, serial);
	bool claimed = false;
	bool executed_setup = false;
	bool result_setup = cypress_libusb_setup_connection(handle, usb_device_desc, &claimed, executed_setup);
	if(result_setup) {
		std::string device_serial_number = read_real_serial(handle, usb_descriptor, usb_device_desc, serial, curr_serial_extra_id, &claimed, executed_setup);
		cypress_insert_device(devices_list, usb_device_desc, device_serial_number);
	}
	if(claimed)
		libusb_release_interface(handle, usb_device_desc->default_interface);
	libusb_close(handle);
	return result;
}

static void cypress_libusb_fix_device_intial_check(const cy_device_usb_device* usb_device_desc, libusb_device *usb_device, libusb_device_descriptor *usb_descriptor) {
	libusb_device_handle *handle = NULL;
	if((usb_descriptor->idVendor != usb_device_desc->vid) || (usb_descriptor->idProduct != usb_device_desc->pid))
		return;
	uint16_t masked_wanted_bcd_device = usb_device_desc->bcd_device_mask & usb_device_desc->bcd_device_wanted_value;

	if((masked_wanted_bcd_device == 0xFFFF) || (usb_descriptor->bcdDevice != 0xFFFF))
		return;

	int result = libusb_open(usb_device, &handle);
	if((result < 0) || (handle == NULL))
		return;
	libusb_close(handle);
	return;
}

cy_device_device_handlers* cypress_libusb_serial_reconnection(const cy_device_usb_device* usb_device_desc, std::string wanted_serial_number, int &curr_serial_extra_id, CaptureDevice* new_device) {
	if(!usb_is_initialized())
		return NULL;
	libusb_device **usb_devices;
	ssize_t num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};
	cy_device_device_handlers* final_handlers = NULL;

	for(ssize_t i = 0; i < num_devices; i++) {
		cy_device_device_handlers handlers;
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if((usb_descriptor.idVendor != usb_device_desc->vid) || (usb_descriptor.idProduct != usb_device_desc->pid))
			continue;
		uint16_t masked_wanted_bcd_device = usb_device_desc->bcd_device_mask & usb_device_desc->bcd_device_wanted_value;
		if(masked_wanted_bcd_device != (usb_descriptor.bcdDevice & usb_device_desc->bcd_device_mask))
			continue;
		result = libusb_open(usb_devices[i], &handlers.usb_handle);
		if((result < 0) || (handlers.usb_handle == NULL))
			continue;
		char manufacturer[0x100];
		char serial[0x100];
		read_strings(handlers.usb_handle, &usb_descriptor, manufacturer, serial);
		bool claimed = false;
		bool executed_setup = false;
		std::string device_serial_number = read_real_serial(handlers.usb_handle, &usb_descriptor, usb_device_desc, serial, curr_serial_extra_id, &claimed, executed_setup);
		if((wanted_serial_number == device_serial_number) && (cypress_libusb_setup_connection(handlers.usb_handle, usb_device_desc, &claimed, executed_setup))) {
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
	if(interface_claimed)
		libusb_release_interface(handlers->usb_handle, device_desc->default_interface);
	libusb_close(handlers->usb_handle);
	handlers->usb_handle = NULL;
}

void cypress_libusb_find_used_serial(const cy_device_usb_device* usb_device_desc, bool* found, size_t num_free_fw_ids, int &curr_serial_extra_id) {
	if(!usb_is_initialized())
		return;
	libusb_device **usb_devices;
	ssize_t num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	#ifdef ANDROID_COMPILATION
	// Fix issue with first look at USB bcdDevice.
	for(ssize_t i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		cypress_libusb_fix_device_intial_check(usb_device_desc, usb_devices[i], &usb_descriptor);
	}
	#endif

	for(ssize_t i = 0; i < num_devices; i++) {
		cy_device_device_handlers handlers;
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		if((usb_descriptor.idVendor != usb_device_desc->vid) || (usb_descriptor.idProduct != usb_device_desc->pid))
			continue;
		uint16_t masked_wanted_bcd_device = usb_device_desc->bcd_device_mask & usb_device_desc->bcd_device_wanted_value;
		if(masked_wanted_bcd_device != (usb_descriptor.bcdDevice & usb_device_desc->bcd_device_mask))
			continue;
		result = libusb_open(usb_devices[i], &handlers.usb_handle);
		if(result || (handlers.usb_handle == NULL))
			continue;
		char manufacturer[0x100];
		char serial[0x100];
		read_strings(handlers.usb_handle, &usb_descriptor, manufacturer, serial);
		libusb_close(handlers.usb_handle);
		// This is only called when we can set this stuff ourselves...
		// For now, do not update it with the setup part, as there is no
		// case in which it would be used for it...
		std::string device_serial_number = cypress_get_serial(NULL, usb_device_desc, (std::string)(serial), usb_descriptor.bcdDevice, curr_serial_extra_id);
		try {
			int pos = std::stoi(device_serial_number);
			if((pos < 0) || (pos >= ((int)num_free_fw_ids)))
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
	if(device_descriptions.size() <= 0)
		return;

	libusb_device **usb_devices;
	ssize_t num_devices = libusb_get_device_list(get_usb_ctx(), &usb_devices);
	libusb_device_descriptor usb_descriptor{};

	#ifdef ANDROID_COMPILATION
	// Fix issue with first look at USB bcdDevice.
	for(ssize_t i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for(size_t j = 0; j < device_descriptions.size(); j++) {
			cypress_libusb_fix_device_intial_check(device_descriptions[j], usb_devices[i], &usb_descriptor);
		}
	}
	#endif

	for(ssize_t i = 0; i < num_devices; i++) {
		int result = libusb_get_device_descriptor(usb_devices[i], &usb_descriptor);
		if(result < 0)
			continue;
		for(size_t j = 0; j < device_descriptions.size(); j++) {
			result = cypress_libusb_insert_device(devices_list, device_descriptions[j], usb_devices[i], &usb_descriptor, curr_serial_extra_id_cypress[j]);
			if(result != LIBUSB_ERROR_NOT_FOUND) {
				if(result == LIBUSB_ERROR_ACCESS)
					no_access_elems[j] = true;
				if(result == LIBUSB_ERROR_NOT_SUPPORTED)
					not_supported_elems[j] = true;
				break;
			}
			#ifdef _WIN32
			// The IntraOral vendor overwrote the Infineon driver in a recent Windows Update...
			// This is terrible, and Microsoft needs to fix this asap.
			// Prepare a warning, so users can at least understand what is the problem.
			// They will need to uninstall the devices and remove the driver, even from hidden devices... :/
			if((device_descriptions[j]->pid == CYPRESS_EZ_FX2_PID) && (device_descriptions[j]->vid == CYPRESS_EZ_FX2_VID)) {
				// Keep it like this in case multiple vendors do this in the future. :/
				if((usb_descriptor.idVendor == CYPRESS_EZ_FX2_BAD_VID_INTRAORAL) && (usb_descriptor.idProduct == CYPRESS_EZ_FX2_BAD_PID_INTRAORAL)) {
					no_access_elems[j] = true;
					ActualConsoleOutTextError("WARNING: IntraOral device detected. Possible driver issue.");
				}
			}
			#endif
		}
	}

	if(num_devices >= 0)
		libusb_free_device_list(usb_devices, 1);
}

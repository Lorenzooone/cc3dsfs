#ifndef __CYPRESS_NISETRO_DEVICE_LIBUSB_HPP
#define __CYPRESS_NISETRO_DEVICE_LIBUSB_HPP

#include <vector>
#include "capture_structs.hpp"
#include "cypress_nisetro_communications.hpp"

void cypress_libusb_list_devices(std::vector<CaptureDevice> &devices_list, bool* no_access_elems, bool* not_supported_elems, int *curr_serial_extra_id_cypress, const size_t num_cypress_desc);
cyni_device_device_handlers* cypress_libusb_serial_reconnection(const cyni_device_usb_device* usb_device_desc, std::string wanted_serial_number, int &curr_serial_extra_id, CaptureDevice* new_device);
void cypress_libusb_find_used_serial(const cyni_device_usb_device* usb_device_desc, bool* found, size_t num_free_fw_ids, int &curr_serial_extra_id);
void cypress_libusb_end_connection(cyni_device_device_handlers* handlers, const cyni_device_usb_device* device_desc, bool interface_claimed);
int cypress_libusb_bulk_in(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int cypress_libusb_ctrl_bulk_in(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int cypress_libusb_ctrl_bulk_out(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
void cypress_libusb_pipe_reset_bulk_in(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc);
void cypress_libusb_pipe_reset_ctrl_bulk_in(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc);
void cypress_libusb_pipe_reset_ctrl_bulk_out(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc);
int cypress_libusb_ctrl_in(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred);
int cypress_libusb_ctrl_out(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t value, uint16_t index, int* transferred);
int cypress_libusb_async_in_start(cyni_device_device_handlers* handlers, const cyni_device_usb_device* usb_device_desc, uint8_t* buf, int length, cyni_async_callback_data* cb_data);
void cypress_libusb_cancell_callback(cyni_async_callback_data* cb_data);

#endif

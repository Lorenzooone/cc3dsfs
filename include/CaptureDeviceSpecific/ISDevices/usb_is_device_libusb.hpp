#ifndef __USB_IS_DEVICE_LIBUSB_HPP
#define __USB_IS_DEVICE_LIBUSB_HPP

#include <vector>
#include "capture_structs.hpp"
#include "usb_is_device_communications.hpp"

void is_device_libusb_list_devices(std::vector<CaptureDevice>& devices_list, bool* no_access_elems, bool* not_supported_elems, int* curr_serial_extra_id_is_device, const size_t num_is_device_desc);
is_device_device_handlers* is_device_libusb_serial_reconnection(const is_device_usb_device* usb_device_desc, CaptureDevice* device, int& curr_serial_extra_id);
void is_device_libusb_end_connection(is_device_device_handlers* handlers, const is_device_usb_device* device_desc, bool interface_claimed);
int is_device_libusb_bulk_out(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int is_device_libusb_bulk_in(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int is_device_libusb_ctrl_in(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, uint8_t request, uint16_t index, int* transferred);
void is_device_libusb_async_in_start(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, isd_async_callback_data* cb_data);
void is_device_libusb_cancell_callback(isd_async_callback_data* cb_data);
int is_device_libusb_reset_device(is_device_device_handlers* handlers);

void is_device_libusb_start_thread(std::thread* thread_ptr, bool* usb_thread_run);
void is_device_libusb_close_thread(std::thread* thread_ptr, bool* usb_thread_run);

#endif

#ifndef __USB_IS_DEVICE_IS_DRIVER_HPP
#define __USB_IS_DEVICE_IS_DRIVER_HPP

#include <vector>
#include "capture_structs.hpp"
#include "utils.hpp"
#include "usb_is_device_communications.hpp"
#include "usb_is_device_acquisition_general.hpp"

void is_driver_list_devices(std::vector<CaptureDevice>& devices_list, bool* not_supported_elems, int* curr_serial_extra_id_is_device, std::vector<const is_device_usb_device*> &device_descriptions);
is_device_device_handlers* is_driver_serial_reconnection(CaptureDevice* device);
void is_driver_end_connection(is_device_device_handlers* handlers);
bool is_driver_prepare_ctrl_in_handle(is_device_device_handlers* handlers);
void is_driver_close_ctrl_in_handle(is_device_device_handlers* handlers);
int is_driver_bulk_out(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int is_driver_bulk_in(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int is_driver_ctrl_in(is_device_device_handlers* handlers, uint8_t* buf, int length, uint8_t request, uint16_t index, int* transferred);
int is_drive_async_in_start(is_device_device_handlers* handlers, const is_device_usb_device* usb_device_desc, uint8_t* buf, int length, isd_async_callback_data* cb_data);
void is_device_is_driver_cancel_callback(isd_async_callback_data* cb_data);
int is_device_is_driver_reset_device(is_device_device_handlers* handlers);

void is_device_is_driver_start_thread(std::thread* thread_ptr, bool* usb_thread_run, ISDeviceCaptureReceivedData* is_device_capture_recv_data, is_device_device_handlers* handlers, ConsumerMutex* AsyncMutexPtr);
void is_device_is_driver_close_thread(std::thread* thread_ptr, bool* usb_thread_run, ISDeviceCaptureReceivedData* is_device_capture_recv_data);

#endif

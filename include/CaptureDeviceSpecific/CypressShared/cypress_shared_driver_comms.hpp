#ifndef __CYPRESS_NISETRO_DRIVER_COMMS_HPP
#define __CYPRESS_NISETRO_DRIVER_COMMS_HPP

#include <vector>
#include "capture_structs.hpp"
#include "utils.hpp"
#include "cypress_shared_communications.hpp"

enum CypressWindowsDriversEnum { CYPRESS_WINDOWS_DEFAULT_USB_DRIVER };

void cypress_driver_list_devices(std::vector<CaptureDevice>& devices_list, bool* not_supported_elems, int* curr_serial_extra_id_cypress, std::vector<const cy_device_usb_device*> &device_descriptions, CypressWindowsDriversEnum driver);
void cypress_driver_find_used_serial(const cy_device_usb_device* usb_device_desc, bool* found, size_t num_free_fw_ids, int &curr_serial_extra_id, CypressWindowsDriversEnum driver);
cy_device_device_handlers* cypress_driver_find_by_serial_number(const cy_device_usb_device* usb_device_desc, std::string wanted_serial_number, int &curr_serial_extra_id, CaptureDevice* new_device, CypressWindowsDriversEnum driver);
cy_device_device_handlers* cypress_driver_serial_reconnection(CaptureDevice* device);
void cypress_nisetro_driver_end_connection(cy_device_device_handlers* handlers);
int cypress_driver_ctrl_transfer_in(cy_device_device_handlers* handlers, uint8_t* buffer, size_t inner_size, uint16_t value, uint16_t index, uint16_t request_code, int* num_read);
int cypress_driver_ctrl_transfer_out(cy_device_device_handlers* handlers, uint8_t* buffer, size_t inner_size, uint16_t value, uint16_t index, uint16_t request_code, int* num_read);
int cypress_driver_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int cypress_driver_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
int cypress_driver_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, int* transferred);
void cypress_driver_pipe_reset_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc);
void cypress_driver_pipe_reset_ctrl_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc);
void cypress_driver_pipe_reset_ctrl_bulk_out(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc);
int cypress_driver_async_in_start(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, uint8_t* buf, int length, cy_async_callback_data* cb_data);
void cypress_driver_start_thread(std::thread* thread_ptr, bool* usb_thread_run, std::vector<cy_async_callback_data*> &cb_data_vector, cy_device_device_handlers* handlers);
void cypress_driver_close_thread(std::thread* thread_ptr, bool* usb_thread_run, std::vector<cy_async_callback_data*> cb_data_vector);
void cypress_driver_cancel_callback(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, cy_async_callback_data* cb_data);
void cypress_driver_set_transfer_size_bulk_in(cy_device_device_handlers* handlers, const cy_device_usb_device* usb_device_desc, size_t new_transfer_size);

#endif

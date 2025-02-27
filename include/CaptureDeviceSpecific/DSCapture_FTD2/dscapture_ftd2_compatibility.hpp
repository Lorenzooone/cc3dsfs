#ifndef __DSCAPTURE_FTD2_COMPATIBILITY_HPP
#define __DSCAPTURE_FTD2_COMPATIBILITY_HPP

#include "devicecapture.hpp"

#define DEFAULT_LIMIT_SIZE_TRANSFER 0x10000

void list_devices_ftd2_compatibility(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list);
void ftd2_capture_main_loop(CaptureData* capture_data);
int ftd2_get_queue_status(void* handle, bool is_libftdi, size_t* bytes_in);
bool ftd2_is_error(int value, bool is_libftdi);
int ftd2_write(void* handle, bool is_libftdi, const uint8_t* data, size_t size, size_t *sent);
int ftd2_read(void* handle, bool is_libftdi, uint8_t* data, size_t size, size_t *bytesIn);
int ftd2_set_timeouts(void* handle, bool is_libftdi, int timeout_read, int timeout_write);
int ftd2_reset_device(void* handle, bool is_libftdi);
int ftd2_set_usb_parameters(void* handle, bool is_libftdi, size_t size_in, size_t size_out);
int ftd2_set_chars(void* handle, bool is_libftdi, unsigned char eventch, unsigned char event_enable, unsigned char errorch, unsigned char error_enable);
int ftd2_set_latency_timer(void* handle, bool is_libftdi, unsigned char latency);
int ftd2_set_flow_ctrl_rts_cts(void* handle, bool is_libftdi);
int ftd2_reset_bitmode(void* handle, bool is_libftdi);
int ftd2_set_mpsse_bitmode(void* handle, bool is_libftdi);
int ftd2_set_fifo_bitmode(void* handle, bool is_libftdi);
int ftd2_purge_all(void* handle, bool is_libftdi);
int ftd2_read_ee(void* handle, bool is_libftdi, int eeprom_addr, int *eeprom_val);
int ftd2_close(void* handle, bool is_libftdi);
int ftd2_open(CaptureDevice* device, void** handle, bool is_libftdi);
void ftd2_init();
void ftd2_end();

#endif

#ifndef __DSCAPTURE_LIBFTDI2_HPP
#define __DSCAPTURE_LIBFTDI2_HPP

#include "dscapture_ftd2_shared.hpp"
#include "capture_structs.hpp"

void libftdi_init();
void libftdi_end();

void list_devices_libftdi(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list);
void ftd2_capture_main_loop_libftdi(CaptureData* capture_data);
int libftdi_reset(void* handle);
int libftdi_set_latency_timer(void* handle, unsigned char latency);
int libftdi_setflowctrl(void* handle, int flowctrl,  unsigned char xon, unsigned char xoff);
int libftdi_set_bitmode(void* handle, unsigned char bitmask, unsigned char mode);
int libftdi_purge(void* handle, bool do_read, bool do_write);
int libftdi_read_eeprom(void* handle, int eeprom_addr, int *eeprom_val);
int libftdi_set_chars(void* handle, unsigned char eventch, unsigned char event_enable, unsigned char errorch, unsigned char error_enable);
int libftdi_set_usb_chunksizes(void* handle, unsigned int chunksize_in, unsigned int chunksize_out);
void libftdi_set_timeouts(void* handle, int timeout_in_ms, int timeout_out_ms);
int libftdi_write(void* handle, const uint8_t* data, size_t size, size_t* bytesOut);
int libftdi_read(void* handle, uint8_t* data, size_t size, size_t* bytesIn);
int get_libftdi_read_queue_size(void* handle, size_t* bytesIn);
int libftdi_open_serial(CaptureDevice* device, void** handle);
int libftdi_close(void* handle);

#endif

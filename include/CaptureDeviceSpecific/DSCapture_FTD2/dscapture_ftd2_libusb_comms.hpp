#ifndef __DSCAPTURE_FTD2_LIBUSB_COMMS_HPP
#define __DSCAPTURE_FTD2_LIBUSB_COMMS_HPP

#include "dscapture_ftd2_shared.hpp"
#include "capture_structs.hpp"

enum ftd2_mpsse_mode {
	FTD2_MPSSE_BITMODE_RESET  = 0x00,
	FTD2_MPSSE_BITMODE_MPSSE  = 0x02,
	FTD2_MPSSE_BITMODE_SYNCFIFO = 0x40,
};
enum ftd2_flow_ctrls {
	FTD2_SIO_RTS_CTS_HS = 0x1 << 8,
	FTD2_SIO_XON_XOFF_HS = 0x4 << 8,
};

void ftd2_libusb_init();
void ftd2_libusb_end();

// Generic functions
void list_devices_ftd2_libusb(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list);
int ftd2_libusb_reset(void* handle);
int ftd2_libusb_set_latency_timer(void* handle, unsigned char latency);
int ftd2_libusb_setflowctrl(void* handle, int flowctrl,  unsigned char xon, unsigned char xoff);
int ftd2_libusb_set_bitmode(void* handle, unsigned char bitmask, unsigned char mode);
int ftd2_libusb_purge(void* handle, bool do_read, bool do_write);
int ftd2_libusb_read_eeprom(void* handle, int eeprom_addr, int *eeprom_val);
int ftd2_libusb_set_chars(void* handle, unsigned char eventch, unsigned char event_enable, unsigned char errorch, unsigned char error_enable);
int ftd2_libusb_set_usb_chunksizes(void* handle, unsigned int chunksize_in, unsigned int chunksize_out);
void ftd2_libusb_set_timeouts(void* handle, int timeout_in_ms, int timeout_out_ms);
int ftd2_libusb_write(void* handle, const uint8_t* data, size_t size, size_t* bytesOut);
int ftd2_libusb_read(void* handle, uint8_t* data, size_t size, size_t* bytesIn);
int get_ftd2_libusb_read_queue_size(void* handle, size_t* bytesIn);
int ftd2_libusb_open_serial(CaptureDevice* device, void** handle);
int ftd2_libusb_close(void* handle);

// Functions which are related to the raw buffer
size_t ftd2_libusb_get_expanded_length(const int max_packet_size, size_t length, size_t header_packet_size);
size_t ftd2_libusb_get_expanded_length(size_t length);
size_t ftd2_libusb_get_actual_length(const int max_packet_size, size_t length, size_t header_packet_size);
size_t ftd2_libusb_get_actual_length(size_t length);
void ftd2_libusb_copy_buffer_to_target(uint8_t* buffer_written, uint8_t* buffer_target, const int max_packet_size, size_t length, size_t header_packet_size);
void ftd2_libusb_copy_buffer_to_target(uint8_t* buffer_written, uint8_t* buffer_target, size_t length);
int ftd2_libusb_force_read_with_timeout(void* handle, uint8_t* buffer_raw, uint8_t* buffer_normal, size_t length, double timeout);
int ftd2_libusb_async_bulk_in_prepare_and_submit(void* handle, void *transfer_in, uint8_t* buffer_raw, size_t size, void* cb_fn, void* cb_data, int timeout_multiplier = 1);

#endif

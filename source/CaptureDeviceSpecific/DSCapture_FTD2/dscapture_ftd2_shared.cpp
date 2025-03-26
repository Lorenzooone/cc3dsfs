#include "dscapture_ftd2_shared.hpp"
#include "dscapture_ftd2_general.hpp"
#include "dscapture_ftd2_compatibility.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"

#include "ftd2_ds2_fw_1.h"
#include "ftd2_ds2_fw_2.h"


#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define LE16(x) ((x) & 0xff), (((x) >> 8) & 0xff)

#define FTD2XX_IN_SIZE DEFAULT_LIMIT_SIZE_TRANSFER
#define FTD2XX_OUT_SIZE DEFAULT_LIMIT_SIZE_TRANSFER

#define CLKRATE 6 // 1 - 25MHz
#define CLKDIV ((30 / CLKRATE) - 1)
#define RESETDELAY ((1 * CLKRATE) / 8) //1uS in bytes (clks/8)
#define INITDELAY ((1200 * CLKRATE) / 8) //1200uS

#define TX_SPI_SIZE (1 << 16)
#define TX_SPI_OFFSET 3

#define ENABLE_AUDIO true

const uint8_t* ftd2_ds2_fws[] = {
ftd2_ds2_fw_1,
ftd2_ds2_fw_2,
};

const size_t ftd2_ds2_sizes[] = {
ftd2_ds2_fw_1_len,
ftd2_ds2_fw_2_len,
};

const std::string valid_descriptions[] = {"NDS.1", "NDS.2"};
const int descriptions_firmware_ids[] = {1, 2};

// Code based on sample provided by Loopy.

int get_num_ftd2_device_types() {
	return sizeof(descriptions_firmware_ids) / sizeof(descriptions_firmware_ids[0]);
}

const std::string get_ftd2_fw_desc(int index) {
	if((index < 0) || (index >= get_num_ftd2_device_types()))
		return "";
	return valid_descriptions[index];
}

const int get_ftd2_fw_index(int index) {
	if((index < 0) || (index >= get_num_ftd2_device_types()))
		return 0;
	return descriptions_firmware_ids[index];
}

void insert_device_ftd2_shared(std::vector<CaptureDevice> &devices_list, char* description, int debug_multiplier, std::string serial_number, void* descriptor, std::string path, std::string extra_particle) {
	for(int j = 0; j < get_num_ftd2_device_types(); j++) {
		if(description == get_ftd2_fw_desc(j)) {
			for(int u = 0; u < debug_multiplier; u++)
				devices_list.emplace_back(serial_number, "DS.2", "DS.2." + extra_particle + "565", CAPTURE_CONN_FTD2, descriptor, false, false, ENABLE_AUDIO, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, get_max_samples(false), 0, 0, 0, 0, HEIGHT_DS, VIDEO_DATA_RGB16, get_ftd2_fw_index(j), false, path);
			break;
		}
	}
}

void list_devices_ftd2_shared(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	list_devices_ftd2_compatibility(devices_list, no_access_list);
}

static uint64_t _ftd2_get_video_in_size(bool is_rgb888) {
	return sizeof(USBOldDSVideoInputData);
}

uint64_t ftd2_get_video_in_size(CaptureData* capture_data) {
	return _ftd2_get_video_in_size(capture_data->status.device.is_rgb_888);
}

uint64_t get_capture_size(bool is_rgb888) {
	return (((sizeof(FTD2OldDSCaptureReceived) - EXTRA_DATA_BUFFER_USB_SIZE) + (MAX_PACKET_SIZE_FTD2 - 1)) / MAX_PACKET_SIZE_FTD2) * MAX_PACKET_SIZE_FTD2; // multiple of maxPacketSize
}

uint64_t get_max_samples(bool is_rgb888) {
	return ((get_capture_size(is_rgb888) - _ftd2_get_video_in_size(is_rgb888)) / 2) - 4; // The last 4 bytes should never be different from 0x43214321
}

static bool pass_if_FT_queue_empty(void* handle, bool is_ftd2_libusb) {
	size_t bytesIn = 0;
	int retval = ftd2_get_queue_status(handle, is_ftd2_libusb, &bytesIn);
	if(ftd2_is_error(retval, is_ftd2_libusb) || (bytesIn != 0)) //should be empty
		return false;
	return true;
}

static bool ftd2_write_all_check(void* handle, bool is_ftd2_libusb, const uint8_t* data, size_t size) {
	size_t sent = 0;
	int retval = ftd2_write(handle, is_ftd2_libusb, data, size, &sent);
	if(ftd2_is_error(retval, is_ftd2_libusb) || (sent != size))
		return false;
	return true;
}

static bool full_ftd2_write(void* handle, bool is_ftd2_libusb, const uint8_t* data, size_t size) {
	if(!pass_if_FT_queue_empty(handle, is_ftd2_libusb)) // maybe MPSSE error?
		return false;
	return ftd2_write_all_check(handle, is_ftd2_libusb, data, size);
}

//verify CDONE==val
static bool check_cdone(void* handle, bool is_ftd2_libusb, bool want_active) {
	static const uint8_t cmd[] = {
		0x81,	//read D
		0x83,	//read C
	};
	uint8_t buf[2];

	if(!full_ftd2_write(handle, is_ftd2_libusb, cmd, sizeof(cmd)))
		return false;

	size_t bytesIn = 0;
	int retval = ftd2_read(handle, is_ftd2_libusb, buf, 2, &bytesIn);
	if(ftd2_is_error(retval, is_ftd2_libusb) || (bytesIn != 2) || (buf[0] == 0xFA))
		return false;

	if(want_active)
		return (buf[0] & 0x40);
	return !(buf[0] & 0x40);
}

static int end_spi_tx(uint8_t *txBuf, int retval) {
	delete []txBuf;
	return retval;
}

static int ftd2_spi_tx(void* handle, bool is_ftd2_libusb, const uint8_t* buf, int size) {
	uint8_t *txBuf = new uint8_t[TX_SPI_SIZE + TX_SPI_OFFSET];
	int retval = 0;
	int len;
	size_t sent;
	size_t bytesIn;
	size_t wrote = 0;

	if(!pass_if_FT_queue_empty(handle, is_ftd2_libusb))
		return end_spi_tx(txBuf, -1);

	while(size > 0) {
		len = size;
		if(len > TX_SPI_SIZE)
			len = TX_SPI_SIZE;
		memcpy(&txBuf[TX_SPI_OFFSET], &buf[wrote], len);

		txBuf[0] = 0x11;
		txBuf[1] = (len - 1) & 0xFF;
		txBuf[2] = ((len - 1) >> 8) & 0xFF;

		retval = ftd2_write(handle, is_ftd2_libusb, txBuf, len + TX_SPI_OFFSET, &sent);
		if(ftd2_is_error(retval, is_ftd2_libusb))
			return end_spi_tx(txBuf, retval);
		if(sent != (len + TX_SPI_OFFSET))
			return end_spi_tx(txBuf, -2);

		if(!pass_if_FT_queue_empty(handle, is_ftd2_libusb))
			return end_spi_tx(txBuf, -1);

		wrote += sent - TX_SPI_OFFSET;
		size -= len;
	}
	return end_spi_tx(txBuf, 0);
}

static bool fpga_config(void* handle, bool is_ftd2_libusb, const uint8_t* bitstream, int size) {
	//D6=CDONE (in)
	//D3=SS (out)
	//D2=TDO (in)
	//D1=TDI (out)
	//D0=TCK (out)
	//C7=reset (out)

	//Lattice programming:
	//  hold SS low
	//  pulse reset 200ns
	//  wait 1200us
	//  release SS
	//  send 8 clocks
	//  hold SS low
	//  send configuration MSB first, sampled on +clk
	//  release SS
	//  CDONE should go high within 100 clocks
	//  send >49 clocks after CDONE=1

	static const uint8_t cmd0[] = {	//configure for lattice SPI:
		0x82, 0x7f, 0x80,			//set Cx pins {val,dir:1=out}: reset=0
		0x80, 0xf0, 0x0b,			//set Dx pins {val,dir:1=out}: SS=0, clk idle low, TDI out, TCK out
		0x8f, LE16(RESETDELAY),		//reset delay
		0x82, 0xff, 0x00,			//set Cx pins: reset=PU (does val=1 enable pullups? seems to...)
		0x8f, LE16(INITDELAY),		//init delay
		0x80, 0xf8, 0x0b,			//set Dx pins: SS=1
		0x8f, LE16(1),				//16 dummy clocks
		0x80, 0xf0, 0x0b,			//set Dx pins: SS=0
	};

	int retval = 0;
	retval = ftd2_set_timeouts(handle, is_ftd2_libusb, 300, 300);
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	if(!full_ftd2_write(handle, is_ftd2_libusb, cmd0, sizeof(cmd0)))
		return false;

	//verify CDONE=0
	if(!check_cdone(handle, is_ftd2_libusb, false))
		return false;

	//send configuration
	retval = ftd2_spi_tx(handle, is_ftd2_libusb, bitstream, size);
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;

	//finish up
	static const uint8_t cmd1[] = {
		0x80, 0xf8, 0x0b,		   //set Dx pins: SS=1
		0x8f, LE16(20),			 //>150 dummy clocks
		0x80, 0x00, 0x00,		   //float Dx
		0x82, 0x00, 0x00,		   //float Cx
	};
	if(!full_ftd2_write(handle, is_ftd2_libusb, cmd1, sizeof(cmd1)))
		return false;

	//verify CDONE=1
	if(!check_cdone(handle, is_ftd2_libusb, true))
		return false;

	return true;
}

static bool init_MPSSE(void* handle, bool is_ftd2_libusb) {
	static const uint8_t cmd[] = {
		0x85,					//no loopback
		0x8d,					//disable 3-phase clocking
		0x8a,					//disable clk divide-by-5
		0x97,					//disable adaptive clocking
		0x86, LE16(CLKDIV),		//set clock rate
	};

	int retval = 0;
	retval = ftd2_reset_device(handle, is_ftd2_libusb);
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	retval = ftd2_set_usb_parameters(handle, is_ftd2_libusb, FTD2XX_IN_SIZE, FTD2XX_OUT_SIZE); //Multiple of 64 bytes up to 64k
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	retval = ftd2_set_chars(handle, is_ftd2_libusb, 0, 0, 0, 0);
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	retval = ftd2_set_timeouts(handle, is_ftd2_libusb, 300, 300); //read,write timeout (ms)
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	retval = ftd2_set_latency_timer(handle, is_ftd2_libusb, 3); //time to wait before incomplete packet is sent (default=16ms). MPSSE read seems to fail on 2 sometimes, too fast?
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	retval = ftd2_set_flow_ctrl_rts_cts(handle, is_ftd2_libusb); //turn on flow control to synchronize IN requests
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	retval = ftd2_reset_bitmode(handle, is_ftd2_libusb); //performs a general reset on MPSSE
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	retval = ftd2_set_mpsse_bitmode(handle, is_ftd2_libusb); //enable MPSSE mode
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;

	//Try improving transfer rate
	retval = ftd2_set_usb_parameters(handle, is_ftd2_libusb, ((sizeof(FTD2OldDSCaptureReceivedRaw) + DEFAULT_LIMIT_SIZE_TRANSFER - 1) / DEFAULT_LIMIT_SIZE_TRANSFER) * DEFAULT_LIMIT_SIZE_TRANSFER, FTD2XX_OUT_SIZE); //The programmer's guide lies?
	if(ftd2_is_error(retval, is_ftd2_libusb))
		retval = ftd2_set_usb_parameters(handle, is_ftd2_libusb, FTD2XX_IN_SIZE, FTD2XX_OUT_SIZE); //Backup to 64 bytes up to 64k, if more is not supported...
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;

	//MPSSE seems to choke on first write sometimes :/  Send, purge, resend.
	size_t sent = 0;
	retval = ftd2_write(handle, is_ftd2_libusb, cmd, 1, &sent); //enable MPSSE mode
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	default_sleep();
	retval = ftd2_purge_all(handle, is_ftd2_libusb);
	if(ftd2_is_error(retval, is_ftd2_libusb))
		return false;
	return full_ftd2_write(handle, is_ftd2_libusb, cmd, sizeof(cmd));
}

static bool preemptive_close_connection(CaptureData* capture_data, bool is_ftd2_libusb) {
	ftd2_reset_device(capture_data->handle, is_ftd2_libusb);
	ftd2_close(capture_data->handle, is_ftd2_libusb);
	return false;
}

bool connect_ftd2_shared(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	bool is_ftd2_libusb = device->descriptor != NULL;
	if(ftd2_open(device, &capture_data->handle, is_ftd2_libusb)) {
		capture_error_print(print_failed, capture_data, "Create failed");
		return false;
	}

	if(!init_MPSSE(capture_data->handle, is_ftd2_libusb)) {
		capture_error_print(print_failed, capture_data, "MPSSE init failed");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}

	if(!fpga_config(capture_data->handle, is_ftd2_libusb, ftd2_ds2_fws[device->firmware_id - 1], ftd2_ds2_sizes[device->firmware_id - 1])) {
		capture_error_print(print_failed, capture_data, "FPGA config failed");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}

	int retval = 0;
	int val = 0;
	retval = ftd2_read_ee(capture_data->handle, is_ftd2_libusb, 1, &val);
	if(ftd2_is_error(retval, is_ftd2_libusb) || (val != 0x0403)) { //=85A8: something went wrong (fpga is configured but FT chip detected wrong eeprom size)
		capture_error_print(print_failed, capture_data, "EEPROM read error");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}

	/*
	int Firmware = 0;
	int Hardware = 0;
	retval = ftd2_read_ee(capture_data->handle, is_ftd2_libusb, 0x10, &Firmware);
	if(ftd2_is_error(retval, is_ftd2_libusb)) {
		capture_error_print(print_failed, capture_data, "Firmware ID read error");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}
	retval = ftd2_read_ee(capture_data->handle, is_ftd2_libusb, 0x11, &Hardware);
	if(ftd2_is_error(retval, is_ftd2_libusb)) {
		capture_error_print(print_failed, capture_data, "Hardware ID read error");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}
	*/

	retval = ftd2_set_fifo_bitmode(capture_data->handle, is_ftd2_libusb);   //to FIFO mode. This takes over port B, pins shouldn't get modified though
	if(ftd2_is_error(retval, is_ftd2_libusb)) {
		capture_error_print(print_failed, capture_data, "Bitmode setup error");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}

	retval = ftd2_set_timeouts(capture_data->handle, is_ftd2_libusb, 50, 50);
	if(ftd2_is_error(retval, is_ftd2_libusb)) {
		capture_error_print(print_failed, capture_data, "Timeouts setup error");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}

	if(!pass_if_FT_queue_empty(capture_data->handle, is_ftd2_libusb)) {// maybe MPSSE error?
		capture_error_print(print_failed, capture_data, "Intermediate error");
		return preemptive_close_connection(capture_data, is_ftd2_libusb);
	}

	return true;
}

bool synchronization_check(uint16_t* data_buffer, size_t size, uint16_t* next_data_buffer, size_t* next_size, bool special_check) {
	size_t size_words = size / 2;
	*next_size = size;
	if((data_buffer[0] != FTD2_OLDDS_SYNCH_VALUES) && (data_buffer[size_words - 1] == FTD2_OLDDS_SYNCH_VALUES))
		return true;
	if(special_check && (data_buffer[0] == FTD2_OLDDS_SYNCH_VALUES) && (data_buffer[size_words - 1] == FTD2_OLDDS_SYNCH_VALUES))
		return true;

	//check sync
	size_t samples = 0;

	// Find the first spot the padding is present
	while((samples < size_words) && (data_buffer[samples] != FTD2_OLDDS_SYNCH_VALUES))
		samples++;
	while((samples < size_words) && (data_buffer[samples] == FTD2_OLDDS_SYNCH_VALUES))
		samples++;

	// Schedule a read to re-synchronize
	if(next_data_buffer != NULL) {
		memset(next_data_buffer, 0, size_words * 2);
		memcpy(&data_buffer[samples], next_data_buffer, (size_words - samples) * 2);
	}
	*next_size = samples * 2;
	return false;
}

size_t remove_synch_from_final_length(uint32_t* out_buffer, size_t real_length) {
	// Ignore synch for final length
	uint32_t check_value = FTD2_OLDDS_SYNCH_VALUES | (FTD2_OLDDS_SYNCH_VALUES << 16);
	size_t check_size = sizeof(uint32_t);
	while((real_length >= check_size) && (out_buffer[(real_length / check_size) - 1] == check_value))
		real_length -= check_size;
	check_value = FTD2_OLDDS_SYNCH_VALUES;
	uint16_t* u16_buffer = (uint16_t*)out_buffer;
	check_size = sizeof(uint16_t);
	while((real_length >= check_size) && (u16_buffer[(real_length / check_size) - 1] == check_value))
		real_length -= check_size;
	if((real_length < check_size) && (u16_buffer[0] == check_value))
		real_length = 0;
	return real_length;
}

bool enable_capture(void* handle, bool is_ftd2_libusb) {
	static const uint8_t cmd[]={ 0x80, 0x01 }; //enable capture
	return ftd2_write_all_check(handle, is_ftd2_libusb, cmd, sizeof(cmd));
}

void ftd2_capture_main_loop_shared(CaptureData* capture_data) {
	ftd2_capture_main_loop(capture_data);
}

void ftd2_capture_cleanup_shared(CaptureData* capture_data) {
	for(int i = 0; i < NUM_CAPTURE_RECEIVED_DATA_BUFFERS; i++)
		capture_data->data_buffers.ReleaseWriterBuffer(i, false);
	bool is_ftd2_libusb = capture_data->status.device.descriptor != NULL;
	if(ftd2_close(capture_data->handle, is_ftd2_libusb)) {
		capture_error_print(true, capture_data, "Disconnected: Close failed");
	}
}

void ftd2_init_shared() {
	ftd2_init();
}

void ftd2_end_shared() {
	ftd2_end();
}

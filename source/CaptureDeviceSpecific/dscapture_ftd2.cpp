#include "dscapture_ftd2.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"
//#include "ftd2xx_symbols_renames.h"

#include "ftd2_ds2_fw_1.h"
#include "ftd2_ds2_fw_2.h"

#define FTD2XX_STATIC
#include <ftd2xx.h>

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define FT_FAILED(x) ((x) != FT_OK)

#define FTD2XX_VID 0x0403

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

#define LE16(x) ((x) & 0xff), (((x) >> 8) & 0xff)

#define FTD2XX_IN_SIZE 0x10000
#define FTD2XX_OUT_SIZE 0x10000

#define CLKRATE 6 // 1 - 25MHz
#define CLKDIV ((30 / CLKRATE) - 1)
#define RESETDELAY ((1 * CLKRATE) / 8) //1uS in bytes (clks/8)
#define INITDELAY ((1200 * CLKRATE) / 8) //1200uS

#define TX_SPI_SIZE (1 << 16)
#define TX_SPI_OFFSET 3

#define MAX_PACKET_SIZE_FTD2 510
#define ENABLE_AUDIO true

const uint16_t ftd2xx_valid_vids[] = {FTD2XX_VID};
const uint16_t ftd2xx_valid_pids[] = {0x6014};

const uint8_t* ftd2_ds2_fws[] = {
ftd2_ds2_fw_1,
ftd2_ds2_fw_2,
};

const size_t ftd2_ds2_sizes[] = {
ftd2_ds2_fw_1_len,
ftd2_ds2_fw_2_len,
};

void list_devices_ftd2(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	FT_STATUS ftStatus;
	DWORD numDevs = 0;
	std::string valid_descriptions[] = {"NDS.1", "NDS.2"};
	int descriptions_firmware_ids[] = {1, 2};
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	size_t num_inserted = 0;
	if (!FT_FAILED(ftStatus) && numDevs > 0)
	{
		const int debug_multiplier = 1;
		FT_HANDLE ftHandle = NULL;
		DWORD Flags = 0;
		DWORD Type = 0;
		DWORD ID = 0;
		char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
		char Description[65] = { 0 };
		for (DWORD i = 0; i < numDevs; i++)
		{
			ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, NULL,
			SerialNumber, Description, &ftHandle);
			if((!FT_FAILED(ftStatus)) && (Flags & FT_FLAGS_HISPEED) && (Type == FT_DEVICE_232H))
			{
				for(int j = 0; j < sizeof(valid_descriptions) / sizeof(*valid_descriptions); j++) {
					if(Description == valid_descriptions[j]) {
						for(int u = 0; u < debug_multiplier; u++)
							devices_list.emplace_back(std::string(SerialNumber), "DS.2", CAPTURE_CONN_FTD2, (void*)NULL, false, false, ENABLE_AUDIO, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, 0, 0, 0, 0, 0, HEIGHT_DS, VIDEO_DATA_RGB16, descriptions_firmware_ids[j], false);
						break;
					}
				}
			}
		}
	}
	if(num_inserted == 0) {
		#if defined(USE_IS_NITRO_USB) || defined(USE_DS_3DS_USB)
		if(get_usb_total_filtered_devices(ftd2xx_valid_vids, sizeof(ftd2xx_valid_vids) / sizeof(ftd2xx_valid_vids[0]), ftd2xx_valid_pids, sizeof(ftd2xx_valid_pids) / sizeof(ftd2xx_valid_pids[0])) != numDevs)
			no_access_list.emplace_back("FTD2XX");
		#endif
	}
}

uint64_t ftd2_get_video_in_size(CaptureData* capture_data) {
	return sizeof(USBOldDSVideoInputData);
}

static uint64_t get_capture_size(CaptureData* capture_data) {
	return (((sizeof(FTD2OldDSCaptureReceived) - EXTRA_DATA_BUFFER_USB_SIZE) + (MAX_PACKET_SIZE_FTD2 - 1)) / MAX_PACKET_SIZE_FTD2) * MAX_PACKET_SIZE_FTD2; // multiple of maxPacketSize
}

static bool pass_if_FT_queue_empty(FT_HANDLE handle) {
	DWORD bytesIn = 0;
	FT_STATUS ftStatus;
	ftStatus = FT_GetQueueStatus(handle, &bytesIn);
	if(FT_FAILED(ftStatus) || (bytesIn != 0)) //should be empty
		return false;
	return true;
}

static bool full_ftd2_write(FT_HANDLE handle, const uint8_t* data, size_t size) {
	FT_STATUS ftStatus;
	if(!pass_if_FT_queue_empty(handle)) // maybe MPSSE error?
		return false;

	DWORD sent;
	ftStatus = FT_Write(handle, (void*)data, size, &sent);
	if(FT_FAILED(ftStatus) || (sent != size))
		return false;
	return true;
}

//verify CDONE==val
static bool check_cdone(FT_HANDLE handle, bool want_active) {
	static const uint8_t cmd[] = {
		0x81,	//read D
		0x83,	//read C
	};
	uint8_t buf[2];

	if(!full_ftd2_write(handle, cmd, sizeof(cmd)))
		return false;

	DWORD bytesIn;
	FT_STATUS ftStatus = FT_Read(handle, buf, 2, &bytesIn);
	if(FT_FAILED(ftStatus) || (bytesIn != 2) || (buf[0] == 0xFA))
		return false;

	if(want_active)
		return (buf[0] & 0x40);
	return !(buf[0] & 0x40);
}

static FT_STATUS end_spi_tx(uint8_t *txBuf, FT_STATUS ftStatus) {
	delete []txBuf;
	return ftStatus;
}

static FT_STATUS ftd2_spi_tx(FT_HANDLE handle, const uint8_t* buf, int size) {
	uint8_t *txBuf = new uint8_t[TX_SPI_SIZE + TX_SPI_OFFSET];
	FT_STATUS ftStatus;
	int len;
	DWORD sent;
	DWORD bytesIn;
	DWORD wrote = 0;

	if(!pass_if_FT_queue_empty(handle))
		return end_spi_tx(txBuf, FT_OTHER_ERROR);

	while(size > 0) {
		len = size;
		if(len > TX_SPI_SIZE)
			len = TX_SPI_SIZE;
		memcpy(&txBuf[TX_SPI_OFFSET], &buf[wrote], len);

		txBuf[0] = 0x11;
		txBuf[1] = (len - 1) & 0xFF;
		txBuf[2] = ((len - 1) >> 8) & 0xFF;

		ftStatus = FT_Write(handle, (void*)txBuf, len + TX_SPI_OFFSET, &sent);
		if(FT_FAILED(ftStatus))
			return end_spi_tx(txBuf, ftStatus);
		if(sent != (len + TX_SPI_OFFSET))
			return end_spi_tx(txBuf, FT_INSUFFICIENT_RESOURCES);

		if(!pass_if_FT_queue_empty(handle))
			return end_spi_tx(txBuf, FT_OTHER_ERROR);

		wrote += sent - TX_SPI_OFFSET;
		size -= len;
	}
	return end_spi_tx(txBuf, FT_OK);
}

static bool fpga_config(FT_HANDLE handle, const uint8_t* bitstream, int size) {
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

	FT_STATUS ftStatus;
	ftStatus = FT_SetTimeouts(handle, 300, 300);
	if(FT_FAILED(ftStatus))
		return false;
	if(!full_ftd2_write(handle, cmd0, sizeof(cmd0)))
		return false;

	//verify CDONE=0
	if(!check_cdone(handle, false))
		return false;

	//send configuration
	ftStatus = ftd2_spi_tx(handle, bitstream, size);
	if(FT_FAILED(ftStatus))
		return false;

	//finish up
	static const uint8_t cmd1[] = {
		0x80, 0xf8, 0x0b,		   //set Dx pins: SS=1
		0x8f, LE16(20),			 //>150 dummy clocks
		0x80, 0x00, 0x00,		   //float Dx
		0x82, 0x00, 0x00,		   //float Cx
	};
	if(!full_ftd2_write(handle, cmd1, sizeof(cmd1)))
		return false;

	//verify CDONE=1
	if(!check_cdone(handle, true))
		return false;

	return true;
}

static bool init_MPSSE(FT_HANDLE handle) {
	static const uint8_t cmd[] = {
		0x85,					//no loopback
		0x8d,					//disable 3-phase clocking
		0x8a,					//disable clk divide-by-5
		0x97,					//disable adaptive clocking
		0x86, LE16(CLKDIV),		//set clock rate
	};

	FT_STATUS ftStatus;
	ftStatus = FT_ResetDevice(handle);
	if(FT_FAILED(ftStatus))
		return false;
	ftStatus = FT_SetUSBParameters(handle, FTD2XX_IN_SIZE, FTD2XX_OUT_SIZE); //Multiple of 64 bytes up to 64k
	if(FT_FAILED(ftStatus))
		return false;
	ftStatus = FT_SetChars(handle, 0, 0, 0, 0);
	if(FT_FAILED(ftStatus))
		return false;
	ftStatus = FT_SetTimeouts(handle, 300, 300); //read,write timeout (ms)
	if(FT_FAILED(ftStatus))
		return false;
	ftStatus = FT_SetLatencyTimer(handle, 3); //time to wait before incomplete packet is sent (default=16ms). MPSSE read seems to fail on 2 sometimes, too fast?
	if(FT_FAILED(ftStatus))
		return false;
	ftStatus = FT_SetFlowControl(handle, FT_FLOW_RTS_CTS, 0, 0); //turn on flow control to synchronize IN requests
	if(FT_FAILED(ftStatus))
		return false;
	ftStatus = FT_SetBitMode(handle, 0x00, FT_BITMODE_RESET); //performs a general reset on MPSSE
	if(FT_FAILED(ftStatus))
		return false;
	ftStatus = FT_SetBitMode(handle, 0x00, FT_BITMODE_MPSSE); //enable MPSSE mode
	if(FT_FAILED(ftStatus))
		return false;

	//MPSSE seems to choke on first write sometimes :/  Send, purge, resend.
	DWORD sent;
	ftStatus = FT_Write(handle, (void*)cmd, 1, &sent); //enable MPSSE mode
	if(FT_FAILED(ftStatus))
		return false;
	default_sleep();
	ftStatus = FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
	if(FT_FAILED(ftStatus))
		return false;
	return full_ftd2_write(handle, cmd, sizeof(cmd));
}

static void preemptive_close_connection(CaptureData* capture_data) {
	FT_ResetDevice(capture_data->handle);
	FT_Close(capture_data->handle);
}

bool connect_ftd2(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
	strncpy(SerialNumber, device->serial_number.c_str(), REAL_SERIAL_NUMBER_SIZE);
	SerialNumber[REAL_SERIAL_NUMBER_SIZE] = 0;
	if(FT_OpenEx(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &capture_data->handle)) {
		capture_error_print(print_failed, capture_data, "Create failed");
		return false;
	}

	if(!init_MPSSE(capture_data->handle)) {
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "MPSSE init failed");
		return false;
	}

	if(!fpga_config(capture_data->handle, ftd2_ds2_fws[device->firmware_id - 1], ftd2_ds2_sizes[device->firmware_id - 1])) {
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "FPGA config failed");
		return false;
	}

	FT_STATUS ftStatus;
	WORD val=0;
	ftStatus = FT_ReadEE(capture_data->handle, 1, &val);
	if(FT_FAILED(ftStatus) || (val != 0x0403)) { //=85A8: something went wrong (fpga is configured but FT chip detected wrong eeprom size)
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "EEPROM read error");
		return false;
	}

	/*
	WORD Firmware = 0;
	WORD Hardware = 0;
	ftStatus = FT_ReadEE(capture_data->handle, 0x10, &Firmware);
	if(FT_FAILED(ftStatus)) {
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "Firmware ID read error");
		return false;
	}
	ftStatus = FT_ReadEE(capture_data->handle, 0x11, &Hardware);
	if(FT_FAILED(ftStatus)) {
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "Hardware ID read error");
		return false;
	}
	*/

	ftStatus = FT_SetBitMode(capture_data->handle, 0x00, FT_BITMODE_SYNC_FIFO);   //to FIFO mode. This takes over port B, pins shouldn't get modified though
	if(FT_FAILED(ftStatus)) {
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "Bitmode setup error");
		return false;
	}

	ftStatus = FT_SetTimeouts(capture_data->handle, 50, 50);
	if(FT_FAILED(ftStatus)) {
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "Timeouts setup error");
		return false;
	}

	static const uint8_t cmd[]={ 0x80, 0x01 }; //enable capture
	if(!full_ftd2_write(capture_data->handle, cmd, sizeof(cmd))) {
		preemptive_close_connection(capture_data);
		capture_error_print(print_failed, capture_data, "Capture enable error");
		return false;
	}

	return true;
}

/*
//DS frame:
//  video (top|bottom interleaved RGB16), 256*192*2*2 bytes
//  audio (16bit stereo, approx. 32729 Hz), 0x8F4 bytes
//      actual number of samples varies, audio is padded with 0x4321
FrameResult DS2Device::getFrame(DSFrame *frame) {
    enum {
        FRAMESIZE = ndsWidth*ndsHeight*2*2,
        S_MAX = 0x47A/2, //#audiosamples(dword)         aligns frame to 510 bytes..
        READSIZE = FRAMESIZE + S_MAX*4,
        MAX = READSIZE/2,   //(usbFifoWords)
    };

    FT_STATUS status;
    DWORD bytesIn;

    status=FT_Read(handle, frame->raw, READSIZE, &bytesIn);
    if(status!=FT_OK) {
        DEBUG_TRACE("err %d\n",status); //disconnected = FT_IO_ERROR
        usb_close();
        return FRAME_ERROR;
    }
    if(bytesIn!=READSIZE) { //stream stopped (lid closed / DS is off)
        DEBUG_TRACE(".");
        return FRAME_SKIP;
    }

    //check sync
    uint16_t *src=(uint16_t*)frame->raw;
    int samples;
    if(src[0]==0x4321) {
        samples=0;
        while(samples<MAX && src[samples]==0x4321)
            samples++;
        if(samples==MAX) //?? shouldn't happen
            DEBUG_TRACE("wut?");
        FT_Read(handle, frame->raw, samples*2, &bytesIn);
        DEBUG_TRACE("resync(%d)",samples);
        return FRAME_SKIP;
    } else if(src[MAX-1]!=0x4321) {
        samples=0;
        while(samples<MAX && src[samples]!=0x4321)
            samples++;
        while(samples<MAX && src[samples]==0x4321)
            samples++;
        if(samples==MAX) //?? shouldn't happen
            DEBUG_TRACE("wut?");
        FT_Read(handle, frame->raw, samples*2, &bytesIn);
        DEBUG_TRACE("resync(%d)",samples);
        return FRAME_SKIP;
    } else {
        //DEBUG_TRACE("."); 
    }

    //count audio samples
    samples=0;
    uint32_t *a=(uint32_t*)(frame->raw+FRAMESIZE);
    while(samples<S_MAX && a[samples]!=0x43214321)
        samples++;

    //sanity check
    if(samples<547 || samples>548)
        DEBUG_TRACE("%d?",samples);

    frame->audioBuf=a;
    frame->audioSamples=samples;

    return FRAME_OK;
}
*/

static bool synchronization_check(CaptureData* capture_data, uint16_t* data_buffer) {
    //check sync
    DWORD bytesIn;
    int samples = 0;
    int size_words = get_capture_size(capture_data) / 2;
    if(data_buffer[0] == FTD2_OLDDS_SYNCH_VALUES) {
        while((samples < size_words) && (data_buffer[samples] == FTD2_OLDDS_SYNCH_VALUES))
            samples++;
    	// Do a read to re-synchronize
        FT_Read(capture_data->handle, data_buffer, samples * 2, &bytesIn);
        return false;
    } else if(data_buffer[size_words - 1] != FTD2_OLDDS_SYNCH_VALUES) {
    	// Find the first spot the padding is present
        while((samples < size_words) && (data_buffer[samples] != FTD2_OLDDS_SYNCH_VALUES))
            samples++;
        while((samples < size_words) && (data_buffer[samples] == FTD2_OLDDS_SYNCH_VALUES))
            samples++;
    	// Do a read to re-synchronize
        FT_Read(capture_data->handle, data_buffer, samples * 2, &bytesIn);
        return false;
    }
    return true;
}

static inline void data_output_update(CaptureReceived* buffer, CaptureData* capture_data, int read_amount, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	capture_data->data_buffers.WriteToBuffer(buffer, read_amount, diff.count(), &capture_data->status.device);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

void ftd2_capture_main_loop(CaptureData* capture_data) {
	int inner_curr_in = 0;
	FT_STATUS ftStatus;
	auto clock_start = std::chrono::high_resolution_clock::now();
	CaptureReceived* data_buffer = new CaptureReceived[2];
    DWORD bytesIn;
    int read_size = get_capture_size(capture_data);

	while (capture_data->status.connected && capture_data->status.running) {
		ftStatus = FT_Read(capture_data->handle, data_buffer, read_size, &bytesIn);
		if(FT_FAILED(ftStatus)) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			return;
		}
		if(bytesIn < read_size)
			continue;
		if(!synchronization_check(capture_data, (uint16_t*)data_buffer))
			continue;
		data_output_update(data_buffer, capture_data, bytesIn, clock_start);
	}
}

void ftd2_capture_cleanup(CaptureData* capture_data) {
	if(FT_Close(capture_data->handle)) {
		capture_error_print(true, capture_data, "Disconnected: Close failed");
	}
}

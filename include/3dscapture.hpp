#ifndef __3DSCAPTURE_HPP
#define __3DSCAPTURE_HPP

#include "utils.hpp"
#include "hw_defs.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define FTD3XX_STATIC
#include <FTD3XX.h>
#define FT_ASYNC_CALL FT_ReadPipeEx
#else
#include <ftd3xx.h>
#define FT_ASYNC_CALL FT_ReadPipeAsync
#endif

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)
#define MAX_SAMPLES_IN 1096

// Max value supported by old FTD3XX versions...
#define NUM_CONCURRENT_DATA_BUFFERS 8

// When is_bad_ftd3xx is true, it may happen that a frame is lost.
// This value prevents showing a black frame for that.
#define MAX_ALLOWED_BLANKS 1

#define FIX_PARTIAL_FIRST_FRAME_NUM 3

#pragma pack(push, 1)

struct PACKED VideoInputData {
	uint8_t screen_data[IN_VIDEO_SIZE][3];
};

struct PACKED CaptureReceived {
	VideoInputData video_data;
	uint16_t audio_data[MAX_SAMPLES_IN];
};

#pragma pack(pop)

struct CaptureData {
	FT_HANDLE handle;
	char chosen_serial_number[SERIAL_NUMBER_SIZE];
	std::string error_text;
	bool new_error_text;
	CaptureReceived capture_buf[NUM_CONCURRENT_DATA_BUFFERS];
	ULONG read[NUM_CONCURRENT_DATA_BUFFERS];
	double time_in_buf[NUM_CONCURRENT_DATA_BUFFERS];
	volatile int curr_in = 0;
	volatile int cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	volatile bool connected = false;
	volatile bool running = true;
	volatile bool close_success = true;
	ConsumerMutex video_wait;
	ConsumerMutex audio_wait;
};

struct DevicesList {
	int numAllocedDevices;
	int numValidDevices;
	char *serialNumbers;
};

bool connect(bool print_failed, CaptureData* capture_data);
void captureCall(CaptureData* capture_data);
#endif

#ifndef __CAPTURESTRUCTS_HPP
#define __CAPTURESTRUCTS_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include <string>

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

#define MAX_SAMPLES_IN 1096

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

struct CaptureStatus {
	std::string error_text;
	bool new_error_text;
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
#endif

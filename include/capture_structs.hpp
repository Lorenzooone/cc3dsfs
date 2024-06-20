#ifndef __CAPTURESTRUCTS_HPP
#define __CAPTURESTRUCTS_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include <string>

// Max value (Due to support of old FTD3XX versions...)
#define NUM_CONCURRENT_DATA_BUFFERS 8

// It may happen that a frame is lost.
// This value prevents showing a black frame for that.
// Shouldn't happen with recent updates, though...
#define MAX_ALLOWED_BLANKS 1

#define FIX_PARTIAL_FIRST_FRAME_NUM 3

#pragma pack(push, 1)

struct PACKED FTDI3DSVideoInputData {
	uint8_t screen_data[IN_VIDEO_SIZE_3DS][3];
};

struct PACKED FTDI3DSVideoInputData_3D {
	uint8_t screen_data[IN_VIDEO_SIZE_3DS_3D][3];
};

struct PACKED USB3DSVideoInputData {
	uint8_t screen_data[IN_VIDEO_SIZE_3DS][3];
};

struct PACKED USBOldDSPixelData {
	uint16_t b : 5;
	uint16_t special : 1;
	uint16_t g : 5;
	uint16_t r : 5;
};

struct PACKED USBOldDSVideoInputData {
	USBOldDSPixelData screen_data[IN_VIDEO_SIZE_DS];
};

struct PACKED FTDI3DSCaptureReceived {
	FTDI3DSVideoInputData video_in;
	uint16_t audio_data[N3DSXL_SAMPLES_IN];
};

struct PACKED FTDI3DSCaptureReceived_3D {
	FTDI3DSVideoInputData_3D video_in;
	uint16_t audio_data[N3DSXL_SAMPLES_IN];
};

struct PACKED USB3DSCaptureReceived {
	USB3DSVideoInputData video_in;
};

struct PACKED USBOldDSFrameInfo {
	uint8_t half_line_flags[(HEIGHT_DS >> 3) << 1];
	uint32_t frame;
	uint8_t valid;
	uint8_t unused[11];
};

struct PACKED USBOldDSCaptureReceived {
	USBOldDSVideoInputData video_in;
	USBOldDSFrameInfo frameinfo;
};

#pragma pack(pop)

union CaptureReceived {
	FTDI3DSCaptureReceived ftdi_received;
	FTDI3DSCaptureReceived_3D ftdi_received_3d;
	USB3DSCaptureReceived usb_received_3ds;
	USBOldDSCaptureReceived usb_received_old_ds;
};

struct CaptureDevice {
	CaptureDevice(std::string serial_number, std::string name, bool is_ftdi, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int rgb_bits_size, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y): serial_number(serial_number), name(name), is_ftdi(is_ftdi), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), rgb_bits_size(rgb_bits_size), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y) {}
	CaptureDevice(): serial_number(""), name(""), is_ftdi(false), is_3ds(false), has_3d(false), has_audio(false), width(0), height(0), max_samples_in(0), rgb_bits_size(0), base_rotation(0), top_screen_x(0), top_screen_y(0), bot_screen_x(0), bot_screen_y(0) {}

	std::string serial_number;
	std::string name;
	bool is_ftdi;
	bool is_3ds;
	bool has_3d;
	bool has_audio;
	int width;
	int height;
	int max_samples_in;
	int rgb_bits_size;
	int base_rotation;
	int top_screen_x;
	int top_screen_y;
	int bot_screen_x;
	int bot_screen_y;
};

struct CaptureStatus {
	CaptureDevice device;
	std::string error_text;
	bool new_error_text;
	bool enabled_3d = false;
	volatile int curr_in = 0;
	volatile int cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	volatile bool connected = false;
	volatile bool running = true;
	volatile bool close_success = true;
	ConsumerMutex video_wait;
	ConsumerMutex audio_wait;
};

struct CaptureData {
	void* handle;
	uint64_t read[NUM_CONCURRENT_DATA_BUFFERS];
	CaptureReceived capture_buf[NUM_CONCURRENT_DATA_BUFFERS];
	double time_in_buf[NUM_CONCURRENT_DATA_BUFFERS];
	CaptureStatus status;
};
#endif

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

#define EXTRA_DATA_BUFFER_USB_SIZE (1 << 9)
#define EXTRA_DATA_BUFFER_FTD3XX_SIZE (1 << 10)

enum CaptureConnectionType { CAPTURE_CONN_FTD3, CAPTURE_CONN_USB, CAPTURE_CONN_FTD2, CAPTURE_CONN_IS_NITRO };
enum CaptureScreensType { CAPTURE_SCREENS_BOTH, CAPTURE_SCREENS_TOP, CAPTURE_SCREENS_BOTTOM, CAPTURE_SCREENS_ENUM_END };
enum CaptureSpeedsType { CAPTURE_SPEEDS_FULL, CAPTURE_SPEEDS_HALF, CAPTURE_SPEEDS_THIRD, CAPTURE_SPEEDS_QUARTER, CAPTURE_SPEEDS_ENUM_END };

#pragma pack(push, 1)

struct PACKED RGB83DSVideoInputData {
	uint8_t screen_data[IN_VIDEO_SIZE_3DS][3];
};

struct PACKED RGB83DSVideoInputData_3D {
	uint8_t screen_data[IN_VIDEO_SIZE_3DS_3D][3];
};

#define OLD_DS_PIXEL_B_BITS 5
#define OLD_DS_PIXEL_G_BITS 6
#define OLD_DS_PIXEL_R_BITS 5

struct PACKED USBOldDSPixelData {
	uint16_t b : OLD_DS_PIXEL_B_BITS;
	uint16_t g : OLD_DS_PIXEL_G_BITS;
	uint16_t r : OLD_DS_PIXEL_R_BITS;
};

struct PACKED USBOldDSVideoInputData {
	USBOldDSPixelData screen_data[IN_VIDEO_SIZE_DS];
};

struct PACKED ISNitroEmulatorVideoInputData {
	uint8_t screen_data[IN_VIDEO_SIZE_DS][3];
};

struct PACKED FTD3_3DSCaptureReceived {
	RGB83DSVideoInputData video_in;
	uint16_t audio_data[N3DSXL_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_FTD3XX_SIZE];
};

struct PACKED FTD3_3DSCaptureReceived_3D {
	RGB83DSVideoInputData_3D video_in;
	uint16_t audio_data[N3DSXL_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_FTD3XX_SIZE];
};

struct PACKED USB3DSCaptureReceived {
	RGB83DSVideoInputData video_in;
	uint16_t audio_data[O3DS_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_USB_SIZE];
};

struct PACKED USB3DSCaptureReceived_3D {
	RGB83DSVideoInputData_3D video_in;
	uint16_t audio_data[O3DS_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_USB_SIZE];
};

struct PACKED USBOldDSFrameInfo {
	uint8_t half_line_flags[(HEIGHT_DS >> 3) << 1];
	uint32_t frame;
	uint8_t valid;
	uint8_t unused[11];
};

struct PACKED USBOldDSCaptureReceived {
	USBOldDSVideoInputData video_in;
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_USB_SIZE];
	USBOldDSFrameInfo frameinfo;
};

struct PACKED ISNitroCaptureReceived {
	ISNitroEmulatorVideoInputData video_in;
};

#pragma pack(pop)

union CaptureReceived {
	FTD3_3DSCaptureReceived ftd3_received;
	FTD3_3DSCaptureReceived_3D ftd3_received_3d;
	USB3DSCaptureReceived usb_received_3ds;
	USB3DSCaptureReceived_3D usb_received_3ds_3d;
	USBOldDSCaptureReceived usb_received_old_ds;
	ISNitroCaptureReceived is_nitro_capture_received;
};

struct CaptureDevice {
	CaptureDevice(std::string serial_number, std::string name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y) : serial_number(serial_number), name(name), cc_type(cc_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), path("") {}
	CaptureDevice(std::string serial_number, std::string name, std::string path, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y) : serial_number(serial_number), name(name), cc_type(cc_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), path(path) {}
	CaptureDevice(): serial_number(""), name(""), cc_type(CAPTURE_CONN_USB), descriptor(NULL), is_3ds(false), has_3d(false), has_audio(false), width(0), height(0), max_samples_in(0), base_rotation(0), top_screen_x(0), top_screen_y(0), bot_screen_x(0), bot_screen_y(0), path("") {}

	std::string serial_number;
	std::string name;
	std::string path;
	CaptureConnectionType cc_type;
	const void* descriptor;
	bool is_3ds;
	bool has_3d;
	bool has_audio;
	int width;
	int height;
	int max_samples_in;
	int base_rotation;
	int top_screen_x;
	int top_screen_y;
	int bot_screen_x;
	int bot_screen_y;
};

struct CaptureStatus {
	CaptureDevice device;
	std::string graphical_error_text;
	std::string detailed_error_text;
	bool new_error_text;
	volatile int curr_in = 0;
	volatile int cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	volatile bool connected = false;
	volatile bool running = true;
	volatile bool close_success = true;
	volatile int curr_delay = 0;
	volatile bool reset_hardware = false;
	bool enabled_3d = false;
	CaptureScreensType capture_type;
	CaptureSpeedsType capture_speed;
	ConsumerMutex video_wait;
	ConsumerMutex audio_wait;
};

struct CaptureData {
	void* handle;
	CaptureScreensType capture_type[NUM_CONCURRENT_DATA_BUFFERS];
	uint64_t read[NUM_CONCURRENT_DATA_BUFFERS];
	CaptureReceived capture_buf[NUM_CONCURRENT_DATA_BUFFERS];
	double time_in_buf[NUM_CONCURRENT_DATA_BUFFERS];
	CaptureStatus status;
};
#endif

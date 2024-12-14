#ifndef __CAPTURESTRUCTS_HPP
#define __CAPTURESTRUCTS_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include <mutex>
#include <string>

// It may happen that a frame is lost.
// This value prevents showing a black frame for that.
// Shouldn't happen with recent updates, though...
#define MAX_ALLOWED_BLANKS 1

#define FIX_PARTIAL_FIRST_FRAME_NUM 3

#define MAX_PACKET_SIZE_USB2 (1 << 9)
#define EXTRA_DATA_BUFFER_USB_SIZE MAX_PACKET_SIZE_USB2
#define EXTRA_DATA_BUFFER_FTD3XX_SIZE (1 << 10)

#define FTD2_INTRA_PACKET_HEADER_SIZE 2
#define MAX_PACKET_SIZE_FTD2 (MAX_PACKET_SIZE_USB2 - FTD2_INTRA_PACKET_HEADER_SIZE)

enum CaptureConnectionType { CAPTURE_CONN_FTD3, CAPTURE_CONN_USB, CAPTURE_CONN_FTD2, CAPTURE_CONN_IS_NITRO };
enum InputVideoDataType { VIDEO_DATA_RGB, VIDEO_DATA_BGR, VIDEO_DATA_RGB16, VIDEO_DATA_BGR16 };
enum CaptureScreensType { CAPTURE_SCREENS_BOTH, CAPTURE_SCREENS_TOP, CAPTURE_SCREENS_BOTTOM, CAPTURE_SCREENS_ENUM_END };
enum CaptureSpeedsType { CAPTURE_SPEEDS_FULL, CAPTURE_SPEEDS_HALF, CAPTURE_SPEEDS_THIRD, CAPTURE_SPEEDS_QUARTER, CAPTURE_SPEEDS_ENUM_END };

// Readers are Audio and Video. So 2.
// Use 2 extra buffers. One for writing in case the other 2 are busy,
// and the other for writing without overwriting the latest stuff in
// a worst case scenario...
enum CaptureReaderType { CAPTURE_READER_VIDEO, CAPTURE_READER_AUDIO, CAPTURE_READER_ENUM_END };
#define NUM_CONCURRENT_DATA_BUFFER_READERS ((int)CAPTURE_READER_ENUM_END)
#define NUM_CONCURRENT_DATA_BUFFERS (NUM_CONCURRENT_DATA_BUFFER_READERS + 2)

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

struct PACKED ISTWLCaptureVideoInputData {
	USBOldDSPixelData screen_data[IN_VIDEO_SIZE_DS];
	uint8_t bit_6_rb_screen_data[(IN_VIDEO_SIZE_DS >> 3) * 2];
};

struct ALIGNED(16) PACKED FTD3_3DSCaptureReceived {
	RGB83DSVideoInputData video_in;
	uint16_t audio_data[N3DSXL_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_FTD3XX_SIZE];
};

struct ALIGNED(16) PACKED FTD3_3DSCaptureReceived_3D {
	RGB83DSVideoInputData_3D video_in;
	uint16_t audio_data[N3DSXL_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_FTD3XX_SIZE];
};

struct ALIGNED(16) PACKED USB3DSCaptureReceived {
	RGB83DSVideoInputData video_in;
	uint16_t audio_data[O3DS_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_USB_SIZE];
};

struct ALIGNED(16) PACKED USB3DSCaptureReceived_3D {
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

struct ALIGNED(16) PACKED USBOldDSCaptureReceived {
	USBOldDSVideoInputData video_in;
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_USB_SIZE];
	USBOldDSFrameInfo frameinfo;
};

struct ALIGNED(16) PACKED FTD2OldDSCaptureReceived {
	USBOldDSVideoInputData video_in;
	uint16_t audio_data[DS_SAMPLES_IN];
	uint8_t unused_buffer[EXTRA_DATA_BUFFER_USB_SIZE];
};

struct ALIGNED(16) PACKED FTD2OldDSCaptureReceivedRaw {
	FTD2OldDSCaptureReceived data;
	uint8_t extra_ftd2_buffer[((sizeof(FTD2OldDSCaptureReceived) + MAX_PACKET_SIZE_FTD2 - 1) / MAX_PACKET_SIZE_FTD2) * FTD2_INTRA_PACKET_HEADER_SIZE];
};

struct ALIGNED(16) PACKED ISNitroCaptureReceived {
	ISNitroEmulatorVideoInputData video_in;
};

struct ALIGNED(16) PACKED ISTWLCaptureVideoReceived {
	ISTWLCaptureVideoInputData video_in;
	uint32_t time;
	uint32_t frame;
	uint8_t unknown[0x40000 - (sizeof(ISTWLCaptureVideoInputData) + (sizeof(uint32_t) * 2))];
};

struct ALIGNED(16) PACKED ISTWLCaptureAudioReceived {
	uint32_t time;
	uint16_t sound_data[TWL_CAPTURE_SAMPLES_IN];
};

struct ALIGNED(16) PACKED ISTWLCaptureReceived {
	ISTWLCaptureVideoReceived video_capture_in;
	ISTWLCaptureAudioReceived audio_capture_in[TWL_CAPTURE_MAX_SAMPLES_CHUNK_NUM];
};

#pragma pack(pop)

union CaptureReceived {
	FTD3_3DSCaptureReceived ftd3_received;
	FTD3_3DSCaptureReceived_3D ftd3_received_3d;
	USB3DSCaptureReceived usb_received_3ds;
	USB3DSCaptureReceived_3D usb_received_3ds_3d;
	USBOldDSCaptureReceived usb_received_old_ds;
	FTD2OldDSCaptureReceived ftd2_received_old_ds;
	FTD2OldDSCaptureReceivedRaw ftd2_received_old_ds_raw;
	ISNitroCaptureReceived is_nitro_capture_received;
	ISTWLCaptureReceived is_twl_capture_received;
};

struct CaptureDevice {
	CaptureDevice(std::string serial_number, std::string name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type) : serial_number(serial_number), name(name), cc_type(cc_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), path(""), video_data_type(video_data_type), firmware_id(0), is_rgb_888(false), long_name(name) {}
	CaptureDevice(std::string serial_number, std::string name, std::string long_name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type) : serial_number(serial_number), name(name), cc_type(cc_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), path(""), video_data_type(video_data_type), firmware_id(0), is_rgb_888(false), long_name(long_name) {}
	CaptureDevice(std::string serial_number, std::string name, std::string long_name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type, int firmware_id, bool is_rgb_888) : serial_number(serial_number), name(name), cc_type(cc_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), path(""), video_data_type(video_data_type), firmware_id(firmware_id), is_rgb_888(is_rgb_888), long_name(name) {}
	CaptureDevice(std::string serial_number, std::string name, std::string long_name, std::string path, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type, int firmware_id, bool is_rgb_888) : serial_number(serial_number), name(name), cc_type(cc_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), path(path), video_data_type(video_data_type), firmware_id(firmware_id), is_rgb_888(is_rgb_888), long_name(long_name) {}
	CaptureDevice(std::string serial_number, std::string name, std::string long_name, std::string path, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type) : serial_number(serial_number), name(name), cc_type(cc_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), path(path), video_data_type(video_data_type), firmware_id(0), is_rgb_888(false), long_name(long_name) {}
	CaptureDevice(): serial_number(""), name(""), cc_type(CAPTURE_CONN_USB), descriptor(NULL), is_3ds(false), has_3d(false), has_audio(false), width(0), height(0), max_samples_in(0), base_rotation(0), top_screen_x(0), top_screen_y(0), bot_screen_x(0), bot_screen_y(0), path(""), firmware_id(0), video_data_type(VIDEO_DATA_RGB), is_rgb_888(false), long_name("") {}

	std::string serial_number;
	std::string name;
	std::string long_name;
	std::string path;
	CaptureConnectionType cc_type;
	InputVideoDataType video_data_type;
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
	int firmware_id;
	bool is_rgb_888;
};

struct CaptureStatus {
	CaptureDevice device;
	std::string graphical_error_text;
	std::string detailed_error_text;
	bool new_error_text;
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

struct CaptureDataSingleBuffer {
	CaptureScreensType capture_type;
	uint64_t read;
	CaptureReceived capture_buf;
	double time_in_buf;
	uint32_t inner_index;
};

class CaptureDataBuffers {
public:
	CaptureDataBuffers();
	CaptureDataSingleBuffer* GetReaderBuffer(CaptureReaderType reader_type);
	void ReleaseReaderBuffer(CaptureReaderType reader_type);
	void WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, CaptureScreensType capture_type = CAPTURE_SCREENS_BOTH);
	CaptureDataSingleBuffer* GetWriterBuffer();
	void ReleaseWriterBuffer();
private:
	uint32_t inner_index = 0;
	std::mutex access_mutex;
	int last_curr_in;
	int curr_writer_pos;
	int curr_reader_pos[NUM_CONCURRENT_DATA_BUFFER_READERS];
	int num_readers[NUM_CONCURRENT_DATA_BUFFERS];
	bool has_read_data[NUM_CONCURRENT_DATA_BUFFERS][NUM_CONCURRENT_DATA_BUFFER_READERS];
	CaptureDataSingleBuffer buffers[NUM_CONCURRENT_DATA_BUFFERS];
};

struct CaptureData {
	void* handle;
	CaptureDataBuffers data_buffers;
	CaptureStatus status;
};
#endif

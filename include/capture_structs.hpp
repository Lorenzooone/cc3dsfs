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

#define OPTIMIZE_3DS_AUDIO_BUFFER_MAX_SIZE 0x200

enum CaptureConnectionType { CAPTURE_CONN_FTD3, CAPTURE_CONN_USB, CAPTURE_CONN_FTD2, CAPTURE_CONN_IS_NITRO, CAPTURE_CONN_CYPRESS_NISETRO, CAPTURE_CONN_CYPRESS_OPTIMIZE };
enum InputVideoDataType { VIDEO_DATA_RGB, VIDEO_DATA_BGR, VIDEO_DATA_RGB16, VIDEO_DATA_BGR16 };
enum CaptureScreensType { CAPTURE_SCREENS_BOTH, CAPTURE_SCREENS_TOP, CAPTURE_SCREENS_BOTTOM, CAPTURE_SCREENS_ENUM_END };
enum CaptureSpeedsType { CAPTURE_SPEEDS_FULL, CAPTURE_SPEEDS_HALF, CAPTURE_SPEEDS_THIRD, CAPTURE_SPEEDS_QUARTER, CAPTURE_SPEEDS_ENUM_END };
enum PossibleCaptureDevices { CC_LOOPY_OLD_DS, CC_LOOPY_NEW_DS, CC_LOOPY_OLD_3DS, CC_LOOPY_NEW_N3DSXL, CC_IS_NITRO_EMULATOR, CC_IS_NITRO_CAPTURE, CC_IS_TWL_CAPTURE, CC_NISETRO_DS, CC_OPTIMIZE_O3DS, CC_OPTIMIZE_N3DS, CC_POSSIBLE_DEVICES_END };

// Readers are Audio and Video. So 2.
// Use 6 extra buffers. 5 for async writing in case the other 2 are busy,
// and the other for writing without overwriting the latest stuff in
// a worst case scenario...
enum CaptureReaderType { CAPTURE_READER_VIDEO, CAPTURE_READER_AUDIO, CAPTURE_READER_ENUM_END };
#define NUM_CONCURRENT_DATA_BUFFER_WRITERS 5
#define NUM_CONCURRENT_DATA_BUFFER_READERS ((int)CAPTURE_READER_ENUM_END)
#define NUM_CONCURRENT_DATA_BUFFERS (NUM_CONCURRENT_DATA_BUFFER_READERS + 1 + NUM_CONCURRENT_DATA_BUFFER_WRITERS)

#pragma pack(push, 1)

struct PACKED RGB83DSVideoInputData {
	uint8_t screen_data[IN_VIDEO_SIZE_3DS][3];
};

struct PACKED RGB83DSVideoInputData_3D {
	uint8_t screen_data[IN_VIDEO_SIZE_3DS_3D][3];
};

#define OLD_DS_PIXEL_R_BITS 5
#define OLD_DS_PIXEL_G_BITS 6
#define OLD_DS_PIXEL_B_BITS 5

// The internal order needs to be reversed... This is so confusing...
struct PACKED ALIGNED(2) USBOldDSPixelData {
	uint16_t r : OLD_DS_PIXEL_R_BITS;
	uint16_t g : OLD_DS_PIXEL_G_BITS;
	uint16_t b : OLD_DS_PIXEL_B_BITS;
};

struct PACKED ALIGNED(2) USBOldDSVideoInputData {
	USBOldDSPixelData screen_data[IN_VIDEO_SIZE_DS];
};

#define OPTIMIZE_3DS_PIXEL_R_BITS 5
#define OPTIMIZE_3DS_PIXEL_G_BITS 6
#define OPTIMIZE_3DS_PIXEL_B_BITS 5

// The internal order needs to be reversed... This is so confusing...
struct PACKED ALIGNED(2) USB5653DSOptimizePixelData {
	uint16_t b : OPTIMIZE_3DS_PIXEL_B_BITS;
	uint16_t g : OPTIMIZE_3DS_PIXEL_G_BITS;
	uint16_t r : OPTIMIZE_3DS_PIXEL_R_BITS;
};

struct PACKED USB8883DSOptimizePixelData {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct PACKED USB3DSOptimizeSingleSoundData {
	uint16_t sample_index;
	uint16_t sample_r;
	uint16_t sample_l;
};

struct PACKED USB3DSOptimizeColumnInfo {
	// Order is Little Endian
	uint16_t column_index : 10;
	uint16_t unk : 4;
	uint16_t buffer_num : 1;
	uint16_t has_extra_header_data : 1;
};

struct PACKED USB3DSOptimizeHeaderData {
	uint16_t magic;
	USB3DSOptimizeColumnInfo column_info;
};

struct PACKED USB3DSOptimizeHeaderSoundData {
	USB3DSOptimizeHeaderData header_info;
	USB3DSOptimizeSingleSoundData samples[2];
};

struct PACKED ALIGNED(16) USB5653DSOptimizeInputColumnData {
	USB3DSOptimizeHeaderSoundData header_sound;
	USB5653DSOptimizePixelData pixel[HEIGHT_3DS][2];
};

struct PACKED ALIGNED(16) USB5653DSOptimizeInputColumnData3D {
	USB3DSOptimizeHeaderSoundData header_sound;
	USB5653DSOptimizePixelData pixel[HEIGHT_3DS][3];
};

struct PACKED ALIGNED(16) USB8883DSOptimizeInputColumnData {
	USB3DSOptimizeHeaderSoundData header_sound;
	USB8883DSOptimizePixelData pixel[HEIGHT_3DS][2];
};

struct PACKED ALIGNED(16) USB8883DSOptimizeInputColumnData3D {
	USB3DSOptimizeHeaderSoundData header_sound;
	USB8883DSOptimizePixelData pixel[HEIGHT_3DS][3];
};

struct PACKED ISNitroEmulatorVideoInputData {
	uint8_t screen_data[IN_VIDEO_SIZE_DS][3];
};

struct PACKED ALIGNED(2) ISTWLCaptureVideoInputData {
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

struct ALIGNED(16) PACKED CypressNisetroDSCaptureReceived {
	ISNitroEmulatorVideoInputData video_in;
};

struct ALIGNED(16) PACKED ISTWLCaptureVideoReceived {
	ISTWLCaptureVideoInputData video_in;
	uint32_t time;
	uint32_t frame;
	uint64_t random_data;
};

struct ALIGNED(16) PACKED ISTWLCaptureVideoInternalReceived {
	ISTWLCaptureVideoReceived real_video_in;
	uint8_t unknown[0x40000 - sizeof(ISTWLCaptureVideoReceived)];
};

struct ALIGNED(4) PACKED ISTWLCaptureSoundData {
	uint16_t data[TWL_CAPTURE_SAMPLES_IN];
};

struct ALIGNED(4) PACKED ISTWLCaptureAudioReceived {
	uint32_t time;
	ISTWLCaptureSoundData sound_data;
};

struct ALIGNED(16) PACKED ISTWLCaptureReceived {
	ISTWLCaptureVideoReceived video_capture_in;
	ISTWLCaptureAudioReceived audio_capture_in[TWL_CAPTURE_MAX_SAMPLES_CHUNK_NUM];
};

struct ALIGNED(16) PACKED USB5653DSOptimizeCaptureReceived {
	USB5653DSOptimizeInputColumnData columns_data[TOP_WIDTH_3DS];
	USB5653DSOptimizePixelData bottom_only_column[HEIGHT_3DS][2];
};

struct ALIGNED(16) PACKED USB5653DSOptimizeCaptureReceived_3D {
	USB5653DSOptimizeInputColumnData3D columns_data[TOP_WIDTH_3DS];
	USB5653DSOptimizePixelData bottom_only_column[HEIGHT_3DS][3];
};

struct ALIGNED(16) PACKED USB8883DSOptimizeCaptureReceived {
	USB8883DSOptimizeInputColumnData columns_data[TOP_WIDTH_3DS];
	USB8883DSOptimizePixelData bottom_only_column[HEIGHT_3DS][2];
};

struct ALIGNED(16) PACKED USB8883DSOptimizeCaptureReceived_3D {
	USB8883DSOptimizeInputColumnData3D columns_data[TOP_WIDTH_3DS];
	USB8883DSOptimizePixelData bottom_only_column[HEIGHT_3DS][3];
};

struct ALIGNED(16) PACKED USB5653DSOptimizeCaptureReceivedExtraHeader {
	USB5653DSOptimizeInputColumnData columns_data[TOP_WIDTH_3DS + 1];
};

struct ALIGNED(16) PACKED USB5653DSOptimizeCaptureReceived_3DExtraHeader {
	USB5653DSOptimizeInputColumnData3D columns_data[TOP_WIDTH_3DS + 1];
};

struct ALIGNED(16) PACKED USB8883DSOptimizeCaptureReceivedExtraHeader {
	USB8883DSOptimizeInputColumnData columns_data[TOP_WIDTH_3DS + 1];
};

struct ALIGNED(16) PACKED USB8883DSOptimizeCaptureReceived_3DExtraHeader {
	USB8883DSOptimizeInputColumnData3D columns_data[TOP_WIDTH_3DS + 1];
};

#pragma pack(pop)

struct ALIGNED(16) FTD2OldDSCaptureReceivedNormalPlusRaw {
	FTD2OldDSCaptureReceived data;
	FTD2OldDSCaptureReceivedRaw raw_data;
};

union CaptureReceived {
	FTD3_3DSCaptureReceived ftd3_received;
	FTD3_3DSCaptureReceived_3D ftd3_received_3d;
	USB3DSCaptureReceived usb_received_3ds;
	USB3DSCaptureReceived_3D usb_received_3ds_3d;
	USBOldDSCaptureReceived usb_received_old_ds;
	FTD2OldDSCaptureReceived ftd2_received_old_ds;
	FTD2OldDSCaptureReceivedNormalPlusRaw ftd2_received_old_ds_normal_plus_raw;
	ISNitroCaptureReceived is_nitro_capture_received;
	ISTWLCaptureReceived is_twl_capture_received;
	CypressNisetroDSCaptureReceived cypress_nisetro_capture_received;
	USB5653DSOptimizeCaptureReceived cypress_optimize_received_565;
	USB5653DSOptimizeCaptureReceived_3D cypress_optimize_received_565_3d;
	USB8883DSOptimizeCaptureReceived cypress_optimize_received_888;
	USB8883DSOptimizeCaptureReceived_3D cypress_optimize_received_888_3d;
	USB5653DSOptimizeCaptureReceivedExtraHeader cypress_optimize_received_565_extra_header;
	USB5653DSOptimizeCaptureReceived_3DExtraHeader cypress_optimize_received_565_3d_extra_header;
	USB8883DSOptimizeCaptureReceivedExtraHeader cypress_optimize_received_888_extra_header;
	USB8883DSOptimizeCaptureReceived_3DExtraHeader cypress_optimize_received_888_3d_extra_header;
};

struct CaptureDevice {
	// This is so messy... Wish there was a clearer way...
	CaptureDevice(std::string serial_number, std::string name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, uint64_t max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type) : serial_number(serial_number), name(name), long_name(name), cc_type(cc_type), video_data_type(video_data_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y) {}
	CaptureDevice(std::string serial_number, std::string name, std::string long_name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, uint64_t max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type, std::string path = "") : serial_number(serial_number), name(name), long_name(long_name), path(path), cc_type(cc_type), video_data_type(video_data_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y) {}
	CaptureDevice(std::string serial_number, std::string name, std::string long_name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, uint64_t max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int bot_screen_x, int bot_screen_y, InputVideoDataType video_data_type, int firmware_id, bool is_rgb_888, std::string path = "") : serial_number(serial_number), name(name), long_name(long_name), path(path), cc_type(cc_type), video_data_type(video_data_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), firmware_id(firmware_id), is_rgb_888(is_rgb_888) {}
	CaptureDevice(std::string serial_number, std::string name, std::string long_name, CaptureConnectionType cc_type, const void* descriptor, bool is_3ds, bool has_3d, bool has_audio, int width, int height, int width_3d, int height_3d, uint64_t max_samples_in, int base_rotation, int top_screen_x, int top_screen_y, int second_top_screen_x, int second_top_screen_y, int bot_screen_x, int bot_screen_y, bool is_second_top_screen_right, InputVideoDataType video_data_type, uint32_t usb_speed, std::string path = "") : serial_number(serial_number), name(name), long_name(long_name), path(path), cc_type(cc_type), video_data_type(video_data_type), descriptor(descriptor), is_3ds(is_3ds), has_3d(has_3d), has_audio(has_audio), width(width), height(height), width_3d(width_3d), height_3d(height_3d), max_samples_in(max_samples_in), base_rotation(base_rotation), top_screen_x(top_screen_x), top_screen_y(top_screen_y), second_top_screen_x(second_top_screen_x), second_top_screen_y(second_top_screen_y), is_second_top_screen_right(is_second_top_screen_right), bot_screen_x(bot_screen_x), bot_screen_y(bot_screen_y), usb_speed(usb_speed) {}
	CaptureDevice() : usb_speed(0) {}

	std::string serial_number = "";
	std::string name = "";
	std::string long_name = "";
	std::string path = "";
	CaptureConnectionType cc_type = CAPTURE_CONN_USB;
	InputVideoDataType video_data_type = VIDEO_DATA_RGB;
	const void* descriptor = NULL;
	bool is_3ds = false;
	bool has_3d = false;
	bool has_audio = false;
	int width = 0;
	int height = 0;
	int width_3d = 0;
	int height_3d = 0;
	uint64_t max_samples_in = 0;
	int base_rotation = 0;
	int top_screen_x = 0;
	int top_screen_y = 0;
	int second_top_screen_x = 0;
	int second_top_screen_y = 0;
	bool is_second_top_screen_right = false;
	int bot_screen_x = 0;
	int bot_screen_y = 0;
	int firmware_id = 0;
	uint32_t usb_speed = 0x200;
	bool is_rgb_888 = false;
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
	bool requested_3d = false;
	bool request_low_bw_format = true;
	CaptureScreensType capture_type;
	CaptureSpeedsType capture_speed;
	int battery_percentage;
	bool ac_adapter_connected;
	// Needed for Nisetro DS(i) and Old Optimize 3DS
	bool devices_allowed_scan[CC_POSSIBLE_DEVICES_END];
	ConsumerMutex video_wait;
	ConsumerMutex audio_wait;
};

struct CaptureDataSingleBuffer {
	CaptureScreensType capture_type;
	uint64_t read;
	size_t unused_offset;
	CaptureReceived capture_buf;
	double time_in_buf;
	uint32_t inner_index;
	bool is_3d;
	InputVideoDataType buffer_video_data_type;
};

class CaptureDataBuffers {
public:
	CaptureDataBuffers();
	CaptureDataSingleBuffer* GetReaderBuffer(CaptureReaderType reader_type);
	void ReleaseReaderBuffer(CaptureReaderType reader_type);
	void WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, CaptureScreensType capture_type, size_t offset, int index, bool is_3d = false);
	void WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, CaptureScreensType capture_type, int index, bool is_3d = false);
	void WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, size_t offset, int index, bool is_3d = false);
	void WriteToBuffer(CaptureReceived* buffer, uint64_t read, double time_in_buf, CaptureDevice* device, int index, bool is_3d = false);
	CaptureDataSingleBuffer* GetWriterBuffer(int index = 0);
	void ReleaseWriterBuffer(int index = 0, bool update_last_curr_in = true);
private:
	std::mutex access_mutex;
	int last_curr_in;
	int curr_writer_pos[NUM_CONCURRENT_DATA_BUFFER_WRITERS];
	int curr_reader_pos[NUM_CONCURRENT_DATA_BUFFER_READERS];
	bool is_being_written_to[NUM_CONCURRENT_DATA_BUFFERS];
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

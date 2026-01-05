#include "conversions.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3_shared.hpp"
#include "dscapture_ftd2_shared.hpp"
#include "usb_ds_3ds_capture.hpp"
#include "usb_is_device_acquisition.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"

#include <cstring>

#define INTERLEAVED_RGB565_PIXEL_NUM 4
#define INTERLEAVED_RGB565_PIXEL_SIZE 2
#define INTERLEAVED_RGB565_DATA_SIZE sizeof(uint16_t)
#define INTERLEAVED_RGB565_TOTAL_SIZE (INTERLEAVED_RGB565_PIXEL_SIZE * INTERLEAVED_RGB565_PIXEL_NUM / INTERLEAVED_RGB565_DATA_SIZE)

#define INTERLEAVED_RGB888_PIXEL_NUM 4
#define INTERLEAVED_RGB888_PIXEL_SIZE 3
#define INTERLEAVED_RGB888_DATA_SIZE sizeof(uint16_t)
#define INTERLEAVED_RGB888_TOTAL_SIZE (INTERLEAVED_RGB888_PIXEL_SIZE * INTERLEAVED_RGB888_PIXEL_NUM / INTERLEAVED_RGB888_DATA_SIZE)

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DS3D(CaptureReceived* buffer, bool is_rgb888, uint16_t column);

struct interleaved_rgb565_pixels {
	uint16_t pixels[INTERLEAVED_RGB565_TOTAL_SIZE][2];
};

struct deinterleaved_rgb565_pixels {
	uint16_t pixels[INTERLEAVED_RGB565_TOTAL_SIZE];
};

struct ALIGNED(INTERLEAVED_RGB888_PIXEL_NUM * 2) interleaved_rgb888_u16_pixels {
	uint16_t pixels[INTERLEAVED_RGB888_TOTAL_SIZE][2];
};

struct ALIGNED(INTERLEAVED_RGB888_PIXEL_NUM) deinterleaved_rgb888_u16_pixels {
	// 4 pixels
	uint16_t pixels[INTERLEAVED_RGB888_TOTAL_SIZE];
};

struct twl_16bit_pixels {
	uint16_t first_r : 5;
	uint16_t first_g : 6;
	uint16_t first_b : 5;
	uint16_t second_r : 5;
	uint16_t second_g : 6;
	uint16_t second_b : 5;
	uint16_t third_r : 5;
	uint16_t third_g : 6;
	uint16_t third_b : 5;
	uint16_t fourth_r : 5;
	uint16_t fourth_g : 6;
	uint16_t fourth_b : 5;
};

struct twl_2bit_pixels {
	uint8_t first_r : 1;
	uint8_t first_b : 1;
	uint8_t second_r : 1;
	uint8_t second_b : 1;
	uint8_t third_r : 1;
	uint8_t third_b : 1;
	uint8_t fourth_r : 1;
	uint8_t fourth_b : 1;
};

// Optimized de-interleave methods...

static inline uint16_t _reverse_endianness(uint16_t data) {
	return (data >> 8) | ((data << 8) & 0xFF00);
}

static inline void memcpy_data_u16le_origin(uint16_t* dst, uint8_t* src, size_t num_iters, bool is_big_endian) {
	if(!is_big_endian)
		memcpy(dst, src, num_iters * 2);
	else
		for(size_t i = 0; i < num_iters; i++)
			dst[i] = (src[(i * 2) + 1] << 8) | src[i * 2];
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptLE(deinterleaved_rgb565_pixels* out_ptr_top, deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline, int multiplier_top = 1) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_bottom = (output_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_top = (output_halfline * real_num_iters * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel_bottom].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_pos];
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(deinterleaved_rgb565_pixels* out_ptr_top, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline, int multiplier_top = 1) {
	//de-interleave pixels
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_pos];
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptBE(deinterleaved_rgb565_pixels* out_ptr_top, deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline, int multiplier_top = 1) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_bottom = (output_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_top = (output_halfline * real_num_iters * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel_bottom].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][bottom_pos]);
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel_top].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_pos]);
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(deinterleaved_rgb565_pixels* out_ptr_top, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline, int multiplier_top = 1) {
	//de-interleave pixels
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_pos]);
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][bottom_pos]);
	}
}

static inline void usb_rgb888convertInterleaveU16VideoToOutputDirectOpt(deinterleaved_rgb888_u16_pixels* out_ptr_top, deinterleaved_rgb888_u16_pixels* out_ptr_bottom, interleaved_rgb888_u16_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_pos];
	}
}

static inline void usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoTop(deinterleaved_rgb888_u16_pixels* out_ptr_top, interleaved_rgb888_u16_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline) {
	//de-interleave pixels
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_pos];
	}
}

static inline void usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoBottom(deinterleaved_rgb888_u16_pixels* out_ptr_bottom, interleaved_rgb888_u16_pixels* in_ptr, uint32_t num_iters, size_t input_halfline, size_t output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
	}
}


static inline void convertVideoToOutputChunk(RGB83DSVideoInputData *p_in, VideoOutputData *p_out, size_t iters, size_t start_in, size_t start_out) {
	memcpy(&p_out->rgb_video_output_data.screen_data[start_out], &p_in->screen_data[start_in], iters * sizeof(VideoPixelRGB));
}

static inline void convertVideoToOutputChunk_3D(RGB83DSVideoInputData_3D *p_in, VideoOutputData *p_out, size_t iters, size_t start_in, size_t start_out) {
	memcpy(&p_out->rgb_video_output_data.screen_data[start_out], &p_in->screen_data[start_in], iters * sizeof(VideoPixelRGB));
}

static void expand_2d_to_3d_convertVideoToOutput(uint8_t *out_screen_data, size_t pixels_size, bool interleaved_3d, bool requested_3d) {
	if(requested_3d && interleaved_3d) {
		for(int i = TOP_WIDTH_3DS - 1; i >= 0; i--) {
			memcpy(&out_screen_data[(BOT_SIZE_3DS + (((2 * i) + 1) * HEIGHT_3DS)) * pixels_size], &out_screen_data[(BOT_SIZE_3DS + (i * HEIGHT_3DS)) * pixels_size], HEIGHT_3DS * pixels_size);
			memcpy(&out_screen_data[(BOT_SIZE_3DS + ((2 * i) * HEIGHT_3DS)) * pixels_size], &out_screen_data[(BOT_SIZE_3DS + (i * HEIGHT_3DS)) * pixels_size], HEIGHT_3DS * pixels_size);
		}
	}
	else if(requested_3d) {
		memcpy(&out_screen_data[(BOT_SIZE_3DS + TOP_SIZE_3DS) * pixels_size], &out_screen_data[BOT_SIZE_3DS * pixels_size], TOP_SIZE_3DS * pixels_size);
	}
}

// Logical conversions
static void ftd3_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d, bool interleaved_3d, bool requested_3d) {
	if(!enabled_3d) {
		convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_NO_BOTTOM_SIZE_3DS, 0, BOT_SIZE_3DS);

		for(int i = 0; i < ((IN_VIDEO_SIZE_3DS - IN_VIDEO_NO_BOTTOM_SIZE_3DS) / (IN_VIDEO_WIDTH_3DS * 2)); i++) {
			convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_WIDTH_3DS, (((i * 2) + 0) * IN_VIDEO_WIDTH_3DS) + IN_VIDEO_NO_BOTTOM_SIZE_3DS, i * IN_VIDEO_WIDTH_3DS);
			convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_WIDTH_3DS, (((i * 2) + 1) * IN_VIDEO_WIDTH_3DS) + IN_VIDEO_NO_BOTTOM_SIZE_3DS, BOT_SIZE_3DS + IN_VIDEO_NO_BOTTOM_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS));
		}
		expand_2d_to_3d_convertVideoToOutput((uint8_t*)p_out->rgb_video_output_data.screen_data, sizeof(VideoPixelRGB), interleaved_3d, requested_3d);
	}
	else {
		size_t last_line_index = ((IN_VIDEO_SIZE_3DS_3D - IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D) / (IN_VIDEO_WIDTH_3DS_3D * 3)) - 1;
		size_t top_left_last_line_out_pos = 0;
		size_t top_right_last_line_out_pos = 0;
		if(!interleaved_3d) {
			// Optimize for speed
			for(int i = 0; i < (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / (IN_VIDEO_WIDTH_3DS_3D * 2)); i++) {
				convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, ((i * 2) + 0) * IN_VIDEO_WIDTH_3DS_3D, BOT_SIZE_3DS + TOP_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS_3D));
				convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, ((i * 2) + 1) * IN_VIDEO_WIDTH_3DS_3D, BOT_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS_3D));
			}

			for(size_t i = 0; i < last_line_index; i++) {
				convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 0) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, BOT_SIZE_3DS + TOP_SIZE_3DS + (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
				convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 1) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, BOT_SIZE_3DS + (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
				convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 2) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, i * IN_VIDEO_WIDTH_3DS_3D);
			}
			top_left_last_line_out_pos = BOT_SIZE_3DS + TOP_SIZE_3DS + (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (last_line_index * IN_VIDEO_WIDTH_3DS_3D);
			top_right_last_line_out_pos = BOT_SIZE_3DS + (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (last_line_index * IN_VIDEO_WIDTH_3DS_3D);
		}
		else {
			// Optimize for speed
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, 0, BOT_SIZE_3DS);

			for(size_t i = 0; i < last_line_index; i++) {
				convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D * 2, (((i * 3) + 0) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, BOT_SIZE_3DS + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D + (i * IN_VIDEO_WIDTH_3DS_3D * 2));
				convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 2) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, i * IN_VIDEO_WIDTH_3DS_3D);
			}
			top_left_last_line_out_pos = BOT_SIZE_3DS + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D + (last_line_index * IN_VIDEO_WIDTH_3DS_3D * 2);
			top_right_last_line_out_pos = BOT_SIZE_3DS + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D + (last_line_index * IN_VIDEO_WIDTH_3DS_3D * 2) + IN_VIDEO_WIDTH_3DS_3D;
		}
		// For some weird reason, the last one is the opposite for bottom and
		// second top screen
		convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((last_line_index * 3) + 0) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, top_left_last_line_out_pos);
		convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((last_line_index * 3) + 2) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, top_right_last_line_out_pos);
		convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((last_line_index * 3) + 1) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, last_line_index * IN_VIDEO_WIDTH_3DS_3D);
	}
}

static inline void usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, size_t input_halfline, size_t output_halfline) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelBGR16);
	const int num_halflines = 2;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	deinterleaved_rgb565_pixels* out_ptr_top = (deinterleaved_rgb565_pixels*)p_out->bgr16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_bottom = out_ptr_top + ((WIDTH_DS * HEIGHT_DS * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->video_in.screen_data;
	const uint32_t halfline_iters = WIDTH_DS / num_halflines;
	usb_rgb565convertInterleaveVideoToOutputDirectOptLE(out_ptr_top, out_ptr_bottom, in_ptr, halfline_iters, input_halfline, output_halfline);
}

static inline void usb_oldDSconvertVideoToOutputHalfLineDirectOptBE(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, size_t input_halfline, size_t output_halfline) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelBGR16);
	const int num_halflines = 2;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	deinterleaved_rgb565_pixels* out_ptr_top = (deinterleaved_rgb565_pixels*)p_out->bgr16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_bottom = out_ptr_top + ((WIDTH_DS * HEIGHT_DS * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->video_in.screen_data;
	const uint32_t halfline_iters = WIDTH_DS / num_halflines;
	usb_rgb565convertInterleaveVideoToOutputDirectOptBE(out_ptr_top, out_ptr_bottom, in_ptr, halfline_iters, input_halfline, output_halfline);
}

static inline bool usb_OptimizeHasExtraHeaderSoundData(USB3DSOptimizeHeaderSoundData* header_sound_data) {
	// The macos compiler requires this... :/
	uint16_t base_data = read_le16((uint8_t*)&header_sound_data->header_info.column_info);
	USB3DSOptimizeColumnInfo column_info;
	column_info.has_extra_header_data_2d_only = (base_data >> 15) & 1;
	return column_info.has_extra_header_data_2d_only;
}

static inline uint16_t usb_OptimizeGetDataBufferNumber(USB3DSOptimizeHeaderSoundData* header_sound_data) {
	// The macos compiler requires this... :/
	uint16_t base_data = read_le16((uint8_t*)&header_sound_data->header_info.column_info);
	USB3DSOptimizeColumnInfo column_info;
	column_info.buffer_num = (base_data >> 14) & 1;
	return column_info.buffer_num;
}

static inline void usb_3DS565OptimizeconvertVideoToOutputLineDirectOptLE(USB5653DSOptimizeCaptureReceived *p_in, VideoOutputData *p_out, uint16_t column, bool is_n3ds) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	size_t target_bot_column_pre_last = BOT_WIDTH_3DS - 2;
	bool is_special_header = usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->bottom_only_column;
	if((!is_n3ds) && is_special_header) {
		column_start_bot_pos -= 1;
		column_pre_last_bot_pos -= 2;
		target_bot_column_pre_last = BOT_WIDTH_3DS - 1;
		if(column == column_last_bot_pos)
			return;
	}
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb565_pixels*)p_in->columns_data[column].pixel;
	else if(is_special_header)
		in_ptr = (interleaved_rgb565_pixels*)(((USB5653DSOptimizeCaptureReceivedExtraHeader*)p_in)->columns_data[column].pixel);
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, target_bot_column_pre_last);
	}
	else if(column < column_start_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
	else {
		out_ptr_top += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb565convertInterleaveVideoToOutputDirectOptLE(out_ptr_top, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos);
	}
}

static inline void usb_3DS565OptimizeconvertVideoToOutputLineDirectOptBE(USB5653DSOptimizeCaptureReceived *p_in, VideoOutputData *p_out, uint16_t column, bool is_n3ds) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	size_t target_bot_column_pre_last = BOT_WIDTH_3DS - 2;
	bool is_special_header = usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->bottom_only_column;
	if((!is_n3ds) && is_special_header) {
		column_start_bot_pos -= 1;
		column_pre_last_bot_pos -= 2;
		target_bot_column_pre_last = BOT_WIDTH_3DS - 1;
		if(column == column_last_bot_pos)
			return;
	}
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb565_pixels*)p_in->columns_data[column].pixel;
	else if(is_special_header)
		in_ptr = (interleaved_rgb565_pixels*)(((USB5653DSOptimizeCaptureReceivedExtraHeader*)p_in)->columns_data[column].pixel);
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, target_bot_column_pre_last);
	}
	else if(column < column_start_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
	else {
		out_ptr_top += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb565convertInterleaveVideoToOutputDirectOptBE(out_ptr_top, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos);
	}
}

static inline void usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptLE(USB5653DSOptimizeCaptureReceived_3D *p_in, VideoOutputData *p_out, uint16_t column, bool is_n3ds, bool interleaved_3d) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	size_t target_bot_column_pre_last = BOT_WIDTH_3DS - 2;
	bool is_special_header = usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].bot_top_l_screens_column.header_sound);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top_l = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	deinterleaved_rgb565_pixels* out_ptr_top_r = out_ptr_bottom + (((TOP_SIZE_3DS + BOT_SIZE_3DS) * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->bottom_only_column.pixel;
	if((!is_n3ds) && is_special_header) {
		column_start_bot_pos -= 1;
		column_pre_last_bot_pos -= 2;
		target_bot_column_pre_last = BOT_WIDTH_3DS - 1;
		if(column == column_last_bot_pos)
			return;
	}
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb565_pixels*)p_in->columns_data[column].bot_top_l_screens_column.pixel;
	int multiplier_top = 1;
	if(interleaved_3d) {
		multiplier_top = 2;
		out_ptr_top_r = out_ptr_top_l + ((HEIGHT_3DS * pixels_size) / ptr_out_size);
	}
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else {
		memcpy_data_u16le_origin((uint16_t*)(out_ptr_top_r + ((column * HEIGHT_3DS * multiplier_top * pixels_size) / ptr_out_size)), (uint8_t*)p_in->columns_data[column].top_r_screen_column.pixel, HEIGHT_3DS, false);
		if(column == column_pre_last_bot_pos) {
			usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(out_ptr_top_l, in_ptr, num_iters, 0, column, multiplier_top);
			usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, target_bot_column_pre_last);
		}
		else if(column < column_start_bot_pos)
			usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(out_ptr_top_l, in_ptr, num_iters, 0, column, multiplier_top);
		else {
			out_ptr_top_l += (column_start_bot_pos * HEIGHT_3DS * multiplier_top * pixels_size) / ptr_out_size;
			usb_rgb565convertInterleaveVideoToOutputDirectOptLE(out_ptr_top_l, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos, multiplier_top);
		}
	}
}

static inline void usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptBE(USB5653DSOptimizeCaptureReceived_3D *p_in, VideoOutputData *p_out, uint16_t column, bool is_n3ds, bool interleaved_3d) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	size_t target_bot_column_pre_last = BOT_WIDTH_3DS - 2;
	bool is_special_header = usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].bot_top_l_screens_column.header_sound);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top_l = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	deinterleaved_rgb565_pixels* out_ptr_top_r = out_ptr_bottom + (((TOP_SIZE_3DS + BOT_SIZE_3DS) * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->bottom_only_column.pixel;
	if((!is_n3ds) && is_special_header) {
		column_start_bot_pos -= 1;
		column_pre_last_bot_pos -= 2;
		target_bot_column_pre_last = BOT_WIDTH_3DS - 1;
		if(column == column_last_bot_pos)
			return;
	}
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb565_pixels*)p_in->columns_data[column].bot_top_l_screens_column.pixel;
	int multiplier_top = 1;
	if(interleaved_3d) {
		multiplier_top = 2;
		out_ptr_top_r = out_ptr_top_l + ((HEIGHT_3DS * pixels_size) / ptr_out_size);
	}
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else {
		memcpy_data_u16le_origin((uint16_t*)(out_ptr_top_r + ((column * HEIGHT_3DS * multiplier_top * pixels_size) / ptr_out_size)), (uint8_t*)p_in->columns_data[column].top_r_screen_column.pixel, HEIGHT_3DS, true);
		if(column == column_pre_last_bot_pos) {
			usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(out_ptr_top_l, in_ptr, num_iters, 0, column, multiplier_top);
			usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, target_bot_column_pre_last);
		}
		else if(column < column_start_bot_pos)
			usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(out_ptr_top_l, in_ptr, num_iters, 0, column, multiplier_top);
		else {
			out_ptr_top_l += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
			usb_rgb565convertInterleaveVideoToOutputDirectOptBE(out_ptr_top_l, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos, multiplier_top);
		}
	}
}

static inline void usb_3DS888OptimizeconvertVideoToOutputLineDirectOpt(USB8883DSOptimizeCaptureReceived *p_in, VideoOutputData *p_out, uint16_t column, bool is_n3ds) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb888_u16_pixels);
	size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	size_t target_bot_column_pre_last = BOT_WIDTH_3DS - 2;
	bool is_special_header = usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound);
	deinterleaved_rgb888_u16_pixels* out_ptr_bottom = (deinterleaved_rgb888_u16_pixels*)p_out->rgb_video_output_data.screen_data;
	deinterleaved_rgb888_u16_pixels* out_ptr_top = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	interleaved_rgb888_u16_pixels* in_ptr = (interleaved_rgb888_u16_pixels*)p_in->bottom_only_column;
	if((!is_n3ds) && is_special_header) {
		column_start_bot_pos -= 1;
		column_pre_last_bot_pos -= 2;
		target_bot_column_pre_last = BOT_WIDTH_3DS - 1;
		if(column == column_last_bot_pos)
			return;
	}
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb888_u16_pixels*)p_in->columns_data[column].pixel;
	else if(is_special_header)
		in_ptr = (interleaved_rgb888_u16_pixels*)(((USB8883DSOptimizeCaptureReceivedExtraHeader*)p_in)->columns_data[column].pixel);
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, target_bot_column_pre_last);
	}
	else if(column < column_start_bot_pos)
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
	else {
		out_ptr_top += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb888convertInterleaveU16VideoToOutputDirectOpt(out_ptr_top, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos);
	}
}

static inline void usb_3DS888Optimizeconvert3DForced2DVideoToOutputLineDirectOpt(USB8883DSOptimizeCaptureReceived_3D_Forced2DSingleScreen *p_in, VideoOutputData *p_out, uint16_t column, bool is_n3ds, bool &do_expand) {
	const int pixels_size = sizeof(VideoPixelRGB);
	bool is_for_bottom = true;
	uint8_t* out_ptr = (uint8_t*)p_out->rgb_video_output_data.screen_data;
	if(usb_OptimizeGetDataBufferNumber(&p_in->columns_data[column].header_sound) == 1) {
		out_ptr = out_ptr + (BOT_SIZE_3DS * pixels_size);
		is_for_bottom = false;
	}
	size_t column_start = 0;
	size_t column_end = TOP_WIDTH_3DS;
	size_t special_column_index = 0xFFFF;
	size_t special_column_target = 0xFFFF;
	if(is_for_bottom) {
		if(is_n3ds) {
			column_start = TOP_WIDTH_3DS - BOT_WIDTH_3DS + 2;
			column_end = TOP_WIDTH_3DS + 1;
			special_column_index = TOP_WIDTH_3DS - BOT_WIDTH_3DS;
			special_column_target = BOT_WIDTH_3DS - 2;
			do_expand = false;
		}
		else {
			column_start = TOP_WIDTH_3DS - BOT_WIDTH_3DS + 1;
			column_end = TOP_WIDTH_3DS;
			special_column_index = TOP_WIDTH_3DS - BOT_WIDTH_3DS;
			special_column_target = BOT_WIDTH_3DS - 1;
			do_expand = false;
		}
	}

	if(column == special_column_index) {
		memcpy(out_ptr + (special_column_target * HEIGHT_3DS * pixels_size), (uint8_t*)p_in->columns_data[column].pixel, HEIGHT_3DS * pixels_size);
		return;
	}
	if(column < column_start)
		return;
	if(column >= column_end)
		return;
	if(column == TOP_WIDTH_3DS) {
		memcpy(out_ptr + ((BOT_WIDTH_3DS - 1) * HEIGHT_3DS * pixels_size), (uint8_t*)p_in->columns_data[column].pixel, HEIGHT_3DS * pixels_size);
		return;
	}
	memcpy(out_ptr + ((column - column_start) * HEIGHT_3DS * pixels_size), (uint8_t*)p_in->columns_data[column].pixel, HEIGHT_3DS * pixels_size);
}

static inline void usb_3DS888Optimizeconvert3DVideoToOutputLineDirectOpt(USB8883DSOptimizeCaptureReceived_3D *p_in, VideoOutputData *p_out, uint16_t column, bool is_n3ds, bool interleaved_3d, bool is_bottom_data) {
	const int pixels_size = sizeof(VideoPixelRGB);
	const size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	const size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	uint8_t* out_ptr_bottom = (uint8_t*)p_out->rgb_video_output_data.screen_data;
	uint8_t* out_ptr_top_l = out_ptr_bottom + (BOT_SIZE_3DS * pixels_size);
	uint8_t* out_ptr_top_r = out_ptr_top_l + (TOP_SIZE_3DS * pixels_size);
	int multiplier_top = 1;
	if(interleaved_3d) {
		multiplier_top = 2;
		out_ptr_top_r = out_ptr_top_l + (HEIGHT_3DS * pixels_size);
	}

	if((!is_bottom_data) && (column >= TOP_WIDTH_3DS))
		return;

	uint8_t* dst_ptr_first = out_ptr_bottom;
	uint8_t* dst_ptr_second = out_ptr_bottom;
	if(is_bottom_data) {
		if(column >= TOP_WIDTH_3DS) {
			memcpy(out_ptr_bottom + ((BOT_WIDTH_3DS - 1) * HEIGHT_3DS * pixels_size), (uint8_t*)p_in->bottom_only_column.pixel, HEIGHT_3DS * pixels_size);
		}
		else {
			int out_column_pos = -1;
			if(column == column_pre_last_bot_pos)
				out_column_pos = BOT_WIDTH_3DS - 2;
			else if(column >= column_start_bot_pos)
				out_column_pos = column - column_start_bot_pos;
			if(out_column_pos == -1)
				return;
			memcpy(out_ptr_bottom + (out_column_pos * HEIGHT_3DS * pixels_size), (uint8_t*)p_in->columns_data[column][1].pixel, HEIGHT_3DS * pixels_size);
		}
		return;
	}

	memcpy(out_ptr_top_l + (column * HEIGHT_3DS * pixels_size * multiplier_top), (uint8_t*)p_in->columns_data[column][0].pixel, HEIGHT_3DS * pixels_size);
	memcpy(out_ptr_top_r + (column * HEIGHT_3DS * pixels_size * multiplier_top), (uint8_t*)p_in->columns_data[column][1].pixel, HEIGHT_3DS * pixels_size);
}

static void usb_oldDSconvertVideoToOutput(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, const bool is_big_endian) {
	#ifndef SIMPLE_DS_FRAME_SKIP
	if(!p_in->frameinfo.valid) { //LCD was off
		memset(p_out->bgr16_video_output_data.screen_data, 0, WIDTH_DS * (2 * HEIGHT_DS) * sizeof(uint16_t));
		return;
	}

	// Handle first line being off, if needed
	memset(p_out->bgr16_video_output_data.screen_data, 0, WIDTH_DS * sizeof(uint16_t));

	if(!is_big_endian) {
		size_t input_halfline = 0;
		for(size_t i = 0; i < 2; i++) {
			if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
				usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(p_in, p_out, input_halfline++, i);
		}

		for(size_t i = 2; i < HEIGHT_DS * 2; i++) {
			if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
				usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(p_in, p_out, input_halfline++, i);
			else { // deal with missing half-line
				uint16_t* out_ptr_top = (uint16_t*)&p_out->bgr16_video_output_data.screen_data;
				uint16_t* out_ptr_bottom = out_ptr_top + (WIDTH_DS * HEIGHT_DS);
				memcpy(&out_ptr_top[i * (WIDTH_DS / 2)], &out_ptr_top[(i - 2) * (WIDTH_DS / 2)], (WIDTH_DS / 2) * sizeof(uint16_t));
				memcpy(&out_ptr_bottom[i * (WIDTH_DS / 2)], &out_ptr_bottom[(i - 2) * (WIDTH_DS / 2)], (WIDTH_DS / 2) * sizeof(uint16_t));
			}
		}
	}
	else {
		size_t input_halfline = 0;
		for(size_t i = 0; i < 2; i++) {
			if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
				usb_oldDSconvertVideoToOutputHalfLineDirectOptBE(p_in, p_out, input_halfline++, i);
		}

		for(size_t i = 2; i < HEIGHT_DS * 2; i++) {
			if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
				usb_oldDSconvertVideoToOutputHalfLineDirectOptBE(p_in, p_out, input_halfline++, i);
			else { // deal with missing half-line
				uint16_t* out_ptr_top = (uint16_t*)&p_out->bgr16_video_output_data.screen_data;
				uint16_t* out_ptr_bottom = out_ptr_top + (WIDTH_DS * HEIGHT_DS);
				memcpy(&out_ptr_top[i * (WIDTH_DS / 2)], &out_ptr_top[(i - 2) * (WIDTH_DS / 2)], (WIDTH_DS / 2) * sizeof(uint16_t));
				memcpy(&out_ptr_bottom[i * (WIDTH_DS / 2)], &out_ptr_bottom[(i - 2) * (WIDTH_DS / 2)], (WIDTH_DS / 2) * sizeof(uint16_t));
			}
		}
	}
	#else
	if(!is_big_endian)
		for(size_t i = 0; i < HEIGHT_DS * 2; i++)
			usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(p_in, p_out, i, i);
	else
		for(size_t i = 0; i < HEIGHT_DS * 2; i++)
			usb_oldDSconvertVideoToOutputHalfLineDirectOptBE(p_in, p_out, i, i);
	#endif
}

static void ftd2_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, const bool is_big_endian) {
	usb_oldDSconvertVideoToOutput(&p_in->usb_received_old_ds, p_out, is_big_endian);
}

static void usb_3DSconvertVideoToOutput(USB3DSCaptureReceived *p_in, VideoOutputData *p_out) {
	memcpy(p_out->rgb_video_output_data.screen_data, p_in->video_in.screen_data, IN_VIDEO_HEIGHT_3DS * IN_VIDEO_WIDTH_3DS * 3);
}

static void usb_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureDevice* capture_device, bool enabled_3d, const bool is_big_endian, bool interleaved_3d, bool requested_3d) {
	if(capture_device->is_3ds) {
		if(!enabled_3d)
			usb_3DSconvertVideoToOutput(&p_in->usb_received_3ds, p_out);
	}
	else
		usb_oldDSconvertVideoToOutput(&p_in->usb_received_old_ds, p_out, is_big_endian);
}

inline static uint8_t to_8_bit_6(uint8_t data) {
	return (data << 2) | (data >> 4);
}

inline static uint8_t to_8_bit_5(uint8_t data) {
	return (data << 3) | (data >> 2);
}

inline static void to_8_bit_6(uint8_t* out, uint8_t* in) {
	out[0] = to_8_bit_6(in[0]);
	out[1] = to_8_bit_6(in[1]);
	out[2] = to_8_bit_6(in[2]);
}

static void usb_cypress_nisetro_ds_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out) {
	int pos_top = 0;
	int pos_bottom = WIDTH_DS * HEIGHT_DS * sizeof(VideoPixelRGB);
	uint8_t* data_in = (uint8_t*)p_in->cypress_nisetro_capture_received.video_in.screen_data;
	uint8_t* data_out = (uint8_t*)p_out->rgb_video_output_data.screen_data;
	for(size_t i = 0; i < sizeof(CypressNisetroDSCaptureReceived); i++) {
		uint8_t conv = to_8_bit_6(data_in[i]);
		if(data_in[i] & 0x40)
			data_out[pos_bottom++] = conv;
		else
			data_out[pos_top++] = conv;
	}
}

static void usb_3ds_optimize_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d, bool should_be_3d, const bool is_big_endian, bool interleaved_3d, bool requested_3d, bool is_rgb888, bool is_n3ds) {
	if(!is_rgb888) {
		if(!enabled_3d) {
			if(!is_big_endian)
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565OptimizeconvertVideoToOutputLineDirectOptLE(&p_in->cypress_optimize_received_565, p_out, i, is_n3ds);
			else
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565OptimizeconvertVideoToOutputLineDirectOptBE(&p_in->cypress_optimize_received_565, p_out, i, is_n3ds);
			expand_2d_to_3d_convertVideoToOutput((uint8_t*)p_out->rgb16_video_output_data.screen_data, sizeof(VideoPixelRGB16), interleaved_3d, requested_3d);
		}
		else {
			if(!is_big_endian)
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptLE(&p_in->cypress_optimize_received_565_3d, p_out, i, is_n3ds, interleaved_3d);
			else
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptBE(&p_in->cypress_optimize_received_565_3d, p_out, i, is_n3ds, interleaved_3d);
		}
	}
	else {
		if(!enabled_3d) {
			bool do_expand = true;
			if(should_be_3d) {
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS888Optimizeconvert3DForced2DVideoToOutputLineDirectOpt(&p_in->cypress_optimize_received_888_3d_2d, p_out, i, is_n3ds, do_expand);
			}
			else {
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS888OptimizeconvertVideoToOutputLineDirectOpt(&p_in->cypress_optimize_received_888, p_out, i, is_n3ds);
			}
			if(do_expand)
				expand_2d_to_3d_convertVideoToOutput((uint8_t*)p_out->rgb_video_output_data.screen_data, sizeof(VideoPixelRGB), interleaved_3d, requested_3d);
		}
		else {
			USB3DSOptimizeHeaderSoundData* first_column_header = getAudioHeaderPtrOptimize3DS3D(p_in, is_rgb888, 0);
			bool is_bottom_data = (((uint8_t*)&first_column_header->header_info.column_info)[1] & 0x40) == 0;
			for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++) {
				usb_3DS888Optimizeconvert3DVideoToOutputLineDirectOpt(&p_in->cypress_optimize_received_888_3d, p_out, i, is_n3ds, interleaved_3d, is_bottom_data);
			}
		}
	}
}

static void usb_is_device_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureStatus* status, CaptureScreensType capture_type) {
	bool is_nitro = true;
	#ifdef USE_IS_DEVICES_USB
	is_nitro = is_device_is_nitro(&status->device);
	#endif
	if(is_nitro) {
		uint64_t num_pixels = usb_is_device_get_video_in_size(status) / 3;
		uint64_t out_start_pos = 0;
		uint64_t out_clear_pos = num_pixels;
		if(capture_type == CAPTURE_SCREENS_BOTTOM) {
			out_start_pos = num_pixels;
			out_clear_pos = 0;
		}
		if((capture_type == CAPTURE_SCREENS_BOTTOM) || (capture_type == CAPTURE_SCREENS_TOP))
			memset((uint8_t*)&p_out->bgr_video_output_data.screen_data[out_clear_pos], 0, (size_t)(num_pixels * sizeof(VideoPixelBGR)));
		memcpy((uint8_t*)&p_out->bgr_video_output_data.screen_data[out_start_pos], p_in->is_nitro_capture_received.video_in.screen_data, (size_t)(num_pixels * sizeof(VideoPixelBGR)));
		return;
	}
	ISTWLCaptureVideoInputData* data = &p_in->is_twl_capture_received.video_capture_in.video_in;
	twl_16bit_pixels* data_16_bit = (twl_16bit_pixels*)data->screen_data;
	twl_2bit_pixels* data_2_bit = (twl_2bit_pixels*)data->bit_6_rb_screen_data;
	const int num_pixels_struct = sizeof(twl_16bit_pixels) / sizeof(uint16_t);
	const int num_screens = 2;
	for(int i = 0; i < HEIGHT_DS; i++) {
		for(int j = 0; j < num_screens; j++) {
			size_t out_pos = ((num_screens - 1 - j) * (WIDTH_DS * HEIGHT_DS)) + (i * WIDTH_DS);
			for(int u = 0; u < (WIDTH_DS / num_pixels_struct); u++) {
				uint8_t pixels[num_pixels_struct][3];
				size_t index = (((i * num_screens * WIDTH_DS) + (j * WIDTH_DS)) / num_pixels_struct) + u;
				pixels[0][0] = (data_16_bit[index].first_r << 1) | (data_2_bit[index].first_r);
				pixels[0][1] = data_16_bit[index].first_g;
				pixels[0][2] = (data_16_bit[index].first_b << 1) | (data_2_bit[index].first_b);
				pixels[1][0] = (data_16_bit[index].second_r << 1) | (data_2_bit[index].second_r);
				pixels[1][1] = data_16_bit[index].second_g;
				pixels[1][2] = (data_16_bit[index].second_b << 1) | (data_2_bit[index].second_b);
				pixels[2][0] = (data_16_bit[index].third_r << 1) | (data_2_bit[index].third_r);
				pixels[2][1] = data_16_bit[index].third_g;
				pixels[2][2] = (data_16_bit[index].third_b << 1) | (data_2_bit[index].third_b);
				pixels[3][0] = (data_16_bit[index].fourth_r << 1) | (data_2_bit[index].fourth_r);
				pixels[3][1] = data_16_bit[index].fourth_g;
				pixels[3][2] = (data_16_bit[index].fourth_b << 1) | (data_2_bit[index].fourth_b);
				for(int k = 0; k < num_pixels_struct; k++)
					to_8_bit_6((uint8_t*)&p_out->rgb_video_output_data.screen_data[out_pos + (u * num_pixels_struct) + k], pixels[k]);
			}
		}
	}
}

// To be moved to the appropriate header, later...
#define PARTNER_CTR_CAPTURE_BASE_COMMAND 0xE007
#define PARTNER_CTR_CAPTURE_COMMAND_INPUT 0x000F
#define PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN 0x00C4
#define PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN 0x00C5
#define PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN 0x00C6
#define PARTNER_CTR_CAPTURE_COMMAND_AUDIO 0x00C7

static PartnerCTRCaptureCommand read_partner_ctr_base_command(uint8_t* data) {
	PartnerCTRCaptureCommand out_cmd;
	// Important: Ensure the data after the buffers for a single frame
	// is stopped by wrong magic value...
	out_cmd.magic = read_le16(data, 0);
	out_cmd.command = read_le16(data, 1);
	out_cmd.payload_size = read_le32(data, 1);
	return out_cmd;
}

static size_t get_partner_ctr_size_command_header(PartnerCTRCaptureCommand read_command) {
	switch (read_command.command) {
		case PARTNER_CTR_CAPTURE_COMMAND_INPUT:
			return sizeof(PartnerCTRCaptureCommandHeader0F);
		case PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN:
			return sizeof(PartnerCTRCaptureCommandHeaderCxScreen);
		case PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN:
			return sizeof(PartnerCTRCaptureCommandHeaderCxScreen);
		case PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN:
			return sizeof(PartnerCTRCaptureCommandHeaderCxScreen);
		case PARTNER_CTR_CAPTURE_COMMAND_AUDIO:
			return sizeof(PartnerCTRCaptureCommandHeaderC7);
		default:
			ActualConsoleOutTextError("Partner CTR Capture: Unknown command found! " + std::to_string(read_command.command));
			return sizeof(PartnerCTRCaptureCommandHeader0F);
	}
}

static uint8_t* get_ptr_next_command_partner_ctr(uint8_t* data) {
	PartnerCTRCaptureCommand read_command = read_partner_ctr_base_command(data);
	return data + get_partner_ctr_size_command_header(read_command) + read_command.payload_size;
}

static uint8_t* find_partner_ctr_x_screen(uint8_t* data) {
	PartnerCTRCaptureCommand read_command = read_partner_ctr_base_command(data);
	if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
		return NULL;

	if((read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN))
		return data;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_AUDIO)
		data = get_ptr_next_command_partner_ctr(data);

	read_command = read_partner_ctr_base_command(data);
	if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
		return NULL;

	if((read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN))
		return data;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_INPUT)
		data = get_ptr_next_command_partner_ctr(data);

	read_command = read_partner_ctr_base_command(data);
	if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
		return NULL;

	if((read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN) || (read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN))
		return data;
	ActualConsoleOutTextError("Partner CTR Capture: Unknown command found! " + std::to_string(read_command.command));
	return NULL;
}

static bool is_valid_frame_partner_ctr(uint8_t* data, bool enabled_3d, uint8_t** first_screen, uint8_t** second_screen, uint8_t** third_screen) {
	bool has_top = false;
	bool has_top_second = !enabled_3d;
	bool has_bottom = false;

	*first_screen = find_partner_ctr_x_screen(data);
	if(*first_screen == NULL)
		return false;
	PartnerCTRCaptureCommand read_command = read_partner_ctr_base_command(*first_screen);
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN)
		has_top = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN)
		has_bottom = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN)
		has_top_second = true;

	*second_screen = find_partner_ctr_x_screen(get_ptr_next_command_partner_ctr(*first_screen));
	if(*second_screen == NULL)
		return false;

	read_command = read_partner_ctr_base_command(*second_screen);
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN)
		has_top = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN)
		has_bottom = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN)
		has_top_second = true;

	if(!enabled_3d)
		return has_top && has_bottom;

	*third_screen = find_partner_ctr_x_screen(get_ptr_next_command_partner_ctr(*second_screen));
	if(*third_screen == NULL)
		return false;

	read_command = read_partner_ctr_base_command(*third_screen);
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN)
		has_top = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN)
		has_bottom = true;
	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN)
		has_top_second = true;

	return has_top && has_top_second && has_bottom;
}

static void convert_partner_ctr_screen_x(uint8_t* screen_ptr, VideoOutputData *p_out) {
	if(screen_ptr == NULL)
		return;

	PartnerCTRCaptureCommand read_command = read_partner_ctr_base_command(screen_ptr);
	VideoPixelRGB* out_screen_data = &p_out->rgb_video_output_data.screen_data[0];
	screen_ptr += get_partner_ctr_size_command_header(read_command);

	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN) {
		for(size_t i = 0; i < HEIGHT_3DS; i++)
			memcpy(&out_screen_data[i * TOP_WIDTH_3DS], screen_ptr + (i * BOT_WIDTH_3DS * sizeof(VideoPixelRGB)), BOT_WIDTH_3DS * sizeof(VideoPixelRGB));
		return;
	}

	if(read_command.command == PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN) {
		memcpy(&out_screen_data[TOP_WIDTH_3DS * HEIGHT_3DS], screen_ptr, TOP_WIDTH_3DS * HEIGHT_3DS * sizeof(VideoPixelRGB));
		return;
	}

	memcpy(&out_screen_data[2 * TOP_WIDTH_3DS * HEIGHT_3DS], screen_ptr, TOP_WIDTH_3DS * HEIGHT_3DS * sizeof(VideoPixelRGB));
}

static void usb_partner_ctr_interleave_3d(VideoOutputData *p_out) {
	VideoPixelRGB buffer_data[TOP_WIDTH_3DS * 2];

	VideoPixelRGB* out_top_screen = &p_out->rgb_video_output_data.screen_data[TOP_WIDTH_3DS * HEIGHT_3DS];
	VideoPixelRGB* out_second_top_screen = &p_out->rgb_video_output_data.screen_data[2 * TOP_WIDTH_3DS * HEIGHT_3DS];
	for(size_t i = 0; i < HEIGHT_3DS; i++) {
		for(size_t j = 0; j < TOP_WIDTH_3DS; j++) {
			buffer_data[j * 2] = out_top_screen[(i * TOP_WIDTH_3DS) + j];
			buffer_data[(j * 2) + 1] = out_second_top_screen[(i * TOP_WIDTH_3DS) + j];
		}
		memcpy(&out_top_screen[i * TOP_WIDTH_3DS], &buffer_data[0], TOP_WIDTH_3DS * sizeof(VideoPixelRGB));
		memcpy(&out_second_top_screen[i * TOP_WIDTH_3DS], &buffer_data[TOP_WIDTH_3DS], TOP_WIDTH_3DS * sizeof(VideoPixelRGB));
	}
}

static void usb_partner_ctr_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d, bool interleaved_3d, bool requested_3d) {
	uint8_t* data = (uint8_t*)p_in;
	uint8_t* first_screen = NULL;
	uint8_t* second_screen = NULL;
	uint8_t* third_screen = NULL;

	if(!is_valid_frame_partner_ctr(data, enabled_3d, &first_screen, &second_screen, &third_screen))
		return;

	convert_partner_ctr_screen_x(first_screen, p_out);
	convert_partner_ctr_screen_x(second_screen, p_out);
	convert_partner_ctr_screen_x(third_screen, p_out);

	if(requested_3d && (!enabled_3d))
		memcpy(&p_out->rgb_video_output_data.screen_data[2 * TOP_WIDTH_3DS * HEIGHT_3DS], &p_out->rgb_video_output_data.screen_data[TOP_WIDTH_3DS * HEIGHT_3DS], TOP_WIDTH_3DS * HEIGHT_3DS * sizeof(VideoPixelRGB));

	if(requested_3d && interleaved_3d)
		usb_partner_ctr_interleave_3d(p_out);
}

bool convertVideoToOutput(VideoOutputData *p_out, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status, bool interleaved_3d) {
	CaptureReceived* p_in = (CaptureReceived*)(((uint8_t*)&data_buffer->capture_buf) + data_buffer->unused_offset);
	bool converted = false;
	CaptureDevice* chosen_device = &status->device;
	bool is_data_3d = data_buffer->is_3d;
	bool should_be_3d = data_buffer->should_be_3d;
	InputVideoDataType video_data_type = data_buffer->buffer_video_data_type;
	bool is_3d_requested = get_3d_enabled(status);
	#ifdef USE_FTD3
	if(chosen_device->cc_type == CAPTURE_CONN_FTD3) {
		ftd3_convertVideoToOutput(p_in, p_out, is_data_3d, interleaved_3d, is_3d_requested);
		converted = true;
	}
	#endif
	#ifdef USE_FTD2
	if(chosen_device->cc_type == CAPTURE_CONN_FTD2) {
		ftd2_convertVideoToOutput(p_in, p_out, is_big_endian);
		converted = true;
	}
	#endif
	#ifdef USE_DS_3DS_USB
	if(chosen_device->cc_type == CAPTURE_CONN_USB) {
		usb_convertVideoToOutput(p_in, p_out, chosen_device, is_data_3d, is_big_endian, interleaved_3d, is_3d_requested);
		converted = true;
	}
	#endif
	#ifdef USE_IS_DEVICES_USB
	if(chosen_device->cc_type == CAPTURE_CONN_IS_NITRO) {
		usb_is_device_convertVideoToOutput(p_in, p_out, status, data_buffer->capture_type);
		converted = true;
	}
	#endif
	#ifdef USE_CYNI_USB
	if(chosen_device->cc_type == CAPTURE_CONN_CYPRESS_NISETRO) {
		usb_cypress_nisetro_ds_convertVideoToOutput(p_in, p_out);
		converted = true;
	}
	#endif
	#ifdef USE_CYPRESS_OPTIMIZE
	if(status->device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE) {
		bool is_rgb888 = video_data_type == VIDEO_DATA_RGB;
		bool is_n3ds = is_device_optimize_n3ds(&status->device);
		usb_3ds_optimize_convertVideoToOutput(p_in, p_out, is_data_3d, should_be_3d, is_big_endian, interleaved_3d, is_3d_requested, is_rgb888, is_n3ds);
		converted = true;
	}
	#endif
	#ifdef USE_PARTNER_CTR
	if(status->device.cc_type == CAPTURE_CONN_PARTNER_CTR) {
		usb_partner_ctr_convertVideoToOutput(p_in, p_out, is_data_3d, interleaved_3d, is_3d_requested);
		converted = true;
	}
	#endif
	return converted;
}

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DS(CaptureReceived* buffer, bool is_rgb888, uint16_t column) {
	if(!is_rgb888)
		return &buffer->cypress_optimize_received_565.columns_data[column].header_sound;
	return &buffer->cypress_optimize_received_888.columns_data[column].header_sound;
}

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DSExtraHeader(CaptureReceived* buffer, bool is_rgb888, uint16_t column) {
	if(!is_rgb888)
		return &buffer->cypress_optimize_received_565_extra_header.columns_data[column].header_sound;
	return &buffer->cypress_optimize_received_888_extra_header.columns_data[column].header_sound;
}

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DS3D(CaptureReceived* buffer, bool is_rgb888, uint16_t column) {
	if(!is_rgb888) {
		int target_column = column / 2;
		if((column % 2) == 0)
			return &buffer->cypress_optimize_received_565_3d.columns_data[target_column].top_r_screen_column.header_sound;
		return &buffer->cypress_optimize_received_565_3d.columns_data[target_column].bot_top_l_screens_column.header_sound;
	}
	return &buffer->cypress_optimize_received_888_3d.columns_data[column / 2][column % 2].header_sound;
}

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DS3DForced2D(CaptureReceived* buffer, uint16_t column) {
	return &buffer->cypress_optimize_received_888_3d_2d.columns_data[column].header_sound;
}

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DS3DExtraHeader(CaptureReceived* buffer, bool is_rgb888) {
	if(!is_rgb888)
		return &buffer->cypress_optimize_received_565_3d.bottom_only_column.header_sound;
	return &buffer->cypress_optimize_received_888_3d.bottom_only_column.header_sound;
}

static inline uint16_t read_sample_indexLE(USB3DSOptimizeSingleSoundData* sample) {
	return sample->sample_index % OPTIMIZE_3DS_AUDIO_BUFFER_MAX_SIZE;
}

static inline uint16_t read_sample_indexBE(USB3DSOptimizeSingleSoundData* sample) {
	return _reverse_endianness(sample->sample_index) % OPTIMIZE_3DS_AUDIO_BUFFER_MAX_SIZE;
}

static inline void copyAudioFromSoundDataOptimize3DSLE(std::int16_t *p_out, USB3DSOptimizeSingleSoundData* sample, uint64_t& num_inserted, int& last_inserted_index) {
	uint16_t read_index = read_sample_indexLE(sample) % OPTIMIZE_3DS_AUDIO_BUFFER_MAX_SIZE;
	if(read_index == last_inserted_index)
		return;
	p_out[num_inserted * 2] = sample->sample_l;
	p_out[(num_inserted * 2) + 1] = sample->sample_r;
	num_inserted += 1;
	last_inserted_index = read_index;
}

static inline void copyAudioFromSoundDataOptimize3DSBE(std::int16_t *p_out, USB3DSOptimizeSingleSoundData* sample, uint64_t& num_inserted, int& last_inserted_index) {
	uint16_t read_index = read_sample_indexBE(sample) % OPTIMIZE_3DS_AUDIO_BUFFER_MAX_SIZE;
	if(read_index == last_inserted_index)
		return;
	p_out[num_inserted * 2] = _reverse_endianness(sample->sample_l);
	p_out[(num_inserted * 2) + 1] = _reverse_endianness(sample->sample_r);
	num_inserted += 1;
	last_inserted_index = read_index;
}

static void copyAudioOptimize3DSLE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer, bool is_rgb888) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < TOP_WIDTH_3DS; i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, i);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	USB3DSOptimizeHeaderSoundData* initial_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, 0);
	if(usb_OptimizeHasExtraHeaderSoundData(initial_column_data)) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DSExtraHeader(buffer, is_rgb888, TOP_WIDTH_3DS);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

static void copyAudioOptimize3DS3DLE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer, bool is_rgb888) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < TOP_WIDTH_3DS * 2; i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS3D(buffer, is_rgb888, i);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS3DExtraHeader(buffer, is_rgb888);
	copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
	copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

static void copyAudioOptimize3DS3DForced2DLE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS3DForced2D(buffer, i);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

static void copyAudioOptimize3DSBE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer, bool is_rgb888) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < TOP_WIDTH_3DS; i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, i);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	USB3DSOptimizeHeaderSoundData* initial_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, 0);
	if(usb_OptimizeHasExtraHeaderSoundData(initial_column_data)) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DSExtraHeader(buffer, is_rgb888, TOP_WIDTH_3DS);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

static void copyAudioOptimize3DS3DBE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer, bool is_rgb888) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < TOP_WIDTH_3DS * 2; i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS3D(buffer, is_rgb888, i);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS3DExtraHeader(buffer, is_rgb888);
	copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
	copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

static void copyAudioOptimize3DS3DForced2DBE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS3DForced2D(buffer, i);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

static void usb_partner_ctr_copyBufferToAudio(uint8_t* buffer_ptr, std::int16_t *p_out, uint64_t &n_samples, const bool is_big_endian) {
	PartnerCTRCaptureCommand read_command = read_partner_ctr_base_command(buffer_ptr);
	if(read_command.magic != PARTNER_CTR_CAPTURE_BASE_COMMAND)
		return;
	if(read_command.command != PARTNER_CTR_CAPTURE_COMMAND_AUDIO)
		return;

	buffer_ptr += get_partner_ctr_size_command_header(read_command);

	memcpy_data_u16le_origin((uint16_t*)p_out + n_samples, buffer_ptr, (size_t)read_command.payload_size / 2, is_big_endian);

	n_samples += read_command.payload_size / 2;
}

static void usb_partner_ctr_copyBufferAfterCommandToAudio(uint8_t* buffer_ptr, std::int16_t *p_out, uint64_t &n_samples, const bool is_big_endian) {
	if(buffer_ptr == NULL)
		return;

	usb_partner_ctr_copyBufferToAudio(get_ptr_next_command_partner_ctr(buffer_ptr), p_out, n_samples, is_big_endian);
}

static void usb_partner_ctr_convertAudioToOutput(CaptureReceived *p_in, std::int16_t *p_out, uint64_t &n_samples, bool enabled_3d, const bool is_big_endian) {
	uint8_t* data = (uint8_t*)p_in;
	uint8_t* first_screen = NULL;
	uint8_t* second_screen = NULL;
	uint8_t* third_screen = NULL;
	n_samples = 0;

	if(!is_valid_frame_partner_ctr(data, enabled_3d, &first_screen, &second_screen, &third_screen))
		return;

	usb_partner_ctr_copyBufferToAudio(data, p_out, n_samples, is_big_endian);
	usb_partner_ctr_copyBufferAfterCommandToAudio(first_screen, p_out, n_samples, is_big_endian);
	usb_partner_ctr_copyBufferAfterCommandToAudio(second_screen, p_out, n_samples, is_big_endian);
	usb_partner_ctr_copyBufferAfterCommandToAudio(third_screen, p_out, n_samples, is_big_endian);
}

bool convertAudioToOutput(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status) {
	if(!status->device.has_audio)
		return true;
	bool is_data_3d = data_buffer->is_3d;
	bool should_be_3d = data_buffer->should_be_3d;
	InputVideoDataType video_data_type = data_buffer->buffer_video_data_type;
	CaptureReceived* p_in = (CaptureReceived*)(((uint8_t*)&data_buffer->capture_buf) + data_buffer->unused_offset);
	uint8_t* base_ptr = NULL;
	#ifdef USE_FTD3
	if(status->device.cc_type == CAPTURE_CONN_FTD3) {
		if(!is_data_3d)
			base_ptr = (uint8_t*)p_in->ftd3_received.audio_data;
		else
			base_ptr = (uint8_t*)p_in->ftd3_received_3d.audio_data;
	}
	#endif
	#ifdef USE_FTD2
	if(status->device.cc_type == CAPTURE_CONN_FTD2)
		base_ptr = (uint8_t*)p_in->ftd2_received_old_ds.audio_data;
	#endif
	#ifdef USE_DS_3DS_USB
	if(status->device.cc_type == CAPTURE_CONN_USB) {
		if(!is_data_3d)
			base_ptr = (uint8_t*)p_in->usb_received_3ds.audio_data;
		else
			base_ptr = (uint8_t*)p_in->usb_received_3ds_3d.audio_data;
	}
	#endif
	#ifdef USE_IS_DEVICES_USB
	if(status->device.cc_type == CAPTURE_CONN_IS_NITRO) {
		base_ptr = (uint8_t*)p_in->is_twl_capture_received.audio_capture_in;
		uint16_t* base_ptr16 = (uint16_t*)base_ptr;
		size_t size_real = (size_t)(n_samples * 2);
		size_t size_packet_converted = sizeof(ISTWLCaptureSoundData);
		size_t num_packets = size_real / size_packet_converted;
		for(size_t i = 0; i < num_packets; i++)
			memcpy(base_ptr + (i * size_packet_converted), ((uint8_t*)&p_in->is_twl_capture_received.audio_capture_in[i].sound_data.data), size_packet_converted);
		// Inverted L and R...
		for(uint64_t i = 0; i < n_samples; i++) {
			uint16_t r_sample = base_ptr16[(i * 2)];
			base_ptr16[(i * 2)] = base_ptr16[(i * 2) + 1];
			base_ptr16[(i * 2) + 1] = r_sample;
		}
			
	}
	#endif
	#ifdef USE_CYPRESS_OPTIMIZE
	if(status->device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE) {
		bool is_rgb888 = video_data_type == VIDEO_DATA_RGB;
		if(is_big_endian) {
			if(is_data_3d)
				copyAudioOptimize3DS3DBE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf, is_rgb888);
			else {
				if(should_be_3d && is_rgb888)
					copyAudioOptimize3DS3DForced2DBE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf);
				else
					copyAudioOptimize3DSBE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf, is_rgb888);
			}
		}
		else {
			if(is_data_3d)
				copyAudioOptimize3DS3DLE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf, is_rgb888);
			else {
				if(should_be_3d && is_rgb888)
					copyAudioOptimize3DS3DForced2DLE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf);
				else
					copyAudioOptimize3DSLE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf, is_rgb888);
			}
		}
		return true;
	}
	#endif
	#ifdef USE_PARTNER_CTR
	if(status->device.cc_type == CAPTURE_CONN_PARTNER_CTR) {
		usb_partner_ctr_convertAudioToOutput(p_in, p_out, n_samples, is_data_3d, is_big_endian);
		return true;
	}
	#endif
	if(base_ptr == NULL)
		return false;
	memcpy_data_u16le_origin((uint16_t*)p_out, base_ptr, (size_t)n_samples, is_big_endian);
	return true;
}

static void convert_rgb16_to_rgb(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height) {
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			size_t pixel = ((height - 1 - i) * width) + (width - 1 - j) + pos_x_data + (pos_y_data * width);
			uint8_t r = to_8_bit_5(src->rgb16_video_output_data.screen_data[pixel].r);
			uint8_t g = to_8_bit_6(src->rgb16_video_output_data.screen_data[pixel].g);
			uint8_t b = to_8_bit_5(src->rgb16_video_output_data.screen_data[pixel].b);
			dst->rgb_video_output_data.screen_data[pixel].r = r;
			dst->rgb_video_output_data.screen_data[pixel].g = g;
			dst->rgb_video_output_data.screen_data[pixel].b = b;
		}
	}
}

static void convert_bgr_to_rgb(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height) {
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			size_t pixel = ((height - 1 - i) * width) + (width - 1 - j) + pos_x_data + (pos_y_data * width);
			uint8_t r = src->bgr_video_output_data.screen_data[pixel].r;
			uint8_t g = src->bgr_video_output_data.screen_data[pixel].g;
			uint8_t b = src->bgr_video_output_data.screen_data[pixel].b;
			dst->rgb_video_output_data.screen_data[pixel].r = r;
			dst->rgb_video_output_data.screen_data[pixel].g = g;
			dst->rgb_video_output_data.screen_data[pixel].b = b;
		}
	}
}

static void convert_bgr16_to_rgb(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height) {
	uint16_t* data_16_ptr = (uint16_t*)src;
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			size_t pixel = ((height - 1 - i) * width) + (width - 1 - j) + pos_x_data + (pos_y_data * width);
			uint8_t r = to_8_bit_5(src->bgr16_video_output_data.screen_data[pixel].r);
			uint8_t g = to_8_bit_6(src->bgr16_video_output_data.screen_data[pixel].g);
			uint8_t b = to_8_bit_5(src->bgr16_video_output_data.screen_data[pixel].b);
			dst->rgb_video_output_data.screen_data[pixel].r = r;
			dst->rgb_video_output_data.screen_data[pixel].g = g;
			dst->rgb_video_output_data.screen_data[pixel].b = b;
		}
	}
}

void manualConvertOutputToRGB(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height, InputVideoDataType video_data_type) {
	switch (video_data_type) {
		case VIDEO_DATA_RGB:
			break;
		case VIDEO_DATA_RGB16:
			convert_rgb16_to_rgb(src, dst, pos_x_data, pos_y_data, width, height);
			break;
		case VIDEO_DATA_BGR:
			convert_bgr_to_rgb(src, dst, pos_x_data, pos_y_data, width, height);
			break;
		case VIDEO_DATA_BGR16:
			convert_bgr16_to_rgb(src, dst, pos_x_data, pos_y_data, width, height);
			break;
		default:
			break;
	}
}

static void convert_rgb_to_rgba(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height) {
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			size_t pixel = ((height - 1 - i) * width) + (width - 1 - j) + pos_x_data + (pos_y_data * width);
			uint8_t r = src->rgb_video_output_data.screen_data[pixel].r;
			uint8_t g = src->rgb_video_output_data.screen_data[pixel].g;
			uint8_t b = src->rgb_video_output_data.screen_data[pixel].b;
			dst->rgba_video_output_data.screen_data[pixel].r = r;
			dst->rgba_video_output_data.screen_data[pixel].g = g;
			dst->rgba_video_output_data.screen_data[pixel].b = b;
			dst->rgba_video_output_data.screen_data[pixel].alpha = 0xFF;
		}
	}
}

static void convert_rgb16_to_rgba(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height) {
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			size_t pixel = ((height - 1 - i) * width) + (width - 1 - j) + pos_x_data + (pos_y_data * width);
			uint8_t r = to_8_bit_5(src->rgb16_video_output_data.screen_data[pixel].r);
			uint8_t g = to_8_bit_6(src->rgb16_video_output_data.screen_data[pixel].g);
			uint8_t b = to_8_bit_5(src->rgb16_video_output_data.screen_data[pixel].b);
			dst->rgba_video_output_data.screen_data[pixel].r = r;
			dst->rgba_video_output_data.screen_data[pixel].g = g;
			dst->rgba_video_output_data.screen_data[pixel].b = b;
			dst->rgba_video_output_data.screen_data[pixel].alpha = 0xFF;
		}
	}
}

static void convert_bgr_to_rgba(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height) {
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			size_t pixel = ((height - 1 - i) * width) + (width - 1 - j) + pos_x_data + (pos_y_data * width);
			uint8_t r = src->bgr_video_output_data.screen_data[pixel].r;
			uint8_t g = src->bgr_video_output_data.screen_data[pixel].g;
			uint8_t b = src->bgr_video_output_data.screen_data[pixel].b;
			dst->rgba_video_output_data.screen_data[pixel].r = r;
			dst->rgba_video_output_data.screen_data[pixel].g = g;
			dst->rgba_video_output_data.screen_data[pixel].b = b;
			dst->rgba_video_output_data.screen_data[pixel].alpha = 0xFF;
		}
	}
}

static void convert_bgr16_to_rgba(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height) {
	uint16_t* data_16_ptr = (uint16_t*)src;
	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			size_t pixel = ((height - 1 - i) * width) + (width - 1 - j) + pos_x_data + (pos_y_data * width);
			uint8_t r = to_8_bit_5(src->bgr16_video_output_data.screen_data[pixel].r);
			uint8_t g = to_8_bit_6(src->bgr16_video_output_data.screen_data[pixel].g);
			uint8_t b = to_8_bit_5(src->bgr16_video_output_data.screen_data[pixel].b);
			dst->rgba_video_output_data.screen_data[pixel].r = r;
			dst->rgba_video_output_data.screen_data[pixel].g = g;
			dst->rgba_video_output_data.screen_data[pixel].b = b;
			dst->rgba_video_output_data.screen_data[pixel].alpha = 0xFF;
		}
	}
}

void manualConvertOutputToRGBA(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height, InputVideoDataType video_data_type) {
	switch (video_data_type) {
		case VIDEO_DATA_RGB:
			convert_rgb_to_rgba(src, dst, pos_x_data, pos_y_data, width, height);
			break;
		case VIDEO_DATA_RGB16:
			convert_rgb16_to_rgba(src, dst, pos_x_data, pos_y_data, width, height);
			break;
		case VIDEO_DATA_BGR:
			convert_bgr_to_rgba(src, dst, pos_x_data, pos_y_data, width, height);
			break;
		case VIDEO_DATA_BGR16:
			convert_bgr16_to_rgba(src, dst, pos_x_data, pos_y_data, width, height);
			break;
		default:
			break;
	}
}

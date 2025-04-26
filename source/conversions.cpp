#include "conversions.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3_shared.hpp"
#include "dscapture_ftd2_shared.hpp"
#include "usb_ds_3ds_capture.hpp"
#include "usb_is_device_acquisition.hpp"

#include <cstring>

#define INTERLEAVED_RGB565_PIXEL_NUM 4
#define INTERLEAVED_RGB565_PIXEL_SIZE 2
#define INTERLEAVED_RGB565_DATA_SIZE sizeof(uint16_t)
#define INTERLEAVED_RGB565_TOTAL_SIZE (INTERLEAVED_RGB565_PIXEL_SIZE * INTERLEAVED_RGB565_PIXEL_NUM / INTERLEAVED_RGB565_DATA_SIZE)

#define INTERLEAVED_RGB888_PIXEL_NUM 4
#define INTERLEAVED_RGB888_PIXEL_SIZE 3
#define INTERLEAVED_RGB888_DATA_SIZE sizeof(uint16_t)
#define INTERLEAVED_RGB888_TOTAL_SIZE (INTERLEAVED_RGB888_PIXEL_SIZE * INTERLEAVED_RGB888_PIXEL_NUM / INTERLEAVED_RGB888_DATA_SIZE)

struct interleaved_rgb565_pixels {
	uint16_t pixels[INTERLEAVED_RGB565_TOTAL_SIZE][2];
};

struct ALIGNED(INTERLEAVED_RGB565_PIXEL_NUM * INTERLEAVED_RGB565_PIXEL_SIZE) interleaved_3d_rgb565_pixels {
	uint16_t pixels[INTERLEAVED_RGB565_TOTAL_SIZE][3];
};

struct deinterleaved_rgb565_pixels {
	uint16_t pixels[INTERLEAVED_RGB565_TOTAL_SIZE];
};

struct ALIGNED(INTERLEAVED_RGB888_PIXEL_NUM * 2) interleaved_rgb888_u16_pixels {
	uint16_t pixels[INTERLEAVED_RGB888_TOTAL_SIZE][2];
};

struct ALIGNED(INTERLEAVED_RGB888_PIXEL_NUM) interleaved_3d_rgb888_u16_pixels {
	uint16_t pixels[INTERLEAVED_RGB888_TOTAL_SIZE][3];
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

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptLE(deinterleaved_rgb565_pixels* out_ptr_top, deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_pos];
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(deinterleaved_rgb565_pixels* out_ptr_top, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_pos];
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
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

static inline void usb_rgb565convertInterleave3DVideoToOutputDirectOptLE(deinterleaved_rgb565_pixels* out_ptr_top_l, deinterleaved_rgb565_pixels* out_ptr_top_r, deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_3d_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline, int multiplier_top) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_l_pos = 1;
	const int top_r_pos = 2;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_bottom = output_halfline_pos + i;
		size_t output_halfline_pixel_top = (output_halfline_pos * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel_bottom].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_l[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_l_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_r[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_r_pos];
	}
}

static inline void usb_rgb565convertInterleave3DVideoToOutputDirectOptLEMonoTop(deinterleaved_rgb565_pixels* out_ptr_top_l, deinterleaved_rgb565_pixels* out_ptr_top_r, interleaved_3d_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline, int multiplier_top) {
	//de-interleave pixels
	const int top_l_pos = 1;
	const int top_r_pos = 2;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_top = (output_halfline_pos * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_l[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_l_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_r[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_r_pos];
	}
}

static inline void usb_rgb565convertInterleave3DVideoToOutputDirectOptLEMonoBottom(deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_3d_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = output_halfline_pos + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptBE(deinterleaved_rgb565_pixels* out_ptr_top, deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][bottom_pos]);
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_pos]);
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(deinterleaved_rgb565_pixels* out_ptr_top, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_pos]);
	}
}

static inline void usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
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

static inline void usb_rgb565convertInterleave3DVideoToOutputDirectOptBE(deinterleaved_rgb565_pixels* out_ptr_top_l, deinterleaved_rgb565_pixels* out_ptr_top_r, deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_3d_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline, int multiplier_top) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_l_pos = 1;
	const int top_r_pos = 2;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_bottom = output_halfline_pos + i;
		size_t output_halfline_pixel_top = (output_halfline_pos * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel_bottom].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][bottom_pos]);
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_l[output_halfline_pixel_top].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_l_pos]);
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_r[output_halfline_pixel_top].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_r_pos]);
	}
}

static inline void usb_rgb565convertInterleave3DVideoToOutputDirectOptBEMonoTop(deinterleaved_rgb565_pixels* out_ptr_top_l, deinterleaved_rgb565_pixels* out_ptr_top_r, interleaved_3d_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline, int multiplier_top) {
	//de-interleave pixels
	const int top_l_pos = 1;
	const int top_r_pos = 2;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_top = (output_halfline_pos * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_l[output_halfline_pixel_top].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_l_pos]);
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_top_r[output_halfline_pixel_top].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_r_pos]);
	}
}

static inline void usb_rgb565convertInterleave3DVideoToOutputDirectOptBEMonoBottom(deinterleaved_rgb565_pixels* out_ptr_bottom, interleaved_3d_rgb565_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB565_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_bottom = output_halfline_pos + i;
		for(size_t j = 0; j < INTERLEAVED_RGB565_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel_bottom].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][bottom_pos]);
	}
}

static inline void usb_rgb888convertInterleaveU16VideoToOutputDirectOpt(deinterleaved_rgb888_u16_pixels* out_ptr_top, deinterleaved_rgb888_u16_pixels* out_ptr_bottom, interleaved_rgb888_u16_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][bottom_pos]);
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_pos]);
	}
}

static inline void usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoTop(deinterleaved_rgb888_u16_pixels* out_ptr_top, interleaved_rgb888_u16_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int top_pos = 1;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][top_pos]);
	}
}

static inline void usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoBottom(deinterleaved_rgb888_u16_pixels* out_ptr_bottom, interleaved_rgb888_u16_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel = (output_halfline * real_num_iters) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel].pixels[j] = _reverse_endianness(in_ptr[input_halfline_pixel].pixels[j][bottom_pos]);
	}
}

static inline void usb_rgb888convertInterleaveU163DVideoToOutputDirectOpt(deinterleaved_rgb888_u16_pixels* out_ptr_top_l, deinterleaved_rgb888_u16_pixels* out_ptr_top_r, deinterleaved_rgb888_u16_pixels* out_ptr_bottom, interleaved_3d_rgb888_u16_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline, int multiplier_top) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const int top_l_pos = 1;
	const int top_r_pos = 2;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_bottom = output_halfline_pos + i;
		size_t output_halfline_pixel_top = (output_halfline_pos * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel_bottom].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top_l[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_l_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top_r[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_r_pos];
	}
}

static inline void usb_rgb888convertInterleaveU163DVideoToOutputDirectOptMonoTop(deinterleaved_rgb888_u16_pixels* out_ptr_top_l, deinterleaved_rgb888_u16_pixels* out_ptr_top_r, interleaved_3d_rgb888_u16_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline, int multiplier_top) {
	//de-interleave pixels
	const int top_l_pos = 1;
	const int top_r_pos = 2;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_top = (output_halfline_pos * multiplier_top) + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top_l[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_l_pos];
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_top_r[output_halfline_pixel_top].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][top_r_pos];
	}
}

static inline void usb_rgb888convertInterleaveU163DVideoToOutputDirectOptMonoBottom(deinterleaved_rgb888_u16_pixels* out_ptr_bottom, interleaved_3d_rgb888_u16_pixels* in_ptr, uint32_t num_iters, int input_halfline, int output_halfline) {
	//de-interleave pixels
	const int bottom_pos = 0;
	const size_t real_num_iters = num_iters / INTERLEAVED_RGB888_PIXEL_NUM;
	const size_t output_halfline_pos = output_halfline * real_num_iters;
	for(size_t i = 0; i < real_num_iters; i++) {
		size_t input_halfline_pixel = (input_halfline * real_num_iters) + i;
		size_t output_halfline_pixel_bottom = output_halfline_pos + i;
		for(size_t j = 0; j < INTERLEAVED_RGB888_TOTAL_SIZE; j++)
			out_ptr_bottom[output_halfline_pixel_bottom].pixels[j] = in_ptr[input_halfline_pixel].pixels[j][bottom_pos];
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

static inline void usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, int input_halfline, int output_halfline) {
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

static inline void usb_oldDSconvertVideoToOutputHalfLineDirectOptBE(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, int input_halfline, int output_halfline) {
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
	column_info.has_extra_header_data = (base_data >> 15) & 1;
	return column_info.has_extra_header_data;
}

static inline void usb_3DS565OptimizeconvertVideoToOutputLineDirectOptLE(USB5653DSOptimizeCaptureReceived *p_in, VideoOutputData *p_out, uint16_t column) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	const size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->bottom_only_column;
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb565_pixels*)p_in->columns_data[column].pixel;
	else if(usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound))
		in_ptr = (interleaved_rgb565_pixels*)(((USB5653DSOptimizeCaptureReceivedExtraHeader*)p_in)->columns_data[column].pixel);
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 2);
	}
	else if(column < column_start_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptLEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
	else {
		out_ptr_top += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb565convertInterleaveVideoToOutputDirectOptLE(out_ptr_top, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos);
	}
}

static inline void usb_3DS565OptimizeconvertVideoToOutputLineDirectOptBE(USB5653DSOptimizeCaptureReceived *p_in, VideoOutputData *p_out, uint16_t column) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	const size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	interleaved_rgb565_pixels* in_ptr = (interleaved_rgb565_pixels*)p_in->bottom_only_column;
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb565_pixels*)p_in->columns_data[column].pixel;
	else if(usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound))
		in_ptr = (interleaved_rgb565_pixels*)(((USB5653DSOptimizeCaptureReceivedExtraHeader*)p_in)->columns_data[column].pixel);
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 2);
	}
	else if(column < column_start_bot_pos)
		usb_rgb565convertInterleaveVideoToOutputDirectOptBEMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
	else {
		out_ptr_top += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb565convertInterleaveVideoToOutputDirectOptBE(out_ptr_top, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos);
	}
}

static inline void usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptLE(USB5653DSOptimizeCaptureReceived_3D *p_in, VideoOutputData *p_out, uint16_t column, bool interleaved_3d) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	const size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top_l = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	deinterleaved_rgb565_pixels* out_ptr_top_r = out_ptr_bottom + (((TOP_SIZE_3DS + BOT_SIZE_3DS) * pixels_size) / ptr_out_size);
	interleaved_3d_rgb565_pixels* in_ptr = (interleaved_3d_rgb565_pixels*)p_in->bottom_only_column;
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_3d_rgb565_pixels*)p_in->columns_data[column].pixel;
	else if(usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound))
		in_ptr = (interleaved_3d_rgb565_pixels*)(((USB5653DSOptimizeCaptureReceived_3DExtraHeader*)p_in)->columns_data[column].pixel);
	int multiplier_top = 1;
	if(interleaved_3d) {
		multiplier_top = 2;
		out_ptr_top_r = out_ptr_top_l + ((HEIGHT_3DS * pixels_size) / ptr_out_size);
	}
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleave3DVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb565convertInterleave3DVideoToOutputDirectOptLEMonoTop(out_ptr_top_l, out_ptr_top_r, in_ptr, num_iters, 0, column, multiplier_top);
		usb_rgb565convertInterleave3DVideoToOutputDirectOptLEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 2);
	}
	else if(column < column_start_bot_pos)
		usb_rgb565convertInterleave3DVideoToOutputDirectOptLEMonoTop(out_ptr_top_l, out_ptr_top_r, in_ptr, num_iters, 0, column, multiplier_top);
	else {
		out_ptr_top_l += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		out_ptr_top_r += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb565convertInterleave3DVideoToOutputDirectOptLE(out_ptr_top_l, out_ptr_top_r, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos, multiplier_top);
	}
}

static inline void usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptBE(USB5653DSOptimizeCaptureReceived_3D *p_in, VideoOutputData *p_out, uint16_t column, bool interleaved_3d) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB16);
	const size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	const size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb565_pixels);
	deinterleaved_rgb565_pixels* out_ptr_bottom = (deinterleaved_rgb565_pixels*)p_out->rgb16_video_output_data.screen_data;
	deinterleaved_rgb565_pixels* out_ptr_top_l = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	deinterleaved_rgb565_pixels* out_ptr_top_r = out_ptr_bottom + (((TOP_SIZE_3DS + BOT_SIZE_3DS) * pixels_size) / ptr_out_size);
	interleaved_3d_rgb565_pixels* in_ptr = (interleaved_3d_rgb565_pixels*)p_in->bottom_only_column;
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_3d_rgb565_pixels*)p_in->columns_data[column].pixel;
	else if(usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound))
		in_ptr = (interleaved_3d_rgb565_pixels*)(((USB5653DSOptimizeCaptureReceived_3DExtraHeader*)p_in)->columns_data[column].pixel);
	int multiplier_top = 1;
	if(interleaved_3d) {
		multiplier_top = 2;
		out_ptr_top_r = out_ptr_top_l + ((HEIGHT_3DS * pixels_size) / ptr_out_size);
	}
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb565convertInterleave3DVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb565convertInterleave3DVideoToOutputDirectOptBEMonoTop(out_ptr_top_l, out_ptr_top_r, in_ptr, num_iters, 0, column, multiplier_top);
		usb_rgb565convertInterleave3DVideoToOutputDirectOptBEMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 2);
	}
	else if(column < column_start_bot_pos)
		usb_rgb565convertInterleave3DVideoToOutputDirectOptBEMonoTop(out_ptr_top_l, out_ptr_top_r, in_ptr, num_iters, 0, column, multiplier_top);
	else {
		out_ptr_top_l += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		out_ptr_top_r += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb565convertInterleave3DVideoToOutputDirectOptBE(out_ptr_top_l, out_ptr_top_r, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos, multiplier_top);
	}
}

static inline void usb_3DS888OptimizeconvertVideoToOutputLineDirectOpt(USB8883DSOptimizeCaptureReceived *p_in, VideoOutputData *p_out, uint16_t column) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB);
	const size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	const size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb888_u16_pixels);
	deinterleaved_rgb888_u16_pixels* out_ptr_bottom = (deinterleaved_rgb888_u16_pixels*)p_out->rgb_video_output_data.screen_data;
	deinterleaved_rgb888_u16_pixels* out_ptr_top = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	interleaved_rgb888_u16_pixels* in_ptr = (interleaved_rgb888_u16_pixels*)p_in->bottom_only_column;
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_rgb888_u16_pixels*)p_in->columns_data[column].pixel;
	else if(usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound))
		in_ptr = (interleaved_rgb888_u16_pixels*)(((USB8883DSOptimizeCaptureReceivedExtraHeader*)p_in)->columns_data[column].pixel);
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 2);
	}
	else if(column < column_start_bot_pos)
		usb_rgb888convertInterleaveU16VideoToOutputDirectOptMonoTop(out_ptr_top, in_ptr, num_iters, 0, column);
	else {
		out_ptr_top += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb888convertInterleaveU16VideoToOutputDirectOpt(out_ptr_top, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos);
	}
}

static inline void usb_3DS888Optimizeconvert3DVideoToOutputLineDirectOpt(USB8883DSOptimizeCaptureReceived_3D *p_in, VideoOutputData *p_out, uint16_t column, bool interleaved_3d) {
	//de-interleave pixels
	const int pixels_size = sizeof(VideoPixelRGB);
	const size_t column_start_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2) + 2;
	const size_t column_pre_last_bot_pos = (SCREEN_WIDTH_FIRST_PIXEL_BOTTOM_3DS * 2);
	const size_t column_last_bot_pos = TOP_WIDTH_3DS;
	const size_t ptr_out_size = sizeof(deinterleaved_rgb888_u16_pixels);
	deinterleaved_rgb888_u16_pixels* out_ptr_bottom = (deinterleaved_rgb888_u16_pixels*)p_out->rgb_video_output_data.screen_data;
	deinterleaved_rgb888_u16_pixels* out_ptr_top_l = out_ptr_bottom + ((BOT_SIZE_3DS * pixels_size) / ptr_out_size);
	deinterleaved_rgb888_u16_pixels* out_ptr_top_r = out_ptr_bottom + (((TOP_SIZE_3DS + BOT_SIZE_3DS) * pixels_size) / ptr_out_size);
	interleaved_3d_rgb888_u16_pixels* in_ptr = (interleaved_3d_rgb888_u16_pixels*)p_in->bottom_only_column;
	if(column < column_last_bot_pos)
		in_ptr = (interleaved_3d_rgb888_u16_pixels*)p_in->columns_data[column].pixel;
	else if(usb_OptimizeHasExtraHeaderSoundData(&p_in->columns_data[0].header_sound))
		in_ptr = (interleaved_3d_rgb888_u16_pixels*)(((USB8883DSOptimizeCaptureReceived_3DExtraHeader*)p_in)->columns_data[column].pixel);
	int multiplier_top = 1;
	if(interleaved_3d) {
		multiplier_top = 2;
		out_ptr_top_r = out_ptr_top_l + (HEIGHT_3DS / ptr_out_size);
	}
	const uint32_t num_iters = HEIGHT_3DS;
	if(column == column_last_bot_pos)
		usb_rgb888convertInterleaveU163DVideoToOutputDirectOptMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 1);
	else if(column == column_pre_last_bot_pos) {
		usb_rgb888convertInterleaveU163DVideoToOutputDirectOptMonoTop(out_ptr_top_l, out_ptr_top_r, in_ptr, num_iters, 0, column, multiplier_top);
		usb_rgb888convertInterleaveU163DVideoToOutputDirectOptMonoBottom(out_ptr_bottom, in_ptr, num_iters, 0, BOT_WIDTH_3DS - 2);
	}
	else if(column < column_start_bot_pos)
		usb_rgb888convertInterleaveU163DVideoToOutputDirectOptMonoTop(out_ptr_top_l, out_ptr_top_r, in_ptr, num_iters, 0, column, multiplier_top);
	else {
		out_ptr_top_l += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		out_ptr_top_r += (column_start_bot_pos * HEIGHT_3DS * pixels_size) / ptr_out_size;
		usb_rgb888convertInterleaveU163DVideoToOutputDirectOpt(out_ptr_top_l, out_ptr_top_r, out_ptr_bottom, in_ptr, num_iters, 0, column - column_start_bot_pos, multiplier_top);
	}
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
		int input_halfline = 0;
		for(int i = 0; i < 2; i++) {
			if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
				usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(p_in, p_out, input_halfline++, i);
		}

		for(int i = 2; i < HEIGHT_DS * 2; i++) {
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
		int input_halfline = 0;
		for(int i = 0; i < 2; i++) {
			if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
				usb_oldDSconvertVideoToOutputHalfLineDirectOptBE(p_in, p_out, input_halfline++, i);
		}

		for(int i = 2; i < HEIGHT_DS * 2; i++) {
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
		for(int i = 0; i < HEIGHT_DS * 2; i++)
			usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(p_in, p_out, i, i);
	else
		for(int i = 0; i < HEIGHT_DS * 2; i++)
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

static void usb_3ds_optimize_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d, const bool is_big_endian, bool interleaved_3d, bool requested_3d, bool is_rgb888) {
	if(!is_rgb888) {
		if(!enabled_3d) {
			if(!is_big_endian)
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565OptimizeconvertVideoToOutputLineDirectOptLE(&p_in->cypress_optimize_received_565, p_out, i);
			else
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565OptimizeconvertVideoToOutputLineDirectOptBE(&p_in->cypress_optimize_received_565, p_out, i);
			expand_2d_to_3d_convertVideoToOutput((uint8_t*)p_out->rgb16_video_output_data.screen_data, sizeof(VideoPixelRGB16), interleaved_3d, requested_3d);
		}
		else {
			if(!is_big_endian)
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptLE(&p_in->cypress_optimize_received_565_3d, p_out, i, interleaved_3d);
			else
				for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
					usb_3DS565Optimizeconvert3DVideoToOutputLineDirectOptBE(&p_in->cypress_optimize_received_565_3d, p_out, i, interleaved_3d);
		}
	}
	else {
		if(!enabled_3d) {
			for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
				usb_3DS888OptimizeconvertVideoToOutputLineDirectOpt(&p_in->cypress_optimize_received_888, p_out, i);
			expand_2d_to_3d_convertVideoToOutput((uint8_t*)p_out->rgb_video_output_data.screen_data, sizeof(VideoPixelRGB), interleaved_3d, requested_3d);
		}
		else {
			for(int i = 0; i < (TOP_WIDTH_3DS + 1); i++)
				usb_3DS888Optimizeconvert3DVideoToOutputLineDirectOpt(&p_in->cypress_optimize_received_888_3d, p_out, i, interleaved_3d);
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

bool convertVideoToOutput(VideoOutputData *p_out, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status, bool interleaved_3d) {
	CaptureReceived* p_in = (CaptureReceived*)(((uint8_t*)&data_buffer->capture_buf) + data_buffer->unused_offset);
	bool converted = false;
	CaptureDevice* chosen_device = &status->device;
	bool is_data_3d = data_buffer->is_3d;
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
		usb_3ds_optimize_convertVideoToOutput(p_in, p_out, is_data_3d, is_big_endian, interleaved_3d, is_3d_requested, is_rgb888);
		converted = true;
	}
	#endif
	return converted;
}

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DS(CaptureReceived* buffer, bool is_rgb888, bool is_data_3d, uint16_t column) {
	if(!is_rgb888) {
		if(!is_data_3d)
			return &buffer->cypress_optimize_received_565.columns_data[column].header_sound;
		return &buffer->cypress_optimize_received_565_3d.columns_data[column].header_sound;
	}
	if(!is_data_3d)
		return &buffer->cypress_optimize_received_888.columns_data[column].header_sound;
	return &buffer->cypress_optimize_received_888_3d.columns_data[column].header_sound;
}

static USB3DSOptimizeHeaderSoundData* getAudioHeaderPtrOptimize3DSExtraHeader(CaptureReceived* buffer, bool is_rgb888, bool is_data_3d, uint16_t column) {
	if(!is_rgb888) {
		if(!is_data_3d)
			return &buffer->cypress_optimize_received_565_extra_header.columns_data[column].header_sound;
		return &buffer->cypress_optimize_received_565_3d_extra_header.columns_data[column].header_sound;
	}
	if(!is_data_3d)
		return &buffer->cypress_optimize_received_888_extra_header.columns_data[column].header_sound;
	return &buffer->cypress_optimize_received_888_3d_extra_header.columns_data[column].header_sound;
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

static void copyAudioOptimize3DSLE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer, bool is_rgb888, bool is_data_3d) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < TOP_WIDTH_3DS; i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, is_data_3d, i);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	USB3DSOptimizeHeaderSoundData* initial_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, is_data_3d, 0);
	if(usb_OptimizeHasExtraHeaderSoundData(initial_column_data)) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DSExtraHeader(buffer, is_rgb888, is_data_3d, TOP_WIDTH_3DS);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSLE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

static void copyAudioOptimize3DSBE(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, CaptureReceived* buffer, bool is_rgb888, bool is_data_3d) {
	uint64_t num_inserted = 0;
	int last_inserted_index = last_buffer_index;
	for(int i = 0; i < TOP_WIDTH_3DS; i++) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, is_data_3d, i);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	USB3DSOptimizeHeaderSoundData* initial_column_data = getAudioHeaderPtrOptimize3DS(buffer, is_rgb888, is_data_3d, 0);
	if(usb_OptimizeHasExtraHeaderSoundData(initial_column_data)) {
		USB3DSOptimizeHeaderSoundData* curr_column_data = getAudioHeaderPtrOptimize3DSExtraHeader(buffer, is_rgb888, is_data_3d, TOP_WIDTH_3DS);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[0], num_inserted, last_inserted_index);
		copyAudioFromSoundDataOptimize3DSBE(p_out, &curr_column_data->samples[1], num_inserted, last_inserted_index);
	}
	last_buffer_index = last_inserted_index;
	n_samples = num_inserted * 2;
}

bool convertAudioToOutput(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status) {
	if(!status->device.has_audio)
		return true;
	bool is_data_3d = data_buffer->is_3d;
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
		if(is_big_endian)
			copyAudioOptimize3DSBE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf, is_rgb888, is_data_3d);
		else
			copyAudioOptimize3DSLE(p_out, n_samples, last_buffer_index, &data_buffer->capture_buf, is_rgb888, is_data_3d);
		return true;
	}
	#endif
	if(base_ptr == NULL)
		return false;
	if(!is_big_endian)
		memcpy(p_out, base_ptr, (size_t)(n_samples * 2));
	else
		for(uint64_t i = 0; i < n_samples; i++)
			p_out[i] = (base_ptr[(i * 2) + 1] << 8) | base_ptr[i * 2];
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

#include "conversions.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3_shared.hpp"
#include "dscapture_ftd2_shared.hpp"
#include "usb_ds_3ds_capture.hpp"
#include "usb_is_device_acquisition.hpp"

#include <cstring>

struct interleaved_ds_pixels {
	uint16_t bottom_first;
	uint16_t top_first;
	uint16_t bottom_second;
	uint16_t top_second;
	uint16_t bottom_third;
	uint16_t top_third;
	uint16_t bottom_fourth;
	uint16_t top_fourth;
};

struct deinterleaved_ds_pixels_le {
	uint64_t first : 16;
	uint64_t second : 16;
	uint64_t third : 16;
	uint64_t fourth : 16;
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

static inline void convertVideoToOutputChunk(RGB83DSVideoInputData *p_in, VideoOutputData *p_out, int iters, int start_in, int start_out) {
	memcpy(&p_out->screen_data[start_out], &p_in->screen_data[start_in], iters * 3);
}

static inline void convertVideoToOutputChunk_3D(RGB83DSVideoInputData_3D *p_in, VideoOutputData *p_out, int iters, int start_in, int start_out) {
	memcpy(&p_out->screen_data[start_out], &p_in->screen_data[start_in], iters * 3);
}

static void ftd3_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d) {
	if(!enabled_3d) {
		convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_NO_BOTTOM_SIZE_3DS, 0, 0);

		for(int i = 0; i < ((IN_VIDEO_SIZE_3DS - IN_VIDEO_NO_BOTTOM_SIZE_3DS) / (IN_VIDEO_WIDTH_3DS * 2)); i++) {
			convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_WIDTH_3DS, (((i * 2) + 0) * IN_VIDEO_WIDTH_3DS) + IN_VIDEO_NO_BOTTOM_SIZE_3DS, TOP_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS));
			convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_WIDTH_3DS, (((i * 2) + 1) * IN_VIDEO_WIDTH_3DS) + IN_VIDEO_NO_BOTTOM_SIZE_3DS, IN_VIDEO_NO_BOTTOM_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS));
		}
	}
	else {
		for(int i = 0; i < (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / (IN_VIDEO_WIDTH_3DS_3D * 2)); i++) {
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, ((i * 2) + 0) * IN_VIDEO_WIDTH_3DS_3D, i * IN_VIDEO_WIDTH_3DS_3D);
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, ((i * 2) + 1) * IN_VIDEO_WIDTH_3DS_3D, TOP_SIZE_3DS + BOT_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS_3D));
		}

		for(int i = 0; i < ((IN_VIDEO_SIZE_3DS_3D - IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D) / (IN_VIDEO_WIDTH_3DS_3D * 3)) - 1; i++) {
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 0) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 1) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, TOP_SIZE_3DS + BOT_SIZE_3DS + (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 2) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, TOP_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS_3D));
		}
		// For some weird reason, the last one is the opposite for bottom and
		// second top screen
		int i = ((IN_VIDEO_SIZE_3DS_3D - IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D) / (IN_VIDEO_WIDTH_3DS_3D * 3)) - 1;
		convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 0) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
		convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 2) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, TOP_SIZE_3DS + BOT_SIZE_3DS + (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
		convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 1) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, TOP_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS_3D));
	}
}

static inline void usb_oldDSconvertVideoToOutputHalfLineDirectOptLE(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, int input_halfline, int output_halfline) {
	//de-interleave pixels
	deinterleaved_ds_pixels_le* out_ptr_top = (deinterleaved_ds_pixels_le*)p_out->screen_data;
	deinterleaved_ds_pixels_le* out_ptr_bottom = out_ptr_top + (WIDTH_DS * HEIGHT_DS / (sizeof(deinterleaved_ds_pixels_le) / 2));
	interleaved_ds_pixels* in_ptr = (interleaved_ds_pixels*)p_in->video_in.screen_data;
	const uint32_t halfline_iters = WIDTH_DS / (sizeof(interleaved_ds_pixels) / 2);
	for(int i = 0; i < halfline_iters; i++) {
		uint32_t input_halfline_pixel = (input_halfline * halfline_iters) + i;
		uint32_t output_halfline_pixel = (output_halfline * halfline_iters) + i;
		out_ptr_bottom[output_halfline_pixel].first = in_ptr[input_halfline_pixel].bottom_first;
		out_ptr_bottom[output_halfline_pixel].second = in_ptr[input_halfline_pixel].bottom_second;
		out_ptr_bottom[output_halfline_pixel].third = in_ptr[input_halfline_pixel].bottom_third;
		out_ptr_bottom[output_halfline_pixel].fourth = in_ptr[input_halfline_pixel].bottom_fourth;
		out_ptr_top[output_halfline_pixel].first = in_ptr[input_halfline_pixel].top_first;
		out_ptr_top[output_halfline_pixel].second = in_ptr[input_halfline_pixel].top_second;
		out_ptr_top[output_halfline_pixel].third = in_ptr[input_halfline_pixel].top_third;
		out_ptr_top[output_halfline_pixel].fourth = in_ptr[input_halfline_pixel].top_fourth;
	}
}

static inline void usb_oldDSconvertVideoToOutputHalfLineDirectOptBE(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, int input_halfline, int output_halfline) {
	//de-interleave pixels
	uint16_t* out_ptr_top = (uint16_t*)p_out->screen_data;
	uint16_t* out_ptr_bottom = out_ptr_top + (WIDTH_DS * HEIGHT_DS / (sizeof(uint16_t) / 2));
	uint32_t* in_ptr = (uint32_t*)p_in->video_in.screen_data;
	const uint32_t halfline_iters = WIDTH_DS / (sizeof(uint32_t) / 2);
	for(int i = 0; i < halfline_iters; i++) {
		uint32_t input_halfline_pixel = (input_halfline * halfline_iters) + i;
		uint32_t output_halfline_pixel = (output_halfline * halfline_iters) + i;
		uint16_t bottom_pixel_rev = in_ptr[input_halfline_pixel] >> 16;
		uint16_t top_pixel_rev = in_ptr[input_halfline_pixel] & 0xFFFF;
		out_ptr_bottom[output_halfline_pixel] = (bottom_pixel_rev >> 8) | ((bottom_pixel_rev << 8) & 0xFF00);
		out_ptr_top[output_halfline_pixel] = (top_pixel_rev >> 8) | ((top_pixel_rev << 8) & 0xFF00);
	}
}

static void usb_oldDSconvertVideoToOutput(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, const bool is_big_endian) {
	#ifndef SIMPLE_DS_FRAME_SKIP
	if(!p_in->frameinfo.valid) { //LCD was off
		memset(p_out->screen_data, 0, WIDTH_DS * (2 * HEIGHT_DS) * sizeof(uint16_t));
		return;
	}

	// Handle first line being off, if needed
	memset(p_out->screen_data, 0, WIDTH_DS * sizeof(uint16_t));

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
				uint16_t* out_ptr_top = (uint16_t*)&p_out->screen_data;
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
				uint16_t* out_ptr_top = (uint16_t*)&p_out->screen_data;
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
	memcpy(p_out->screen_data, p_in->video_in.screen_data, IN_VIDEO_HEIGHT_3DS * IN_VIDEO_WIDTH_3DS * 3);
}

static void usb_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureDevice* capture_device, bool enabled_3d, const bool is_big_endian) {
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

inline static void to_8_bit_6(uint8_t* out, uint8_t* in) {
	out[0] = to_8_bit_6(in[0]);
	out[1] = to_8_bit_6(in[1]);
	out[2] = to_8_bit_6(in[2]);
}

static void usb_cypress_nisetro_ds_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out) {
	int pos_top = 0;
	int pos_bottom = WIDTH_DS * HEIGHT_DS * 3;
	uint8_t* data_in = (uint8_t*)p_in->cypress_nisetro_capture_received.video_in.screen_data;
	uint8_t* data_out = (uint8_t*)p_out->screen_data;
	for(int i = 0; i < sizeof(CypressNisetroDSCaptureReceived); i++) {
		uint8_t conv = to_8_bit_6(data_in[i]);
		if(data_in[i] & 0x40)
			data_out[pos_bottom++] = conv;
		else
			data_out[pos_top++] = conv;
	}
}

static void usb_is_device_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureStatus* status, CaptureScreensType capture_type) {
	bool is_nitro = true;
	#ifdef USE_IS_DEVICES_USB
	is_nitro = is_device_is_nitro(&status->device);
	#endif
	if(is_nitro) {
		int num_pixels = usb_is_device_get_video_in_size(status) / 3;
		int out_start_pos = 0;
		int out_clear_pos = num_pixels;
		if(capture_type == CAPTURE_SCREENS_BOTTOM) {
			out_start_pos = num_pixels;
			out_clear_pos = 0;
		}
		if((capture_type == CAPTURE_SCREENS_BOTTOM) || (capture_type == CAPTURE_SCREENS_TOP))
			memset(p_out->screen_data[out_clear_pos], 0, num_pixels * 3);
		memcpy(p_out->screen_data[out_start_pos], p_in->is_nitro_capture_received.video_in.screen_data, num_pixels * 3);
		return;
	}
	ISTWLCaptureVideoInputData* data = &p_in->is_twl_capture_received.video_capture_in.video_in;
	twl_16bit_pixels* data_16_bit = (twl_16bit_pixels*)data->screen_data;
	twl_2bit_pixels* data_2_bit = (twl_2bit_pixels*)data->bit_6_rb_screen_data;
	int num_pixels = IN_VIDEO_SIZE_DS;
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
					to_8_bit_6(p_out->screen_data[out_pos + (u * num_pixels_struct) + k], pixels[k]);
			}
		}
	}
}

bool convertVideoToOutput(VideoOutputData *p_out, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status) {
	CaptureReceived *p_in = &data_buffer->capture_buf;
	bool converted = false;
	CaptureDevice* chosen_device = &status->device;
	#ifdef USE_FTD3
	if(chosen_device->cc_type == CAPTURE_CONN_FTD3) {
		ftd3_convertVideoToOutput(p_in, p_out, status->enabled_3d);
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
		usb_convertVideoToOutput(p_in, p_out, chosen_device, status->enabled_3d, is_big_endian);
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
	return converted;
}

bool convertAudioToOutput(std::int16_t *p_out, uint64_t &n_samples, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status) {
	if(!status->device.has_audio)
		return true;
	CaptureReceived *p_in = &data_buffer->capture_buf;
	uint8_t* base_ptr = NULL;
	#ifdef USE_FTD3
	if(status->device.cc_type == CAPTURE_CONN_FTD3) {
		if(!status->enabled_3d)
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
		if(!status->enabled_3d)
			base_ptr = (uint8_t*)p_in->usb_received_3ds.audio_data;
		else
			base_ptr = (uint8_t*)p_in->usb_received_3ds_3d.audio_data;
	}
	#endif
	#ifdef USE_IS_DEVICES_USB
	if(status->device.cc_type == CAPTURE_CONN_IS_NITRO) {
		base_ptr = (uint8_t*)p_in->is_twl_capture_received.audio_capture_in;
		uint16_t* base_ptr16 = (uint16_t*)base_ptr;
		size_t size_real = n_samples * 2;
		size_t size_packet_converted = sizeof(ISTWLCaptureSoundData);
		size_t num_packets = size_real / size_packet_converted;
		for(int i = 0; i < num_packets; i++)
			memcpy(base_ptr + (i * size_packet_converted), ((uint8_t*)&p_in->is_twl_capture_received.audio_capture_in[i].sound_data.data), size_packet_converted);
		// Inverted L and R...
		for(int i = 0; i < n_samples; i++) {
			uint16_t r_sample = base_ptr16[(i * 2)];
			base_ptr16[(i * 2)] = base_ptr16[(i * 2) + 1];
			base_ptr16[(i * 2) + 1] = r_sample;
		}
			
	}
	#endif
	if(base_ptr == NULL)
		return false;
	if(!is_big_endian)
		memcpy(p_out, base_ptr, n_samples * 2);
	else
		for(int i = 0; i < n_samples; i++)
			p_out[i] = (base_ptr[(i * 2) + 1] << 8) | base_ptr[i * 2];
	return true;
}

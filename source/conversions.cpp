#include "conversions.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3.hpp"
#include "dscapture_ftd2.hpp"
#include "usb_ds_3ds_capture.hpp"
#include "usb_is_nitro_acquisition.hpp"

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

struct deinterleaved_ds_pixels {
	uint32_t first : 16;
	uint32_t second : 16;
	uint32_t third : 16;
	uint32_t fourth : 16;
};

#ifdef USE_FTD3
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
			convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_WIDTH_3DS, ((i * 2) * IN_VIDEO_WIDTH_3DS) + IN_VIDEO_NO_BOTTOM_SIZE_3DS, TOP_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS));
			convertVideoToOutputChunk(&p_in->ftd3_received.video_in, p_out, IN_VIDEO_WIDTH_3DS, (((i * 2) + 1) * IN_VIDEO_WIDTH_3DS) + IN_VIDEO_NO_BOTTOM_SIZE_3DS, IN_VIDEO_NO_BOTTOM_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS));
		}
	}
	else {
		for(int i = 0; i < (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / (IN_VIDEO_WIDTH_3DS_3D * 2)); i++) {
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (i * 2) * IN_VIDEO_WIDTH_3DS_3D, i * IN_VIDEO_WIDTH_3DS_3D);
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, ((i * 2) + 1) * IN_VIDEO_WIDTH_3DS_3D, TOP_SIZE_3DS + BOT_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS_3D));
		}

		for(int i = 0; i < ((IN_VIDEO_SIZE_3DS_3D - IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D) / (IN_VIDEO_WIDTH_3DS_3D * 3)); i++) {
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, ((i * 3) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, TOP_SIZE_3DS + (i * IN_VIDEO_WIDTH_3DS_3D));
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 1) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
			convertVideoToOutputChunk_3D(&p_in->ftd3_received_3d.video_in, p_out, IN_VIDEO_WIDTH_3DS_3D, (((i * 3) + 1) * IN_VIDEO_WIDTH_3DS_3D) + IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D, TOP_SIZE_3DS + BOT_SIZE_3DS + (IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D / 2) + (i * IN_VIDEO_WIDTH_3DS_3D));
		}
	}
}
#endif

#ifdef USE_FTD2
static void ftd2_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out) {
}
#endif

#ifdef USE_DS_3DS_USB
static inline void usb_oldDSconvertVideoToOutputHalfLineDirectOpt(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out, int input_halfline, int output_halfline) {
	//de-interleave pixels
	deinterleaved_ds_pixels* out_ptr_top = (deinterleaved_ds_pixels*)p_out->screen_data;
	deinterleaved_ds_pixels* out_ptr_bottom = out_ptr_top + (WIDTH_DS * HEIGHT_DS / (sizeof(deinterleaved_ds_pixels) / 2));
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

static void usb_oldDSconvertVideoToOutput(USBOldDSCaptureReceived *p_in, VideoOutputData *p_out) {
	#ifndef SIMPLE_DS_FRAME_SKIP
	if(!p_in->frameinfo.valid) { //LCD was off
		memset(p_out->screen_data, 0, WIDTH_DS * (2 * HEIGHT_DS) * sizeof(uint16_t));
		return;
	}

	// Handle first line being off, if needed
	memset(p_out->screen_data, 0, WIDTH_DS * sizeof(uint16_t));

	int input_halfline = 0;
	for(int i = 0; i < 2; i++) {
		if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
			usb_oldDSconvertVideoToOutputHalfLineDirectOpt(p_in, p_out, input_halfline++, i);
	}

	for(int i = 2; i < HEIGHT_DS * 2; i++) {
		if(p_in->frameinfo.half_line_flags[(i >> 3)] & (1 << (i & 7)))
			usb_oldDSconvertVideoToOutputHalfLineDirectOpt(p_in, p_out, input_halfline++, i);
		else { // deal with missing half-line
			uint16_t* out_ptr_top = (uint16_t*)&p_out->screen_data;
			uint16_t* out_ptr_bottom = out_ptr_top + (WIDTH_DS * HEIGHT_DS);
			memcpy(&out_ptr_top[i * (WIDTH_DS / 2)], &out_ptr_top[(i - 2) * (WIDTH_DS / 2)], (WIDTH_DS / 2) * sizeof(uint16_t));
			memcpy(&out_ptr_bottom[i * (WIDTH_DS / 2)], &out_ptr_bottom[(i - 2) * (WIDTH_DS / 2)], (WIDTH_DS / 2) * sizeof(uint16_t));
		}
	}
	#else
	for(int i = 0; i < HEIGHT_DS * 2; i++)
		usb_oldDSconvertVideoToOutputHalfLineDirectOpt(p_in, p_out, i, i);
	#endif
}

static void usb_3DSconvertVideoToOutput(USB3DSCaptureReceived *p_in, VideoOutputData *p_out) {
	memcpy(p_out->screen_data, p_in->video_in.screen_data, IN_VIDEO_HEIGHT_3DS * IN_VIDEO_WIDTH_3DS * 3);
}

static void usb_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureDevice* capture_device, bool enabled_3d) {
	if(capture_device->is_3ds) {
		if(!enabled_3d)
			usb_3DSconvertVideoToOutput(&p_in->usb_received_3ds, p_out);
	}
	else
		usb_oldDSconvertVideoToOutput(&p_in->usb_received_old_ds, p_out);
}
#endif

#ifdef USE_IS_NITRO_USB
static void usb_is_nitro_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureScreensType capture_type) {
	int num_pixels = usb_is_nitro_get_video_in_size(capture_type) / 3;
	int out_start_pos = 0;
	int out_clear_pos = num_pixels;
	if(capture_type == CAPTURE_SCREENS_BOTTOM) {
		out_start_pos = num_pixels;
		out_clear_pos = 0;
	}
	if((capture_type == CAPTURE_SCREENS_BOTTOM) || (capture_type == CAPTURE_SCREENS_TOP))
		memset(p_out->screen_data[out_clear_pos], 0, num_pixels * 3);
	memcpy(p_out->screen_data[out_start_pos], p_in->is_nitro_capture_received.video_in.screen_data, num_pixels * 3);
}
#endif

bool convertVideoToOutput(VideoOutputData *p_out, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status) {
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
		ftd2_convertVideoToOutput(p_in, p_out);
		converted = true;
	}
	#endif
	#ifdef USE_DS_3DS_USB
	if(chosen_device->cc_type == CAPTURE_CONN_USB) {
		usb_convertVideoToOutput(p_in, p_out, chosen_device, status->enabled_3d);
		converted = true;
	}
	#endif
	#ifdef USE_IS_NITRO_USB
	if(chosen_device->cc_type == CAPTURE_CONN_IS_NITRO) {
		usb_is_nitro_convertVideoToOutput(p_in, p_out, data_buffer->capture_type);
		converted = true;
	}
	#endif
	return converted;
}

bool convertAudioToOutput(std::int16_t *p_out, uint64_t n_samples, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status) {
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
	if(status->device.cc_type == CAPTURE_CONN_FTD2) {
	}
	#endif
	#ifdef USE_DS_3DS_USB
	if(status->device.cc_type == CAPTURE_CONN_USB) {
		if(!status->enabled_3d)
			base_ptr = (uint8_t*)p_in->usb_received_3ds.audio_data;
		else
			base_ptr = (uint8_t*)p_in->usb_received_3ds_3d.audio_data;
	}
	#endif
	if(base_ptr == NULL)
		return false;
	if(!is_big_endian)
		memcpy(p_out, base_ptr, n_samples * 2);
	else
		for(int i = 0; i < n_samples; i += 2)
			p_out[i] = (base_ptr[i + 1] << 8) | base_ptr[i];
	return true;
}

#include "conversions.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3.hpp"
#include "dscapture_ftd2.hpp"
#include "usb_ds_3ds_capture.hpp"
#include "usb_is_nitro_acquisition.hpp"

#include <cstring>

bool convertVideoToOutput(VideoOutputData *p_out, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status) {
	CaptureReceived *p_in = &data_buffer->capture_buf;
	bool converted = false;
	#ifdef USE_FTD3
	if(status->device.cc_type == CAPTURE_CONN_FTD3) {
		ftd3_convertVideoToOutput(p_in, p_out, status->enabled_3d);
		converted = true;
	}
	#endif
	#ifdef USE_FTD2
	if(status->device.cc_type == CAPTURE_CONN_FTD2) {
		ftd2_convertVideoToOutput(p_in, p_out, status->enabled_3d);
		converted = true;
	}
	#endif
	#ifdef USE_DS_3DS_USB
	if(status->device.cc_type == CAPTURE_CONN_USB) {
		usb_convertVideoToOutput(p_in, p_out, &status->device, status->enabled_3d);
		converted = true;
	}
	#endif
	#ifdef USE_IS_NITRO_USB
	if(status->device.cc_type == CAPTURE_CONN_IS_NITRO) {
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

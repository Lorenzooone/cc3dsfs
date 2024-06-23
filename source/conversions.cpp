#include "conversions.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3.hpp"
#include "dscapture_ftd2.hpp"
#include "usb_ds_3ds_capture.hpp"

#include <cstring>

void convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureData* capture_data) {
	#ifdef USE_FTD3
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3)
		ftd3_convertVideoToOutput(p_in, p_out, capture_data->status.enabled_3d);
	#endif
	#ifdef USE_FTD2
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2)
		ftd2_convertVideoToOutput(p_in, p_out, capture_data->status.enabled_3d);
	#endif
	#ifdef USE_USB
	if(capture_data->status.device.cc_type == CAPTURE_CONN_USB)
		usb_convertVideoToOutput(p_in, p_out, &capture_data->status.device, capture_data->status.enabled_3d);
	#endif
}

void convertAudioToOutput(CaptureReceived *p_in, sf::Int16 *p_out, uint64_t n_samples, const bool is_big_endian, CaptureData* capture_data) {
	if(!capture_data->status.device.has_audio)
		return;
	uint8_t* base_ptr = NULL;
	#ifdef USE_FTD3
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3) {
		if(!capture_data->status.enabled_3d)
			base_ptr = (uint8_t*)p_in->ftd3_received.audio_data;
		else
			base_ptr = (uint8_t*)p_in->ftd3_received_3d.audio_data;
	}
	#endif
	#ifdef USE_FTD2
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2) {
	}
	#endif
	#ifdef USE_USB
	if(capture_data->status.device.cc_type == CAPTURE_CONN_USB) {
		if(!capture_data->status.enabled_3d)
			base_ptr = (uint8_t*)p_in->usb_received_3ds.audio_data;
		else
			base_ptr = (uint8_t*)p_in->usb_received_3ds_3d.audio_data;
	}
	#endif
	if(base_ptr == NULL)
		return;
	if(!is_big_endian)
		memcpy(p_out, base_ptr, n_samples * 2);
	else
		for(int i = 0; i < n_samples; i += 2)
			p_out[i] = (base_ptr[i + 1] << 8) | base_ptr[i];
}

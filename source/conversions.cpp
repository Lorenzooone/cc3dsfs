#include <SFML/Audio.hpp>
#include "conversions.hpp"

#include <cstring>

static inline void convertVideoToOutputChunk(VideoInputData *p_in, VideoOutputData *p_out, int iters, int start_in, int start_out) {
	for(int i = 0; i < iters; i++)  {
		for(int u = 0; u < 3; u++)
			p_out->screen_data[start_out + i][u] = p_in->screen_data[start_in + i][u];
		p_out->screen_data[start_out + i][3] = 0xff;
	}
}

void convertVideoToOutput(VideoInputData *p_in, VideoOutputData *p_out) {
	convertVideoToOutputChunk(p_in, p_out, IN_VIDEO_NO_BOTTOM_SIZE, 0, 0);

	for(int i = 0; i < ((IN_VIDEO_SIZE - IN_VIDEO_NO_BOTTOM_SIZE) / (IN_VIDEO_WIDTH * 2)); i++) {
		convertVideoToOutputChunk(p_in, p_out, IN_VIDEO_WIDTH, ((i * 2) * IN_VIDEO_WIDTH) + IN_VIDEO_NO_BOTTOM_SIZE, TOP_SIZE_3DS + (i * IN_VIDEO_WIDTH));
		convertVideoToOutputChunk(p_in, p_out, IN_VIDEO_WIDTH, (((i * 2) + 1) * IN_VIDEO_WIDTH) + IN_VIDEO_NO_BOTTOM_SIZE, IN_VIDEO_NO_BOTTOM_SIZE + (i * IN_VIDEO_WIDTH));
	}
}

void convertAudioToOutput(CaptureReceived *p_in, sf::Int16 *p_out, const int n_samples, const bool is_big_endian) {
	if(!is_big_endian)
		memcpy(p_out, p_in->audio_data, n_samples * 2);
	else
		for(int i = 0; i < n_samples; i++)
			p_out[i] = ((p_in->audio_data[i] & 0xFF) << 8) | (p_in->audio_data[i] >> 8);
}


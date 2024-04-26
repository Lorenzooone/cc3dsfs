#ifndef __CONVERSIONS_HPP
#define __CONVERSIONS_HPP

#include <SFML/Audio.hpp>
#include "capture_structs.hpp"
#include "frontend.hpp"

void convertVideoToOutput(VideoInputData *p_in, VideoOutputData *p_out);
void convertAudioToOutput(CaptureReceived *p_in, sf::Int16 *p_out, const int n_samples, const bool is_big_endian);
#endif

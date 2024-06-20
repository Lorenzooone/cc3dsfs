#ifndef __CONVERSIONS_HPP
#define __CONVERSIONS_HPP

#include <SFML/Audio.hpp>
#include "capture_structs.hpp"
#include "display_structs.hpp"

void convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, CaptureData* capture_data);
void convertAudioToOutput(CaptureReceived *p_in, sf::Int16 *p_out, uint64_t n_samples, const bool is_big_endian, CaptureData* capture_data);
#endif

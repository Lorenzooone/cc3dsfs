#ifndef __CONVERSIONS_HPP
#define __CONVERSIONS_HPP

#include "capture_structs.hpp"
#include "display_structs.hpp"

bool convertVideoToOutput(int index, VideoOutputData *p_out, CaptureData* capture_data);
bool convertAudioToOutput(int index, std::int16_t *p_out, uint64_t n_samples, const bool is_big_endian, CaptureData* capture_data);
#endif

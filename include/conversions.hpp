#ifndef __CONVERSIONS_HPP
#define __CONVERSIONS_HPP

#include "capture_structs.hpp"
#include "display_structs.hpp"

bool convertVideoToOutput(VideoOutputData *p_out, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status);
bool convertAudioToOutput(std::int16_t *p_out, uint64_t &n_samples, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status);
#endif

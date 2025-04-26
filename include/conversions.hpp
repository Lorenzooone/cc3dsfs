#ifndef __CONVERSIONS_HPP
#define __CONVERSIONS_HPP

#include "capture_structs.hpp"
#include "display_structs.hpp"

bool convertVideoToOutput(VideoOutputData *p_out, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status, bool interleaved_3d);
bool convertAudioToOutput(std::int16_t *p_out, uint64_t &n_samples, uint16_t &last_buffer_index, const bool is_big_endian, CaptureDataSingleBuffer* data_buffer, CaptureStatus* status);
void manualConvertOutputToRGB(VideoOutputData* src, VideoOutputData* dst, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height, InputVideoDataType video_data_type);

#endif

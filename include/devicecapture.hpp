#ifndef __DEVICECAPTURE_HPP
#define __DEVICECAPTURE_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "frontend.hpp"

void capture_init();
void capture_close();

bool connect(bool print_failed, CaptureData* capture_data, FrontendData* frontend_data);
void captureCall(CaptureData* capture_data);
void capture_error_print(bool print_failed, CaptureData* capture_data, std::string error_string);
uint64_t get_audio_n_samples(CaptureData* capture_data, uint64_t read);
uint64_t get_video_in_size(CaptureData* capture_data);
std::string get_name_of_device(CaptureStatus* capture_status);
#endif

#ifndef __DEVICECAPTURE_HPP
#define __DEVICECAPTURE_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "frontend.hpp"

struct no_access_recap_data {
	no_access_recap_data(std::string name) : name(name), vid(-1), pid(-1) {}
	no_access_recap_data(uint16_t vid, uint16_t pid) : name(""), vid(vid), pid(pid) {}
	std::string name;
	int vid;
	int pid;
};

void capture_init();
void capture_close();

bool connect(bool print_failed, CaptureData* capture_data, FrontendData* frontend_data, bool auto_connect_to_first = false);
void captureCall(CaptureData* capture_data);
void capture_error_print(bool print_failed, CaptureData* capture_data, std::string error_string);
void capture_error_print(bool print_failed, CaptureData* capture_data, std::string graphical_string, std::string detailed_string);
uint64_t get_audio_n_samples(CaptureData* capture_data, uint64_t read);
uint64_t get_video_in_size(CaptureData* capture_data);
std::string get_name_of_device(CaptureStatus* capture_status, bool use_long = false);
int get_usb_speed_of_device(CaptureStatus* capture_status);
bool get_device_can_do_3d(CaptureStatus* capture_status);
bool get_device_3d_implemented(CaptureStatus* capture_status);
bool get_3d_enabled(CaptureStatus* capture_status);
#endif

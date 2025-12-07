#ifndef __DEVICECAPTURE_HPP
#define __DEVICECAPTURE_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "frontend.hpp"

enum KeySaveError { KEY_SAVED, KEY_INVALID, KEY_ALREADY_PRESENT, KEY_SAVE_METHOD_NOT_FOUND };

struct no_access_recap_data {
	no_access_recap_data(std::string name) : name(name), vid(-1), pid(-1) {}
	no_access_recap_data(uint16_t vid, uint16_t pid) : name(""), vid(vid), pid(pid) {}
	std::string name;
	int vid;
	int pid;
};

void capture_init();
void capture_close();

bool connect(bool print_failed, CaptureData* capture_data, FrontendData* frontend_data, bool* force_cc_disables, bool auto_connect_to_first = false);
void captureCall(CaptureData* capture_data);
void capture_error_print(bool print_failed, CaptureData* capture_data, std::string error_string);
void capture_error_print(bool print_failed, CaptureData* capture_data, std::string graphical_string, std::string detailed_string);
void capture_warning_print(CaptureData* capture_data, std::string warning_string);
void capture_warning_print(CaptureData* capture_data, std::string graphical_string, std::string detailed_string);
uint64_t get_audio_n_samples(CaptureData* capture_data, CaptureDataSingleBuffer* data_buffer);
uint64_t get_video_in_size(CaptureData* capture_data, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type);
std::string get_device_id_string(CaptureStatus* capture_status);
std::string get_device_serial_key_string(CaptureStatus* capture_status);
std::string get_name_of_device(CaptureStatus* capture_status, bool use_long = false, bool want_real_serial = false);
int get_usb_speed_of_device(CaptureStatus* capture_status);
bool get_device_can_do_3d(CaptureStatus* capture_status);
bool get_device_3d_implemented(CaptureStatus* capture_status);
bool is_usb_speed_of_device_enough_3d(CaptureStatus* capture_status);
bool get_3d_enabled(CaptureStatus* capture_status, bool skip_requested_3d_check = false);
bool update_3d_enabled(CaptureStatus* capture_status);
bool set_3d_enabled(CaptureStatus* capture_status, bool new_value);
float get_framerate_multiplier(CaptureStatus* capture_status);
KeySaveError save_cc_key(std::string key, CaptureConnectionType conn_type, bool differentiator);
void check_device_serial_key_update(CaptureStatus* capture_status, bool differentiator, std::string key);
void setup_reconnection_device(void* info);
bool wait_reconnection_device(void* info);
void end_reconnection_device(void* info);
#endif

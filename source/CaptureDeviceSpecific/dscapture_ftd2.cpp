#include "dscapture_ftd2.hpp"
#include "devicecapture.hpp"

#define FTD2XX_STATIC
#include <ftd2xx.h>

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

void list_devices_ftd2(std::vector<CaptureDevice> &devices_list) {
}

uint64_t ftd2_get_video_in_size(CaptureData* capture_data) {
	return 0;
}

static uint64_t get_capture_size(CaptureData* capture_data) {
	return 0;
}

bool connect_ftd2(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	return false;
}

void ftd2_capture_main_loop(CaptureData* capture_data) {
}

void ftd2_capture_cleanup(CaptureData* capture_data) {
}

void ftd2_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d) {
}


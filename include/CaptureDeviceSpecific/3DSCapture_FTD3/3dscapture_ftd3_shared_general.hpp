#ifndef __3DSCAPTURE_FTD3_SHARED_GENERAL_HPP
#define __3DSCAPTURE_FTD3_SHARED_GENERAL_HPP

#include <vector>
#include <chrono>
#include "capture_structs.hpp"

uint64_t ftd3_get_capture_size(bool is_3d_enabled);
uint64_t ftd3_get_capture_size(CaptureData* capture_data);
std::string ftd3_get_serial(std::string serial_string, int &curr_serial_extra_id);
void ftd3_insert_device(std::vector<CaptureDevice> &devices_list, std::string serial_string, int &curr_serial_extra_id, uint32_t usb_speed, bool is_driver);
void data_output_update(int inner_index, size_t read_data, CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time);
bool ftd3_capture_3d_setup(CaptureData* capture_data, bool first_pass, bool& stored_3d_status);

#endif

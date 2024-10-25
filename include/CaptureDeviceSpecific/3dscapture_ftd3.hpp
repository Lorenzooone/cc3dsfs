#ifndef __3DSCAPTURE_FTD3_HPP
#define __3DSCAPTURE_FTD3_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

void list_devices_ftd3(std::vector<CaptureDevice> &devices_list);
bool connect_ftd3(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void ftd3_capture_main_loop(CaptureData* capture_data);
void ftd3_capture_cleanup(CaptureData* capture_data);
void ftd3_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d);
uint64_t ftd3_get_video_in_size(CaptureData* capture_data);

#endif

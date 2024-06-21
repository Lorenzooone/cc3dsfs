#ifndef __3DSCAPTURE_FTDI_HPP
#define __3DSCAPTURE_FTDI_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"

#define USE_FTDI

void list_devices_ftdi(std::vector<CaptureDevice> &devices_list);
bool connect_ftdi(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void ftdi_capture_main_loop(CaptureData* capture_data);
void ftdi_capture_cleanup(CaptureData* capture_data);
void ftdi_convertVideoToOutput(CaptureReceived *p_in, VideoOutputData *p_out, bool enabled_3d);
uint64_t ftdi_get_video_in_size(CaptureData* capture_data);

#endif

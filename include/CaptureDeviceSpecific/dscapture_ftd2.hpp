#ifndef __DSCAPTURE_FTD2_HPP
#define __DSCAPTURE_FTD2_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

#define FTD2_OLDDS_SYNCH_VALUES 0x4321

void list_devices_ftd2(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list);
bool connect_ftd2(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void ftd2_capture_main_loop(CaptureData* capture_data);
void ftd2_capture_cleanup(CaptureData* capture_data);
uint64_t ftd2_get_video_in_size(CaptureData* capture_data);

#endif

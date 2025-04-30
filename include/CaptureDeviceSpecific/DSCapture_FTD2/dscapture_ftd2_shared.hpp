#ifndef __DSCAPTURE_FTD2_SHARED_HPP
#define __DSCAPTURE_FTD2_SHARED_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

#define FTD2_OLDDS_SYNCH_VALUES 0x4321

void list_devices_ftd2_shared(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan);
bool connect_ftd2_shared(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
void ftd2_capture_main_loop_shared(CaptureData* capture_data);
void ftd2_capture_cleanup_shared(CaptureData* capture_data);
uint64_t ftd2_get_video_in_size(CaptureData* capture_data);
void ftd2_init_shared();
void ftd2_end_shared();

#endif

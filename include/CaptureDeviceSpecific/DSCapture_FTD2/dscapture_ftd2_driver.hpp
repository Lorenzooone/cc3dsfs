#ifndef __DSCAPTURE_FTD2_DRIVER_HPP
#define __DSCAPTURE_FTD2_DRIVER_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

#define FTD2_OLDDS_SYNCH_VALUES 0x4321

void list_devices_ftd2_driver(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list);
int ftd2_driver_open_serial(CaptureDevice* device, void** handle);
void ftd2_capture_main_loop_driver(CaptureData* capture_data);

#endif

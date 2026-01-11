#ifndef __CYPRESS_PARTNER_CTR_ACQUISITION_HPP
#define __CYPRESS_PARTNER_CTR_ACQUISITION_HPP

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

#define PARTNER_CTR_CAPTURE_BASE_COMMAND 0xE007
#define PARTNER_CTR_CAPTURE_COMMAND_INPUT 0x000F
#define PARTNER_CTR_CAPTURE_COMMAND_TOP_SCREEN 0x00C4
#define PARTNER_CTR_CAPTURE_COMMAND_SECOND_TOP_SCREEN 0x00C5
#define PARTNER_CTR_CAPTURE_COMMAND_BOT_SCREEN 0x00C6
#define PARTNER_CTR_CAPTURE_COMMAND_AUDIO 0x00C7

void list_devices_cypart_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan);
bool cypart_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
uint64_t cypart_device_get_video_in_size(CaptureStatus* status, bool is_3d);
uint64_t cypart_device_get_video_in_size(CaptureData* capture_data, bool is_3d);
void cypart_device_acquisition_main_loop(CaptureData* capture_data);
void usb_cypart_device_acquisition_cleanup(CaptureData* capture_data);
void usb_cypart_device_init();
void usb_cypart_device_close();

#endif

#ifndef __CYPRESS_OPTIMIZE_3DS_ACQUISITION_HPP
#define __CYPRESS_OPTIMIZE_3DS_ACQUISITION_HPP

#define SERIAL_KEY_OPTIMIZE_NO_DASHES_SIZE 20
#define SERIAL_KEY_OPTIMIZE_DASHES_REPEATED_POS 4
// Not the last one, so -1
#define SERIAL_KEY_OPTIMIZE_DASHES_NUM ((SERIAL_KEY_OPTIMIZE_NO_DASHES_SIZE / SERIAL_KEY_OPTIMIZE_DASHES_REPEATED_POS) - 1)
#define SERIAL_KEY_OPTIMIZE_WITH_DASHES_SIZE (SERIAL_KEY_OPTIMIZE_NO_DASHES_SIZE + SERIAL_KEY_OPTIMIZE_DASHES_NUM)

#include <vector>
#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "display_structs.hpp"
#include "devicecapture.hpp"

void list_devices_cyop_device(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan);
bool cyop_device_connect_usb(bool print_failed, CaptureData* capture_data, CaptureDevice* device);
uint64_t cyop_device_get_video_in_size(CaptureStatus* status, bool is_3d, InputVideoDataType video_data_type);
uint64_t cyop_device_get_video_in_size(CaptureData* capture_data, bool is_3d, InputVideoDataType video_data_type);
void cyop_device_acquisition_main_loop(CaptureData* capture_data);
void usb_cyop_device_acquisition_cleanup(CaptureData* capture_data);
bool is_device_optimize_3ds(CaptureDevice* device);
bool is_device_optimize_o3ds(CaptureDevice* device);
bool is_device_optimize_n3ds(CaptureDevice* device);
bool cyop_is_key_for_device_id(CaptureDevice* device, std::string key);
KeySaveError add_key_to_file(std::string key, bool is_for_new_3ds);
void usb_cyop_device_init();
void usb_cyop_device_close();

#endif

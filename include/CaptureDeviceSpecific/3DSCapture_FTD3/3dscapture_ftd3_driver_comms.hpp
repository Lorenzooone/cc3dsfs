#ifndef __3DSCAPTURE_FTD3_DRIVER_COMMS_HPP
#define __3DSCAPTURE_FTD3_DRIVER_COMMS_HPP

#include <vector>
#include "utils.hpp"
#include "capture_structs.hpp"
#include "devicecapture.hpp"
#include "3dscapture_ftd3_compatibility.hpp"

bool ftd3_driver_get_is_bad();
bool ftd3_driver_get_skip_initial_pipe_abort();
ftd3_device_device_handlers* ftd3_driver_serial_reconnection(std::string wanted_serial_number);
void ftd3_driver_list_devices(std::vector<CaptureDevice> &devices_list, int *curr_serial_extra_id_ftd3, const std::vector<std::string> &valid_descriptions);
void ftd3_driver_end_connection(ftd3_device_device_handlers* handlers);

#endif

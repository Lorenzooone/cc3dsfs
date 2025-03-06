#ifndef __3DSCAPTURE_FTD3_DRIVER_ACQUISITION_HPP
#define __3DSCAPTURE_FTD3_DRIVER_ACQUISITION_HPP

#include "capture_structs.hpp"

void ftd3_driver_capture_main_loop(CaptureData* capture_data, int pipe);

#endif

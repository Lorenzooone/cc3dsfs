#ifndef __3DSCAPTURE_FTD3_LIBUSB_ACQUISITION_HPP
#define __3DSCAPTURE_FTD3_LIBUSB_ACQUISITION_HPP

#include "capture_structs.hpp"

void ftd3_libusb_capture_main_loop(CaptureData* capture_data, int pipe);

#endif

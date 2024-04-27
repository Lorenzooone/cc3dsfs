#ifndef __3DSCAPTURE_HPP
#define __3DSCAPTURE_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include "capture_structs.hpp"
#include "frontend.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define FTD3XX_STATIC
#include <FTD3XX.h>
#define FT_ASYNC_CALL FT_ReadPipeEx
#else
#include <ftd3xx.h>
#define FT_ASYNC_CALL FT_ReadPipeAsync
#endif

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

// Max value (Due to support of old FTD3XX versions...)
#define NUM_CONCURRENT_DATA_BUFFERS 8

// When is_bad_ftd3xx is true, it may happen that a frame is lost.
// This value prevents showing a black frame for that.
#define MAX_ALLOWED_BLANKS 1

struct CaptureData {
	FT_HANDLE handle;
	ULONG read[NUM_CONCURRENT_DATA_BUFFERS];
	CaptureReceived capture_buf[NUM_CONCURRENT_DATA_BUFFERS];
	double time_in_buf[NUM_CONCURRENT_DATA_BUFFERS];
	char chosen_serial_number[SERIAL_NUMBER_SIZE];
	CaptureStatus status;
};

bool connect(bool print_failed, CaptureData* capture_data, FrontendData* frontend_data);
void captureCall(CaptureData* capture_data);
#endif

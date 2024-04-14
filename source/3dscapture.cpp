#include "utils.hpp"
#include "3dscapture.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define FTD3XX_STATIC
#include <FTD3XX.h>
#define FT_ASYNC_CALL FT_ReadPipeEx
#else
#include <ftd3xx.h>
#define FT_ASYNC_CALL FT_ReadPipeAsync
#endif

#include <cstring>
#include <thread>
#include <chrono>

#define BULK_OUT 0x02
#define BULK_IN 0x82

#if defined(_WIN32) || defined(_WIN64)
#define FIFO_CHANNEL 0x82
#else
#define FIFO_CHANNEL 0
#endif

static void list_devices(DevicesList &devices_list) {
	FT_STATUS ftStatus;
	DWORD numDevs = 0;
	std::string valid_descriptions[] = {"N3DSXL", "N3DSXL.2"};
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	devices_list.numAllocedDevices = 0;
	devices_list.numValidDevices = 0;
	if (!FT_FAILED(ftStatus) && numDevs > 0)
	{
		devices_list.numAllocedDevices = numDevs;
		devices_list.serialNumbers = new char[devices_list.numAllocedDevices * 17];
		FT_HANDLE ftHandle = NULL;
		DWORD Flags = 0;
		DWORD Type = 0;
		DWORD ID = 0;
		char SerialNumber[16] = { 0 };
		char Description[32] = { 0 };
		for (DWORD i = 0; i < numDevs; i++)
		{
			ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, NULL,
			SerialNumber, Description, &ftHandle);
			if (!FT_FAILED(ftStatus))
			{
				for(int i = 0; i < sizeof(valid_descriptions) / sizeof(*valid_descriptions); i++) {
					if(Description == valid_descriptions[i]) {
						for(int i = 0; i < 16; i++)
							devices_list.serialNumbers[(17 * devices_list.numValidDevices) + i] = SerialNumber[i];
						devices_list.serialNumbers[(17 * devices_list.numValidDevices) + 16] = 0;
						++devices_list.numValidDevices;
						break;
					}
				}
			}
		}
	}
}

static int choose_device(DevicesList &devices_list) {
	return 0;
}

bool connect(bool print_failed, CaptureData* capture_data) {
	if (capture_data->connected) {
		capture_data->close_success = false;
		return false;
	}
	
	if(!capture_data->close_success) {
		return false;
	}

	DevicesList devices_list;
	list_devices(devices_list);

	if(devices_list.numValidDevices <= 0) {
		if(print_failed)
			printf("[%s] No device was found.\n", NAME);
		if(devices_list.numAllocedDevices > 0)
			delete []devices_list.serialNumbers;
		return false;
	}

	int chosen_device = choose_device(devices_list);
	if(chosen_device == -1) {
		if(print_failed)
			printf("[%s] No device was selected.\n", NAME);
		delete []devices_list.serialNumbers;
		return false;
	}

	if (FT_Create(&devices_list.serialNumbers[17 * chosen_device], FT_OPEN_BY_SERIAL_NUMBER, &capture_data->handle)) {
		if(print_failed)
			printf("[%s] Create failed.\n", NAME);
		delete []devices_list.serialNumbers;
		return false;
	}

	delete []devices_list.serialNumbers;

	UCHAR buf[4] = {0x40, 0x80, 0x00, 0x00};
	ULONG written = 0;

	if (FT_WritePipe(capture_data->handle, BULK_OUT, buf, 4, &written, 0)) {
		if(print_failed)
			printf("[%s] Write failed.\n", NAME);
		return false;
	}

	buf[1] = 0x00;

	if (FT_WritePipe(capture_data->handle, BULK_OUT, buf, 4, &written, 0)) {
		if(print_failed)
			printf("[%s] Write failed.\n", NAME);
		return false;
	}

	if (FT_SetStreamPipe(capture_data->handle, false, false, BULK_IN, sizeof(CaptureReceived))) {
		if(print_failed)
			printf("[%s] Stream failed.\n", NAME);
		return false;
	}

	if(FT_AbortPipe(capture_data->handle, BULK_IN)) {
		if(print_failed)
			printf("[%s] Abort failed.\n", NAME);
	}

	if (FT_SetStreamPipe(capture_data->handle, false, false, BULK_IN, sizeof(CaptureReceived))) {
		if(print_failed)
			printf("[%s] Stream failed.\n", NAME);
		return false;
	}

	// Avoid having old open locks
	capture_data->video_wait.try_lock();
	capture_data->audio_wait.try_lock();

	return true;
}

static void fast_capture_call(CaptureData* capture_data, OVERLAPPED overlap[NUM_CONCURRENT_DATA_BUFFERS]) {
	int inner_curr_in = 0;
	FT_STATUS ftStatus;
	for (inner_curr_in = 0; inner_curr_in < NUM_CONCURRENT_DATA_BUFFERS; ++inner_curr_in) {
		ftStatus = FT_InitializeOverlapped(capture_data->handle, &overlap[inner_curr_in]);
		if (ftStatus) {
			printf("[%s] Initialize failed.\n", NAME);
			return;
		}
	}

	for (inner_curr_in = 0; inner_curr_in < NUM_CONCURRENT_DATA_BUFFERS - 1; ++inner_curr_in) {
		ftStatus = FT_ASYNC_CALL(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&capture_data->capture_buf[inner_curr_in], sizeof(CaptureReceived), &capture_data->read[inner_curr_in], &overlap[inner_curr_in]);
		if (ftStatus != FT_IO_PENDING) {
			printf("[%s] Read failed.\n", NAME);
			return;
		}
	}

	inner_curr_in = NUM_CONCURRENT_DATA_BUFFERS - 1;

	auto clock_start = std::chrono::high_resolution_clock::now();

	while (capture_data->connected && capture_data->running) {

		ftStatus = FT_ASYNC_CALL(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&capture_data->capture_buf[inner_curr_in], sizeof(CaptureReceived), &capture_data->read[inner_curr_in], &overlap[inner_curr_in]);
		if (ftStatus != FT_IO_PENDING) {
			printf("[%s] Read failed.\n", NAME);
			return;
		}

		inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;

		ftStatus = FT_GetOverlappedResult(capture_data->handle, &overlap[inner_curr_in], &capture_data->read[inner_curr_in], true);
		if(FT_FAILED(ftStatus)) {
			printf("[%s] USB error.\n", NAME);
			return;
		}
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;
		capture_data->time_in_buf[inner_curr_in] = diff.count();
		clock_start = curr_time;

		capture_data->curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
		if(capture_data->cooldown_curr_in)
			capture_data->cooldown_curr_in--;
		// Signal that there is data available
		capture_data->video_wait.unlock();
		capture_data->audio_wait.unlock();
	}
}

#if not(defined(_WIN32) || defined(_WIN64))
static bool safe_capture_call(CaptureData* capture_data) {
	int inner_curr_in = 0;

	while (capture_data->connected && capture_data->running) {

		auto clock_start = std::chrono::high_resolution_clock::now();

		FT_STATUS ftStatus = FT_ReadPipeEx(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&capture_data->capture_buf[inner_curr_in], sizeof(CaptureReceived), &capture_data->read[inner_curr_in], 1000);
		if(FT_FAILED(ftStatus)) {
			printf("[%s] Read failed.\n", NAME);
			return true;
		}

		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;
		capture_data->time_in_buf[inner_curr_in] = diff.count();

		inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
		capture_data->curr_in = inner_curr_in;
		if(capture_data->cooldown_curr_in)
			capture_data->cooldown_curr_in--;
		capture_data->video_wait.unlock();
		capture_data->audio_wait.unlock();
	}

	return false;
}
#endif

void captureCall(CaptureData* capture_data) {
	OVERLAPPED overlap[NUM_CONCURRENT_DATA_BUFFERS];
	FT_STATUS ftStatus;
	int inner_curr_in = 0;
	capture_data->curr_in = inner_curr_in;
	capture_data->cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	bool is_bad_ftd3xx = false;
	DWORD ftd3xx_lib_version;

	if(FT_FAILED(FT_GetLibraryVersion(&ftd3xx_lib_version))) {
		ftd3xx_lib_version = 0;
	}
	if(ftd3xx_lib_version == 0x0100001A) {
		is_bad_ftd3xx = true;
	}

	while(capture_data->running) {
		if (!capture_data->connected) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000/USB_CHECKS_PER_SECOND));
			continue;
		}

		bool bad_close = false;
		#if not(defined(_WIN32) || defined(_WIN64))
		if(!is_bad_ftd3xx)
			fast_capture_call(capture_data, overlap);
		else
			bad_close = safe_capture_call(capture_data);
		#else
			is_bad_ftd3xx = false;
			fast_capture_call(capture_data, overlap);
		#endif

		capture_data->close_success = false;
		capture_data->connected = false;
		capture_data->cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
		// Needed in case the threads went in the connected loop
		// before it is set right above, and they are waiting on the locks!
		capture_data->video_wait.unlock();
		capture_data->audio_wait.unlock();

		if(!is_bad_ftd3xx) {
			for (inner_curr_in = 0; inner_curr_in < NUM_CONCURRENT_DATA_BUFFERS; ++inner_curr_in) {
				ftStatus = FT_GetOverlappedResult(capture_data->handle, &overlap[inner_curr_in], &capture_data->read[inner_curr_in], true);
				if (FT_ReleaseOverlapped(capture_data->handle, &overlap[inner_curr_in])) {
					printf("[%s] Release failed.\n", NAME);
				}
			}
		}

		if(FT_AbortPipe(capture_data->handle, BULK_IN)) {
			printf("[%s] Abort failed.\n", NAME);
		}

		if (FT_Close(capture_data->handle)) {
			printf("[%s] Close failed.\n", NAME);
		}

		capture_data->close_success = false;
		capture_data->connected = false;

		capture_data->close_success = true;
	}
}

#include "3dscapture.hpp"

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define BULK_OUT 0x02
#define BULK_IN 0x82

#if defined(_WIN32) || defined(_WIN64)
#define FIFO_CHANNEL 0x82
#else
#define FIFO_CHANNEL 0
#endif

//#define DEBUG_DEVICES_PRINT_DESCRIPTION_INFOS

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
		devices_list.serialNumbers = new char[devices_list.numAllocedDevices * SERIAL_NUMBER_SIZE];
		FT_HANDLE ftHandle = NULL;
		DWORD Flags = 0;
		DWORD Type = 0;
		DWORD ID = 0;
		char SerialNumber[REAL_SERIAL_NUMBER_SIZE] = { 0 };
		char Description[32] = { 0 };
		for (DWORD i = 0; i < numDevs; i++)
		{
			ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, NULL,
			SerialNumber, Description, &ftHandle);
			#ifdef DEBUG_DEVICES_PRINT_DESCRIPTION_INFOS
			std::cout << "Total: " << numDevs << " - Index: " << i << " - " << std::string(Description) << " - " << std::string(SerialNumber) << std::endl;
			#endif
			if (!FT_FAILED(ftStatus))
			{
				for(int j = 0; j < sizeof(valid_descriptions) / sizeof(*valid_descriptions); j++) {
					if(Description == valid_descriptions[j]) {
						for(int k = 0; k < REAL_SERIAL_NUMBER_SIZE; k++)
							devices_list.serialNumbers[(SERIAL_NUMBER_SIZE * devices_list.numValidDevices) + k] = SerialNumber[k];
						devices_list.serialNumbers[(SERIAL_NUMBER_SIZE * devices_list.numValidDevices) + REAL_SERIAL_NUMBER_SIZE] = 0;
						++devices_list.numValidDevices;
						break;
					}
				}
			}
		}
	}
}

static bool poll_connection_window_screen(WindowScreen *screen, int &chosen_index) {
	screen->poll();
	if(screen->check_connection_menu_result() != -1) {
		chosen_index = screen->check_connection_menu_result();
		return true;
	}
	if(screen->close_capture()) {
		return true;
	}
	return false;
}

static int choose_device(DevicesList *devices_list, FrontendData* frontend_data) {
	if(devices_list->numValidDevices == 1)
		return 0;
	int chosen_index = -1;
	frontend_data->top_screen->setup_connection_menu(devices_list);
	frontend_data->bot_screen->setup_connection_menu(devices_list);
	frontend_data->joint_screen->setup_connection_menu(devices_list);
	bool done = false;
	while(!done) {
		update_output(frontend_data);
		done = poll_connection_window_screen(frontend_data->top_screen, chosen_index);
		if(done)
			break;
		done = poll_connection_window_screen(frontend_data->bot_screen, chosen_index);
		if(done)
			break;
		done = poll_connection_window_screen(frontend_data->joint_screen, chosen_index);
		if(done)
			break;
		default_sleep();
	}
	frontend_data->top_screen->end_connection_menu();
	frontend_data->bot_screen->end_connection_menu();
	frontend_data->joint_screen->end_connection_menu();
	return chosen_index;
}

static void preemptive_close_connection(CaptureData* capture_data) {
	FT_AbortPipe(capture_data->handle, BULK_IN);
	FT_Close(capture_data->handle);
}

bool connect(bool print_failed, CaptureData* capture_data, FrontendData* frontend_data) {
	capture_data->status.new_error_text = false;
	if (capture_data->status.connected) {
		capture_data->status.close_success = false;
		return false;
	}
	
	if(!capture_data->status.close_success) {
		if(print_failed) {
			capture_data->status.error_text = "Previous device still closing...";
			capture_data->status.new_error_text = true;
		}
		return false;
	}

	DevicesList devices_list;
	list_devices(devices_list);

	if(devices_list.numValidDevices <= 0) {
		if(print_failed) {
			capture_data->status.error_text = "No device was found";
			capture_data->status.new_error_text = true;
		}
		if(devices_list.numAllocedDevices > 0)
			delete []devices_list.serialNumbers;
		return false;
	}

	int chosen_device = choose_device(&devices_list, frontend_data);
	if(chosen_device == -1) {
		if(print_failed) {
			capture_data->status.error_text = "No device was selected";
			capture_data->status.new_error_text = true;
		}
		delete []devices_list.serialNumbers;
		return false;
	}

	char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
	for(int i = 0; i < REAL_SERIAL_NUMBER_SIZE; i++)
		SerialNumber[i] = devices_list.serialNumbers[(SERIAL_NUMBER_SIZE * chosen_device) + i];
	SerialNumber[REAL_SERIAL_NUMBER_SIZE] = 0;
	delete []devices_list.serialNumbers;
	capture_data->status.serial_number = "N3DSXL - " + std::string(SerialNumber);

	if (FT_Create(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &capture_data->handle)) {
		if(print_failed) {
			capture_data->status.error_text = "Create failed";
			capture_data->status.new_error_text = true;
		}
		return false;
	}

	UCHAR buf[4] = {0x40, 0x80, 0x00, 0x00};
	ULONG written = 0;

	if (FT_WritePipe(capture_data->handle, BULK_OUT, buf, 4, &written, 0)) {
		if(print_failed) {
			capture_data->status.error_text = "Write failed";
			capture_data->status.new_error_text = true;
		}
		preemptive_close_connection(capture_data);
		return false;
	}

	buf[1] = 0x00;

	if (FT_WritePipe(capture_data->handle, BULK_OUT, buf, 4, &written, 0)) {
		if(print_failed) {
			capture_data->status.error_text = "Write failed";
			capture_data->status.new_error_text = true;
		}
		preemptive_close_connection(capture_data);
		return false;
	}

	if (FT_SetStreamPipe(capture_data->handle, false, false, BULK_IN, sizeof(CaptureReceived))) {
		if(print_failed) {
			capture_data->status.error_text = "Stream failed";
			capture_data->status.new_error_text = true;
		}
		preemptive_close_connection(capture_data);
		return false;
	}

	if(FT_AbortPipe(capture_data->handle, BULK_IN)) {
		if(print_failed) {
			capture_data->status.error_text = "Abort failed";
			capture_data->status.new_error_text = true;
		}
		preemptive_close_connection(capture_data);
		return false;
	}

	if (FT_SetStreamPipe(capture_data->handle, false, false, BULK_IN, sizeof(CaptureReceived))) {
		if(print_failed) {
			capture_data->status.error_text = "Stream failed";
			capture_data->status.new_error_text = true;
		}
		preemptive_close_connection(capture_data);
		return false;
	}

	// Avoid having old open locks
	capture_data->status.video_wait.try_lock();
	capture_data->status.audio_wait.try_lock();

	return true;
}

static void fast_capture_call(CaptureData* capture_data, OVERLAPPED overlap[NUM_CONCURRENT_DATA_BUFFERS]) {
	int inner_curr_in = 0;
	FT_STATUS ftStatus;
	for (inner_curr_in = 0; inner_curr_in < NUM_CONCURRENT_DATA_BUFFERS; ++inner_curr_in) {
		ftStatus = FT_InitializeOverlapped(capture_data->handle, &overlap[inner_curr_in]);
		if (ftStatus) {
			capture_data->status.error_text = "Disconnected: Initialize failed";
			capture_data->status.new_error_text = true;
			return;
		}
	}

	for (inner_curr_in = 0; inner_curr_in < NUM_CONCURRENT_DATA_BUFFERS - 1; ++inner_curr_in) {
		ftStatus = FT_ASYNC_CALL(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&capture_data->capture_buf[inner_curr_in], sizeof(CaptureReceived), &capture_data->read[inner_curr_in], &overlap[inner_curr_in]);
		if (ftStatus != FT_IO_PENDING) {
			capture_data->status.error_text = "Disconnected: Read failed";
			capture_data->status.new_error_text = true;
			return;
		}
	}

	inner_curr_in = NUM_CONCURRENT_DATA_BUFFERS - 1;

	auto clock_start = std::chrono::high_resolution_clock::now();

	while (capture_data->status.connected && capture_data->status.running) {

		ftStatus = FT_ASYNC_CALL(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&capture_data->capture_buf[inner_curr_in], sizeof(CaptureReceived), &capture_data->read[inner_curr_in], &overlap[inner_curr_in]);
		if (ftStatus != FT_IO_PENDING) {
			capture_data->status.error_text = "Disconnected: Read failed";
			capture_data->status.new_error_text = true;
			return;
		}

		inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;

		ftStatus = FT_GetOverlappedResult(capture_data->handle, &overlap[inner_curr_in], &capture_data->read[inner_curr_in], true);
		if(FT_FAILED(ftStatus)) {
			capture_data->status.error_text = "Disconnected: USB error";
			capture_data->status.new_error_text = true;
			return;
		}
		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;
		capture_data->time_in_buf[inner_curr_in] = diff.count();
		clock_start = curr_time;

		capture_data->status.curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
		if(capture_data->status.cooldown_curr_in)
			capture_data->status.cooldown_curr_in--;
		// Signal that there is data available
		capture_data->status.video_wait.unlock();
		capture_data->status.audio_wait.unlock();
	}
}

#if not(defined(_WIN32) || defined(_WIN64))
static bool safe_capture_call(CaptureData* capture_data) {
	int inner_curr_in = 0;

	while (capture_data->status.connected && capture_data->status.running) {

		auto clock_start = std::chrono::high_resolution_clock::now();

		FT_STATUS ftStatus = FT_ReadPipeEx(capture_data->handle, FIFO_CHANNEL, (UCHAR*)&capture_data->capture_buf[inner_curr_in], sizeof(CaptureReceived), &capture_data->read[inner_curr_in], 1000);
		if(FT_FAILED(ftStatus)) {
			capture_data->status.error_text = "Disconnected: Read failed";
			capture_data->status.new_error_text = true;
			return true;
		}

		const auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - clock_start;
		capture_data->time_in_buf[inner_curr_in] = diff.count();

		inner_curr_in = (inner_curr_in + 1) % NUM_CONCURRENT_DATA_BUFFERS;
		capture_data->status.curr_in = inner_curr_in;
		if(capture_data->status.cooldown_curr_in)
			capture_data->status.cooldown_curr_in--;
		capture_data->status.video_wait.unlock();
		capture_data->status.audio_wait.unlock();
	}

	return false;
}
#endif

void captureCall(CaptureData* capture_data) {
	OVERLAPPED overlap[NUM_CONCURRENT_DATA_BUFFERS];
	FT_STATUS ftStatus;
	int inner_curr_in = 0;
	capture_data->status.curr_in = inner_curr_in;
	capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	bool is_bad_ftd3xx = false;
	DWORD ftd3xx_lib_version;

	if(FT_FAILED(FT_GetLibraryVersion(&ftd3xx_lib_version))) {
		ftd3xx_lib_version = 0;
	}
	if(ftd3xx_lib_version == 0x0100001A) {
		is_bad_ftd3xx = true;
	}

	while(capture_data->status.running) {
		if (!capture_data->status.connected) {
			default_sleep();
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

		capture_data->status.close_success = false;
		capture_data->status.connected = false;
		capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
		// Needed in case the threads went in the connected loop
		// before it is set right above, and they are waiting on the locks!
		capture_data->status.video_wait.unlock();
		capture_data->status.audio_wait.unlock();

		if(!is_bad_ftd3xx) {
			for (inner_curr_in = 0; inner_curr_in < NUM_CONCURRENT_DATA_BUFFERS; ++inner_curr_in) {
				ftStatus = FT_GetOverlappedResult(capture_data->handle, &overlap[inner_curr_in], &capture_data->read[inner_curr_in], true);
				if (FT_ReleaseOverlapped(capture_data->handle, &overlap[inner_curr_in])) {
					capture_data->status.error_text = "Disconnected: Release failed";
					capture_data->status.new_error_text = true;
				}
			}
		}

		if(FT_AbortPipe(capture_data->handle, BULK_IN)) {
			capture_data->status.error_text = "Disconnected: Abort failed";
			capture_data->status.new_error_text = true;
		}

		if (FT_Close(capture_data->handle)) {
			capture_data->status.error_text = "Disconnected: Close failed";
			capture_data->status.new_error_text = true;
		}

		capture_data->status.close_success = false;
		capture_data->status.connected = false;

		capture_data->status.close_success = true;
	}
}

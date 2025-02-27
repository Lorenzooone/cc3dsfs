#include "dscapture_ftd2_driver.hpp"
#include "devicecapture.hpp"
#include "usb_generic.hpp"
#include "dscapture_ftd2_general.hpp"
#include "dscapture_ftd2_compatibility.hpp"

#include "ftd2xx_symbols_renames.h"
#define FTD2XX_STATIC
#include <ftd2xx.h>

#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#define FT_FAILED(x) ((x) != FT_OK)

#define REAL_SERIAL_NUMBER_SIZE 16
#define SERIAL_NUMBER_SIZE (REAL_SERIAL_NUMBER_SIZE+1)

#define ENABLE_AUDIO true

// Code based on sample provided by Loopy.

void list_devices_ftd2_driver(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	FT_STATUS ftStatus;
	DWORD numDevs = 0;
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	size_t num_inserted = 0;
	if (!FT_FAILED(ftStatus) && numDevs > 0)
	{
		const int debug_multiplier = 1;
		FT_HANDLE ftHandle = NULL;
		DWORD Flags = 0;
		DWORD Type = 0;
		DWORD ID = 0;
		char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
		char Description[65] = { 0 };
		for (DWORD i = 0; i < numDevs; i++)
		{
			ftStatus = FT_GetDeviceInfoDetail(i, &Flags, &Type, &ID, NULL,
			SerialNumber, Description, &ftHandle);
			if((!FT_FAILED(ftStatus)) && (Flags & FT_FLAGS_HISPEED) && (Type == FT_DEVICE_232H))
			{
				for(int j = 0; j < get_num_ftd2_device_types(); j++) {
					if(Description == get_ftd2_fw_desc(j)) {
						for(int u = 0; u < debug_multiplier; u++)
							devices_list.emplace_back(std::string(SerialNumber), "DS.2", "DS.2.d565", CAPTURE_CONN_FTD2, (void*)NULL, false, false, ENABLE_AUDIO, WIDTH_DS, HEIGHT_DS + HEIGHT_DS, get_max_samples(false), 0, 0, 0, 0, HEIGHT_DS, VIDEO_DATA_RGB16, get_ftd2_fw_index(j), false);
						break;
					}
				}
			}
		}
	}
}

int ftd2_driver_open_serial(CaptureDevice* device, void** handle) {
	char SerialNumber[SERIAL_NUMBER_SIZE] = { 0 };
	strncpy(SerialNumber, device->serial_number.c_str(), SERIAL_NUMBER_SIZE);
	SerialNumber[REAL_SERIAL_NUMBER_SIZE] = 0;
	return FT_OpenEx(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, handle);
}

static size_t get_initial_offset_buffer(uint16_t* in_u16, size_t real_length) {
	if(real_length <= 0)
		return 0;
	// This is because the actual data seems to always start with a SYNCH
	size_t ignored_halfwords = 0;
	while((ignored_halfwords < (real_length / 2)) && (in_u16[ignored_halfwords] == FTD2_OLDDS_SYNCH_VALUES))
		ignored_halfwords++;
	return ignored_halfwords * 2;
}

static void data_output_update(CaptureReceived* buffer, CaptureData* capture_data, int read_amount, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	size_t buffer_real_len = remove_synch_from_final_length((uint32_t*)buffer, read_amount);
	size_t initial_offset = get_initial_offset_buffer((uint16_t*) buffer, buffer_real_len);
	capture_data->data_buffers.WriteToBuffer(buffer, buffer_real_len, diff.count(), &capture_data->status.device, initial_offset);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

void ftd2_capture_main_loop_driver(CaptureData* capture_data) {
	bool is_libftdi = false;
	// Separate Capture Enable and put it here, for better control
	if(!enable_capture(capture_data->handle, is_libftdi)) {
		capture_error_print(true, capture_data, "Capture enable error");
		return;
	}
	int inner_curr_in = 0;
	int retval = 0;
	auto clock_start = std::chrono::high_resolution_clock::now();
	CaptureReceived* data_buffer = new CaptureReceived[NUM_CAPTURE_RECEIVED_DATA_BUFFERS];
	int curr_data_buffer = 0;
	int next_data_buffer = 0;
	const size_t full_size = get_capture_size(capture_data->status.device.is_rgb_888);
	size_t next_size = full_size;
	size_t bytesIn;

	while (capture_data->status.connected && capture_data->status.running) {
		curr_data_buffer = next_data_buffer;
		retval = ftd2_read(capture_data->handle, is_libftdi, ((uint8_t*)(&data_buffer[curr_data_buffer]) + (full_size - next_size)), next_size, &bytesIn);
		if(ftd2_is_error(retval, is_libftdi)) {
			capture_error_print(true, capture_data, "Disconnected: Read failed");
			break;
		}
		if(bytesIn < next_size)
			continue;
		next_data_buffer = (curr_data_buffer + 1) % NUM_CAPTURE_RECEIVED_DATA_BUFFERS;
		bool has_synch_failed = !synchronization_check((uint16_t*)(&data_buffer[curr_data_buffer]), full_size, (uint16_t*)(&data_buffer[next_data_buffer]), &next_size, true);
		if(has_synch_failed) {
			continue;
		}
		data_output_update(&data_buffer[curr_data_buffer], capture_data, full_size, clock_start);
	}
	delete []data_buffer;
}

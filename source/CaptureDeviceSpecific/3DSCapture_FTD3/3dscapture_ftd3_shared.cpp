#include "3dscapture_ftd3_shared.hpp"
#include "3dscapture_ftd3_shared_general.hpp"
#include "3dscapture_ftd3_compatibility.hpp"
#include "devicecapture.hpp"
//#include "ftd3xx_symbols_renames.h"

#include <cstring>
#include <chrono>

#define BULK_OUT 0x02
#define BULK_IN 0x82
 
#define FTD3_N3DSXL_CFG_WAIT_MS 120
#define CFG_3DS_3D_SCREEN_POS 3

const std::vector<std::string> valid_3dscapture_descriptions = {"N3DSXL", "N3DSXL.2"};

std::string ftd3_get_serial(std::string serial_string, int &curr_serial_extra_id) {
	if(serial_string != "")
		return serial_string;
	return std::to_string(curr_serial_extra_id++);
}

void ftd3_insert_device(std::vector<CaptureDevice> &devices_list, std::string serial_string, int &curr_serial_extra_id, uint32_t usb_speed, bool is_driver) {
	std::string short_name = "N3DSXL";
	std::string long_name = short_name;
	if(is_driver)
		long_name += ".d";
	else
		long_name += ".l";
	devices_list.emplace_back(ftd3_get_serial(serial_string, curr_serial_extra_id), short_name, long_name, CAPTURE_CONN_FTD3, (void*)NULL, true, true, true, HEIGHT_3DS, TOP_WIDTH_3DS + BOT_WIDTH_3DS, N3DSXL_SAMPLES_IN, 90, 0, 0, TOP_WIDTH_3DS, 0, VIDEO_DATA_RGB, usb_speed);
}

void list_devices_ftd3(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list) {
	ftd3_list_devices_compat(devices_list, no_access_list, valid_3dscapture_descriptions);
}

void data_output_update(int inner_index, size_t read_data, CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time) {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	capture_data->data_buffers.WriteToBuffer(NULL, read_data, diff.count(), &capture_data->status.device, inner_index);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

uint64_t ftd3_get_video_in_size(CaptureData* capture_data) {
	if(!get_3d_enabled(&capture_data->status))
		return sizeof(RGB83DSVideoInputData);
	return sizeof(RGB83DSVideoInputData_3D);
}

static uint64_t ftd3_get_capture_size(CaptureStatus* capture_status) {
	if(!get_3d_enabled(capture_status))
		return sizeof(FTD3_3DSCaptureReceived) & (~(EXTRA_DATA_BUFFER_FTD3XX_SIZE - 1));
	return sizeof(FTD3_3DSCaptureReceived_3D) & (~(EXTRA_DATA_BUFFER_FTD3XX_SIZE - 1));
}

uint64_t ftd3_get_capture_size(CaptureData* capture_data) {
	return ftd3_get_capture_size(&capture_data->status);
}

static void preemptive_close_connection(CaptureData* capture_data) {
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;
	ftd3_abort_pipe_compat(handlers, BULK_IN);
	ftd3_close_compat(handlers);
	delete handlers;
	capture_data->handle = NULL;
}

static bool reset_3ds_state(bool print_failed, CaptureData* capture_data) {
	uint8_t buf[4] = {0x80, 0x01, 0xAB, 0x00};
	int transferred = 0;
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	buf[0] = 0x43;
	buf[1] = 0;
	buf[2] = 0;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}
	default_sleep(FTD3_N3DSXL_CFG_WAIT_MS);

	return true;
}

static bool read_3ds_config_3d(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	uint8_t buf[4] = {0x40, 0x80, 0x00, 0x00};
	uint8_t in_buf[0x10];
	int transferred = 0;
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	buf[0] = 0x98;
	buf[1] = 0x05;
	buf[2] = 0x9F;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if(ftd3_is_error_compat(handlers, ftd3_read_pipe_compat(handlers, BULK_IN, in_buf, 0x10, &transferred))) {
		capture_error_print(print_failed, capture_data, "Read failed");
		preemptive_close_connection(capture_data);
		return false;
	}
	default_sleep(FTD3_N3DSXL_CFG_WAIT_MS);

	if(transferred >= CFG_3DS_3D_SCREEN_POS) {
		if(in_buf[CFG_3DS_3D_SCREEN_POS] != 0xc1)
			device->has_3d = false;
	}

	buf[0] = 0x40;
	buf[1] = 0;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}
	default_sleep(FTD3_N3DSXL_CFG_WAIT_MS);

	return true;
}

static bool set_3ds_state(bool print_failed, CaptureData* capture_data, uint8_t cfg_value) {
	uint8_t buf[4] = {0x40, 0x80, 0x00, 0x00};
	int transferred = 0;
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	buf[0] = cfg_value;
	buf[1] = 0x00;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}
	default_sleep(FTD3_N3DSXL_CFG_WAIT_MS);

	return true;
}

bool connect_ftd3(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	capture_data->handle = ftd3_reconnect_compat(device->serial_number, valid_3dscapture_descriptions);
	if(capture_data->handle == NULL) {
		capture_error_print(print_failed, capture_data, "Create failed");
		return false;
	}
	preemptive_close_connection(capture_data);
	capture_data->handle = ftd3_reconnect_compat(device->serial_number, valid_3dscapture_descriptions);
	if(capture_data->handle == NULL) {
		capture_error_print(print_failed, capture_data, "Second Create failed");
		return false;
	}

	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;
	
	if(!reset_3ds_state(print_failed, capture_data))
		return false;

	if(!set_3ds_state(print_failed, capture_data, 0x42))
		return false;

	if(!set_3ds_state(print_failed, capture_data, 0x40))
		return false;

	if(!set_3ds_state(print_failed, capture_data, 0x43))
		return false;

	if(!set_3ds_state(print_failed, capture_data, 0x40))
		return false;

	if(!read_3ds_config_3d(print_failed, capture_data, device))
		return false;

	CaptureStatus tmp_status;
	tmp_status.device = *device;
	tmp_status.connected = true;
	tmp_status.requested_3d = capture_data->status.requested_3d;
	if(!get_3d_enabled(&tmp_status)) {
		if(!set_3ds_state(print_failed, capture_data, 0x42))
			return false;

		if(!set_3ds_state(print_failed, capture_data, 0x40))
			return false;
	}

	if(ftd3_is_error_compat(handlers, ftd3_set_stream_pipe_compat(handlers, BULK_IN, ftd3_get_capture_size(&tmp_status)))) {
		capture_error_print(print_failed, capture_data, "Stream failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if(!ftd3_get_skip_initial_pipe_abort_compat(handlers)) {
		if(ftd3_is_error_compat(handlers, ftd3_abort_pipe_compat(handlers, BULK_IN))) {
			capture_error_print(print_failed, capture_data, "Abort failed");
			preemptive_close_connection(capture_data);
			return false;
		}

		if(ftd3_is_error_compat(handlers, ftd3_set_stream_pipe_compat(handlers, BULK_IN, ftd3_get_capture_size(&tmp_status)))) {
			capture_error_print(print_failed, capture_data, "Stream failed");
			preemptive_close_connection(capture_data);
			return false;
		}
	}
	return true;
}

void ftd3_capture_main_loop(CaptureData* capture_data) {
	ftd3_main_loop_compat(capture_data, BULK_IN);
}

void ftd3_capture_cleanup(CaptureData* capture_data) {
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	ftd3_is_error_compat(handlers, ftd3_abort_pipe_compat(handlers, BULK_IN));

	ftd3_close_compat(handlers);

	delete handlers;
	capture_data->handle = NULL;
}

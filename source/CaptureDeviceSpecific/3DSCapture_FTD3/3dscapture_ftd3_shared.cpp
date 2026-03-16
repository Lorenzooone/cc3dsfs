#include "3dscapture_ftd3_shared.hpp"
#include "3dscapture_ftd3_shared_general.hpp"
#include "3dscapture_ftd3_compatibility.hpp"
#include "devicecapture.hpp"
//#include "ftd3xx_symbols_renames.h"

#include <cstring>
#include <chrono>

#define BULK_OUT 0x02
#define BULK_IN 0x82
 
#define FTD3_N3DSXL_CFG_WAIT_MS 200
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
	devices_list.emplace_back(ftd3_get_serial(serial_string, curr_serial_extra_id), short_name, long_name, CAPTURE_CONN_FTD3, (void*)NULL, true, true, true, HEIGHT_3DS, TOP_WIDTH_3DS + BOT_WIDTH_3DS, HEIGHT_3DS, (TOP_WIDTH_3DS * 2) + BOT_WIDTH_3DS, N3DSXL_SAMPLES_IN, 90, BOT_WIDTH_3DS, 0, BOT_WIDTH_3DS + TOP_WIDTH_3DS, 0, 0, 0, false, VIDEO_DATA_RGB, usb_speed);
}

void list_devices_ftd3(std::vector<CaptureDevice> &devices_list, std::vector<no_access_recap_data> &no_access_list, bool* devices_allowed_scan) {
	// If more devices used this, it would need to be expanded...
	if(!devices_allowed_scan[CC_LOOPY_NEW_N3DSXL])
		return;
	ftd3_list_devices_compat(devices_list, no_access_list, valid_3dscapture_descriptions);
}

static uint64_t ftd3_get_video_in_size(bool is_3d_enabled) {
	if(!is_3d_enabled)
		return sizeof(RGB83DSVideoInputData);
	return sizeof(RGB83DSVideoInputData_3D);
}

uint64_t ftd3_get_video_in_size(CaptureData* capture_data) {
	return ftd3_get_video_in_size(get_3d_enabled(&capture_data->status));
}

uint64_t ftd3_get_video_in_size(CaptureData* capture_data, bool override_3d) {
	return ftd3_get_video_in_size(override_3d);
}

uint64_t ftd3_get_capture_size(bool is_3d_enabled) {
	if(!is_3d_enabled)
		return sizeof(FTD3_3DSCaptureReceived) & (~(EXTRA_DATA_BUFFER_FTD3XX_SIZE - 1));
	return sizeof(FTD3_3DSCaptureReceived_3D) & (~(EXTRA_DATA_BUFFER_FTD3XX_SIZE - 1));
}

uint64_t ftd3_get_capture_size(CaptureData* capture_data) {
	return ftd3_get_capture_size(get_3d_enabled(&capture_data->status));
}

static void early_data_output_update_exit(int inner_index, CaptureData* capture_data) {
	capture_data->data_buffers.ReleaseWriterBuffer(inner_index, false);
}

void data_output_update(int inner_index, size_t read_data, CaptureData* capture_data, std::chrono::time_point<std::chrono::high_resolution_clock> &base_time, bool is_3d) {
	if(is_3d && (read_data < ftd3_get_video_in_size(is_3d)) && (read_data >= ftd3_get_video_in_size(false)))
		is_3d = false;

	// Error detection for bad frames which may happen in some games...
	// Examples: Kirby Planet Robobot and M&L: Dream Team
	if(read_data > (ftd3_get_capture_size(is_3d) - ERROR_DATA_BUFFER_FTD3XX_SIZE))
		return early_data_output_update_exit(inner_index, capture_data);
	if(read_data < ftd3_get_video_in_size(is_3d))
		return early_data_output_update_exit(inner_index, capture_data);

	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	base_time = curr_time;
	capture_data->data_buffers.WriteToBuffer(NULL, read_data, diff.count(), &capture_data->status.device, inner_index, is_3d);

	if(capture_data->status.cooldown_curr_in)
		capture_data->status.cooldown_curr_in = capture_data->status.cooldown_curr_in - 1;
	// Signal that there is data available
	capture_data->status.video_wait.unlock();
	capture_data->status.audio_wait.unlock();
}

static void preemptive_close_connection(CaptureData* capture_data) {
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;
	ftd3_abort_pipe_compat(handlers, BULK_IN);
	ftd3_close_compat(handlers);
	delete handlers;
	capture_data->handle = NULL;
}

static bool set_spi_access(bool print_failed, CaptureData* capture_data, bool enable, bool skip_close = false) {
	uint8_t buf[4] = {0x40, 0x00, 0x00, 0x00};
	int transferred = 0;
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	if(enable)
		buf[1] |= 0x80;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred)) && (!skip_close)) {
		capture_error_print(print_failed, capture_data, "Setting SPI access failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	return true;
}

// Checks the upgradable firmware that is available... Not really needed, but
// it is nice documentation in case of future stuff...
static bool spi_3ds_cc_stuff(bool print_failed, CaptureData* capture_data) {
	uint8_t buf[4] = {0x80, 0x01, 0xAB, 0x00};
	uint8_t buf2[8] = {0x90, 0x08, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00};
	uint8_t in_buf[0x10];
	int transferred = 0;
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	if(!set_spi_access(print_failed, capture_data, true))
		return false;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf2, 8, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if(ftd3_is_error_compat(handlers, ftd3_read_pipe_compat(handlers, BULK_IN, in_buf, 0x10, &transferred))) {
		capture_error_print(print_failed, capture_data, "Read failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	if(!set_spi_access(print_failed, capture_data, false))
		return false;

	return true;
}

static bool read_3ds_config_3d(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	uint8_t buf[4] = {0x98, 0x05, 0x9F, 0x00};
	uint8_t in_buf[0x10];
	int transferred = 0;

	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	if(!set_spi_access(print_failed, capture_data, true))
		return false;

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

	if(transferred >= CFG_3DS_3D_SCREEN_POS) {
		if(in_buf[CFG_3DS_3D_SCREEN_POS] != 0xc1)
			device->has_3d = false;
	}

	if(!set_spi_access(print_failed, capture_data, false))
		return false;

	return true;
}

static bool load_3ds_cc_firmware(bool print_failed, CaptureData* capture_data, uint8_t firmware_id, bool is_firmware_switch) {
	uint8_t buf[4] = {0x42, 0x00, 0x00, 0x00};
	int transferred = 0;
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;

	if(firmware_id >= 2)
		firmware_id = 1;

	if(!set_spi_access(print_failed, capture_data, true))
		return false;

	buf[0] += firmware_id;

	if(ftd3_is_error_compat(handlers, ftd3_write_pipe_compat(handlers, BULK_OUT, buf, 4, &transferred))) {
		capture_error_print(print_failed, capture_data, "Write failed");
		preemptive_close_connection(capture_data);
		return false;
	}

	default_sleep(FTD3_N3DSXL_CFG_WAIT_MS);

	if(is_firmware_switch) {
		// WHY DOES ADDING THIS HERE ENABLE SOUND WHEN SWITCHING FIRMWARE?!
		if(!set_spi_access(print_failed, capture_data, true))
			return false;
	}

	if(!set_spi_access(print_failed, capture_data, false))
		return false;

	return true;
}

// Try draining frames without doing any safety checks...
// Should "fix" some issues with the CC not answering, at times.
static void drain_data(bool print_failed, CaptureData* capture_data) {
	set_spi_access(print_failed, capture_data, true, true);

	uint8_t* in_buf = new uint8_t[0x100000];
	int transferred = 0;
	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;
	const auto base_time = std::chrono::high_resolution_clock::now();

	ftd3_read_pipe_compat(handlers, BULK_IN, in_buf, 0x100000, &transferred, FTD3_N3DSXL_CFG_WAIT_MS);

	const auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - base_time;
	int ms_passed = (int)(diff.count() * 1000.0);
	if(ms_passed < FTD3_N3DSXL_CFG_WAIT_MS)
		default_sleep((float)(FTD3_N3DSXL_CFG_WAIT_MS - ms_passed + 1));

	delete []in_buf;
}

bool connect_ftd3(bool print_failed, CaptureData* capture_data, CaptureDevice* device) {
	capture_data->handle = ftd3_reconnect_compat(device->serial_number, valid_3dscapture_descriptions);
	if(capture_data->handle == NULL) {
		capture_error_print(print_failed, capture_data, "Create failed");
		return false;
	}
	drain_data(print_failed, capture_data);
	preemptive_close_connection(capture_data);

	capture_data->handle = ftd3_reconnect_compat(device->serial_number, valid_3dscapture_descriptions);
	if(capture_data->handle == NULL) {
		capture_error_print(print_failed, capture_data, "Second Create failed");
		return false;
	}
	
	if(!spi_3ds_cc_stuff(print_failed, capture_data))
		return false;

	if(!load_3ds_cc_firmware(print_failed, capture_data, 1, false))
		return false;

	if(!read_3ds_config_3d(print_failed, capture_data, device))
		return false;

	return true;
}

bool ftd3_capture_3d_setup(CaptureData* capture_data, bool first_pass, bool& stored_3d_status, bool is_driver) {
	CaptureStatus tmp_status;
	tmp_status.device = capture_data->status.device;
	tmp_status.connected = capture_data->status.connected;
	tmp_status.requested_3d = capture_data->status.requested_3d;

	if(first_pass)
		stored_3d_status = true;

	if((!first_pass) && ((stored_3d_status == tmp_status.requested_3d) || (!get_3d_enabled(&tmp_status, true)))) {
		stored_3d_status = tmp_status.requested_3d;
		return true;
	}

	ftd3_device_device_handlers* handlers = (ftd3_device_device_handlers*)capture_data->handle;
	bool print_failed = true;

	if(!first_pass) {
		if(ftd3_is_error_compat(handlers, ftd3_abort_pipe_compat(handlers, BULK_IN))) {
			capture_error_print(print_failed, capture_data, "Abort failed");
			preemptive_close_connection(capture_data);
			return false;
		}
	}

	bool _3d_enabled_result = get_3d_enabled(&tmp_status);
	bool update_state = stored_3d_status != _3d_enabled_result;
	if(tmp_status.device.has_3d && update_state) {
		uint8_t firmware_to_use = 0;
		if(_3d_enabled_result)
			firmware_to_use = 1;
		if(!load_3ds_cc_firmware(print_failed, capture_data, firmware_to_use, true))
			return false;
	}

	if(ftd3_is_error_compat(handlers, ftd3_set_stream_pipe_compat(handlers, BULK_IN, (size_t)ftd3_get_capture_size(_3d_enabled_result || (first_pass && is_driver))))) {
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

		if(ftd3_is_error_compat(handlers, ftd3_set_stream_pipe_compat(handlers, BULK_IN, (size_t)ftd3_get_capture_size(_3d_enabled_result || (first_pass && is_driver))))) {
			capture_error_print(print_failed, capture_data, "Stream failed");
			preemptive_close_connection(capture_data);
			return false;
		}
	}
	stored_3d_status = tmp_status.requested_3d;
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

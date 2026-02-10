#include "devicecapture.hpp"
#include "3dscapture_ftd3_shared.hpp"
#include "dscapture_ftd2_shared.hpp"
#include "usb_ds_3ds_capture.hpp"
#include "usb_is_device_acquisition.hpp"
#include "cypress_partner_ctr_acquisition.hpp"
#include "cypress_nisetro_acquisition.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>

#define CONNECTION_NO_DEVICE_SELECTED (-1)
#define NO_SERIAL_KEY_STR "No Serial Key"

static bool poll_connection_window_screen(WindowScreen *screen, int &chosen_index) {
	screen->poll();
	if(screen->check_connection_menu_result() != CONNECTION_MENU_NO_ACTION) {
		chosen_index = screen->check_connection_menu_result();
		return true;
	}
	if(screen->close_capture())
		return true;
	return false;
}

static int choose_device(std::vector<CaptureDevice> *devices_list, FrontendData* frontend_data, bool auto_connect_to_first) {
	if(devices_list->size() == 1)
		return 0;
	if(auto_connect_to_first)
		return 0;
	int chosen_index = CONNECTION_NO_DEVICE_SELECTED;
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

void setup_reconnection_device(void* info) {
	FrontendData* frontend_data = static_cast<FrontendData*>(info);
	frontend_data->top_screen->setup_reconnection_menu();
	frontend_data->bot_screen->setup_reconnection_menu();
	frontend_data->joint_screen->setup_reconnection_menu();
}

bool wait_reconnection_device(void* info) {
	FrontendData* frontend_data = static_cast<FrontendData*>(info);
	update_output(frontend_data);
	frontend_data->top_screen->poll();
	frontend_data->bot_screen->poll();
	frontend_data->joint_screen->poll();
	if(frontend_data->top_screen->close_capture())
		return false;
	if(frontend_data->bot_screen->close_capture())
		return false;
	if(frontend_data->joint_screen->close_capture())
		return false;
	return true;
}

void end_reconnection_device(void* info) {
	FrontendData* frontend_data = static_cast<FrontendData*>(info);
	frontend_data->top_screen->end_reconnection_menu();
	frontend_data->bot_screen->end_reconnection_menu();
	frontend_data->joint_screen->end_reconnection_menu();
}

void capture_error_print(bool print_failed, CaptureData* capture_data, std::string error_string) {
	capture_error_print(print_failed, capture_data, error_string, error_string);
}

void capture_error_print(bool print_failed, CaptureData* capture_data, std::string graphical_string, std::string detailed_string) {
	if(capture_data->status.connected) {
		capture_data->status.close_success = false;
		capture_data->status.connected = false;
	}
	if(print_failed) {
		capture_data->status.graphical_error_text = graphical_string;
		capture_data->status.detailed_error_text = detailed_string;
		capture_data->status.new_error_text = true;
	}
}

void capture_warning_print(CaptureData* capture_data, std::string graphical_string, std::string detailed_string) {
	capture_data->status.graphical_error_text = graphical_string;
	capture_data->status.detailed_error_text = detailed_string;
	capture_data->status.new_error_text = true;
}

void capture_warning_print(CaptureData* capture_data, std::string warning_string) {
	capture_warning_print(capture_data, warning_string, warning_string);
}

bool connect(bool print_failed, CaptureData* capture_data, FrontendData* frontend_data, bool* force_cc_disables, bool auto_connect_to_first) {
	capture_data->status.new_error_text = false;
	if (capture_data->status.connected) {
		capture_data->status.close_success = false;
		return false;
	}

	if(!capture_data->status.close_success) {
		capture_error_print(print_failed, capture_data, "Previous device still closing...");
		return false;
	}

	capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	// Device Listing
	std::vector<CaptureDevice> devices_list;
	std::vector<no_access_recap_data> no_access_list;
	bool devices_allowed_scan[CC_POSSIBLE_DEVICES_END];
	for(size_t i = 0; i < CC_POSSIBLE_DEVICES_END; i++)
		devices_allowed_scan[i] = capture_data->status.devices_allowed_scan[i] & (!force_cc_disables[i]);

	#ifdef USE_CYNI_USB
	list_devices_cyni_device(devices_list, no_access_list, devices_allowed_scan);
	#endif
	#ifdef USE_CYPRESS_OPTIMIZE
	list_devices_cyop_device(devices_list, no_access_list, devices_allowed_scan);
	#endif
	#ifdef USE_FTD3
	list_devices_ftd3(devices_list, no_access_list, devices_allowed_scan);
	#endif
	#ifdef USE_FTD2
	list_devices_ftd2_shared(devices_list, no_access_list, devices_allowed_scan);
	#endif
	#ifdef USE_DS_3DS_USB
	list_devices_usb_ds_3ds(devices_list, no_access_list, devices_allowed_scan);
	#endif
	#ifdef USE_IS_DEVICES_USB
	list_devices_is_device(devices_list, no_access_list, devices_allowed_scan);
	#endif
	#ifdef USE_PARTNER_CTR
	list_devices_cypart_device(devices_list, no_access_list, devices_allowed_scan);
	#endif

	if(devices_list.size() <= 0) {
		if(no_access_list.size() <= 0)
			capture_error_print(print_failed, capture_data, "No device was found");
		else {
			std::string full_error_part = "";
			for(size_t i = 0; i < no_access_list.size(); i++) {
				full_error_part += " - ";
				if(no_access_list[i].vid == -1)
					full_error_part += no_access_list[i].name;
				else
					full_error_part += "VID: " + to_hex_u16(no_access_list[i].vid) + ", PID: " + to_hex_u16(no_access_list[i].pid);
			}
			#ifdef _WIN32
			std::string base_error_string = "No device was found\nPossible driver issue";
			std::string long_error_string = "No device was found - Possible driver issue" + full_error_part;
			#else
			std::string base_error_string = "No device was found\nPossible permission error";
			std::string long_error_string = "No device was found - Possible permission error" + full_error_part;
			#endif
			capture_error_print(print_failed, capture_data, base_error_string, long_error_string);
		}
		return false;
	}

	int chosen_device = choose_device(&devices_list, frontend_data, auto_connect_to_first);
	if(chosen_device == CONNECTION_NO_DEVICE_SELECTED) {
		capture_error_print(print_failed, capture_data, "No device was selected");
		return false;
	}

	// Actual connection
	#ifdef USE_CYNI_USB
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_CYPRESS_NISETRO) && (!cyni_device_connect_usb(print_failed, capture_data, &devices_list[chosen_device], frontend_data)))
		return false;
	#endif
	#ifdef USE_CYPRESS_OPTIMIZE
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE) && (!cyop_device_connect_usb(print_failed, capture_data, &devices_list[chosen_device], frontend_data)))
		return false;
	#endif
	#ifdef USE_FTD3
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_FTD3) && (!connect_ftd3(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	#ifdef USE_FTD2
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_FTD2) && (!connect_ftd2_shared(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	#ifdef USE_DS_3DS_USB
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_USB) && (!connect_usb(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	#ifdef USE_IS_DEVICES_USB
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_IS_NITRO) && (!is_device_connect_usb(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	#ifdef USE_PARTNER_CTR
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_PARTNER_CTR) && (!cypart_device_connect_usb(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	update_connected_3ds_ds(frontend_data, capture_data->status.device, devices_list[chosen_device]);
	capture_data->status.device = devices_list[chosen_device];

	// Avoid having old open locks
	capture_data->status.video_wait.try_lock();
	capture_data->status.audio_wait.try_lock();

	return true;
}

void captureCall(CaptureData* capture_data) {
	capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;

	while(capture_data->status.running) {
		if (!capture_data->status.connected) {
			default_sleep();
			continue;
		}

		// Main capture loop
		#ifdef USE_CYNI_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_CYPRESS_NISETRO)
			cyni_device_acquisition_main_loop(capture_data);
		#endif
		#ifdef USE_CYPRESS_OPTIMIZE
		if(capture_data->status.device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE)
			cyop_device_acquisition_main_loop(capture_data);
		#endif
		#ifdef USE_FTD3
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3)
			ftd3_capture_main_loop(capture_data);
		#endif
		#ifdef USE_FTD2
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2)
			ftd2_capture_main_loop_shared(capture_data);
		#endif
		#ifdef USE_DS_3DS_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_USB)
			usb_capture_main_loop(capture_data);
		#endif
		#ifdef USE_IS_DEVICES_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_IS_NITRO)
			is_device_acquisition_main_loop(capture_data);
		#endif
		#ifdef USE_PARTNER_CTR
		if(capture_data->status.device.cc_type == CAPTURE_CONN_PARTNER_CTR)
			cypart_device_acquisition_main_loop(capture_data);
		#endif

		capture_data->status.close_success = false;
		capture_data->status.connected = false;
		capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
		// Needed in case the threads went in the connected loop
		// before it is set right above, and they are waiting on the locks!
		capture_data->status.video_wait.unlock();
		capture_data->status.audio_wait.unlock();

		// Capture cleanup
		#ifdef USE_CYNI_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_CYPRESS_NISETRO)
			usb_cyni_device_acquisition_cleanup(capture_data);
		#endif
		#ifdef USE_CYPRESS_OPTIMIZE
		if(capture_data->status.device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE)
			usb_cyop_device_acquisition_cleanup(capture_data);
		#endif
		#ifdef USE_FTD3
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3)
			ftd3_capture_cleanup(capture_data);
		#endif
		#ifdef USE_FTD2
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2)
			ftd2_capture_cleanup_shared(capture_data);
		#endif
		#ifdef USE_DS_3DS_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_USB)
			usb_capture_cleanup(capture_data);
		#endif
		#ifdef USE_IS_DEVICES_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_IS_NITRO)
			usb_is_device_acquisition_cleanup(capture_data);
		#endif
		#ifdef USE_PARTNER_CTR
		if(capture_data->status.device.cc_type == CAPTURE_CONN_PARTNER_CTR)
			usb_cypart_device_acquisition_cleanup(capture_data);
		#endif

		capture_data->status.close_success = false;
		capture_data->status.connected = false;

		capture_data->status.close_success = true;
	}
}

uint64_t get_audio_n_samples(CaptureData* capture_data, CaptureDataSingleBuffer* data_buffer) {
	if(!capture_data->status.device.has_audio)
		return 0;
	#ifdef USE_CYPRESS_OPTIMIZE
	if(capture_data->status.device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE)
		return -1;
	#endif
	uint64_t n_samples = (data_buffer->read - get_video_in_size(capture_data, data_buffer->is_3d, data_buffer->should_be_3d, data_buffer->buffer_video_data_type)) / 2;
	if(n_samples > capture_data->status.device.max_samples_in)
		n_samples = capture_data->status.device.max_samples_in;
	// Avoid entering a glitched state due to a partial packet or something
	if((n_samples % 2) != 0)
		n_samples -= 1;
	return n_samples;
}

uint64_t get_video_in_size(CaptureData* capture_data, bool is_3d, bool should_be_3d, InputVideoDataType video_data_type) {
	#ifdef USE_CYNI_USB
	if(capture_data->status.device.cc_type == CAPTURE_CONN_CYPRESS_NISETRO)
		return cyni_device_get_video_in_size(capture_data);
	#endif
	#ifdef USE_CYPRESS_OPTIMIZE
	if(capture_data->status.device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE)
		return cyop_device_get_video_in_size(capture_data, is_3d, should_be_3d, video_data_type);
	#endif
	#ifdef USE_FTD3
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3)
		return ftd3_get_video_in_size(capture_data, is_3d);
	#endif
	#ifdef USE_FTD2
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2)
		return ftd2_get_video_in_size(capture_data);
	#endif
	#ifdef USE_DS_3DS_USB
	if(capture_data->status.device.cc_type == CAPTURE_CONN_USB)
		return usb_get_video_in_size(capture_data, is_3d);
	#endif
	#ifdef USE_IS_DEVICES_USB
	if(capture_data->status.device.cc_type == CAPTURE_CONN_IS_NITRO)
		return usb_is_device_get_video_in_size(capture_data);
	#endif
	#ifdef USE_PARTNER_CTR
	if(capture_data->status.device.cc_type == CAPTURE_CONN_PARTNER_CTR)
		return cypart_device_get_video_in_size(capture_data, is_3d);
	#endif
	return 0;
}

std::string get_device_id_string(CaptureStatus* capture_status) {
	if(!capture_status->connected)
		return "";
	if(capture_status->device.device_id == 0)
		return "";

	if(capture_status->device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE)
		return to_hex_u60(capture_status->device.device_id);

	return "";
}

std::string get_device_serial_key_string(CaptureStatus* capture_status) {
	if(!capture_status->connected)
		return NO_SERIAL_KEY_STR;
	if(capture_status->device.device_id == 0)
		return NO_SERIAL_KEY_STR;
	if(capture_status->device.key == "")
		return NO_SERIAL_KEY_STR;

	return capture_status->device.key;
}

void check_device_serial_key_update(CaptureStatus* capture_status, bool differentiator, std::string key) {
	if(get_device_serial_key_string(capture_status) != NO_SERIAL_KEY_STR)
		return;
	if(capture_status->key_updated)
		return;

	switch(capture_status->device.cc_type) {
		case CAPTURE_CONN_CYPRESS_OPTIMIZE:
			#ifdef USE_CYPRESS_OPTIMIZE
			if(differentiator != is_device_optimize_n3ds(&capture_status->device))
				break;
			if(cyop_is_key_for_device_id(&capture_status->device, key))
				capture_status->key_updated = true;
			#endif
			break;
		default:
			break;
	}
}

std::string get_name_of_device(CaptureStatus* capture_status, bool use_long, bool want_real_serial) {
	if(!capture_status->connected)
		return "Not connected";
	std:: string serial_str = " - " + capture_status->device.serial_number;
	std::string device_id_string = get_device_id_string(capture_status);
	if(want_real_serial && (device_id_string != ""))
		serial_str = " - " + device_id_string;
	if(!use_long)
		return capture_status->device.name + serial_str;
	return capture_status->device.long_name + serial_str;
}

int get_usb_speed_of_device(CaptureStatus* capture_status) {
	if(!capture_status->connected)
		return 0;
	return capture_status->device.usb_speed >> 8;
}

bool is_usb_speed_of_device_enough_3d(CaptureStatus* capture_status) {
	switch(capture_status->device.cc_type) {
		case CAPTURE_CONN_FTD3:
			if(get_usb_speed_of_device(capture_status) < 3)
				return false;
			break;
		default:
			break;
	}
	return true;
}

bool get_device_can_do_3d(CaptureStatus* capture_status) {
	if(!capture_status->connected)
		return false;
	if(!capture_status->device.is_3ds)
		return false;
	return capture_status->device.has_3d;
}

bool get_device_3d_implemented(CaptureStatus* capture_status) {
	if(!capture_status->connected)
		return false;
	switch(capture_status->device.cc_type) {
		case CAPTURE_CONN_FTD3:
			return true;
		case CAPTURE_CONN_CYPRESS_OPTIMIZE:
			return true;
		case CAPTURE_CONN_PARTNER_CTR:
			return true;
		default:
			return false;
	}
}

bool get_3d_enabled(CaptureStatus* capture_status, bool skip_requested_3d_check) {
	if((!skip_requested_3d_check) && (!capture_status->requested_3d))
		return false;
	if(!capture_status->connected)
		return false;
	if(!get_device_can_do_3d(capture_status))
		return false;
	if(!get_device_3d_implemented(capture_status))
		return false;
	if(!is_usb_speed_of_device_enough_3d(capture_status))
		return false;
	return true;
}

bool set_3d_enabled(CaptureStatus* capture_status, bool new_value) {
	bool would_3d_be_enabled = get_3d_enabled(capture_status, true);
	if(would_3d_be_enabled && (new_value != capture_status->requested_3d) && (capture_status->cooldown_curr_in < FIX_PARTIAL_FIRST_FRAME_NUM))
		capture_status->cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
	capture_status->requested_3d = new_value;
	return would_3d_be_enabled;
}

bool update_3d_enabled(CaptureStatus* capture_status) {
	return set_3d_enabled(capture_status, !capture_status->requested_3d);
}

float get_framerate_multiplier(CaptureStatus* capture_status) {
	if(!capture_status->connected)
		return 1;
	if(capture_status->device.cc_type != CAPTURE_CONN_CYPRESS_OPTIMIZE)
		return 1;
	if(!get_3d_enabled(capture_status))
		return 1;
	if(capture_status->request_low_bw_format)
		return 1;
	return 0.5;
}

KeySaveError save_cc_key(std::string key, CaptureConnectionType conn_type, bool differentiator) {
	KeySaveError error = KEY_SAVE_METHOD_NOT_FOUND;

	switch(conn_type) {
		case CAPTURE_CONN_CYPRESS_OPTIMIZE:
			#ifdef USE_CYPRESS_OPTIMIZE
			error = add_key_to_file(key, differentiator);
			#endif
			break;
		default:
			break;
	}

	return error;
}

void capture_init() {
	#ifdef USE_DS_3DS_USB
	usb_ds_3ds_init();
	#endif
	#ifdef USE_IS_DEVICES_USB
	usb_is_device_init();
	#endif
	#ifdef USE_PARTNER_CTR
	usb_cypart_device_init();
	#endif
	#ifdef USE_FTD2
	ftd2_init_shared();
	#endif
	#ifdef USE_CYNI_USB
	usb_cyni_device_init();
	#endif
	#ifdef USE_CYPRESS_OPTIMIZE
	usb_cyop_device_init();
	#endif
}

void capture_close() {
	#ifdef USE_DS_3DS_USB
	usb_ds_3ds_close();
	#endif
	#ifdef USE_IS_DEVICES_USB
	usb_is_device_close();
	#endif
	#ifdef USE_PARTNER_CTR
	usb_cypart_device_close();
	#endif
	#ifdef USE_FTD2
	ftd2_end_shared();
	#endif
	#ifdef USE_CYNI_USB
	usb_cyni_device_close();
	#endif
	#ifdef USE_CYPRESS_OPTIMIZE
	usb_cyop_device_close();
	#endif
}

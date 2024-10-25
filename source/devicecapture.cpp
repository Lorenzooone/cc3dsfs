#include "devicecapture.hpp"
#include "3dscapture_ftd3.hpp"
#include "dscapture_ftd2.hpp"
#include "usb_ds_3ds_capture.hpp"
#include "usb_is_nitro_acquisition.hpp"

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>

#define CONNECTION_NO_DEVICE_SELECTED (-1)

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

static int choose_device(std::vector<CaptureDevice> *devices_list, FrontendData* frontend_data) {
	if(devices_list->size() == 1)
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

void capture_error_print(bool print_failed, CaptureData* capture_data, std::string error_string) {
	if(print_failed) {
		capture_data->status.error_text = error_string;
		capture_data->status.new_error_text = true;
	}
}

bool connect(bool print_failed, CaptureData* capture_data, FrontendData* frontend_data) {
	capture_data->status.new_error_text = false;
	if (capture_data->status.connected) {
		capture_data->status.close_success = false;
		return false;
	}
	
	if(!capture_data->status.close_success) {
		capture_error_print(print_failed, capture_data, "Previous device still closing...");
		return false;
	}

	// Device Listing
	std::vector<CaptureDevice> devices_list;
	#ifdef USE_FTD3
	list_devices_ftd3(devices_list);
	#endif
	#ifdef USE_FTD2
	list_devices_ftd2(devices_list);
	#endif
	#ifdef USE_DS_3DS_USB
	list_devices_usb_ds_3ds(devices_list);
	#endif
	#ifdef USE_IS_NITRO_USB
	list_devices_is_nitro(devices_list);
	#endif

	if(devices_list.size() <= 0) {
		capture_error_print(print_failed, capture_data, "No device was found");
		return false;
	}

	int chosen_device = choose_device(&devices_list, frontend_data);
	if(chosen_device == CONNECTION_NO_DEVICE_SELECTED) {
		capture_error_print(print_failed, capture_data, "No device was selected");
		return false;
	}

	// Actual connection
	#ifdef USE_FTD3
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_FTD3) && (!connect_ftd3(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	#ifdef USE_FTD2
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_FTD2) && (!connect_ftd2(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	#ifdef USE_DS_3DS_USB
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_USB) && (!connect_usb(print_failed, capture_data, &devices_list[chosen_device])))
		return false;
	#endif
	#ifdef USE_IS_NITRO_USB
	if((devices_list[chosen_device].cc_type == CAPTURE_CONN_IS_NITRO) && (!is_nitro_connect_usb(print_failed, capture_data, &devices_list[chosen_device])))
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
	int inner_curr_in = 0;
	capture_data->status.curr_in = inner_curr_in;
	capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;

	while(capture_data->status.running) {
		if (!capture_data->status.connected) {
			default_sleep();
			continue;
		}

		// Main capture loop
		#ifdef USE_FTD3
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3)
			ftd3_capture_main_loop(capture_data);
		#endif
		#ifdef USE_FTD2
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2)
			ftd2_capture_main_loop(capture_data);
		#endif
		#ifdef USE_DS_3DS_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_USB)
			usb_capture_main_loop(capture_data);
		#endif
		#ifdef USE_IS_NITRO_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_IS_NITRO)
			is_nitro_acquisition_main_loop(capture_data);
		#endif

		capture_data->status.close_success = false;
		capture_data->status.connected = false;
		capture_data->status.cooldown_curr_in = FIX_PARTIAL_FIRST_FRAME_NUM;
		// Needed in case the threads went in the connected loop
		// before it is set right above, and they are waiting on the locks!
		capture_data->status.video_wait.unlock();
		capture_data->status.audio_wait.unlock();

		// Capture cleanup
		#ifdef USE_FTD3
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3)
			ftd3_capture_cleanup(capture_data);
		#endif
		#ifdef USE_FTD2
		if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2)
			ftd2_capture_cleanup(capture_data);
		#endif
		#ifdef USE_DS_3DS_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_USB)
			usb_capture_cleanup(capture_data);
		#endif
		#ifdef USE_IS_NITRO_USB
		if(capture_data->status.device.cc_type == CAPTURE_CONN_IS_NITRO)
			usb_is_nitro_acquisition_cleanup(capture_data);
		#endif

		capture_data->status.close_success = false;
		capture_data->status.connected = false;

		capture_data->status.close_success = true;
	}
}

uint64_t get_audio_n_samples(CaptureData* capture_data, uint64_t read) {
	if(!capture_data->status.device.has_audio)
		return 0;
	uint64_t n_samples = (read - get_video_in_size(capture_data)) / 2;
	if(n_samples > capture_data->status.device.max_samples_in)
		n_samples = capture_data->status.device.max_samples_in;
	return n_samples;
}

uint64_t get_video_in_size(CaptureData* capture_data) {
	#ifdef USE_FTD3
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD3)
		return ftd3_get_video_in_size(capture_data);
	#endif
	#ifdef USE_FTD2
	if(capture_data->status.device.cc_type == CAPTURE_CONN_FTD2)
		return ftd2_get_video_in_size(capture_data);
	#endif
	#ifdef USE_DS_3DS_USB
	if(capture_data->status.device.cc_type == CAPTURE_CONN_USB)
		return usb_get_video_in_size(capture_data);
	#endif
	#ifdef USE_IS_NITRO_USB
	if(capture_data->status.device.cc_type == CAPTURE_CONN_IS_NITRO)
		return usb_is_nitro_get_video_in_size(capture_data);
	#endif
	return 0;
}

void capture_init() {
	#ifdef USE_DS_3DS_USB
	usb_ds_3ds_init();
	#endif
	#ifdef USE_IS_NITRO_USB
	usb_is_nitro_init();
	#endif
}

void capture_close() {
	#ifdef USE_DS_3DS_USB
	usb_ds_3ds_close();
	#endif
	#ifdef USE_IS_NITRO_USB
	usb_is_nitro_close();
	#endif
}

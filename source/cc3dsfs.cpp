#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#if defined (__linux__) && defined(XLIB_BASED)
#include <X11/Xlib.h>
#endif

#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <queue>

#include "utils.hpp"
#include "3dscapture.hpp"
#include "hw_defs.hpp"
#include "frontend.hpp"
#include "audio.hpp"
#include "conversions.hpp"

// Threshold to keep the audio latency limited in "amount of frames".
#define AUDIO_LATENCY_LIMIT 4
#define NUM_CONCURRENT_AUDIO_BUFFERS ((AUDIO_LATENCY_LIMIT * 2) + 1)

struct OutTextData {
	std::string full_text;
	std::string small_text;
	bool consumed;
	TextKind kind;
};

static void ConsoleOutText(std::string full_text) {
	if(full_text != "")
		std::cout << "[" << NAME << "] " << full_text << std::endl;
}

static void UpdateOutText(OutTextData &out_text_data, std::string full_text, std::string small_text, TextKind kind) {
	if(!out_text_data.consumed)
		ConsoleOutText(out_text_data.full_text);
	out_text_data.full_text = full_text;
	out_text_data.small_text = small_text;
	out_text_data.kind = kind;
	out_text_data.consumed = false;
}

static void SuccessConnectionOutTextGenerator(OutTextData &out_text_data, CaptureData* capture_data) {
	if(capture_data->status.connected)
		UpdateOutText(out_text_data, "Connected to " + capture_data->status.serial_number, "Connected", TEXT_KIND_SUCCESS);
	else if(!capture_data->status.new_error_text) {
		UpdateOutText(out_text_data, "Disconnected", "Disconnected", TEXT_KIND_WARNING);
	}
}

static bool load(const std::string path, const std::string name, ScreenInfo &top_info, ScreenInfo &bottom_info, ScreenInfo &joint_info, DisplayData &display_data, AudioData *audio_data, OutTextData &out_text_data) {
	std::ifstream file(path + name);
	std::string line;

	if(!file.good()) {
		UpdateOutText(out_text_data, "File " + path + name + " load failed.\nDefaults re-loaded.", "Load failed\nDefaults re-loaded", TEXT_KIND_ERROR);
		return false;
	}

	bool result = true;

	try {
		while(std::getline(file, line)) {
			std::istringstream kvp(line);
			std::string key;

			if(std::getline(kvp, key, '=')) {
				std::string value;

				if(std::getline(kvp, value)) {

					if(load_screen_info(key, value, "bot_", bottom_info))
						continue;
					if(load_screen_info(key, value, "joint_", joint_info))
						continue;
					if(load_screen_info(key, value, "top_", top_info))
						continue;

					if(key == "split") {
						display_data.split = std::stoi(value);
						continue;
					}

					if(audio_data->load_audio_data(key, value))
						continue;
				}
			}
		}
	}
	catch(...) {
		UpdateOutText(out_text_data, "File " + path + name + " load failed.\nDefaults re-loaded.", "Load failed\nDefaults re-loaded", TEXT_KIND_ERROR);
		result = false;
	}

	file.close();
	return result;
}

static void defaults_reload(FrontendData *frontend_data, AudioData* audio_data) {
	reset_screen_info(frontend_data->top_screen->m_info);
	reset_screen_info(frontend_data->bot_screen->m_info);
	reset_screen_info(frontend_data->joint_screen->m_info);
	audio_data->reset();
	reset_display_data(&frontend_data->display_data);
	if(frontend_data->display_data.mono_app_mode)
		frontend_data->joint_screen->m_info.is_fullscreen = true;
	frontend_data->reload = true;
}

static void load_layout_file(int load_index, FrontendData *frontend_data, AudioData* audio_data, OutTextData &out_text_data, bool skip_io, bool do_print) {
	if(skip_io)
		return;

	defaults_reload(frontend_data, audio_data);

	if(load_index == SIMPLE_RESET_DATA_INDEX) {
		UpdateOutText(out_text_data, "Reset detected. Defaults re-loaded", "Reset detected\nDefaults re-loaded", TEXT_KIND_WARNING);
		return;
	}

	bool name_load_success = false;
	std::string layout_name = LayoutNameGenerator(load_index);
	std::string layout_path = LayoutPathGenerator(load_index);
	bool op_success = load(layout_path, layout_name, frontend_data->top_screen->m_info, frontend_data->bot_screen->m_info, frontend_data->joint_screen->m_info, frontend_data->display_data, audio_data, out_text_data);
	if(do_print && op_success) {
		std::string load_name = load_layout_name(load_index, name_load_success);
		UpdateOutText(out_text_data, "Layout loaded from: " + layout_path + layout_name, "Layout " + load_name + " loaded", TEXT_KIND_SUCCESS);
	}
	else if(!op_success)
		defaults_reload(frontend_data, audio_data);
}

static bool save(const std::string path, const std::string name, const std::string save_name, const ScreenInfo &top_info, const ScreenInfo &bottom_info, const ScreenInfo &joint_info, DisplayData &display_data, AudioData *audio_data, OutTextData &out_text_data) {
	#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
	std::filesystem::create_directories(path);
	#else
	std::experimental::filesystem::create_directories(path);
	#endif
	std::ofstream file(path + name);
	if(!file.good()) {
		UpdateOutText(out_text_data, "File " + path + name + " save failed.", "Save failed", TEXT_KIND_ERROR);
		return false;
	}

	file << "name=" << save_name << std::endl;
	file << save_screen_info("bot_", bottom_info);
	file << save_screen_info("joint_", joint_info);
	file << save_screen_info("top_", top_info);
	file << "split=" << display_data.split << std::endl;
	file << audio_data->save_audio_data();

	file.close();
	return true;
}

static void save_layout_file(int save_index, FrontendData *frontend_data, AudioData* audio_data, OutTextData &out_text_data, bool skip_io, bool do_print) {
	if(skip_io)
		return;

	if(save_index == SIMPLE_RESET_DATA_INDEX)
		return;

	bool name_load_success = false;
	std::string save_name = load_layout_name(save_index, name_load_success);
	std::string layout_name = LayoutNameGenerator(save_index);
	std::string layout_path = LayoutPathGenerator(save_index);
	bool op_success = save(layout_path, layout_name, save_name, frontend_data->top_screen->m_info, frontend_data->bot_screen->m_info, frontend_data->joint_screen->m_info, frontend_data->display_data, audio_data, out_text_data);
	if(do_print && op_success) {
		UpdateOutText(out_text_data, "Layout saved to: " + layout_path + layout_name, "Layout " + save_name + " saved", TEXT_KIND_SUCCESS);
	}
}

static void soundCall(AudioData *audio_data, CaptureData* capture_data) {
	Audio audio(audio_data);
	sf::Int16 out_buf[NUM_CONCURRENT_AUDIO_BUFFERS][MAX_SAMPLES_IN];
	int curr_out, prev_out = NUM_CONCURRENT_DATA_BUFFERS - 1, audio_buf_counter = 0;
	const bool endianness = is_big_endian();
	volatile int loaded_samples;

	while(capture_data->status.running) {
		if(capture_data->status.connected) {
			bool timed_out = !capture_data->status.audio_wait.timed_lock();
			curr_out = (capture_data->status.curr_in - 1 + NUM_CONCURRENT_DATA_BUFFERS) % NUM_CONCURRENT_DATA_BUFFERS;

			if((!capture_data->status.cooldown_curr_in) && (curr_out != prev_out)) {
				loaded_samples = audio.samples.size();
				if((capture_data->read[curr_out] > sizeof(VideoInputData)) && (loaded_samples < AUDIO_LATENCY_LIMIT)) {
					int n_samples = (capture_data->read[curr_out] - sizeof(VideoInputData)) / 2;
					if(n_samples > MAX_SAMPLES_IN) {
						n_samples = MAX_SAMPLES_IN;
					}
					double out_time = capture_data->time_in_buf[curr_out];
					convertAudioToOutput(&capture_data->capture_buf[curr_out], out_buf[audio_buf_counter], n_samples, endianness);
					audio.samples.emplace(out_buf[audio_buf_counter], n_samples, out_time);
					if(++audio_buf_counter == NUM_CONCURRENT_AUDIO_BUFFERS) {
						audio_buf_counter = 0;
					}
					audio.samples_wait.unlock();
				}
			}
			prev_out = curr_out;
			
			loaded_samples = audio.samples.size();
			if(audio.getStatus() != sf::SoundStream::Playing) {
				audio.stop_audio();
				if(loaded_samples > 0) {
					if(audio.restart) {
						audio.stop();
						audio.~Audio();
						::new(&audio) Audio(audio_data);
					}
					audio.start_audio();
					audio.play();
				}
			}
			else {
				audio.update_volume();
			}
		}
		else {
			audio.stop_audio();
			audio.stop();
			default_sleep();
		}
	}

	audio.stop_audio();
	audio.stop();
}

static void check_close_application(WindowScreen &screen, CaptureData* capture_data, int &ret_val) {
	if(screen.close_capture() && capture_data->status.running) {
		capture_data->status.running = false;
		ret_val = screen.get_ret_val();
	}
}

static int mainVideoOutputCall(AudioData* audio_data, CaptureData* capture_data, bool mono_app) {
	VideoOutputData *out_buf;
	double last_frame_time = 0.0;
	int num_elements_fps_array = 0;
	int curr_out, prev_out = NUM_CONCURRENT_DATA_BUFFERS - 1;
	FrontendData frontend_data;
	reset_display_data(&frontend_data.display_data);
	frontend_data.display_data.mono_app_mode = mono_app;
	frontend_data.reload = true;
	bool skip_io = false;
	int num_allowed_blanks = MAX_ALLOWED_BLANKS;
	std::mutex events_access;
	OutTextData out_text_data;
	out_text_data.consumed = true;
	int ret_val = 0;

	out_buf = new VideoOutputData;
	memset(out_buf, 0, sizeof(VideoOutputData));

	WindowScreen top_screen(ScreenType::TOP, &capture_data->status, &frontend_data.display_data, audio_data, &events_access);
	WindowScreen bot_screen(ScreenType::BOTTOM, &capture_data->status, &frontend_data.display_data, audio_data, &events_access);
	WindowScreen joint_screen(ScreenType::JOINT, &capture_data->status, &frontend_data.display_data, audio_data, &events_access);
	frontend_data.top_screen = &top_screen;
	frontend_data.bot_screen = &bot_screen;
	frontend_data.joint_screen = &joint_screen;

	load_layout_file(STARTUP_FILE_INDEX, &frontend_data, audio_data, out_text_data, skip_io, false);
	// Due to the risk for seizures, at the start of the program, set BFI to false!
	top_screen.m_info.bfi = false;
	bot_screen.m_info.bfi = false;
	joint_screen.m_info.bfi = false;

	top_screen.build();
	bot_screen.build();
	joint_screen.build();

	std::thread top_thread(screen_display_thread, &top_screen);
	std::thread bot_thread(screen_display_thread, &bot_screen);
	std::thread joint_thread(screen_display_thread, &joint_screen);

	capture_data->status.connected = connect(true, capture_data, &frontend_data);
	SuccessConnectionOutTextGenerator(out_text_data, capture_data);

	while(capture_data->status.running) {

		VideoOutputData *chosen_buf = out_buf;
		bool blank_out = false;
		if(capture_data->status.connected) {
			bool timed_out = !capture_data->status.video_wait.timed_lock();
			curr_out = (capture_data->status.curr_in - 1 + NUM_CONCURRENT_DATA_BUFFERS) % NUM_CONCURRENT_DATA_BUFFERS;

			if((!capture_data->status.cooldown_curr_in) && (curr_out != prev_out)) {
				last_frame_time = capture_data->time_in_buf[curr_out];
				if(capture_data->read[curr_out] >= sizeof(VideoInputData)) {
					convertVideoToOutput(&capture_data->capture_buf[curr_out].video_data, out_buf);
					num_allowed_blanks = MAX_ALLOWED_BLANKS;
				}
				else {
					if(num_allowed_blanks > 0) {
						num_allowed_blanks--;
					}
					else {
						memset(out_buf, 0, sizeof(VideoOutputData));
					}
				}
			}
			else if(capture_data->status.cooldown_curr_in)
				blank_out = true;
			prev_out = curr_out;
		}
		else {
			default_sleep();
			blank_out = true;
		}

		if(blank_out) {
			last_frame_time = 0.0;
			chosen_buf = NULL;
		}

		update_output(&frontend_data, last_frame_time, chosen_buf);

		int load_index = 0;
		int save_index = 0;
		top_screen.poll();
		bot_screen.poll();
		joint_screen.poll();

		if(top_screen.open_capture() || bot_screen.open_capture() || joint_screen.open_capture()) {
			capture_data->status.connected = connect(true, capture_data, &frontend_data);
			SuccessConnectionOutTextGenerator(out_text_data, capture_data);
		}

		check_close_application(top_screen, capture_data, ret_val);
		check_close_application(bot_screen, capture_data, ret_val);
		check_close_application(joint_screen, capture_data, ret_val);
		
		if((load_index = top_screen.load_data()) || (load_index = bot_screen.load_data()) || (load_index = joint_screen.load_data()))
			load_layout_file(load_index, &frontend_data, audio_data, out_text_data, skip_io, true);
		
		if((save_index = top_screen.save_data()) || (save_index = bot_screen.save_data()) || (save_index = joint_screen.save_data())) {
			save_layout_file(save_index, &frontend_data, audio_data, out_text_data, skip_io, true);
			top_screen.update_save_menu();
			bot_screen.update_save_menu();
			joint_screen.update_save_menu();
		}

		if(audio_data->has_text_to_print()) {
			UpdateOutText(out_text_data, "", audio_data->text_to_print(), TEXT_KIND_NORMAL);
		}

		if(capture_data->status.new_error_text) {
			UpdateOutText(out_text_data, capture_data->status.error_text, capture_data->status.error_text, TEXT_KIND_ERROR);
			capture_data->status.new_error_text = false;
		}

		if((!out_text_data.consumed) && (!frontend_data.reload)) {
			ConsoleOutText(out_text_data.full_text);
			top_screen.print_notification(out_text_data.small_text, out_text_data.kind);
			bot_screen.print_notification(out_text_data.small_text, out_text_data.kind);
			joint_screen.print_notification(out_text_data.small_text, out_text_data.kind);
			out_text_data.consumed = true;
		}
	}

	top_screen.end();
	bot_screen.end();
	joint_screen.end();

	top_thread.join();
	bot_thread.join();
	joint_thread.join();

	top_screen.after_thread_join();
	bot_screen.after_thread_join();
	joint_screen.after_thread_join();

	save_layout_file(STARTUP_FILE_INDEX, &frontend_data, audio_data, out_text_data, skip_io, false);

	if(!out_text_data.consumed) {
		ConsoleOutText(out_text_data.full_text);
		out_text_data.consumed = true;
	}

	delete out_buf;
	return ret_val;
}

static bool parse_existence_arg(int &index, char **argv, bool &target, bool existence_value, std::string to_check) {
	if(argv[index] != to_check)
		return false;
	target = existence_value;
	return true;
}

static bool parse_int_arg(int &index, int argc, char **argv, int &target, std::string to_check) {
	if(argv[index] != to_check)
		return false;
	if((++index) >= argc)
		return true;
	try {
		target = std::stoi(argv[index]);
	}
	catch(...) {
		std::cerr << "Error with input for: " << to_check << std::endl;
	}
	return true;
}

int main(int argc, char **argv) {
	#if defined(__linux__) && defined(XLIB_BASED)
	XInitThreads();
	#endif
	bool mono_app = false;
	int page_up_id = -1;
	int page_down_id = -1;
	int enter_id = -1;
	int power_id = -1;
	for (int i = 1; i < argc; i++) {
		if(parse_existence_arg(i, argv, mono_app, true, "--mono_app"))
			continue;
		#ifdef RASPI
		if(parse_int_arg(i, argc, argv, page_up_id, "--pi_select"))
			continue;
		if(parse_int_arg(i, argc, argv, page_down_id, "--pi_menu"))
			continue;
		if(parse_int_arg(i, argc, argv, enter_id, "--pi_enter"))
			continue;
		if(parse_int_arg(i, argc, argv, power_id, "--pi_power"))
			continue;
		#endif
		std::cout << "Help:" << std::endl;
		std::cout << "  --mono_app        Enables special mode for when only this application" << std::endl;
		std::cout << "                    should run on the system. Disabled by default." << std::endl;
		#ifdef RASPI
		std::cout << "  --pi_select ID  Specifies ID for the select GPIO button." << std::endl;
		std::cout << "  --pi_menu ID    Specifies ID for the menu GPIO button." << std::endl;
		std::cout << "  --pi_enter ID   Specifies ID for the enter GPIO button." << std::endl;
		std::cout << "  --pi_power ID   Specifies ID for the poweroff GPIO button." << std::endl;
		#endif
		return 0;
	}
	init_extra_buttons_poll(page_up_id, page_down_id, enter_id, power_id);
	AudioData audio_data;
	audio_data.reset();
	CaptureData* capture_data = new CaptureData;

	std::thread capture_thread(captureCall, capture_data);
	std::thread audio_thread(soundCall, &audio_data, capture_data);

	int ret_val = mainVideoOutputCall(&audio_data, capture_data, mono_app);
	audio_thread.join();
	capture_thread.join();
	delete capture_data;
	end_extra_buttons_poll();

	return ret_val;
}

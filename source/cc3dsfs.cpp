#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

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
#include "devicecapture.hpp"
#include "hw_defs.hpp"
#include "frontend.hpp"
#include "audio.hpp"
#include "conversions.hpp"

// Threshold to keep the audio latency limited in "amount of frames".
#define NUM_CONCURRENT_AUDIO_BUFFERS ((MAX_MAX_AUDIO_LATENCY * 2) + 1)

#define LOW_POLL_DIVISOR 6
#define NO_DATA_CONSECUTIVE_THRESHOLD 4
#define TIME_AUDIO_DEVICE_CHECK 0.1

struct OutTextData {
	std::string full_text;
	std::string small_text;
	bool consumed;
	TextKind kind;
};

struct override_all_data {
	override_win_data override_top_bot_data;
	override_win_data override_top_data;
	override_win_data override_bot_data;
	bool no_audio = false;
	int volume = DEFAULT_NO_VOLUME_VALUE;
	bool always_prevent_mouse_showing = false;
	bool auto_connect_to_first = false;
	bool auto_close = false;
	int loaded_profile = STARTUP_FILE_INDEX;
	bool mono_app = false;
	bool recovery_mode = false;
	bool quit_on_first_connection_failure = false;
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
		UpdateOutText(out_text_data, "Connected to " + capture_data->status.device.name + " - " + capture_data->status.device.serial_number, "Connected", TEXT_KIND_SUCCESS);
	else if(!capture_data->status.new_error_text) {
		UpdateOutText(out_text_data, "Disconnected", "Disconnected", TEXT_KIND_WARNING);
	}
}

static bool load_shared(const std::string path, const std::string name, SharedData* shared_data, OutTextData &out_text_data, bool do_print) {
	std::ifstream file(path + name);
	std::string line;

	if((!file) || (!file.is_open()) || (!file.good())) {
		if(do_print)
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

					if(key == "extra_button_enter_short") {
						shared_data->input_data.extra_button_shortcuts.enter_shortcut = get_window_command(static_cast<PossibleWindowCommands>(std::stoi(value)));
						continue;
					}

					if(key == "extra_button_page_up_short") {
						shared_data->input_data.extra_button_shortcuts.page_up_shortcut = get_window_command(static_cast<PossibleWindowCommands>(std::stoi(value)));
						continue;
					}

					if(key == "fast_poll") {
						shared_data->input_data.fast_poll = std::stoi(value);
						continue;
					}

					if(key == "enable_keyboard_input") {
						shared_data->input_data.enable_keyboard_input = std::stoi(value);
						continue;
					}

					if(key == "enable_controller_input") {
						shared_data->input_data.enable_controller_input = std::stoi(value);
						continue;
					}

					if(key == "enable_mouse_input") {
						shared_data->input_data.enable_mouse_input = std::stoi(value);
						continue;
					}

					if(key == "enable_buttons_input") {
						shared_data->input_data.enable_buttons_input = std::stoi(value);
						continue;
					}
				}
			}
		}
	}
	catch(...) {
		if(do_print)
			UpdateOutText(out_text_data, "File " + path + name + " load failed.\nDefaults re-loaded.", "Load failed\nDefaults re-loaded", TEXT_KIND_ERROR);
		result = false;
	}

	file.close();
	return result;
}

static bool load(const std::string path, const std::string name, ScreenInfo &top_info, ScreenInfo &bottom_info, ScreenInfo &joint_info, DisplayData &display_data, AudioData *audio_data, OutTextData &out_text_data, CaptureStatus* capture_status) {
	std::ifstream file(path + name);
	std::string line;

	if((!file) || (!file.is_open()) || (!file.good())) {
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
						bool split = std::stoi(value);
						joint_info.window_enabled = !split;
						top_info.window_enabled = split;
						bottom_info.window_enabled = split;
						continue;
					}

					if(key == "last_connected_ds") {
						display_data.last_connected_ds = std::stoi(value);
						continue;
					}

					if(key == "is_screen_capture_type") {
						capture_status->capture_type =  static_cast<CaptureScreensType>(std::stoi(value) % CaptureScreensType::CAPTURE_SCREENS_ENUM_END);
						continue;
					}

					if(key == "is_speed_capture") {
						capture_status->capture_speed =  static_cast<CaptureSpeedsType>(std::stoi(value) % CaptureSpeedsType::CAPTURE_SPEEDS_ENUM_END);
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

static void defaults_reload(FrontendData *frontend_data, AudioData* audio_data, CaptureStatus* capture_status) {
	capture_status->enabled_3d = false;
	capture_status->capture_type = CAPTURE_SCREENS_BOTH;
	capture_status->capture_speed = CAPTURE_SPEEDS_FULL;
	reset_screen_info(frontend_data->top_screen->m_info);
	reset_screen_info(frontend_data->bot_screen->m_info);
	reset_screen_info(frontend_data->joint_screen->m_info);
	sanitize_enabled_info(frontend_data->joint_screen->m_info, frontend_data->top_screen->m_info, frontend_data->bot_screen->m_info);
	audio_data->reset();
	reset_display_data(&frontend_data->display_data);
	if(frontend_data->display_data.mono_app_mode)
		frontend_data->joint_screen->m_info.is_fullscreen = true;
	frontend_data->reload = true;
}

static void load_layout_file(int load_index, FrontendData *frontend_data, AudioData* audio_data, OutTextData &out_text_data, CaptureStatus* capture_status, bool skip_io, bool do_print, bool created_proper_folder, bool is_first_load) {
	if(skip_io)
		return;

	defaults_reload(frontend_data, audio_data, capture_status);

	if(load_index == SIMPLE_RESET_DATA_INDEX) {
		reset_shared_data(&frontend_data->shared_data);
		UpdateOutText(out_text_data, "Reset detected. Defaults re-loaded", "Reset detected\nDefaults re-loaded", TEXT_KIND_WARNING);
		return;
	}

	bool name_load_success = false;
	std::string layout_name = LayoutNameGenerator(load_index);
	std::string layout_path = LayoutPathGenerator(load_index, created_proper_folder);
	std::string shared_layout_path = LayoutPathGenerator(STARTUP_FILE_INDEX, created_proper_folder);
	std::string shared_layout_name = "shared.cfg";
	bool op_success = load(layout_path, layout_name, frontend_data->top_screen->m_info, frontend_data->bot_screen->m_info, frontend_data->joint_screen->m_info, frontend_data->display_data, audio_data, out_text_data, capture_status);
	if(do_print && op_success) {
		std::string load_name = load_layout_name(load_index, created_proper_folder, name_load_success);
		UpdateOutText(out_text_data, "Layout loaded from: " + layout_path + layout_name, "Layout " + load_name + " loaded", TEXT_KIND_SUCCESS);
	}
	else if(!op_success)
		defaults_reload(frontend_data, audio_data, capture_status);
	sanitize_enabled_info(frontend_data->joint_screen->m_info, frontend_data->top_screen->m_info, frontend_data->bot_screen->m_info);
	if(!is_first_load)
		return;
	bool shared_op_success = load_shared(shared_layout_path, shared_layout_name, &frontend_data->shared_data, out_text_data, op_success);
	if(!shared_op_success)
		reset_shared_data(&frontend_data->shared_data);
	if(!is_input_data_valid(&frontend_data->shared_data.input_data, are_extra_buttons_usable()))
		reset_input_data(&frontend_data->shared_data.input_data);
}

static bool save_shared(const std::string path, const std::string name, SharedData* shared_data, OutTextData &out_text_data, bool do_print) {
	std::ofstream file(path + name);
	if(!file.good()) {
		if(do_print)
			UpdateOutText(out_text_data, "File " + path + name + " save failed.", "Save failed", TEXT_KIND_ERROR);
		return false;
	}

	file << "extra_button_enter_short=" << shared_data->input_data.extra_button_shortcuts.enter_shortcut->cmd << std::endl;
	file << "extra_button_page_up_short=" << shared_data->input_data.extra_button_shortcuts.page_up_shortcut->cmd << std::endl;
	file << "fast_poll=" << shared_data->input_data.fast_poll << std::endl;
	file << "enable_keyboard_input=" << shared_data->input_data.enable_keyboard_input << std::endl;
	file << "enable_controller_input=" << shared_data->input_data.enable_controller_input << std::endl;
	file << "enable_mouse_input=" << shared_data->input_data.enable_mouse_input << std::endl;
	file << "enable_buttons_input=" << shared_data->input_data.enable_buttons_input << std::endl;

	file.close();
	return true;
}

static bool save(const std::string path, const std::string name, const std::string save_name, const ScreenInfo &top_info, const ScreenInfo &bottom_info, const ScreenInfo &joint_info, DisplayData &display_data, AudioData *audio_data, OutTextData &out_text_data, CaptureStatus* capture_status) {
	std::ofstream file(path + name);
	if(!file.good()) {
		UpdateOutText(out_text_data, "File " + path + name + " save failed.", "Save failed", TEXT_KIND_ERROR);
		return false;
	}

	file << "name=" << save_name << std::endl;
	file << "version=" << get_version_string(false) << std::endl;
	file << save_screen_info("bot_", bottom_info);
	file << save_screen_info("joint_", joint_info);
	file << save_screen_info("top_", top_info);
	file << "last_connected_ds=" << display_data.last_connected_ds << std::endl;
	file << "is_screen_capture_type=" << capture_status->capture_type << std::endl;
	file << "is_speed_capture=" << capture_status->capture_speed << std::endl;
	file << audio_data->save_audio_data();

	file.close();
	return true;
}

static void save_layout_file(int save_index, FrontendData *frontend_data, AudioData* audio_data, OutTextData &out_text_data, CaptureStatus* capture_status, bool skip_io, bool do_print, bool created_proper_folder) {
	if(skip_io)
		return;

	if(save_index == SIMPLE_RESET_DATA_INDEX)
		return;

	bool name_load_success = false;
	std::string save_name = load_layout_name(save_index, created_proper_folder, name_load_success);
	std::string layout_name = LayoutNameGenerator(save_index);
	std::string layout_path = LayoutPathGenerator(save_index, created_proper_folder);
	std::string shared_layout_path = LayoutPathGenerator(STARTUP_FILE_INDEX, created_proper_folder);
	std::string shared_layout_name = "shared.cfg";
	bool op_success = save(layout_path, layout_name, save_name, frontend_data->top_screen->m_info, frontend_data->bot_screen->m_info, frontend_data->joint_screen->m_info, frontend_data->display_data, audio_data, out_text_data, capture_status);
	if(do_print && op_success) {
		UpdateOutText(out_text_data, "Layout saved to: " + layout_path + layout_name, "Layout " + save_name + " saved", TEXT_KIND_SUCCESS);
	}
	bool shared_op_success = save_shared(shared_layout_path, shared_layout_name, &frontend_data->shared_data, out_text_data, op_success);
}

static void executeSoundRestart(Audio &audio, AudioData* audio_data, bool do_restart) {
	if(do_restart) {
		audio.stop();
		audio.~Audio();
		::new(&audio) Audio(audio_data);
	}
	audio.start_audio();
	audio.play();
}

static bool setDefaultAudioDevice(Audio &audio) {
	bool success = false;
	std::optional<std::string> default_device = sf::PlaybackDevice::getDefaultDevice();
	if(default_device) {
		audio.stop_audio();
		audio.stop();
		success = sf::PlaybackDevice::setDevice(default_device.value());
	}
	return success;
}

static void handleAudioDeviceChanges(Audio &audio, AudioData *audio_data, std::optional<std::string> &curr_device, audio_output_device_data &in_use_audio_output_device_data) {
		// Code for audio device selection
		in_use_audio_output_device_data = audio_data->get_audio_output_device_data();
		int index = -1;
		bool success = false;
		if(in_use_audio_output_device_data.preference_requested) {
			std::vector<std::string> audio_devices =  sf::PlaybackDevice::getAvailableDevices();
			index = searchAudioDevice(in_use_audio_output_device_data.preferred, audio_devices);
			if((index != -1) && (curr_device != audio_devices[index])) {
				audio.stop_audio();
				audio.stop();
				success = sf::PlaybackDevice::setDevice(audio_devices[index]);
				curr_device = audio_devices[index];
			}
		}
		if(index == -1) {
			std::optional<std::string> default_device = sf::PlaybackDevice::getDefaultDevice();
			if(default_device != curr_device)
				success = setDefaultAudioDevice(audio);
			curr_device = default_device;
		}
}

static void soundCall(AudioData *audio_data, CaptureData* capture_data) {
	std::int16_t (*out_buf)[MAX_SAMPLES_IN] = new std::int16_t[NUM_CONCURRENT_AUDIO_BUFFERS][MAX_SAMPLES_IN];
	Audio audio(audio_data);
	int audio_buf_counter = 0;
	const bool endianness = is_big_endian();
	volatile int loaded_samples;
	audio_output_device_data in_use_audio_output_device_data;
	std::optional<std::string> curr_device = sf::PlaybackDevice::getDevice();
	std::chrono::time_point<std::chrono::high_resolution_clock> last_device_check_time = std::chrono::high_resolution_clock::now();

	while(capture_data->status.running) {
		if(capture_data->status.connected && capture_data->status.device.has_audio) {
			bool timed_out = !capture_data->status.audio_wait.timed_lock();

			if(!capture_data->status.cooldown_curr_in) {
				CaptureDataSingleBuffer* data_buffer = capture_data->data_buffers.GetReaderBuffer(CAPTURE_READER_AUDIO);
				if(data_buffer != NULL) {
					loaded_samples = audio.samples.size();
					if((data_buffer->read > get_video_in_size(capture_data)) && (loaded_samples < MAX_MAX_AUDIO_LATENCY)) {
						uint64_t n_samples = get_audio_n_samples(capture_data, data_buffer->read);
						double out_time = data_buffer->time_in_buf;
						bool conversion_success = convertAudioToOutput(out_buf[audio_buf_counter], n_samples, endianness, data_buffer, &capture_data->status);
						if(!conversion_success)
							audio_data->signal_conversion_error();
						audio.samples.emplace(out_buf[audio_buf_counter], n_samples, out_time);
						if(++audio_buf_counter >= NUM_CONCURRENT_AUDIO_BUFFERS)
							audio_buf_counter = 0;
						audio.samples_wait.unlock();
					}
					capture_data->data_buffers.ReleaseReaderBuffer(CAPTURE_READER_AUDIO);
				}
			}

			loaded_samples = audio.samples.size();
			if(audio.getStatus() != sf::SoundStream::Status::Playing) {
				audio.stop_audio();
				if(loaded_samples > 0) {
					bool do_restart = audio.restart;
					executeSoundRestart(audio, audio_data, do_restart);
					if(do_restart) {
						in_use_audio_output_device_data.preference_requested = false;
						curr_device = sf::PlaybackDevice::getDevice();
						last_device_check_time = std::chrono::high_resolution_clock::now();
					}
				}
			}
			else {
				if(loaded_samples > 0) {
					if(audio.hasTooMuchTimeElapsed()) {
						audio.stop_audio();
						executeSoundRestart(audio, audio_data, true);
						in_use_audio_output_device_data.preference_requested = false;
						curr_device = sf::PlaybackDevice::getDevice();
						last_device_check_time = std::chrono::high_resolution_clock::now();
					}
				}
				audio.update_volume();
			}
		}
		else {
			audio.stop_audio();
			audio.stop();
			default_sleep();
		}
		
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - last_device_check_time;
		if(diff.count() >= TIME_AUDIO_DEVICE_CHECK) {
			last_device_check_time = curr_time;
			handleAudioDeviceChanges(audio, audio_data, curr_device, in_use_audio_output_device_data);
		}
	}

	audio.stop_audio();
	audio.stop();
	delete []out_buf;
}

static void poll_all_windows(FrontendData *frontend_data, bool do_everything, bool &polled) {
	if(!polled) {
		frontend_data->top_screen->poll(do_everything);
		frontend_data->bot_screen->poll(do_everything);
		frontend_data->joint_screen->poll(do_everything);
		polled = true;
	}
}

static void check_close_application(WindowScreen *screen, CaptureData* capture_data, int &ret_val) {
	if(screen->close_capture() && capture_data->status.running) {
		capture_data->status.running = false;
		ret_val = screen->get_ret_val();
	}
}

static float get_time_multiplier(CaptureData* capture_data, bool should_ignore_data_rate) {
	if(should_ignore_data_rate)
		return 1.0;
	if(capture_data->status.device.cc_type != CAPTURE_CONN_IS_NITRO)
		return 1.0;
	switch(capture_data->status.capture_speed) {
		case CAPTURE_SPEEDS_HALF:
			return 2.0;
		case CAPTURE_SPEEDS_THIRD:
			return 3.0;
		case CAPTURE_SPEEDS_QUARTER:
			return 4.0;
		default:
			return 1.0;
	}
}

static int mainVideoOutputCall(AudioData* audio_data, CaptureData* capture_data, bool created_proper_folder, override_all_data &override_data) {
	VideoOutputData *out_buf;
	double last_frame_time = 0.0;
	int num_elements_fps_array = 0;
	FrontendData frontend_data;
	ConsumerMutex draw_lock;
	reset_display_data(&frontend_data.display_data);
	reset_shared_data(&frontend_data.shared_data);
	frontend_data.display_data.mono_app_mode = override_data.mono_app;
	frontend_data.display_data.force_disable_mouse = override_data.always_prevent_mouse_showing;
	frontend_data.reload = true;
	bool skip_io = false;
	int num_allowed_blanks = MAX_ALLOWED_BLANKS;
	OutTextData out_text_data;
	out_text_data.consumed = true;
	int ret_val = 0;
	int poll_timeout = 0;
	const bool endianness = is_big_endian();

	out_buf = new VideoOutputData;
	memset(out_buf, 0, sizeof(VideoOutputData));

	draw_lock.unlock();
	WindowScreen *top_screen = new WindowScreen(ScreenType::TOP, &capture_data->status, &frontend_data.display_data, &frontend_data.shared_data, audio_data, &draw_lock, created_proper_folder);
	WindowScreen *bot_screen = new WindowScreen(ScreenType::BOTTOM, &capture_data->status, &frontend_data.display_data, &frontend_data.shared_data, audio_data, &draw_lock, created_proper_folder);
	WindowScreen *joint_screen = new WindowScreen(ScreenType::JOINT, &capture_data->status, &frontend_data.display_data, &frontend_data.shared_data, audio_data, &draw_lock, created_proper_folder);
	frontend_data.top_screen = top_screen;
	frontend_data.bot_screen = bot_screen;
	frontend_data.joint_screen = joint_screen;

	if(!override_data.recovery_mode)
		load_layout_file(override_data.loaded_profile, &frontend_data, audio_data, out_text_data, &capture_data->status, skip_io, false, created_proper_folder, true);
	if(override_data.volume != DEFAULT_NO_VOLUME_VALUE)
		audio_data->set_audio_volume(override_data.volume);
	override_set_data_to_screen_info(override_data.override_top_bot_data, frontend_data.joint_screen->m_info);
	override_set_data_to_screen_info(override_data.override_top_data, frontend_data.top_screen->m_info);
	override_set_data_to_screen_info(override_data.override_bot_data, frontend_data.bot_screen->m_info);
	sanitize_enabled_info(frontend_data.joint_screen->m_info, frontend_data.top_screen->m_info, frontend_data.bot_screen->m_info);
	// Due to the risk for seizures, at the start of the program, set BFI to false!
	top_screen->m_info.bfi = false;
	bot_screen->m_info.bfi = false;
	joint_screen->m_info.bfi = false;

	top_screen->build();
	bot_screen->build();
	joint_screen->build();

	std::thread top_thread(screen_display_thread, top_screen);
	std::thread bot_thread(screen_display_thread, bot_screen);
	std::thread joint_thread(screen_display_thread, joint_screen);

	capture_data->status.connected = connect(true, capture_data, &frontend_data, override_data.auto_connect_to_first);
	if((override_data.quit_on_first_connection_failure || override_data.auto_close) && (!capture_data->status.connected)) {
		capture_data->status.running = false;
		ret_val = -3;
	}
	bool last_connected = capture_data->status.connected;
	SuccessConnectionOutTextGenerator(out_text_data, capture_data);
	int no_data_consecutive = 0;

	while(capture_data->status.running) {

		bool polled = false;
		bool poll_everything = true;
		if(poll_timeout > 0) {
			poll_everything = false;
			poll_timeout--;
		}
		else if(frontend_data.shared_data.input_data.fast_poll)
			poll_timeout = LOW_POLL_DIVISOR;
		VideoOutputData *chosen_buf = out_buf;
		bool blank_out = false;
		bool is_connected = capture_data->status.connected;
		if(is_connected != last_connected) {
			update_connected_specific_settings(&frontend_data, capture_data->status.device);
			no_data_consecutive = 0;
			num_allowed_blanks = MAX_ALLOWED_BLANKS;
		}
		last_connected = is_connected;
		if(is_connected) {
			if(no_data_consecutive > NO_DATA_CONSECUTIVE_THRESHOLD)
				no_data_consecutive = NO_DATA_CONSECUTIVE_THRESHOLD;
			capture_data->status.video_wait.update_time_multiplier(get_time_multiplier(capture_data, no_data_consecutive >= NO_DATA_CONSECUTIVE_THRESHOLD));
			bool timed_out = !capture_data->status.video_wait.timed_lock();
			bool data_processed = false;
			CaptureDataSingleBuffer* data_buffer = capture_data->data_buffers.GetReaderBuffer(CAPTURE_READER_VIDEO);

			if(data_buffer != NULL) {
				last_frame_time = data_buffer->time_in_buf;
				if(data_buffer->read >= get_video_in_size(capture_data)) {
					if(capture_data->status.cooldown_curr_in)
						blank_out = true;
					else {
						bool conversion_success = convertVideoToOutput(out_buf, endianness, data_buffer, &capture_data->status);
						if(!conversion_success)
							UpdateOutText(out_text_data, "", "Video conversion failed...", TEXT_KIND_NORMAL);
					}
					num_allowed_blanks = MAX_ALLOWED_BLANKS;
					no_data_consecutive = 0;
					data_processed = true;
				}
				capture_data->data_buffers.ReleaseReaderBuffer(CAPTURE_READER_VIDEO);
			}
			if(!data_processed) {
				if(num_allowed_blanks > 0)
					num_allowed_blanks--;
				else 
					blank_out = true;
				no_data_consecutive++;
			}
		}
		else {
			default_sleep();
			blank_out = true;
		}

		if(blank_out) {
			last_frame_time = 0.0;
			chosen_buf = NULL;
		}

		if(frontend_data.shared_data.input_data.fast_poll)
			poll_all_windows(&frontend_data, poll_everything, polled);

		update_output(&frontend_data, last_frame_time, chosen_buf);

		if(!frontend_data.shared_data.input_data.fast_poll)
			poll_all_windows(&frontend_data, poll_everything, polled);

		int load_index = 0;
		int save_index = 0;

		if(top_screen->scheduled_split() || bot_screen->scheduled_split()) {
			top_screen->m_info.window_enabled = false;
			bot_screen->m_info.window_enabled = false;
			joint_screen->m_info.window_enabled = true;
		}
		if(joint_screen->scheduled_split()) {
			top_screen->m_info.window_enabled = true;
			bot_screen->m_info.window_enabled = true;
			joint_screen->m_info.window_enabled = false;
		}

		if(top_screen->open_capture() || bot_screen->open_capture() || joint_screen->open_capture()) {
			capture_data->status.connected = connect(true, capture_data, &frontend_data);
			SuccessConnectionOutTextGenerator(out_text_data, capture_data);
		}

		check_close_application(top_screen, capture_data, ret_val);
		check_close_application(bot_screen, capture_data, ret_val);
		check_close_application(joint_screen, capture_data, ret_val);
		if(override_data.auto_close && (!capture_data->status.connected)) {
			capture_data->status.running = false;
			ret_val = -4;
		}
		
		if((load_index = top_screen->load_data()) || (load_index = bot_screen->load_data()) || (load_index = joint_screen->load_data())) {
			// This value should only be loaded when starting the program...
			bool previous_last_connected_ds = frontend_data.display_data.last_connected_ds;
			load_layout_file(load_index, &frontend_data, audio_data, out_text_data, &capture_data->status, skip_io, true, created_proper_folder, false);
			frontend_data.display_data.last_connected_ds = previous_last_connected_ds;
		}
		
		if((save_index = top_screen->save_data()) || (save_index = bot_screen->save_data()) || (save_index = joint_screen->save_data())) {
			save_layout_file(save_index, &frontend_data, audio_data, out_text_data, &capture_data->status, skip_io, true, created_proper_folder);
			top_screen->update_save_menu();
			bot_screen->update_save_menu();
			joint_screen->update_save_menu();
		}

		if(audio_data->has_text_to_print()) {
			UpdateOutText(out_text_data, "", audio_data->text_to_print(), TEXT_KIND_NORMAL);
		}

		if(capture_data->status.new_error_text) {
			UpdateOutText(out_text_data, capture_data->status.detailed_error_text, capture_data->status.graphical_error_text, TEXT_KIND_ERROR);
			capture_data->status.new_error_text = false;
		}

		if((!out_text_data.consumed) && (!frontend_data.reload)) {
			ConsoleOutText(out_text_data.full_text);
			top_screen->print_notification(out_text_data.small_text, out_text_data.kind);
			bot_screen->print_notification(out_text_data.small_text, out_text_data.kind);
			joint_screen->print_notification(out_text_data.small_text, out_text_data.kind);
			out_text_data.consumed = true;
		}
	}

	top_screen->end();
	bot_screen->end();
	joint_screen->end();

	top_thread.join();
	bot_thread.join();
	joint_thread.join();

	top_screen->after_thread_join();
	bot_screen->after_thread_join();
	joint_screen->after_thread_join();

	save_layout_file(override_data.loaded_profile, &frontend_data, audio_data, out_text_data, &capture_data->status, skip_io, false, created_proper_folder);

	if(!out_text_data.consumed) {
		ConsoleOutText(out_text_data.full_text);
		out_text_data.consumed = true;
	}

	delete out_buf;
	return ret_val;
}

static bool create_folder(const std::string path) {
	try {
		#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
		std::filesystem::create_directories(path);
		#else
		std::experimental::filesystem::create_directories(path);
		#endif
		return true;
	}
	catch(...) {
		std::cerr << "Error creating folder " << path << std::endl;
	}
	return false;
}

static bool create_out_folder() {
	bool success = create_folder(LayoutPathGenerator(STARTUP_FILE_INDEX, true));
	if(!success)
		create_folder(LayoutPathGenerator(STARTUP_FILE_INDEX, false));
	create_folder(LayoutPathGenerator(1, success));
	return success;
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
	init_threads();
	init_start_time();
	int page_up_id = -1;
	int page_down_id = -1;
	int enter_id = -1;
	int power_id = -1;
	bool use_pud_up = true;
	bool created_proper_folder = create_out_folder();
	override_all_data override_data;
	for (int i = 1; i < argc; i++) {
		if(parse_existence_arg(i, argv, override_data.mono_app, true, "--mono_app"))
			continue;
		if(parse_existence_arg(i, argv, override_data.recovery_mode, true, "--recovery_mode"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_bot_data.pos_x, "--pos_x_both"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_bot_data.pos_y, "--pos_y_both"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_bot_data.scaling, "--scaling_both"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_bot_data.enabled, "--enabled_both"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_data.pos_x, "--pos_x_top"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_data.pos_y, "--pos_y_top"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_data.scaling, "--scaling_top"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_top_data.enabled, "--enabled_top"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_bot_data.pos_x, "--pos_x_bot"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_bot_data.pos_y, "--pos_y_bot"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_bot_data.scaling, "--scaling_bot"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.override_bot_data.enabled, "--enabled_bot"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.volume, "--volume"))
			continue;
		if(parse_existence_arg(i, argv, override_data.no_audio, true, "--no_audio"))
			continue;
		if(parse_existence_arg(i, argv, override_data.always_prevent_mouse_showing, true, "--no_cursor"))
			continue;
		if(parse_existence_arg(i, argv, override_data.auto_connect_to_first, true, "--auto_connect"))
			continue;
		if(parse_existence_arg(i, argv, override_data.auto_close, true, "--auto_close"))
			continue;
		if(parse_existence_arg(i, argv, override_data.quit_on_first_connection_failure, true, "--failure_close"))
			continue;
		if(parse_int_arg(i, argc, argv, override_data.loaded_profile, "--profile"))
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
		if(parse_existence_arg(i, argv, use_pud_up, false, "--pi_pud_down"))
			continue;
		#endif
		std::cout << "Help:" << std::endl;
		std::cout << "  --mono_app       Enables special mode for when only this application" << std::endl;
		std::cout << "                   should run on the system. Disabled by default." << std::endl;
		std::cout << "  --recovery_mode  Resets to the defaults." << std::endl;
		std::cout << "  --pos_x_both     Set default x position for the window with both screens." << std::endl;
		std::cout << "  --pos_y_both     Set default y position for the window with both screens." << std::endl;
		std::cout << "  --scaling_both   Overrides the scale factor for the window with both screens." << std::endl;
		std::cout << "  --enabled_both   Overrides the presence of the window with both screens." << std::endl;
		std::cout << "                   1 On, 0 Off." << std::endl;
		std::cout << "  --pos_x_top      Set default x position for the top screen's window." << std::endl;
		std::cout << "  --pos_y_top      Set default y position for the top screen's window." << std::endl;
		std::cout << "  --scaling_top    Overrides the top screen window's scale factor." << std::endl;
		std::cout << "  --enabled_top    Overrides the presence of the top screen's window." << std::endl;
		std::cout << "                   1 On, 0 Off." << std::endl;
		std::cout << "  --pos_x_bot      Set default x position for the bottom screen's window." << std::endl;
		std::cout << "  --pos_y_bot      Set default y position for the bottom screen's window." << std::endl;
		std::cout << "  --scaling_bot    Overrides the bottom screen window's scale factor." << std::endl;
		std::cout << "  --enabled_bot    Overrides the presence of the bottom screen's window." << std::endl;
		std::cout << "                   1 On, 0 Off." << std::endl;
		std::cout << "  --volume         Overrides the saved volume for the audio. 0 - 200" << std::endl;
		std::cout << "  --no_audio       Disables audio output and processing completely." << std::endl;
		std::cout << "  --no_cursor      Prevents the mouse cursor from showing, unless moved." << std::endl;
		std::cout << "  --auto_connect   Automatically connects to the first available device," << std::endl;
		std::cout << "                   even if multiple are present." << std::endl;
		std::cout << "  --failure_close  Automatically closes the software if the first connection" << std::endl;
		std::cout << "                   doesn't succeed." << std::endl;
		std::cout << "  --auto_close  Automatically closes the software on disconnect." << std::endl;
		std::cout << "  --profile        Loads the profile with the specified ID at startup" << std::endl;
		std::cout << "                   instead of the default one. When the program closes," << std::endl;
		std::cout << "                   the data is also saved to the specified profile." << std::endl;
		#ifdef RASPI
		std::cout << "  --pi_select ID   Specifies ID for the select GPIO button." << std::endl;
		std::cout << "  --pi_menu ID     Specifies ID for the menu GPIO button." << std::endl;
		std::cout << "  --pi_enter ID    Specifies ID for the enter GPIO button." << std::endl;
		std::cout << "  --pi_power ID    Specifies ID for the poweroff GPIO button." << std::endl;
		std::cout << "  --pi_pud_down    Sets the pull-up GPIO mode to down. Default is up." << std::endl;
		#endif
		return 0;
	}
	init_extra_buttons_poll(page_up_id, page_down_id, enter_id, power_id, use_pud_up);
	AudioData audio_data;
	audio_data.reset();
	CaptureData* capture_data = new CaptureData;
	capture_init();

	std::thread capture_thread(captureCall, capture_data);
	std::thread audio_thread;
	if(!override_data.no_audio)
		audio_thread = std::thread(soundCall, &audio_data, capture_data);

	int ret_val = mainVideoOutputCall(&audio_data, capture_data, created_proper_folder, override_data);
	if(!override_data.no_audio)
		audio_thread.join();
	capture_thread.join();
	delete capture_data;
	end_extra_buttons_poll();
	capture_close();

	return ret_val;
}

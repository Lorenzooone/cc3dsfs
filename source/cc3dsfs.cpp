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

#define FPS_WINDOW_SIZE 64

// Threshold to keep the audio latency limited in "amount of frames".
#define AUDIO_LATENCY_LIMIT 4
#define NUM_CONCURRENT_AUDIO_BUFFERS ((AUDIO_LATENCY_LIMIT * 2) + 1)

struct OutTextData {
	std::string full_text;
	std::string small_text;
	bool consumed;
	TextKind kind;
};

void ConsoleOutText(std::string full_text) {
	if(full_text != "")
		std::cout << "[" << NAME << "] " << full_text << std::endl;
}

void UpdateOutText(OutTextData &out_text_data, std::string full_text, std::string small_text, TextKind kind) {
	if(!out_text_data.consumed)
		ConsoleOutText(out_text_data.full_text);
	out_text_data.full_text = full_text;
	out_text_data.small_text = small_text;
	out_text_data.kind = kind;
	out_text_data.consumed = false;
}

void ConnectedOutTextGenerator(OutTextData &out_text_data, CaptureData* capture_data) {
	UpdateOutText(out_text_data, "Connected to " + std::string(capture_data->chosen_serial_number), "Connected", TEXT_KIND_SUCCESS);
}

std::string LayoutNameGenerator(int index) {
	return "layout" + std::to_string(index) + ".cfg";
}

bool load(const std::string path, const std::string name, ScreenInfo &top_info, ScreenInfo &bottom_info, ScreenInfo &joint_info, DisplayData &display_data, AudioData *audio_data, OutTextData &out_text_data) {
	std::ifstream file(path + name);
	std::string line;

	reset_screen_info(top_info);
	reset_screen_info(bottom_info);
	reset_screen_info(joint_info);

	if (!file.good()) {
		UpdateOutText(out_text_data, "File " + path + name + " load failed.\nDefaults re-loaded.", "Load failed\nDefaults re-loaded", TEXT_KIND_ERROR);
		return false;
	}

	while (std::getline(file, line)) {
		std::istringstream kvp(line);
		std::string key;

		if (std::getline(kvp, key, '=')) {
			std::string value;

			if (std::getline(kvp, value)) {

				if(load_screen_info(key, value, "bot_", bottom_info)) {
					if(bottom_info.crop_kind != Crop::NATIVE_DS)
						bottom_info.crop_kind = Crop::DEFAULT_3DS;
					continue;
				}
				if(load_screen_info(key, value, "joint_", joint_info))
					continue;
				if(load_screen_info(key, value, "top_", top_info))
					continue;

				if (key == "split") {
					display_data.split = std::stoi(value);
					continue;
				}

				if(audio_data->load_audio_data(key, value))
					continue;
			}
		}
	}

	file.close();
	return true;
}

bool save(const std::string path, const std::string name, const ScreenInfo &top_info, const ScreenInfo &bottom_info, const ScreenInfo &joint_info, DisplayData &display_data, AudioData *audio_data, OutTextData &out_text_data) {
	#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
	std::filesystem::create_directories(path);
	#else
	std::experimental::filesystem::create_directories(path);
	#endif
	std::ofstream file(path + name);
	if (!file.good()) {
		UpdateOutText(out_text_data, "File " + path + name + " save failed.", "Save failed", TEXT_KIND_ERROR);
		return false;
	}

	file << save_screen_info("bot_", bottom_info);
	file << save_screen_info("joint_", joint_info);
	file << save_screen_info("top_", top_info);
	file << "split=" << display_data.split << std::endl;
	file << audio_data->save_audio_data();

	file.close();
	return true;
}

void soundCall(AudioData *audio_data, CaptureData* capture_data) {
	Audio audio(audio_data);
	sf::Int16 out_buf[NUM_CONCURRENT_AUDIO_BUFFERS][MAX_SAMPLES_IN];
	int curr_out, prev_out = NUM_CONCURRENT_DATA_BUFFERS - 1, audio_buf_counter = 0;
	const bool endianness = is_big_endian();
	volatile int loaded_samples;

	while (capture_data->running) {
		if(capture_data->connected) {
			bool timed_out = !capture_data->audio_wait.timed_lock();
			curr_out = (capture_data->curr_in - 1 + NUM_CONCURRENT_DATA_BUFFERS) % NUM_CONCURRENT_DATA_BUFFERS;

			if((!capture_data->cooldown_curr_in) && (curr_out != prev_out)) {
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
			sf::sleep(sf::milliseconds(1000/USB_CHECKS_PER_SECOND));
		}
	}

	audio.stop_audio();
	audio.stop();
}

void mainVideoOutputCall(AudioData* audio_data, CaptureData* capture_data) {
	VideoOutputData *out_buf;
	VideoOutputData *null_buf;
	double *curr_fps_array;
	int num_elements_fps_array = 0;
	int curr_out, prev_out = NUM_CONCURRENT_DATA_BUFFERS - 1;
	DisplayData display_data;
	display_data.split = false;
	bool reload = true;
	bool skip_io = false;
	int num_allowed_blanks = MAX_ALLOWED_BLANKS;
	double last_frame_time = 0;
	std::mutex events_access;
	OutTextData out_text_data;
	out_text_data.consumed = true;

	#if not(defined(_WIN32) || defined(_WIN64))
	std::string cfg_dir = std::string(std::getenv("HOME")) + "/.config/" + std::string(NAME);
	#else
	std::string cfg_dir = ".config/" + std::string(NAME);
	#endif
	std::string base_path = cfg_dir + "/";
	std::string base_name = std::string(NAME) + ".cfg";
	std::string layout_path = cfg_dir + "/presets/";

	out_buf = new VideoOutputData;
	null_buf = new VideoOutputData;
	curr_fps_array = new double[FPS_WINDOW_SIZE];
	memset(out_buf, 0, sizeof(VideoOutputData));
	memset(null_buf, 0, sizeof(VideoOutputData));

	WindowScreen top_screen(WindowScreen::ScreenType::TOP, &display_data, audio_data, &events_access);
	WindowScreen bot_screen(WindowScreen::ScreenType::BOTTOM, &display_data, audio_data, &events_access);
	WindowScreen joint_screen(WindowScreen::ScreenType::JOINT, &display_data, audio_data, &events_access);

	if(capture_data->connected)
		ConnectedOutTextGenerator(out_text_data, capture_data);

	if(!skip_io) {
		load(base_path, base_name, top_screen.m_info, bot_screen.m_info, joint_screen.m_info, display_data, audio_data, out_text_data);
	}

	top_screen.build();
	bot_screen.build();
	joint_screen.build();

	std::thread top_thread(screen_display_thread, &top_screen, capture_data);
	std::thread bot_thread(screen_display_thread, &bot_screen, capture_data);
	std::thread joint_thread(screen_display_thread, &joint_screen, capture_data);

	while (capture_data->running) {

		if(capture_data->connected) {
			bool timed_out = !capture_data->video_wait.timed_lock();
			curr_out = (capture_data->curr_in - 1 + NUM_CONCURRENT_DATA_BUFFERS) % NUM_CONCURRENT_DATA_BUFFERS;

			if ((!capture_data->cooldown_curr_in) && (curr_out != prev_out)) {
				double frame_time = capture_data->time_in_buf[curr_out];
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

				update_output(top_screen, bot_screen, joint_screen, reload, display_data.split, frame_time, out_buf);
				last_frame_time = frame_time;

				curr_fps_array[num_elements_fps_array % FPS_WINDOW_SIZE] = 1.0 / frame_time;
				num_elements_fps_array++;
				if(num_elements_fps_array >= (2 * FPS_WINDOW_SIZE))
					num_elements_fps_array -= FPS_WINDOW_SIZE;
			}
			else {
				VideoOutputData *chosen_buf = out_buf;
				if(capture_data->cooldown_curr_in) {
					last_frame_time = 0;
					chosen_buf = null_buf;
				}
				update_output(top_screen, bot_screen, joint_screen, reload, display_data.split, last_frame_time, chosen_buf);
			}

			prev_out = curr_out;
		}
		else {
			curr_fps_array[num_elements_fps_array % FPS_WINDOW_SIZE] = 0;
			num_elements_fps_array++;
			if(num_elements_fps_array >= (2 * FPS_WINDOW_SIZE))
				num_elements_fps_array -= FPS_WINDOW_SIZE;
			sf::sleep(sf::milliseconds(1000/USB_CHECKS_PER_SECOND));
		}
		
		int available_fps = FPS_WINDOW_SIZE;
		if(num_elements_fps_array < available_fps)
			available_fps = num_elements_fps_array;
		double fps_sum = 0;
		for(int i = 0; i < available_fps; i++)
			fps_sum += curr_fps_array[i];

		if(!capture_data->connected)
			update_output(top_screen, bot_screen, joint_screen, reload, display_data.split, 0, null_buf);

		int load_index = 0;
		int save_index = 0;
		top_screen.poll();
		bot_screen.poll();
		joint_screen.poll();

		if(top_screen.open_capture() || bot_screen.open_capture() || joint_screen.open_capture()) {
			capture_data->connected = connect(true, capture_data);
			if(capture_data->connected)
				ConnectedOutTextGenerator(out_text_data, capture_data);
		}

		if(top_screen.close_capture() || bot_screen.close_capture() || joint_screen.close_capture()) {
			capture_data->running = false;
		}
		
		if((load_index = top_screen.load_data()) || (load_index = bot_screen.load_data()) || (load_index = joint_screen.load_data())) {
			if (!skip_io) {
				std::string layout_name = LayoutNameGenerator(load_index);
				bool op_success = load(layout_path, layout_name, top_screen.m_info, bot_screen.m_info, joint_screen.m_info, display_data, audio_data, out_text_data);
				if(op_success)
					UpdateOutText(out_text_data, "Layout loaded from: " + layout_path + layout_name, "Layout " + std::to_string(load_index) + " loaded", TEXT_KIND_SUCCESS);
				reload = true;
			}
		}
		
		if((save_index = top_screen.save_data()) || (save_index = bot_screen.save_data()) || (save_index = joint_screen.save_data())) {
			if (!skip_io) {
				std::string layout_name = LayoutNameGenerator(save_index);
				bool op_success = save(layout_path, layout_name, top_screen.m_info, bot_screen.m_info, joint_screen.m_info, display_data, audio_data, out_text_data);
				if(op_success)
					UpdateOutText(out_text_data, "Layout saved to: " + layout_path + layout_name, "Layout " + std::to_string(save_index) + " saved", TEXT_KIND_SUCCESS);
			}
		}

		if(audio_data->has_text_to_print()) {
			UpdateOutText(out_text_data, "", audio_data->text_to_print(), TEXT_KIND_NORMAL);
		}

		if(capture_data->new_error_text) {
			UpdateOutText(out_text_data, capture_data->error_text, capture_data->error_text, TEXT_KIND_ERROR);
			capture_data->new_error_text = false;
		}

		if(!out_text_data.consumed) {
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

	if (!skip_io) {
		save(base_path, base_name, top_screen.m_info, bot_screen.m_info, joint_screen.m_info, display_data, audio_data, out_text_data);
	}

	if(!out_text_data.consumed) {
		ConsoleOutText(out_text_data.full_text);
		out_text_data.consumed = true;
	}

	delete out_buf;
	delete null_buf;
	delete []curr_fps_array;
}

int main(int argc, char **argv) {
	#if defined(__linux__) && defined(XLIB_BASED)
	XInitThreads();
	#endif
	AudioData audio_data;
	audio_data.reset();
	CaptureData* capture_data = new CaptureData;

	capture_data->connected = connect(true, capture_data);
	std::thread capture_thread(captureCall, capture_data);
	std::thread audio_thread(soundCall, &audio_data, capture_data);

	mainVideoOutputCall(&audio_data, capture_data);
	audio_thread.join();
	capture_thread.join();
	delete capture_data;

	return 0;
}

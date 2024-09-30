#ifndef __AUDIO_DATA_HPP
#define __AUDIO_DATA_HPP

#include <string>
#define MAX_MAX_AUDIO_LATENCY 10

enum AudioOutputType {AUDIO_OUTPUT_STEREO, AUDIO_OUTPUT_MONO, AUDIO_OUTPUT_END};

class AudioData {
public:
	void reset();
	void change_max_audio_latency(bool is_change_positive);
	void change_audio_output_type(bool is_change_positive);
	void change_audio_volume(bool is_change_positive);
	void change_audio_mute();
	void request_audio_restart();
	void signal_conversion_error();
	bool check_audio_restart_request();
	AudioOutputType get_audio_output_type();
	std::string get_audio_output_name();
	int get_max_audio_latency();
	int get_final_volume();
	bool has_text_to_print();
	std::string text_to_print();
	bool get_mute();
	int get_real_volume();
	bool load_audio_data(std::string key, std::string value);
	std::string save_audio_data();

private:
	int volume;
	int max_audio_latency;
	bool mute;
	AudioOutputType output_type;
	bool restart_request = false;
	bool text_updated;
	std::string text;
	void set_max_audio_latency(int new_value);
	void set_audio_output_type(int new_value);
	void set_audio_volume(int new_volume);
	void set_audio_mute(bool new_mute);
	void update_text(std::string text);
	const std::string max_audio_latency_str = "max_audio_latency";
	const std::string volume_str = "volume";
	const std::string mute_str = "mute";
	const std::string output_type_str = "audio_output_type";
};

#endif

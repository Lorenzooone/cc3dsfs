#include "audio_data.hpp"

void AudioData::reset() {
	this->volume = 100;
	this->mute = false;
	this->max_audio_latency = 2;
	this->output_type = AUDIO_OUTPUT_STEREO;
	this->restart_request = false;
	this->text_updated = false;
}

void AudioData::change_max_audio_latency(bool is_change_positive) {
	int change = 1;
	if(!is_change_positive)
		change = -1;
	int initial_max_audio_latency = this->max_audio_latency;
	this->set_max_audio_latency(this->max_audio_latency + change);
	if(this->max_audio_latency != initial_max_audio_latency)
		this->update_text("Max Audio Latency: " + std::to_string(this->max_audio_latency));
}

void AudioData::change_audio_output_type(bool is_change_positive) {
	int change = 1;
	if(!is_change_positive)
		change = -1;
	AudioOutputType initial_audio_output_type = this->output_type;
	this->set_audio_output_type(this->output_type + change);
	if(this->output_type != initial_audio_output_type)
		this->update_text("Audio Output: " + this->get_audio_output_name());
}

void AudioData::change_audio_volume(bool is_change_positive) {
	bool initial_audio_mute = this->mute;
	int initial_audio_volume = this->volume;
	int volume_change = 5;
	if(!is_change_positive)
		volume_change *= -1;
	this->set_audio_volume(this->volume + volume_change);
	this->set_audio_mute(false);
	if((this->volume != initial_audio_volume) || (initial_audio_mute))
		this->update_text("Volume: " + std::to_string(this->get_final_volume()) + "%");
}

void AudioData::change_audio_mute() {
	this->set_audio_mute(!this->mute);
	if(this->mute)
		this->update_text("Audio muted");
	else
		this->update_text("Volume: " + std::to_string(this->get_final_volume()) + "%");
}

void AudioData::request_audio_restart() {
	this->restart_request = true;
	this->update_text("Restarting audio...");
}

bool AudioData::check_audio_restart_request() {
	bool retval = this->restart_request;
	if(retval)
		this->restart_request = false;
	return retval;
}

void AudioData::update_text(std::string text) {
	this->text = text;
	this->text_updated = true;
}

bool AudioData::has_text_to_print() {
	bool retval = this->text_updated;
	if(retval)
		this->text_updated = false;
	return retval;
}

std::string AudioData::text_to_print() {
	return this->text;
}

AudioOutputType AudioData::get_audio_output_type() {
	return this->output_type;
}

std::string AudioData::get_audio_output_name() {
	std::string chosen_str = "";
	switch(this->output_type) {
		case AUDIO_OUTPUT_STEREO:
			chosen_str = "Stereo";
			break;
		case AUDIO_OUTPUT_MONO:
			chosen_str = "Mono";
			break;
		default:
			this->output_type = AUDIO_OUTPUT_STEREO;
			chosen_str = "Stereo";
			break;
	}
	return chosen_str;
}

int AudioData::get_max_audio_latency() {
	return this->max_audio_latency;
}

int AudioData::get_final_volume() {
	if(this->mute)
		return 0;
	int read_volume = this->volume;
	if(read_volume < 0)
		read_volume = 0;
	if(read_volume > 200)
		read_volume = 200;
	return read_volume;
}

int AudioData::get_real_volume() {
	int read_volume = this->volume;
	if(read_volume < 0)
		read_volume = 0;
	if(read_volume > 200)
		read_volume = 200;
	return read_volume;
}

bool AudioData::get_mute() {
	return this->mute;
}

bool AudioData::load_audio_data(std::string key, std::string value) {
	if (key == this->mute_str) {
		this->set_audio_mute(std::stoi(value));
		return true;
	}
	if (key == this->volume_str) {
		this->set_audio_volume(std::stoi(value));
		return true;
	}
	if (key == this->max_audio_latency_str) {
		this->set_max_audio_latency(std::stoi(value));
		return true;
	}
	if (key == this->output_type_str) {
		this->set_audio_output_type(std::stoi(value));
		return true;
	}
	return false;
}

std::string AudioData::save_audio_data() {
	std::string out_str = "";
	out_str += this->mute_str + "=" + std::to_string(this->mute) + "\n";
	out_str += this->volume_str + "=" + std::to_string(this->volume) + "\n";
	out_str += this->max_audio_latency_str + "=" + std::to_string(this->max_audio_latency) + "\n";
	out_str += this->output_type_str + "=" + std::to_string(this->output_type) + "\n";
	return out_str;
}

void AudioData::set_max_audio_latency(int new_value) {
	if(new_value > MAX_MAX_AUDIO_LATENCY)
		new_value = MAX_MAX_AUDIO_LATENCY;
	if(new_value <= 0)
		new_value = 1;
	this->max_audio_latency = new_value;
}

void AudioData::set_audio_output_type(int new_value) {
	if(new_value >= AUDIO_OUTPUT_END)
		new_value = 0;
	if(new_value < 0)
		new_value = AUDIO_OUTPUT_END - 1;
	this->output_type = static_cast<AudioOutputType>(new_value);
}

void AudioData::set_audio_volume(int new_volume) {
	if(new_volume < 0)
		new_volume = 0;
	if(new_volume > 200)
		new_volume = 200;
	this->volume = new_volume;
}

void AudioData::set_audio_mute(bool new_mute) {
	this->mute = new_mute;
}

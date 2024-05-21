#include "audio_data.hpp"

void AudioData::reset() {
	this->volume = 50;
	this->mute = false;
	this->restart_request = false;
	this->text_updated = false;
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

int AudioData::get_final_volume() {
	if(this->mute)
		return 0;
	int read_volume = this->volume;
	if(read_volume < 0)
		read_volume = 0;
	if(read_volume > 100)
		read_volume = 100;
	return read_volume;
}

int AudioData::get_real_volume() {
	int read_volume = this->volume;
	if(read_volume < 0)
		read_volume = 0;
	if(read_volume > 100)
		read_volume = 100;
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
	return false;
}

std::string AudioData::save_audio_data() {
	std::string out_str = "";
	out_str += this->mute_str + "=" + std::to_string(this->mute) + "\n";
	out_str += this->volume_str + "=" + std::to_string(this->volume) + "\n";
	return out_str;
}

void AudioData::set_audio_volume(int new_volume) {
	if(new_volume < 0)
		new_volume = 0;
	if(new_volume > 100)
		new_volume = 100;
	this->volume = new_volume;
}

void AudioData::set_audio_mute(bool new_mute) {
	this->mute = new_mute;
}

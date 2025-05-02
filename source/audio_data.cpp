#include "audio_data.hpp"
#include "utils.hpp"

int searchAudioDevice(std::string device_name, std::vector<std::string> &audio_devices) {
	int index = -1;
	for(size_t i = 0; i < audio_devices.size(); i++)
		if(audio_devices[i] == device_name) {
			index = (int)i;
			break;
		}
	if(index < 0)
		index = -1;
	return index;
}

void AudioData::reset() {
	this->volume = 100;
	this->mute = false;
	this->max_audio_latency = 2;
	this->output_type = AUDIO_OUTPUT_STEREO;
	this->mode_output = AUDIO_MODE_LOW_LATENCY;
	this->restart_request = false;
	this->text_updated = false;
	#if (defined(RASPI) || defined(ANDROID_COMPILATION))
	this->periodic_output_audio_device_scan = false;
	#else
	this->periodic_output_audio_device_scan = true;
	#endif
	this->output_device.preference_requested = false;
	this->output_device.preferred = "";
}

void AudioData::change_max_audio_latency(bool is_change_positive) {
	int change = 1;
	if(!is_change_positive)
		change = -1;
	size_t initial_max_audio_latency = this->max_audio_latency;
	this->set_max_audio_latency(((int)this->max_audio_latency) + change);
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
		this->update_text("Sound: " + this->get_audio_output_name());
}

void AudioData::change_audio_mode_output(bool is_change_positive) {
	int change = 1;
	if(!is_change_positive)
		change = -1;
	AudioMode initial_audio_mode_output = this->mode_output;
	this->set_audio_mode_output(this->mode_output + change);
	if(this->mode_output != initial_audio_mode_output)
		this->update_text("Audio Priority: " + this->get_audio_mode_name());
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

void AudioData::change_auto_device_scan() {
	this->set_auto_device_scan(!this->periodic_output_audio_device_scan);
}

void AudioData::request_audio_restart() {
	this->restart_request = true;
	this->update_text("Restarting audio...");
}

void AudioData::signal_conversion_error() {
	this->update_text("Audio conversion failed...");
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

void AudioData::set_audio_output_device_data(audio_output_device_data new_device_data) {
	bool had_preference = this->output_device.preference_requested;
	if(had_preference)
		this->output_device.preference_requested = new_device_data.preference_requested;
	this->output_device.preferred = new_device_data.preferred;
	if(!had_preference)
		this->output_device.preference_requested = new_device_data.preference_requested;
}

audio_output_device_data AudioData::get_audio_output_device_data() {
	return this->output_device;
}

std::string AudioData::text_to_print() {
	return this->text;
}

AudioOutputType AudioData::get_audio_output_type() {
	return this->output_type;
}

AudioMode AudioData::get_audio_mode_output() {
	return this->mode_output;
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

std::string AudioData::get_audio_mode_name() {
	std::string chosen_str = "";
	switch(this->mode_output) {
		case AUDIO_MODE_LOW_LATENCY:
			chosen_str = "Low Latency";
			break;
		case AUDIO_MODE_STABLE:
			chosen_str = "Stability";
			break;
		default:
			this->mode_output = AUDIO_MODE_LOW_LATENCY;
			chosen_str = "Low Latency";
			break;
	}
	return chosen_str;
}

size_t AudioData::get_max_audio_latency() {
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

bool AudioData::get_auto_device_scan() {
	return this->periodic_output_audio_device_scan;
}

bool AudioData::load_audio_data(std::string key, std::string value) {
	if (key == this->mute_str) {
		this->set_audio_mute(std::stoi(value));
		return true;
	}
	if (key == this->auto_device_scan_str) {
		this->set_auto_device_scan(std::stoi(value));
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
	if (key == this->audio_mode_output_str) {
		this->set_audio_mode_output(std::stoi(value));
		return true;
	}
	if (key == this->device_request_str) {
		this->output_device.preference_requested = std::stoi(value);
		return true;
	}
	if (key == this->device_name_str) {
		this->output_device.preferred = value;
		return true;
	}
	return false;
}

std::string AudioData::save_audio_data() {
	std::string out_str = "";
	out_str += this->mute_str + "=" + std::to_string(this->mute) + "\n";
	out_str += this->auto_device_scan_str + "=" + std::to_string(this->periodic_output_audio_device_scan) + "\n";
	out_str += this->volume_str + "=" + std::to_string(this->volume) + "\n";
	out_str += this->max_audio_latency_str + "=" + std::to_string(this->max_audio_latency) + "\n";
	out_str += this->output_type_str + "=" + std::to_string(this->output_type) + "\n";
	out_str += this->audio_mode_output_str + "=" + std::to_string(this->mode_output) + "\n";
	out_str += this->device_request_str + "=" + std::to_string(this->output_device.preference_requested) + "\n";
	out_str += this->device_name_str + "=" + this->output_device.preferred + "\n";
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

void AudioData::set_audio_mode_output(int new_value) {
	if(new_value >= AUDIO_MODE_END)
		new_value = 0;
	if(new_value < 0)
		new_value = AUDIO_MODE_END - 1;
	this->mode_output = static_cast<AudioMode>(new_value);
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

void AudioData::set_auto_device_scan(bool new_value) {
	this->periodic_output_audio_device_scan = new_value;
}

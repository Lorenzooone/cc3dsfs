#include <SFML/Audio.hpp>
#include "utils.hpp"
#include "audio.hpp"
#include "hw_defs.hpp"

#include <chrono>
#include <queue>
#include <cstring>

// Number of consecutive failures to restart the audio when switching output.
#define AUDIO_FAILURE_THRESHOLD 20

Sample::Sample(sf::Int16 *bytes, std::size_t size, float time) : bytes(bytes), size(size), time(time) {}

//============================================================================

Audio::Audio() {
	sf::SoundStream::initialize(AUDIO_CHANNELS, SAMPLE_RATE);
	#if (SFML_VERSION_MAJOR > 2) || ((SFML_VERSION_MAJOR == 2) && (SFML_VERSION_MINOR >= 6))
	// Since we receive data every 16.6 ms, this is useless,
	// and only increases CPU load... (Default is 10 ms)
	//sf::SoundStream::setProcessingInterval(sf::Time::Zero);
	#endif
	start_audio();
}
	
void Audio::update_volume(int volume, bool mute) {
	if(mute && (mute == loaded_mute)) {
		return;
	}

	if(volume < 0)
		volume = 0;
	if(volume > 100)
		volume = 100;

	if(mute != loaded_mute) {
		loaded_mute = mute;
		loaded_volume = volume;
	}
	else if(volume != loaded_volume) {
		loaded_mute = mute;
		loaded_volume = volume;
	}
	else {
		return;
	}
	setVolume(mute ? 0 : volume);
}
	
void Audio::start_audio() {
	samples_wait.try_lock();
	terminate = false;
	num_consecutive_fast_seek = 0;
	this->clock_time_start = std::chrono::high_resolution_clock::now();
}

void Audio::stop_audio() {
	terminate = true;
	samples_wait.unlock();
}

bool Audio::onGetData(sf::SoundStream::Chunk &data) {
	if(terminate)
		return false;

	// Do all of this to understand if the audio has errored out and
	// if it should be restarted...
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->clock_time_start;
	if(diff.count() < 0.0005)
		num_consecutive_fast_seek++;
	else
		num_consecutive_fast_seek = 0;
	if(num_consecutive_fast_seek > AUDIO_FAILURE_THRESHOLD)
		restart = true;

	if(restart) {
		terminate = true;
		return false;
	}

	int loaded_samples = samples.size();
	while(loaded_samples <= 0) {
		samples_wait.lock();
		if(terminate)
			return false;
		loaded_samples = samples.size();
	}
	data.samples = (const sf::Int16*)buffer;

	while(loaded_samples > 2) {
		samples.pop();
		loaded_samples = samples.size();
	}

	const sf::Int16 *data_samples = samples.front().bytes;
	int count = samples.front().size;
	memcpy(buffer, data_samples, count * sizeof(sf::Int16));

	data.sampleCount = count;

	// Unused, but could be useful info
	//double real_sample_rate = (1.0 / samples.front().time) * count / 2;

	samples.pop();

	// Basically, look into how low the time between calls of the function is
	curr_time = std::chrono::high_resolution_clock::now();
	clock_time_start = curr_time;

	return true;
}

void Audio::onSeek(sf::Time timeOffset) {}

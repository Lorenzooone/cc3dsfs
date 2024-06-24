#include <SFML/Audio.hpp>
#include "utils.hpp"
#include "audio.hpp"
#include "hw_defs.hpp"

#include <chrono>
#include <queue>
#include <cstring>

// Number of consecutive failures to restart the audio when switching output.
#define AUDIO_FAILURE_THRESHOLD 20

// Test for audio panning issues
//#define AUDIO_PANNING_TEST

#ifdef AUDIO_PANNING_TEST
static int greatest_diff = 0;
#endif

Sample::Sample(sf::Int16 *bytes, std::size_t size, float time) : bytes(bytes), size(size), time(time) {}

//============================================================================

Audio::Audio(AudioData *audio_data) {
	this->audio_data = audio_data;
	// Consume old events
	this->buffer = new sf::Int16[MAX_SAMPLES_IN * (MAX_MAX_AUDIO_LATENCY + 1)];
	this->audio_data->check_audio_restart_request();
	sf::SoundStream::initialize(AUDIO_CHANNELS, SAMPLE_RATE_BASE);
	this->setPitch(1.0 / SAMPLE_RATE_DIVISOR);
	start_audio();
	setVolume(0);
	this->final_volume = 0;
}

Audio::~Audio() {
	delete []this->buffer;
}
	
void Audio::update_volume() {
	int new_final_volume = this->audio_data->get_final_volume();
	if(this->final_volume != new_final_volume)
		setVolume(new_final_volume);
	this->final_volume = new_final_volume;
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
	if(this->audio_data->check_audio_restart_request())
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

	while(loaded_samples > this->audio_data->get_max_audio_latency()) {
		samples.pop();
		loaded_samples = samples.size();
	}

	data.sampleCount = 0;

	for(int i = 0; i < loaded_samples; i++) {
		const sf::Int16 *data_samples = samples.front().bytes;
		int count = samples.front().size;
		memcpy(buffer + data.sampleCount, data_samples, count * sizeof(sf::Int16));
		// Unused, but could be useful info
		//double real_sample_rate = (1.0 / samples.front().time) * count / 2;
		data.sampleCount += count;
		samples.pop();
	}

	if(this->audio_data->get_audio_output_type() == AUDIO_OUTPUT_MONO)
		for(int i = 0; i < data.sampleCount; i++) {
			int sum = ((int)buffer[i * 2]) + buffer[(i * 2) + 1];
			// >> is apparently implementation-dependent. Do it like this...
			int sign_mult = 1;
			if(sum < 0)
				sign_mult = -1;
			int abs_sum = sum * sign_mult;
			if(abs_sum >= 32768)
				abs_sum += 1;
			int avg = sign_mult * (abs_sum / 2);
			buffer[i * 2] = avg;
			buffer[(i * 2) + 1] = avg;
		}

	#ifdef AUDIO_PANNING_TEST
	int max_diff = 0;
	for(int i = 0; i < data.sampleCount; i++) {
		int diff = abs(buffer[i * 2] - buffer[(i * 2) + 1]);
		if(diff > max_diff)
			max_diff = diff;
	}
	if(max_diff > greatest_diff)
		greatest_diff = max_diff;
	printf("Current diff: %d - Greatest measured diff: %d\n", max_diff, greatest_diff);
	#endif

	// Basically, look into how low the time between calls of the function is
	curr_time = std::chrono::high_resolution_clock::now();
	clock_time_start = curr_time;

	return true;
}

void Audio::onSeek(sf::Time timeOffset) {}

#ifndef __AUDIO_HPP
#define __AUDIO_HPP

#include <SFML/Audio.hpp>
#include <queue>
#include "audio_data.hpp"
#include "utils.hpp"

struct Sample {
	Sample(std::int16_t *bytes, std::size_t size, double time) : bytes(bytes), size(size), time(time) {}

	std::int16_t *bytes;
	std::size_t size;
	double time;
};

class Audio : public sf::SoundStream {
public:
	volatile bool restart = false;
	std::queue<Sample> samples;
	ConsumerMutex samples_wait;

	Audio(AudioData *audio_data);
	~Audio();
	void update_volume();
	void start_audio();
	void stop_audio();
	bool hasTooMuchTimeElapsed();

private:
	AudioData *audio_data;
	int final_volume = -1;
	volatile bool inside_onGetData = false;
	volatile bool terminate = false;
	int num_consecutive_fast_seek;
	std::int16_t *buffer;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_time_start;
	std::chrono::time_point<std::chrono::high_resolution_clock> inside_clock_time_start;

	bool onGetData(sf::SoundStream::Chunk &data) override;
	void onSeek(sf::Time timeOffset) override;
	bool hasTooMuchTimeElapsedInside();
};

#endif

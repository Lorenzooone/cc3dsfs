#ifndef __AUDIO_HPP
#define __AUDIO_HPP

#include <SFML/Audio.hpp>
#include <queue>
#include "audio_data.hpp"
#include "utils.hpp"
#include "audio.hpp"
#include "capture_structs.hpp"
#include "hw_defs.hpp"

struct Sample {
	Sample(sf::Int16 *bytes, std::size_t size, float time);

	sf::Int16 *bytes;
	std::size_t size;
	float time;
};

class Audio : public sf::SoundStream {
public:
	bool restart = false;
	std::queue<Sample> samples;
	ConsumerMutex samples_wait;

	Audio(AudioData *audio_data);
	void update_volume();
	void start_audio();
	void stop_audio();

private:
	AudioData *audio_data;
	int final_volume = -1;
	bool terminate;
	int num_consecutive_fast_seek;
	sf::Int16 buffer[MAX_SAMPLES_IN];
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_time_start;

	bool onGetData(sf::SoundStream::Chunk &data) override;
	void onSeek(sf::Time timeOffset) override;
};

#endif

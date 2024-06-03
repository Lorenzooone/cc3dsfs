#ifndef __EXTRABUTTONS_HPP
#define __EXTRABUTTONS_HPP

#include <chrono>
#include "sfml_gfx_structs.hpp"

class ExtraButton {
public:
	void initialize(int pi_value, int id, sf::Keyboard::Key corresponding_key, bool is_power, float first_re_press_time, float later_re_press_time, bool use_pud_up);
	int get_pi_value();
	void poll(std::queue<SFEvent> &events_queue);
private:
	bool initialized = false;
	int id;
	sf::Keyboard::Key corresponding_key;
	bool is_power;
	int pi_value;
	bool started;
	bool after_first;
	float first_re_press_time;
	float later_re_press_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_press_time;
	bool is_time_valid;

	bool is_pressed();
	bool is_valid();
};
#endif

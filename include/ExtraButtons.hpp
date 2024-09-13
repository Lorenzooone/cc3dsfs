#ifndef __EXTRABUTTONS_HPP
#define __EXTRABUTTONS_HPP

#include <chrono>
#include "event_structs.hpp"
#ifdef RASPI
#include <gpiod.h>
#endif

class ExtraButton {
public:
	void initialize(int id, sf::Keyboard::Key corresponding_key, bool is_power, float first_re_press_time, float later_re_press_time, bool use_pud_up, std::string name);
	std::string get_name();
	bool is_button_x(sf::Keyboard::Key corresponding_key);
	void poll(std::queue<SFEvent> &events_queue);
	void end();
private:
	bool initialized = false;
	int id;
	sf::Keyboard::Key corresponding_key;
	bool is_power;
	bool started;
	bool after_first;
	float first_re_press_time;
	float later_re_press_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_press_time;
	bool is_time_valid;
	std::string name;
	#ifdef RASPI
	gpiod_line *gpioline_ptr;
	#endif

	bool is_pressed();
	bool is_valid();
};
#endif

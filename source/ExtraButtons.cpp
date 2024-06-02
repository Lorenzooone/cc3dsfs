#include "ExtraButtons.hpp"
#ifdef RASPI
#include <pigpiod_if2.h>
#endif

static ExtraButton pi_page_up, pi_page_down, pi_enter, pi_power;

void ExtraButton::initialize(int pi_value, int id, sf::Keyboard::Key corresponding_key, bool is_power, float first_re_press_time, float later_re_press_time) {
	this->pi_value = pi_value;
	this->id = id;
	this->is_power = is_power;
	this->corresponding_key = corresponding_key;
	this->started = false;
	this->initialized = true;
	this->is_time_valid = false;
	this->last_press_time = std::chrono::high_resolution_clock::now();
	this->first_re_press_time = first_re_press_time;
	this->later_re_press_time = later_re_press_time;
	#ifdef RASPI
	set_mode(this->pi_value, this->id, PI_INPUT);
	set_pull_up_down(this->pi_value, this->id, PI_PUD_UP);
	#endif
}

int ExtraButton::get_pi_value() {
	if(!this->initialized)
		return -1;
	return this->pi_value;
}

bool ExtraButton::is_pressed() {
	#ifdef RASPI
	return gpio_read(this->pi_value, this->id) == 0;
	#else
	return false;
	#endif
}

bool ExtraButton::is_valid() {
	return this->initialized && (this->id >= 0) && (this->pi_value >= 0);
}

void ExtraButton::poll(std::queue<SFEvent> &events_queue) {
	if(!this->is_valid())
		return;
	sf::Event::EventType event_kind = sf::Event::KeyReleased;
	if(this->is_pressed())
		event_kind = sf::Event::KeyPressed;
	if(event_kind == sf::Event::KeyPressed) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_press_time;
		float press_frequency_limit = this->first_re_press_time;
		if((!this->started) || this->after_first)
			press_frequency_limit = this->later_re_press_time;
		// Do this regardless, even if it was previously released.
		// This allows being more "lenient" with bad hw connections...
		if(this->is_time_valid && (diff.count() < press_frequency_limit))
			return;
		this->is_time_valid = true;
		this->last_press_time = curr_time;
		if(!this->started) {
			this->started = true;
			this->after_first = false;
		}
		else
			this->after_first = true;
	}
	else
		this->started = false;
	events_queue.emplace(event_kind, this->corresponding_key, 0, 0, 0, sf::Joystick::Axis::X, 0, sf::Mouse::Left, 0, 0, this->is_power);
}

void init_extra_buttons_poll(int page_up_id, int page_down_id, int enter_id, int power_id) {
	int pi_value = -1;
	#ifdef RASPI
	pi_value = pigpio_start(NULL, NULL);
	#endif
	pi_page_up.initialize(pi_value, page_up_id, sf::Keyboard::PageUp, false, 0.5, 0.03);
	pi_page_down.initialize(pi_value, page_down_id, sf::Keyboard::PageDown, false, 0.5, 0.03);
	pi_enter.initialize(pi_value, enter_id, sf::Keyboard::Enter, false, 0.5, 0.075);
	pi_power.initialize(pi_value, power_id, sf::Keyboard::Escape, true, 30.0, 30.0);
}

void end_extra_buttons_poll() {
	#ifdef RASPI
	int pi_value = pi_page_up.get_pi_value();
	if(pi_value >= 0)
		pigpio_stop(pi_value);
	#endif
}

void extra_buttons_poll(std::queue<SFEvent> &events_queue) {
	pi_page_down.poll(events_queue);
	pi_page_up.poll(events_queue);
	pi_enter.poll(events_queue);
	pi_power.poll(events_queue);
}

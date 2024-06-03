#include "ExtraButtons.hpp"

static ExtraButton pi_page_up, pi_page_down, pi_enter, pi_power;

void ExtraButton::initialize(int id, sf::Keyboard::Key corresponding_key, bool is_power, float first_re_press_time, float later_re_press_time, bool use_pud_up) {
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
	if(this->id >= 0) {
		std::string gpio_str = "GPIO" + std::to_string(this->id);
		this->gpioline_ptr = gpiod_line_find(gpio_str.c_str());
	}
	else
		this->gpioline_ptr = NULL;
	if(this->gpioline_ptr) {
		if(use_pud_up)
			gpiod_line_request_input_flags(this->gpioline_ptr, "cc3dsfs", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
		else
			gpiod_line_request_input_flags(this->gpioline_ptr, "cc3dsfs", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
	}
	#endif
}

void ExtraButton::end() {
	if(!initialized)
		return;
	#ifdef RASPI
	if(!this->gpioline_ptr)
		return;
	gpiod_line_close_chip(this->gpioline_ptr);
	gpiod_line_release(this->gpioline_ptr);
	this->gpioline_ptr = NULL;
	#endif
}

bool ExtraButton::is_pressed() {
	#ifdef RASPI
	if(this->gpioline_ptr)
		return gpiod_line_get_value(this->gpioline_ptr) == 0;
	#endif
	return false;
}

bool ExtraButton::is_valid() {
	#ifdef RASPI
	return this->initialized && (this->id >= 0) && this->gpioline_ptr;
	#else
	return false;
	#endif
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
	events_queue.emplace(event_kind, this->corresponding_key, 0, 0, 0, sf::Joystick::Axis::X, 0, sf::Mouse::Left, 0, 0, this->is_power, true);
}

void init_extra_buttons_poll(int page_up_id, int page_down_id, int enter_id, int power_id, bool use_pud_up) {
	pi_page_up.initialize(page_up_id, sf::Keyboard::PageUp, false, 0.5, 0.03, use_pud_up);
	pi_page_down.initialize(page_down_id, sf::Keyboard::PageDown, false, 0.5, 0.03, use_pud_up);
	pi_enter.initialize(enter_id, sf::Keyboard::Enter, false, 0.5, 0.075, use_pud_up);
	pi_power.initialize(power_id, sf::Keyboard::Escape, true, 30.0, 30.0, use_pud_up);
}

void end_extra_buttons_poll() {
	pi_page_up.end();
	pi_page_down.end();
	pi_enter.end();
	pi_power.end();
}

void extra_buttons_poll(std::queue<SFEvent> &events_queue) {
	pi_page_down.poll(events_queue);
	pi_page_up.poll(events_queue);
	pi_enter.poll(events_queue);
	pi_power.poll(events_queue);
}

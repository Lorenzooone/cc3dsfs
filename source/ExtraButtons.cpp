#include "ExtraButtons.hpp"

static ExtraButton pi_page_up, pi_page_down, pi_enter, pi_power;
static ExtraButton* pi_buttons[] = {&pi_page_up, &pi_page_down, &pi_enter, &pi_power};

void ExtraButton::initialize(int id, sf::Keyboard::Key corresponding_key, bool is_power, float first_re_press_time, float later_re_press_time, bool use_pud_up, std::string name) {
	this->id = id;
	this->is_power = is_power;
	this->corresponding_key = corresponding_key;
	this->started = false;
	this->initialized = true;
	this->is_time_valid = false;
	this->last_press_time = std::chrono::high_resolution_clock::now();
	this->first_re_press_time = first_re_press_time;
	this->later_re_press_time = later_re_press_time;
	this->name = name;
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

std::string ExtraButton::get_name() {
	if(this->is_valid())
		return this->name;
	return "";
}

bool ExtraButton::is_button_x(sf::Keyboard::Key corresponding_key) {
	return this->is_valid() && this->corresponding_key == corresponding_key;
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
	bool pressed = false;
	if(this->is_pressed())
		pressed = true;
	if(pressed) {
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
	events_queue.emplace(pressed, this->corresponding_key, this->is_power, true);
}

std::string get_extra_button_name(sf::Keyboard::Key corresponding_key) {
	for(int i = 0; i < sizeof(pi_buttons) / sizeof(pi_buttons[0]); i++)
		if(pi_buttons[i]->is_button_x(corresponding_key))
			return pi_buttons[i]->get_name();
	return "";
}

void init_extra_buttons_poll(int page_up_id, int page_down_id, int enter_id, int power_id, bool use_pud_up) {
	pi_page_up.initialize(page_up_id, sf::Keyboard::Key::PageUp, false, 0.5, 0.03, use_pud_up, "Select");
	pi_page_down.initialize(page_down_id, sf::Keyboard::Key::PageDown, false, 0.5, 0.03, use_pud_up, "Menu");
	pi_enter.initialize(enter_id, sf::Keyboard::Key::Enter, false, 0.5, 0.075, use_pud_up, "Enter");
	pi_power.initialize(power_id, sf::Keyboard::Key::Escape, true, 30.0, 30.0, use_pud_up, "Power");
}

void end_extra_buttons_poll() {
	for(int i = 0; i < sizeof(pi_buttons) / sizeof(pi_buttons[0]); i++)
		pi_buttons[i]->end();
}

void extra_buttons_poll(std::queue<SFEvent> &events_queue) {
	for(int i = 0; i < sizeof(pi_buttons) / sizeof(pi_buttons[0]); i++)
		pi_buttons[i]->poll(events_queue);
}

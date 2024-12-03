#ifndef __EVENT_STRUCTS_HPP
#define __EVENT_STRUCTS_HPP

#include <queue>
#include <SFML/Graphics.hpp>

enum JoystickDirection {JOY_DIR_NONE, JOY_DIR_UP, JOY_DIR_DOWN, JOY_DIR_LEFT, JOY_DIR_RIGHT};
enum JoystickAction {JOY_ACTION_NONE, JOY_ACTION_CONFIRM, JOY_ACTION_NEGATE, JOY_ACTION_MENU};

enum EventType {EVENT_NONE, EVENT_JOYSTICK_BTN_PRESSED, EVENT_JOYSTICK_BTN_RELEASED, EVENT_JOYSTICK_MOVED, EVENT_MOUSE_BTN_PRESSED, EVENT_MOUSE_MOVED, EVENT_KEY_PRESSED, EVENT_TEXT_ENTERED, EVENT_CLOSED, EVENT_KEY_RELEASED, EVENT_MOUSE_BTN_RELEASED};

struct SFEvent {
	SFEvent(bool pressed, sf::Keyboard::Key code, bool poweroff_cmd = false, bool is_extra = false) : type(pressed ? EVENT_KEY_PRESSED : EVENT_KEY_RELEASED), code(code), poweroff_cmd(poweroff_cmd), is_extra(is_extra) {}
	SFEvent(uint32_t unicode) : type(EVENT_TEXT_ENTERED), unicode(unicode) {}
	SFEvent(bool pressed, uint32_t joystickId, uint32_t joy_button) : type(pressed ? EVENT_JOYSTICK_BTN_PRESSED : EVENT_MOUSE_BTN_RELEASED), joystickId(joystickId), joy_button(joy_button) {}
	SFEvent(uint32_t joystickId, sf::Joystick::Axis axis, float position) : type(EVENT_JOYSTICK_MOVED), joystickId(joystickId), axis(axis), position(position) {}
	SFEvent(bool pressed, sf::Mouse::Button mouse_button, sf::Vector2i mouse_position) : type(pressed ? EVENT_MOUSE_BTN_PRESSED : EVENT_MOUSE_BTN_RELEASED), mouse_button(mouse_button), mouse_x(mouse_position.x), mouse_y(mouse_position.y) {}
	SFEvent(sf::Vector2i mouse_position) : type(EVENT_MOUSE_MOVED), mouse_x(mouse_position.x), mouse_y(mouse_position.y) {}
	SFEvent(EventType type) : type(type) {}

	EventType type;
	sf::Keyboard::Key code;
	uint32_t unicode;
	uint32_t joystickId;
	uint32_t joy_button;
	sf::Joystick::Axis axis;
	float position;
	sf::Mouse::Button mouse_button;
	int mouse_x;
	int mouse_y;
	bool poweroff_cmd;
	bool is_extra;
};

void joystick_axis_poll(std::queue<SFEvent> &events_queue);
JoystickDirection get_joystick_direction(uint32_t joystickId, sf::Joystick::Axis axis, float position);
JoystickAction get_joystick_action(uint32_t joystickId, uint32_t joy_button);
void init_extra_buttons_poll(int page_up_id, int page_down_id, int enter_id, int power_id, bool use_pud_up);
void end_extra_buttons_poll();
void extra_buttons_poll(std::queue<SFEvent> &events_queue);
std::string get_extra_button_name(sf::Keyboard::Key corresponding_key);
bool are_extra_buttons_usable();

#endif

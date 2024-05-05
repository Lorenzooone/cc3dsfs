#ifndef __SFML_GFX_STRUCTS_HPP
#define __SFML_GFX_STRUCTS_HPP

#include <queue>
#include <SFML/Graphics.hpp>

enum JoystickDirection {JOY_DIR_NONE, JOY_DIR_UP, JOY_DIR_DOWN, JOY_DIR_LEFT, JOY_DIR_RIGHT};
enum JoystickAction {JOY_ACTION_NONE, JOY_ACTION_CONFIRM, JOY_ACTION_NEGATE, JOY_ACTION_MENU};

struct out_rect_data {
	sf::RectangleShape out_rect;
	sf::RenderTexture out_tex;
};

struct SFEvent {
	SFEvent(sf::Event::EventType type, sf::Keyboard::Key code, uint32_t unicode, uint32_t joystickId, uint32_t joy_button, sf::Joystick::Axis axis, float position, sf::Mouse::Button mouse_button, int mouse_x, int mouse_y) : type(type), code(code), unicode(unicode), joystickId(joystickId), joy_button(joy_button), axis(axis), position(position), mouse_button(mouse_button), mouse_x(mouse_x), mouse_y(mouse_y) {}

	sf::Event::EventType type;
	sf::Keyboard::Key code;
	uint32_t unicode;
	uint32_t joystickId;
	uint32_t joy_button;
	sf::Joystick::Axis axis;
	float position;
	sf::Mouse::Button mouse_button;
	int mouse_x;
	int mouse_y;
};

void joystick_axis_poll(std::queue<SFEvent> &events_queue);
JoystickDirection get_joystick_direction(uint32_t joystickId, sf::Joystick::Axis axis, float position);
JoystickAction get_joystick_action(uint32_t joystickId, uint32_t joy_button);

#endif

#include "frontend.hpp"
#include <iostream>

void reset_screen_info(ScreenInfo &info) {
	info.is_blurred = false;
	info.crop_kind = DEFAULT_3DS;
	info.scaling = 1.0;
	info.is_fullscreen = false;
	info.bottom_pos = UNDER_TOP;
	info.subscreen_offset_algorithm = HALF_DISTANCE;
	info.subscreen_attached_offset_algorithm = NO_DISTANCE;
	info.total_offset_algorithm_x = HALF_DISTANCE;
	info.total_offset_algorithm_y = HALF_DISTANCE;
	info.top_rotation = 0;
	info.bot_rotation = 0;
	info.show_mouse = true;
	info.v_sync_enabled = false;
	#if defined(_WIN32) || defined(_WIN64)
	info.async = false;
	#else
	info.async = true;
	#endif
	info.top_scaling = -1;
	info.bot_scaling = -1;
	info.bfi = false;
	info.bfi_divider = 2.0;
	info.menu_scaling_factor = 1.0;
}

bool load_screen_info(std::string key, std::string value, std::string base, ScreenInfo &info) {
	if(key == (base + "blur")) {
		info.is_blurred = std::stoi(value);
		return true;
	}
	if(key == (base + "crop")) {
		info.crop_kind = static_cast<Crop>(std::stoi(value) % Crop::CROP_END);
		return true;
	}
	if(key == (base + "scale")) {
		info.scaling = std::stod(value);
		if(info.scaling < 1.25)
			info.scaling = 1.0;
		if(info.scaling > 44.75)
			info.scaling = 45.0;
		return true;
	}
	if(key == (base + "fullscreen")) {
		info.is_fullscreen = std::stoi(value);
		return true;
	}
	if(key == (base + "bot_pos")) {
		info.bottom_pos = static_cast<BottomRelativePosition>(std::stoi(value) % BottomRelativePosition::BOT_REL_POS_END);
		return true;
	}
	if(key == (base + "sub_off_algo")) {
		info.subscreen_offset_algorithm = static_cast<OffsetAlgorithm>(std::stoi(value) % OffsetAlgorithm::OFF_ALGO_END);
		return true;
	}
	if(key == (base + "sub_att_off_algo")) {
		info.subscreen_attached_offset_algorithm = static_cast<OffsetAlgorithm>(std::stoi(value) % OffsetAlgorithm::OFF_ALGO_END);
		return true;
	}
	if(key == (base + "off_algo_x")) {
		info.total_offset_algorithm_x = static_cast<OffsetAlgorithm>(std::stoi(value) % OffsetAlgorithm::OFF_ALGO_END);
		return true;
	}
	if(key == (base + "off_algo_y")) {
		info.total_offset_algorithm_y = static_cast<OffsetAlgorithm>(std::stoi(value) % OffsetAlgorithm::OFF_ALGO_END);
		return true;
	}
	if(key == (base + "top_rot")) {
		info.top_rotation = std::stoi(value);
		info.top_rotation %= 360;
		info.top_rotation += (info.top_rotation < 0) ? 360 : 0;
		return true;
	}
	if(key == (base + "bot_rot")) {
		info.bot_rotation = std::stoi(value);
		info.bot_rotation %= 360;
		info.bot_rotation += (info.bot_rotation < 0) ? 360 : 0;
		return true;
	}
	if(key == (base + "vsync")) {
		info.v_sync_enabled = std::stoi(value);
		return true;
	}
	if(key == (base + "async")) {
		info.async = std::stoi(value);
		return true;
	}
	if(key == (base + "top_scaling")) {
		info.top_scaling = std::stoi(value);
		return true;
	}
	if(key == (base + "bot_scaling")) {
		info.bot_scaling = std::stoi(value);
		return true;
	}
	if(key == (base + "bfi")) {
		info.bfi = std::stoi(value);
		return true;
	}
	if(key == (base + "bfi_divider")) {
		info.bfi_divider = std::stod(value);
		if(info.bfi_divider < 1.0)
			info.bfi_divider = 1.0;
		return true;
	}
	if(key == (base + "menu_scaling_factor")) {
		info.menu_scaling_factor = std::stod(value);
		if(info.menu_scaling_factor < 0.3)
			info.menu_scaling_factor = 0.3;
		if(info.menu_scaling_factor > 5.0)
			info.menu_scaling_factor = 5.0;
		return true;
	}
	return false;
}

std::string save_screen_info(std::string base, const ScreenInfo &info) {
	std::string out = "";
	out += base + "blur=" + std::to_string(info.is_blurred) + "\n";
	out += base + "crop=" + std::to_string(info.crop_kind) + "\n";
	out += base + "scale=" + std::to_string(info.scaling) + "\n";
	out += base + "fullscreen=" + std::to_string(info.is_fullscreen) + "\n";
	out += base + "bot_pos=" + std::to_string(info.bottom_pos) + "\n";
	out += base + "sub_off_algo=" + std::to_string(info.subscreen_offset_algorithm) + "\n";
	out += base + "sub_att_off_algo=" + std::to_string(info.subscreen_attached_offset_algorithm) + "\n";
	out += base + "off_algo_x=" + std::to_string(info.total_offset_algorithm_x) + "\n";
	out += base + "off_algo_y=" + std::to_string(info.total_offset_algorithm_y) + "\n";
	out += base + "top_rot=" + std::to_string(info.top_rotation) + "\n";
	out += base + "bot_rot=" + std::to_string(info.bot_rotation) + "\n";
	out += base + "vsync=" + std::to_string(info.v_sync_enabled) + "\n";
	out += base + "async=" + std::to_string(info.async) + "\n";
	out += base + "top_scaling=" + std::to_string(info.top_scaling) + "\n";
	out += base + "bot_scaling=" + std::to_string(info.bot_scaling) + "\n";
	out += base + "bfi=" + std::to_string(info.bfi) + "\n";
	out += base + "bfi_divider=" + std::to_string(info.bfi_divider) + "\n";
	out += base + "menu_scaling_factor=" + std::to_string(info.menu_scaling_factor) + "\n";
	return out;
}

void joystick_axis_poll(std::queue<SFEvent> &events_queue) {
	for(int i = 0; i < sf::Joystick::Count; i++) {
		if(!sf::Joystick::isConnected(i))
			continue;
		for(int j = 0; j < sf::Joystick::AxisCount; j++) {
			sf::Joystick::Axis axis = sf::Joystick::Axis(sf::Joystick::Axis::X + j);
			if(sf::Joystick::hasAxis(i, axis))
				events_queue.emplace(sf::Event::JoystickMoved, sf::Keyboard::Backspace, 0, i, 0, axis, sf::Joystick::getAxisPosition(i, axis), sf::Mouse::Left, 0, 0);
		}
	}
}

JoystickDirection get_joystick_direction(uint32_t joystickId, sf::Joystick::Axis axis, float position) {
	bool is_horizontal = false;
	if((axis == sf::Joystick::Z) || (axis == sf::Joystick::R))
		return JOY_DIR_NONE;
	if((axis == sf::Joystick::X) || (axis == sf::Joystick::U) || (axis == sf::Joystick::PovX))
		is_horizontal = true;
	int direction = 0;
	if(position >= 90.0)
		direction = 1;
	if(position <= -90.0)
		direction = -1;
	if(direction == 0)
		return JOY_DIR_NONE;
	if(direction > 0) {
		if(is_horizontal)
			return JOY_DIR_RIGHT;
		return JOY_DIR_DOWN;
	}
	if(is_horizontal)
		return JOY_DIR_LEFT;
	return JOY_DIR_UP;
}

JoystickAction get_joystick_action(uint32_t joystickId, uint32_t joy_button) {
	if((joy_button == 0) || (joy_button == 1))
		return JOY_ACTION_CONFIRM;
	if((joy_button == 2) || (joy_button == 3))
		return JOY_ACTION_NEGATE;
	return JOY_ACTION_NONE;
}

void default_sleep() {
	sf::sleep(sf::milliseconds(1000/USB_CHECKS_PER_SECOND));
}

void update_output(FrontendData* frontend_data, double frame_time, VideoOutputData *out_buf) {
	if(frontend_data->reload) {
		frontend_data->top_screen->reload();
		frontend_data->bot_screen->reload();
		frontend_data->joint_screen->reload();
		frontend_data->reload = false;
	}
	// Make sure the window is closed before showing split/non-split
	if(frontend_data->display_data.split)
		frontend_data->joint_screen->draw(frame_time, out_buf);
	frontend_data->top_screen->draw(frame_time, out_buf);
	frontend_data->bot_screen->draw(frame_time, out_buf);
	if(!frontend_data->display_data.split)
		frontend_data->joint_screen->draw(frame_time, out_buf);
}

void screen_display_thread(WindowScreen *screen, CaptureStatus* capture_status) {
	screen->display_thread(capture_status);
}

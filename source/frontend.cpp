#include "frontend.hpp"
#include <iostream>

static float par_width_multiplier[] = {1.0, 8.0, 8.0};
static float par_width_divisor[] = {1.0, 7.0, 7.0};
static bool par_width_main[] = {true, true, false};

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
	info.rounded_corners_fix = false;
	info.top_par = ParCorrection::PAR_NORMAL;
	info.bot_par = ParCorrection::PAR_NORMAL;
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
	if(key == (base + "rounded_corners_fix")) {
		info.rounded_corners_fix = std::stoi(value);
		return true;
	}
	if(key == (base + "top_par")) {
		info.top_par = static_cast<ParCorrection>(std::stoi(value) % ParCorrection::PAR_END);
		return true;
	}
	if(key == (base + "bot_par")) {
		info.bot_par = static_cast<ParCorrection>(std::stoi(value) % ParCorrection::PAR_END);
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
	out += base + "rounded_corners_fix=" + std::to_string(info.rounded_corners_fix) + "\n";
	out += base + "top_par=" + std::to_string(info.top_par) + "\n";
	out += base + "bot_par=" + std::to_string(info.bot_par) + "\n";
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

void get_par_size(int &width, int &height, float multiplier_factor, ParCorrection &correction_factor) {
	if(correction_factor >= ParCorrection::PAR_END)
		correction_factor = ParCorrection::PAR_NORMAL;
	int par_corr_index = correction_factor - ParCorrection::PAR_NORMAL;
	width *= multiplier_factor;
	height *= multiplier_factor;
	if(par_width_main[par_corr_index])
		width = (width * par_width_multiplier[par_corr_index]) / par_width_divisor[par_corr_index];
	else
		height = (height * par_width_divisor[par_corr_index]) / par_width_multiplier[par_corr_index];
}

void get_par_size(float &width, float &height, float multiplier_factor, ParCorrection &correction_factor) {
	if(correction_factor >= ParCorrection::PAR_END)
		correction_factor = ParCorrection::PAR_NORMAL;
	int par_corr_index = correction_factor - ParCorrection::PAR_NORMAL;
	width *= multiplier_factor;
	height *= multiplier_factor;
	if(par_width_main[par_corr_index]) {
		width = (width * par_width_multiplier[par_corr_index]) / par_width_divisor[par_corr_index];
	}
	else
		height = (height * par_width_divisor[par_corr_index]) / par_width_multiplier[par_corr_index];
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
	if((joy_button == 4) || (joy_button == 5))
		return JOY_ACTION_MENU;
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

void screen_display_thread(WindowScreen *screen) {
	screen->display_thread();
}

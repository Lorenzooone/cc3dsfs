#include "frontend.hpp"
#include <iostream>

const CropData default_3ds_crop = {
.top_width = TOP_WIDTH_3DS, .top_height = HEIGHT_3DS,
.top_x = 0, .top_y = 0,
.bot_width = BOT_WIDTH_3DS, .bot_height = HEIGHT_3DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.name = "3DS"};

const CropData special_ds_crop = {
.top_width = TOP_SPECIAL_DS_WIDTH_3DS, .top_height = HEIGHT_3DS,
.top_x = (TOP_WIDTH_3DS - TOP_SPECIAL_DS_WIDTH_3DS) / 2, .top_y = 0,
.bot_width = BOT_WIDTH_3DS, .bot_height = HEIGHT_3DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "16:10"};

const CropData scaled_ds_crop = {
.top_width = TOP_SCALED_DS_WIDTH_3DS, .top_height = HEIGHT_3DS,
.top_x = (TOP_WIDTH_3DS - TOP_SCALED_DS_WIDTH_3DS) / 2, .top_y = 0,
.bot_width = BOT_WIDTH_3DS, .bot_height = HEIGHT_3DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "Scaled DS"};

const CropData native_ds_crop = {
.top_width = WIDTH_DS, .top_height = HEIGHT_DS,
.top_x = (TOP_WIDTH_3DS - WIDTH_DS) / 2, .top_y = 0,
.bot_width = WIDTH_DS, .bot_height = HEIGHT_DS,
.bot_x = (BOT_WIDTH_3DS - WIDTH_DS) / 2, .bot_y = HEIGHT_3DS - HEIGHT_DS,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.name = "Native DS"};

const CropData scaled_gba_crop = {
.top_width = WIDTH_SCALED_GBA, .top_height = HEIGHT_SCALED_GBA,
.top_x = (TOP_WIDTH_3DS - WIDTH_SCALED_GBA) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SCALED_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "Scaled GBA"};

const CropData native_gba_crop = {
.top_width = WIDTH_GBA, .top_height = HEIGHT_GBA,
.top_x = (TOP_WIDTH_3DS - WIDTH_GBA) / 2, .top_y = (HEIGHT_3DS - HEIGHT_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "Native GBA"};

const CropData scaled_vc_gb_crop = {
.top_width = WIDTH_SCALED_GB, .top_height = HEIGHT_SCALED_GB,
.top_x = (TOP_WIDTH_3DS - WIDTH_SCALED_GB) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SCALED_GB) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "Scaled VC GB"};

const CropData vc_gb_crop = {
.top_width = WIDTH_GB, .top_height = HEIGHT_GB,
.top_x = (TOP_WIDTH_3DS - WIDTH_GB) / 2, .top_y = (HEIGHT_3DS - HEIGHT_GB) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "VC GB"};

const CropData scaled_snes_crop = {
.top_width = WIDTH_SCALED_SNES, .top_height = HEIGHT_SCALED_SNES,
.top_x = (TOP_WIDTH_3DS - WIDTH_SCALED_SNES) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SCALED_SNES) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "Scaled SNES"};

const CropData vc_snes_crop = {
.top_width = WIDTH_SNES, .top_height = HEIGHT_SNES,
.top_x = (TOP_WIDTH_3DS - WIDTH_SNES) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SNES) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "VC SNES"};

const CropData vc_nes_crop = {
.top_width = WIDTH_NES, .top_height = HEIGHT_NES,
.top_x = (TOP_WIDTH_3DS - WIDTH_NES) / 2, .top_y = (HEIGHT_3DS - HEIGHT_NES) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.name = "VC NES"};

static const CropData* basic_possible_crops[] = {
&default_3ds_crop,
&special_ds_crop,
&scaled_ds_crop,
&native_ds_crop,
&scaled_gba_crop,
&native_gba_crop,
&scaled_vc_gb_crop,
&vc_gb_crop,
&scaled_snes_crop,
&vc_snes_crop,
&vc_nes_crop,
};

const PARData base_par = {
.width_multiplier = 1.0, .width_divisor = 1.0,
.is_width_main = true,
.name = "1:1"};

const PARData snes_horizontal_par = {
.width_multiplier = 8.0, .width_divisor = 7.0,
.is_width_main = true,
.name = "SNES Horizontal"};

const PARData snes_vertical_par = {
.width_multiplier = 8.0, .width_divisor = 7.0,
.is_width_main = false,
.name = "SNES Vertical"};

const PARData special_3ds_horizontal_par = {
.width_multiplier = 1.0, .width_divisor = 2.0,
.is_width_main = true,
.name = "3DS 800x240 Horizontal"};

const PARData special_3ds_vertical_par = {
.width_multiplier = 1.0, .width_divisor = 2.0,
.is_width_main = false,
.name = "3DS 800x240 Vertical"};

static const PARData* basic_possible_pars[] = {
&base_par,
&snes_horizontal_par,
&snes_vertical_par,
//&special_3ds_horizontal_par,
//&special_3ds_vertical_par,
};

bool is_allowed_crop(const CropData* crop_data, ScreenType s_type) {
	if(s_type == ScreenType::JOINT && crop_data->allowed_joint)
		return true;
	if(s_type == ScreenType::TOP && crop_data->allowed_top)
		return true;
	if(s_type == ScreenType::BOTTOM && crop_data->allowed_bottom)
		return true;
	return false;
}

void insert_basic_crops(std::vector<const CropData*> &crop_vector, ScreenType s_type) {
	for(int i = 0; i < (sizeof(basic_possible_crops) / sizeof(basic_possible_crops[0])); i++) {
		if(is_allowed_crop(basic_possible_crops[i], s_type))
			crop_vector.push_back(basic_possible_crops[i]);
	}
}

void insert_basic_pars(std::vector<const PARData*> &par_vector) {
	for(int i = 0; i < (sizeof(basic_possible_pars) / sizeof(basic_possible_pars[0])); i++) {
		par_vector.push_back(basic_possible_pars[i]);
	}
}

void reset_display_data(DisplayData *display_data) {
	display_data->split = false;
	display_data->fast_poll = false;
}

void reset_fullscreen_info(ScreenInfo &info) {
	info.fullscreen_mode_width = 0;
	info.fullscreen_mode_height = 0;
	info.fullscreen_mode_bpp = 0;
}

void reset_screen_info(ScreenInfo &info) {
	info.is_blurred = false;
	info.crop_kind = 0;
	info.scaling = 1.0;
	info.is_fullscreen = false;
	info.bottom_pos = UNDER_TOP;
	info.subscreen_offset = 0.5;
	info.subscreen_attached_offset = 0.0;
	info.total_offset_x = 0.5;
	info.total_offset_y = 0.5;
	info.top_rotation = 0;
	info.bot_rotation = 0;
	info.show_mouse = true;
	info.v_sync_enabled = false;
	info.async = true;
	info.top_scaling = -1;
	info.bot_scaling = -1;
	info.bfi = false;
	info.bfi_divider = 2;
	info.bfi_amount = 1;
	info.menu_scaling_factor = 1.0;
	info.rounded_corners_fix = false;
	info.top_par = 0;
	info.bot_par = 0;
	reset_fullscreen_info(info);
}

static float offset_sanitization(float value) {
	if(value <= 0.0)
		return 0.0;
	if(value >= 1.0)
		return 1.0;
	return value;
}

bool load_screen_info(std::string key, std::string value, std::string base, ScreenInfo &info) {
	if(key == (base + "blur")) {
		info.is_blurred = std::stoi(value);
		return true;
	}
	if(key == (base + "crop")) {
		info.crop_kind = std::stoi(value);
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
	if(key == (base + "sub_off")) {
		info.subscreen_offset = offset_sanitization(std::stod(value));
		return true;
	}
	if(key == (base + "sub_att_off")) {
		info.subscreen_attached_offset = offset_sanitization(std::stod(value));
		return true;
	}
	if(key == (base + "off_x")) {
		info.total_offset_x = offset_sanitization(std::stod(value));
		return true;
	}
	if(key == (base + "off_y")) {
		info.total_offset_y = offset_sanitization(std::stod(value));
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
		info.bfi_divider = std::stoi(value);
		if(info.bfi_divider < 2)
			info.bfi_divider = 2;
		if(info.bfi_divider > 10)
			info.bfi_divider = 10;
		if(info.bfi_amount > (info.bfi_divider - 1))
			info.bfi_amount = info.bfi_divider - 1;
		return true;
	}
	if(key == (base + "bfi_amount")) {
		info.bfi_amount = std::stoi(value);
		if(info.bfi_amount < 1)
			info.bfi_amount = 1;
		if(info.bfi_amount > (info.bfi_divider - 1))
			info.bfi_amount = info.bfi_divider - 1;
		return true;
	}
	if(key == (base + "menu_scaling_factor")) {
		info.menu_scaling_factor = std::stod(value);
		if(info.menu_scaling_factor < 0.3)
			info.menu_scaling_factor = 0.3;
		if(info.menu_scaling_factor > 10.0)
			info.menu_scaling_factor = 10.0;
		return true;
	}
	if(key == (base + "rounded_corners_fix")) {
		info.rounded_corners_fix = std::stoi(value);
		return true;
	}
	if(key == (base + "top_par")) {
		info.top_par = std::stoi(value);
		return true;
	}
	if(key == (base + "bot_par")) {
		info.bot_par = std::stoi(value);
		return true;
	}
	if(key == (base + "fullscreen_mode_width")) {
		info.fullscreen_mode_width = std::stoi(value);
		return true;
	}
	if(key == (base + "fullscreen_mode_height")) {
		info.fullscreen_mode_height = std::stoi(value);
		return true;
	}
	if(key == (base + "fullscreen_mode_bpp")) {
		info.fullscreen_mode_bpp = std::stoi(value);
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
	out += base + "sub_off=" + std::to_string(info.subscreen_offset) + "\n";
	out += base + "sub_att_off=" + std::to_string(info.subscreen_attached_offset) + "\n";
	out += base + "off_x=" + std::to_string(info.total_offset_x) + "\n";
	out += base + "off_y=" + std::to_string(info.total_offset_y) + "\n";
	out += base + "top_rot=" + std::to_string(info.top_rotation) + "\n";
	out += base + "bot_rot=" + std::to_string(info.bot_rotation) + "\n";
	out += base + "vsync=" + std::to_string(info.v_sync_enabled) + "\n";
	out += base + "async=" + std::to_string(info.async) + "\n";
	out += base + "top_scaling=" + std::to_string(info.top_scaling) + "\n";
	out += base + "bot_scaling=" + std::to_string(info.bot_scaling) + "\n";
	out += base + "bfi=" + std::to_string(info.bfi) + "\n";
	out += base + "bfi_divider=" + std::to_string(info.bfi_divider) + "\n";
	out += base + "bfi_amount=" + std::to_string(info.bfi_amount) + "\n";
	out += base + "menu_scaling_factor=" + std::to_string(info.menu_scaling_factor) + "\n";
	out += base + "rounded_corners_fix=" + std::to_string(info.rounded_corners_fix) + "\n";
	out += base + "top_par=" + std::to_string(info.top_par) + "\n";
	out += base + "bot_par=" + std::to_string(info.bot_par) + "\n";
	out += base + "fullscreen_mode_width=" + std::to_string(info.fullscreen_mode_width) + "\n";
	out += base + "fullscreen_mode_height=" + std::to_string(info.fullscreen_mode_height) + "\n";
	out += base + "fullscreen_mode_bpp=" + std::to_string(info.fullscreen_mode_bpp) + "\n";
	return out;
}

void joystick_axis_poll(std::queue<SFEvent> &events_queue) {
	for(int i = 0; i < sf::Joystick::Count; i++) {
		if(!sf::Joystick::isConnected(i))
			continue;
		for(int j = 0; j < sf::Joystick::AxisCount; j++) {
			sf::Joystick::Axis axis = sf::Joystick::Axis(sf::Joystick::Axis::X + j);
			if(sf::Joystick::hasAxis(i, axis))
				events_queue.emplace(sf::Event::JoystickMoved, sf::Keyboard::Backspace, 0, i, 0, axis, sf::Joystick::getAxisPosition(i, axis), sf::Mouse::Left, 0, 0, false, false);
		}
	}
}

void get_par_size(int &width, int &height, float multiplier_factor, const PARData *correction_factor) {
	width *= multiplier_factor;
	height *= multiplier_factor;
	if(correction_factor->is_width_main)
		width = (width * correction_factor->width_multiplier) / correction_factor->width_divisor;
	else
		height = (height * correction_factor->width_divisor) / correction_factor->width_multiplier;
}

void get_par_size(float &width, float &height, float multiplier_factor, const PARData *correction_factor) {
	width *= multiplier_factor;
	height *= multiplier_factor;
	if(correction_factor->is_width_main) {
		width = (width * correction_factor->width_multiplier) / correction_factor->width_divisor;
	}
	else
		height = (height * correction_factor->width_divisor) / correction_factor->width_multiplier;
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
	if((joy_button == 6) || (joy_button == 7))
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

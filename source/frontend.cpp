#include "frontend.hpp"

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

void update_output(WindowScreen &top_screen, WindowScreen &bot_screen, WindowScreen &joint_screen, bool &reload, bool split, double frame_time, VideoOutputData *out_buf) {
	if(reload) {
		top_screen.reload();
		bot_screen.reload();
		joint_screen.reload();
		reload = false;
	}
	// Make sure the window is closed before showing split/non-split
	if(split)
		joint_screen.draw(frame_time, out_buf);
	top_screen.draw(frame_time, out_buf);
	bot_screen.draw(frame_time, out_buf);
	if(!split)
		joint_screen.draw(frame_time, out_buf);
}

void screen_display_thread(WindowScreen *screen, CaptureStatus* capture_status) {
	screen->display_thread(capture_status);
}

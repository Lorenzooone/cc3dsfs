#ifndef __DISPLAY_STRUCTS_HPP
#define __DISPLAY_STRUCTS_HPP

#include "utils.hpp"
#include "hw_defs.hpp"

enum ScreenType { TOP, BOTTOM, JOINT };
enum BottomRelativePosition { UNDER_TOP, LEFT_TOP, ABOVE_TOP, RIGHT_TOP, BOT_REL_POS_END };
enum OffsetAlgorithm { NO_DISTANCE, HALF_DISTANCE, MAX_DISTANCE, OFF_ALGO_END };
enum CurrMenuType { DEFAULT_MENU_TYPE, CONNECT_MENU_TYPE, MAIN_MENU_TYPE, VIDEO_MENU_TYPE, AUDIO_MENU_TYPE };

struct ScreenInfo {
	bool is_blurred;
	int crop_kind;
	double scaling;
	bool is_fullscreen;
	BottomRelativePosition bottom_pos;
	OffsetAlgorithm subscreen_offset_algorithm, subscreen_attached_offset_algorithm, total_offset_algorithm_x, total_offset_algorithm_y;
	int top_rotation, bot_rotation;
	bool show_mouse;
	bool v_sync_enabled;
	bool async;
	int top_scaling, bot_scaling;
	bool bfi;
	double bfi_divider;
	double menu_scaling_factor;
	bool rounded_corners_fix;
	int top_par;
	int bot_par;
};

struct DisplayData {
	bool split = false;
};

#pragma pack(push, 1)

struct PACKED VideoOutputData {
	uint8_t screen_data[IN_VIDEO_SIZE][4];
};

#pragma pack(pop)

struct CropData {
	int top_width;
	int top_height;
	int top_x;
	int top_y;
	int bot_width;
	int bot_height;
	int bot_x;
	int bot_y;
	bool allowed_joint;
	bool allowed_top;
	bool allowed_bottom;
	std::string name;
};

struct PARData {
	float width_multiplier;
	float width_divisor;
	bool is_width_main;
	std::string name;
};

#endif

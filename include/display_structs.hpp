#ifndef __DISPLAY_STRUCTS_HPP
#define __DISPLAY_STRUCTS_HPP

#include "utils.hpp"
#include "hw_defs.hpp"

enum ScreenType { TOP, BOTTOM, JOINT };
enum Crop { DEFAULT_3DS, SPECIAL_DS, SCALED_DS, NATIVE_DS, SCALED_GBA, NATIVE_GBA, SCALED_GB, NATIVE_GB, SCALED_SNES, NATIVE_SNES, NATIVE_NES, CROP_END };
enum ParCorrection { PAR_NORMAL, PAR_SNES_HORIZONTAL, PAR_SNES_VERTICAL, PAR_END };
enum BottomRelativePosition { UNDER_TOP, LEFT_TOP, ABOVE_TOP, RIGHT_TOP, BOT_REL_POS_END };
enum OffsetAlgorithm { NO_DISTANCE, HALF_DISTANCE, MAX_DISTANCE, OFF_ALGO_END };
enum CurrMenuType { DEFAULT_MENU_TYPE, CONNECT_MENU_TYPE, MAIN_MENU_TYPE };

struct ScreenInfo {
	bool is_blurred;
	Crop crop_kind;
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
	ParCorrection top_par;
	ParCorrection bot_par;
};

struct DisplayData {
	bool split = false;
};

#pragma pack(push, 1)

struct PACKED VideoOutputData {
	uint8_t screen_data[IN_VIDEO_SIZE][4];
};

#pragma pack(pop)

#endif

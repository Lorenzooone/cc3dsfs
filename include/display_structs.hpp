#ifndef __DISPLAY_STRUCTS_HPP
#define __DISPLAY_STRUCTS_HPP

#include "utils.hpp"
#include "hw_defs.hpp"
#include "shaders_list.hpp"
#include "WindowCommands.hpp"

#define DEFAULT_NO_POS_WINDOW_VALUE -1
#define DEFAULT_NO_VOLUME_VALUE -1
#define DEFAULT_NO_ENABLED_VALUE -1
#define DEFAULT_NO_SCALING_VALUE -1

#define DEFAULT_WINDOW_SCALING_VALUE 1.0
#define MIN_WINDOW_SCALING_VALUE 1.0
#define MAX_WINDOW_SCALING_VALUE 45.0
#define WINDOW_SCALING_CHANGE 0.5

enum ScreenType { TOP, BOTTOM, JOINT };
enum BottomRelativePosition { UNDER_TOP, LEFT_TOP, ABOVE_TOP, RIGHT_TOP, BOT_REL_POS_END };
enum NonIntegerScalingModes { SMALLER_PRIORITY, INVERSE_PROPORTIONAL_PRIORITY, EQUAL_PRIORITY, PROPORTIONAL_PRIORITY, BIGGER_PRIORITY, END_NONINT_SCALE_MODES };
enum CurrMenuType { DEFAULT_MENU_TYPE, CONNECT_MENU_TYPE, MAIN_MENU_TYPE, VIDEO_MENU_TYPE, AUDIO_MENU_TYPE, CROP_MENU_TYPE, TOP_PAR_MENU_TYPE, BOTTOM_PAR_MENU_TYPE, ROTATION_MENU_TYPE, OFFSET_MENU_TYPE, BFI_MENU_TYPE, LOAD_MENU_TYPE, SAVE_MENU_TYPE, RESOLUTION_MENU_TYPE, EXTRA_MENU_TYPE, STATUS_MENU_TYPE, LICENSES_MENU_TYPE, RELATIVE_POS_MENU_TYPE, SHORTCUTS_MENU_TYPE, ACTION_SELECTION_MENU_TYPE, SCALING_RATIO_MENU_TYPE, ISN_MENU_TYPE, VIDEO_EFFECTS_MENU_TYPE, INPUT_MENU_TYPE, AUDIO_DEVICE_MENU_TYPE };
enum InputColorspaceMode { FULL_COLORSPACE, DS_COLORSPACE, GBA_COLORSPACE, INPUT_COLORSPACE_END };
enum FrameBlendingMode { NO_FRAME_BLENDING, FULL_FRAME_BLENDING, DS_3D_BOTH_SCREENS_FRAME_BLENDING, FRAME_BLENDING_END };

struct override_win_data {
	int pos_x = DEFAULT_NO_POS_WINDOW_VALUE;
	int pos_y = DEFAULT_NO_POS_WINDOW_VALUE;
	int enabled = DEFAULT_NO_ENABLED_VALUE;
	double scaling = DEFAULT_NO_SCALING_VALUE;
};

struct ScreenInfo {
	bool is_blurred;
	int crop_kind;
	int crop_kind_ds;
	bool allow_games_crops;
	double scaling;
	bool is_fullscreen;
	BottomRelativePosition bottom_pos;
	float subscreen_offset, subscreen_attached_offset, total_offset_x, total_offset_y;
	int top_rotation, bot_rotation;
	bool show_mouse;
	bool v_sync_enabled;
	bool async;
	int top_scaling, bot_scaling;
	bool force_same_scaling;
	float non_integer_top_scaling, non_integer_bot_scaling;
	bool bfi;
	int bfi_divider;
	int bfi_amount;
	double menu_scaling_factor;
	bool rounded_corners_fix;
	int top_par;
	int bot_par;
	int fullscreen_mode_width;
	int fullscreen_mode_height;
	int fullscreen_mode_bpp;
	NonIntegerScalingModes non_integer_mode;
	bool use_non_integer_scaling_top;
	bool use_non_integer_scaling_bottom;
	bool failed_fullscreen;
	bool have_titlebar;
	InputColorspaceMode in_colorspace_top;
	InputColorspaceMode in_colorspace_bot;
	FrameBlendingMode frame_blending_top;
	FrameBlendingMode frame_blending_bot;
	bool window_enabled;
	int initial_pos_x;
	int initial_pos_y; 
};

struct DisplayData {
	bool mono_app_mode;
	bool last_connected_ds;
	bool force_disable_mouse;
};

struct ExtraButtonShortcuts {
	const WindowCommand *enter_shortcut;
	const WindowCommand *page_up_shortcut;
};

struct InputData {
	bool fast_poll;
	bool enable_controller_input;
	bool enable_keyboard_input;
	bool enable_mouse_input;
	bool enable_buttons_input;
	ExtraButtonShortcuts extra_button_shortcuts;
};

struct SharedData {
	InputData input_data;
};

struct ALIGNED(8) VideoOutputData {
	uint8_t screen_data[MAX_IN_VIDEO_SIZE][3];
};

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
	bool allowed_3ds;
	bool allowed_ds;
	bool is_game_specific;
	std::string name;
};

struct PARData {
	float width_multiplier;
	float width_divisor;
	bool is_width_main;
	bool is_fit;
	std::string name;
};

#endif

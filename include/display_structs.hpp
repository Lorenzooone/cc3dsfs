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

#define SEP_FOLLOW_SCALING_MULTIPLIER WINDOW_SCALING_CHANGE
#define SEP_WINDOW_SCALING_MIN_MULTIPLIER SEP_FOLLOW_SCALING_MULTIPLIER
#define SEP_FULLSCREEN_SCALING_MIN_MULTIPLIER DEFAULT_WINDOW_SCALING_VALUE
#define DEFAULT_SEP_SIZE 0
#define MAX_SEP_SIZE 1000

enum ScreenType { TOP, BOTTOM, JOINT };
enum BottomRelativePosition { UNDER_TOP, LEFT_TOP, ABOVE_TOP, RIGHT_TOP, BOT_REL_POS_END };
enum SecondScreen3DRelativePosition { UNDER_FIRST, LEFT_FIRST, ABOVE_FIRST, RIGHT_FIRST, SECOND_SCREEN_3D_REL_POS_END };
enum NonIntegerScalingModes { SMALLER_PRIORITY, INVERSE_PROPORTIONAL_PRIORITY, EQUAL_PRIORITY, PROPORTIONAL_PRIORITY, BIGGER_PRIORITY, END_NONINT_SCALE_MODES };
enum CurrMenuType { DEFAULT_MENU_TYPE, CONNECT_MENU_TYPE, MAIN_MENU_TYPE, VIDEO_MENU_TYPE, AUDIO_MENU_TYPE, CROP_MENU_TYPE, TOP_PAR_MENU_TYPE, BOTTOM_PAR_MENU_TYPE, ROTATION_MENU_TYPE, OFFSET_MENU_TYPE, BFI_MENU_TYPE, LOAD_MENU_TYPE, SAVE_MENU_TYPE, RESOLUTION_MENU_TYPE, EXTRA_MENU_TYPE, STATUS_MENU_TYPE, LICENSES_MENU_TYPE, RELATIVE_POS_MENU_TYPE, SHORTCUTS_MENU_TYPE, ACTION_SELECTION_MENU_TYPE, SCALING_RATIO_MENU_TYPE, ISN_MENU_TYPE, VIDEO_EFFECTS_MENU_TYPE, INPUT_MENU_TYPE, AUDIO_DEVICE_MENU_TYPE, SEPARATOR_MENU_TYPE, COLOR_CORRECTION_MENU_TYPE, MAIN_3D_MENU_TYPE, SECOND_SCREEN_RELATIVE_POS_MENU_TYPE, USB_CONFLICT_RESOLUTION_MENU_TYPE, OPTIMIZE_3DS_MENU_TYPE, OPTIMIZE_SERIAL_KEY_ADD_MENU_TYPE };
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
	SecondScreen3DRelativePosition second_screen_pos;
	bool match_bottom_pos_and_second_screen_pos;
	float subscreen_offset, subscreen_attached_offset, total_offset_x, total_offset_y;
	int top_rotation, bot_rotation;
	bool show_mouse;
	bool v_sync_enabled;
	bool async;
	int top_scaling, bot_scaling;
	int separator_pixel_size;
	float separator_windowed_multiplier;
	float separator_fullscreen_multiplier;
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
	int top_color_correction;
	int bot_color_correction;
	InputColorspaceMode in_colorspace_top;
	InputColorspaceMode in_colorspace_bot;
	FrameBlendingMode frame_blending_top;
	FrameBlendingMode frame_blending_bot;
	bool squish_3d_top;
	bool squish_3d_bot;
	bool window_enabled;
	int initial_pos_x;
	int initial_pos_y; 
};

struct DisplayData {
	bool mono_app_mode;
	bool last_connected_ds;
	bool interleaved_3d;
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

#pragma pack(push, 1)

// The internal order needs to be reversed... This is so confusing...
struct PACKED ALIGNED(2) VideoPixelRGB16 {
	uint16_t b : 5;
	uint16_t g : 6;
	uint16_t r : 5;
};

// The internal order needs to be reversed... This is so confusing...
struct PACKED ALIGNED(2) VideoPixelBGR16 {
	uint16_t r : 5;
	uint16_t g : 6;
	uint16_t b : 5;
};

struct PACKED VideoPixelRGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct PACKED VideoPixelBGR {
	uint8_t b;
	uint8_t g;
	uint8_t r;
};

struct PACKED VideoPixelRGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t alpha;
};

struct PACKED ALIGNED(16) VideoOutputDataRGB16 {
	VideoPixelRGB16 screen_data[MAX_IN_VIDEO_SIZE];
};

struct PACKED ALIGNED(16) VideoOutputDataBGR16 {
	VideoPixelBGR16 screen_data[MAX_IN_VIDEO_SIZE];
};

struct PACKED ALIGNED(16) VideoOutputDataRGB {
	VideoPixelRGB screen_data[MAX_IN_VIDEO_SIZE];
};

struct PACKED ALIGNED(16) VideoOutputDataBGR {
	VideoPixelBGR screen_data[MAX_IN_VIDEO_SIZE];
};

struct PACKED ALIGNED(16) VideoOutputDataRGBA {
	VideoPixelRGBA screen_data[MAX_IN_VIDEO_SIZE];
};

union PACKED ALIGNED(16) VideoOutputData {
	VideoOutputDataRGB16 rgb16_video_output_data;
	VideoOutputDataBGR16 bgr16_video_output_data;
	VideoOutputDataRGB rgb_video_output_data;
	VideoOutputDataBGR bgr_video_output_data;
	VideoOutputDataRGBA rgba_video_output_data;
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

struct ShaderColorEmulationData {
	float targetGamma;
	float lum;
	float rgb_mod[3][3];
	float displayGamma;
	bool is_valid;
	std::string name;
};

#endif

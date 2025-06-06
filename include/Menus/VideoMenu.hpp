#ifndef __VIDEOMENU_HPP
#define __VIDEOMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum VideoMenuOutAction{
	VIDEO_MENU_NO_ACTION,
	VIDEO_MENU_BACK,
	VIDEO_MENU_VSYNC,
	VIDEO_MENU_ASYNC,
	VIDEO_MENU_BLUR,
	VIDEO_MENU_PADDING,
	VIDEO_MENU_CROPPING,
	VIDEO_MENU_GAMES_CROPPING,
	VIDEO_MENU_TOP_PAR,
	VIDEO_MENU_BOT_PAR,
	VIDEO_MENU_ONE_PAR,
	VIDEO_MENU_BOTTOM_SCREEN_POS,
	VIDEO_MENU_SMALL_SCREEN_OFFSET_DEC,
	VIDEO_MENU_SMALL_SCREEN_OFFSET_INC,
	VIDEO_MENU_OFFSET_SETTINGS,
	VIDEO_MENU_WINDOW_SCALING_DEC,
	VIDEO_MENU_WINDOW_SCALING_INC,
	VIDEO_MENU_BFI_SETTINGS,
	VIDEO_MENU_MENU_SCALING_DEC,
	VIDEO_MENU_MENU_SCALING_INC,
	VIDEO_MENU_RESOLUTION_SETTINGS,
	VIDEO_MENU_ROTATION_SETTINGS,
	VIDEO_MENU_TOP_ROTATION_DEC,
	VIDEO_MENU_TOP_ROTATION_INC,
	VIDEO_MENU_BOTTOM_ROTATION_DEC,
	VIDEO_MENU_BOTTOM_ROTATION_INC,
	//VIDEO_MENU_FAST_POLL,
	VIDEO_MENU_NON_INT_SCALING,
	VIDEO_MENU_SCALING_RATIO_SETTINGS,
	VIDEO_MENU_CHANGE_TITLEBAR,
	VIDEO_MENU_VIDEO_EFFECTS_SETTINGS,
	VIDEO_MENU_SEPARATOR_SETTINGS,
	VIDEO_MENU_3D_SETTINGS,
};

class VideoMenu : public OptionSelectionMenu {
public:
	VideoMenu(TextRectanglePool* text_pool);
	~VideoMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, ScreenType screen_type);
	void insert_data(ScreenType s_type, bool is_fullscreen, bool can_have_titlebar);
	VideoMenuOutAction selected_index = VideoMenuOutAction::VIDEO_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_inc_dec(int index);
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

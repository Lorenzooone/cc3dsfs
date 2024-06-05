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
	VIDEO_MENU_TOP_PAR,
	VIDEO_MENU_BOT_PAR,
	VIDEO_MENU_ONE_PAR,
	VIDEO_MENU_BOTTOM_SCREEN_POS,
	VIDEO_MENU_SMALL_SCREEN_OFFSET_DEC,
	VIDEO_MENU_SMALL_SCREEN_OFFSET_INC,
	VIDEO_MENU_OFFSET_SETTINGS,
	VIDEO_MENU_WINDOW_SCALING_DEC,
	VIDEO_MENU_WINDOW_SCALING_INC,
	VIDEO_MENU_FULLSCREEN_SCALING_TOP,
	VIDEO_MENU_FULLSCREEN_SCALING_BOTTOM,
	VIDEO_MENU_BFI_SETTINGS,
	VIDEO_MENU_MENU_SCALING_DEC,
	VIDEO_MENU_MENU_SCALING_INC,
	VIDEO_MENU_RESOLUTION_SETTINGS,
	VIDEO_MENU_ROTATION_SETTINGS,
	VIDEO_MENU_TOP_ROTATION_DEC,
	VIDEO_MENU_TOP_ROTATION_INC,
	VIDEO_MENU_BOTTOM_ROTATION_DEC,
	VIDEO_MENU_BOTTOM_ROTATION_INC,
	VIDEO_MENU_FAST_POLL,
};

class VideoMenu : public OptionSelectionMenu {
public:
	VideoMenu(bool font_load_success, sf::Font &text_font);
	~VideoMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, bool fast_poll);
	void insert_data(ScreenType s_type, bool is_fullscreen);
	VideoMenuOutAction selected_index = VideoMenuOutAction::VIDEO_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_inc_dec(int index);
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	int num_enabled_options;
	std::string setTextOptionDualPercentage(int index, int value_1, int value_2);
};
#endif

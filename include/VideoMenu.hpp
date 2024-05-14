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
	VIDEO_MENU_SMALL_SCREEN_OFFSET,
	VIDEO_MENU_SMALL_SCREEN_DISTANCE,
	VIDEO_MENU_SCREENS_X_POS,
	VIDEO_MENU_SCREENS_Y_POS,
	VIDEO_MENU_WINDOW_SCALING,
	VIDEO_MENU_FULLSCREEN_SCALING,
	VIDEO_MENU_BFI_SETTINGS,
	VIDEO_MENU_MENU_SCALING,
	VIDEO_MENU_RESOLUTION_SETTINGS,
	VIDEO_MENU_TOP_ROTATION,
	VIDEO_MENU_BOTTOM_ROTATION,
};

class VideoMenu : public OptionSelectionMenu {
public:
	VideoMenu(bool font_load_success, sf::Font &text_font);
	~VideoMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info);
	void insert_data(ScreenType s_type, bool is_fullscreen);
	VideoMenuOutAction selected_index = VideoMenuOutAction::VIDEO_MENU_NO_ACTION;
protected:
	void reset_output_option();
	void set_output_option(int index);
	int get_num_options();
	std::string get_string_option(int index);
	void class_setup();
private:
	int *options_indexes;
	int num_enabled_options;
};
#endif

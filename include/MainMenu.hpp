#ifndef __MAINMENU_HPP
#define __MAINMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum MainMenuOutAction{
	MAIN_MENU_NO_ACTION,
	MAIN_MENU_OPEN,
	MAIN_MENU_CLOSE_MENU,
	MAIN_MENU_QUIT_APPLICATION,
	MAIN_MENU_FULLSCREEN,
	MAIN_MENU_SPLIT,
	MAIN_MENU_VSYNC,
	MAIN_MENU_ASYNC,
	MAIN_MENU_BLUR,
	MAIN_MENU_PADDING,
	MAIN_MENU_CROPPING,
	MAIN_MENU_TOP_PAR,
	MAIN_MENU_BOT_PAR,
	MAIN_MENU_ONE_PAR,
	MAIN_MENU_BOTTOM_SCREEN_POS,
	MAIN_MENU_SMALL_SCREEN_OFFSET,
	MAIN_MENU_SMALL_SCREEN_DISTANCE,
	MAIN_MENU_SCREENS_X_POS,
	MAIN_MENU_SCREENS_Y_POS,
	MAIN_MENU_WINDOW_SCALING,
	MAIN_MENU_FULLSCREEN_SCALING,
	MAIN_MENU_BFI_SETTINGS,
	MAIN_MENU_MENU_SCALING,
	MAIN_MENU_RESOLUTION_SETTINGS,
	MAIN_MENU_AUDIO_SETTINGS,
	MAIN_MENU_SAVE_PROFILES,
	MAIN_MENU_LOAD_PROFILES,
	MAIN_MENU_STATUS,
	MAIN_MENU_LICENSES,
	MAIN_MENU_EXTRA_SETTINGS,
	MAIN_MENU_TOP_ROTATION,
	MAIN_MENU_BOTTOM_ROTATION,
};

class MainMenu : public OptionSelectionMenu {
public:
	MainMenu(bool font_load_success, sf::Font &text_font);
	~MainMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, bool connected);
	void insert_data(ScreenType s_type, bool is_fullscreen);
	MainMenuOutAction selected_index = MainMenuOutAction::MAIN_MENU_NO_ACTION;
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

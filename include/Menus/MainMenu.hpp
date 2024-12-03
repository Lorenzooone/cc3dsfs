#ifndef __MAINMENU_HPP
#define __MAINMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum MainMenuOutAction{
	MAIN_MENU_NO_ACTION,
	MAIN_MENU_OPEN,
	MAIN_MENU_CLOSE_MENU,
	MAIN_MENU_QUIT_APPLICATION,
	MAIN_MENU_FULLSCREEN,
	MAIN_MENU_SPLIT,
	MAIN_MENU_VIDEO_SETTINGS,
	MAIN_MENU_AUDIO_SETTINGS,
	MAIN_MENU_SAVE_PROFILES,
	MAIN_MENU_LOAD_PROFILES,
	MAIN_MENU_STATUS,
	MAIN_MENU_LICENSES,
	MAIN_MENU_EXTRA_SETTINGS,
	MAIN_MENU_SHUTDOWN,
	//MAIN_MENU_SHORTCUT_SETTINGS,
	MAIN_MENU_ISN_SETTINGS,
	MAIN_MENU_INPUT_SETTINGS,
};

class MainMenu : public OptionSelectionMenu {
public:
	MainMenu(bool font_load_success, sf::Font &text_font);
	~MainMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, bool connected);
	void insert_data(ScreenType s_type, bool is_fullscreen, bool mono_app_mode, CaptureConnectionType cc_type, bool connected);
	MainMenuOutAction selected_index = MainMenuOutAction::MAIN_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	int num_enabled_options;
};
#endif

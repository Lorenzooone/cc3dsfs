#ifndef __MAINMENU_HPP
#define __MAINMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

#define NUM_TOTAL_MAIN_MENU_OPTIONS 4

#define MAIN_NUM_ELEMENTS_PER_SCREEN 5
#define MAIN_NUM_PAGE_ELEMENTS 3
#define MAIN_NUM_ELEMENTS_DISPLAYED_PER_SCREEN (MAIN_NUM_ELEMENTS_PER_SCREEN + MAIN_NUM_PAGE_ELEMENTS)
#define MAIN_NUM_VERTICAL_SLICES (MAIN_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - (MAIN_NUM_PAGE_ELEMENTS - 1))

enum MainMenuOutAction{
	MAIN_MENU_NO_ACTION,
	MAIN_MENU_OPEN,
	MAIN_MENU_CLOSE_MENU,
	MAIN_MENU_QUIT_APPLICATION,
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
	int options_indexes[NUM_TOTAL_MAIN_MENU_OPTIONS];
	int num_enabled_options;
};
#endif

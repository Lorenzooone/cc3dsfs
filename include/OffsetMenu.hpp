#ifndef __OFFSETMENU_HPP
#define __OFFSETMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum OffsetMenuOutAction{
	OFFSET_MENU_NO_ACTION,
	OFFSET_MENU_BACK,
	OFFSET_MENU_SMALL_OFFSET_DEC,
	OFFSET_MENU_SMALL_OFFSET_INC,
	OFFSET_MENU_SMALL_SCREEN_DISTANCE_DEC,
	OFFSET_MENU_SMALL_SCREEN_DISTANCE_INC,
	OFFSET_MENU_SCREENS_X_POS_DEC,
	OFFSET_MENU_SCREENS_X_POS_INC,
	OFFSET_MENU_SCREENS_Y_POS_DEC,
	OFFSET_MENU_SCREENS_Y_POS_INC,
};

class OffsetMenu : public OptionSelectionMenu {
public:
	OffsetMenu(bool font_load_success, sf::Font &text_font);
	~OffsetMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info);
	void insert_data();
	OffsetMenuOutAction selected_index = OffsetMenuOutAction::OFFSET_MENU_NO_ACTION;
protected:
	bool is_option_inc_dec(int index);
	void reset_output_option();
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	int num_enabled_options;
};
#endif

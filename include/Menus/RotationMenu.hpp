#ifndef __ROTATIONMENU_HPP
#define __ROTATIONMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum RotationMenuOutAction{
	ROTATION_MENU_NO_ACTION,
	ROTATION_MENU_BACK,
	ROTATION_MENU_TOP_ROTATION_DEC,
	ROTATION_MENU_TOP_ROTATION_INC,
	ROTATION_MENU_BOTTOM_ROTATION_DEC,
	ROTATION_MENU_BOTTOM_ROTATION_INC,
	ROTATION_MENU_BOTH_ROTATION_DEC,
	ROTATION_MENU_BOTH_ROTATION_INC,
};

class RotationMenu : public OptionSelectionMenu {
public:
	RotationMenu(bool font_load_success, sf::Font &text_font);
	~RotationMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info);
	void insert_data();
	RotationMenuOutAction selected_index = RotationMenuOutAction::ROTATION_MENU_NO_ACTION;
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

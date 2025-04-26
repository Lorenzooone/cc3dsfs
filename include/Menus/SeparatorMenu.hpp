#ifndef __SEPARATORMENU_HPP
#define __SEPARATORMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum SeparatorMenuOutAction{
	SEPARATOR_MENU_NO_ACTION,
	SEPARATOR_MENU_BACK,
	SEPARATOR_MENU_SIZE_DEC_1,
	SEPARATOR_MENU_SIZE_INC_1,
	SEPARATOR_MENU_SIZE_DEC_10,
	SEPARATOR_MENU_SIZE_INC_10,
	SEPARATOR_MENU_WINDOW_MUL_DEC,
	SEPARATOR_MENU_WINDOW_MUL_INC,
	SEPARATOR_MENU_FULLSCREEN_MUL_DEC,
	SEPARATOR_MENU_FULLSCREEN_MUL_INC,
};

class SeparatorMenu : public OptionSelectionMenu {
public:
	SeparatorMenu(TextRectanglePool* text_pool);
	~SeparatorMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info);
	void insert_data(bool is_fullscreen);
	SeparatorMenuOutAction selected_index = SeparatorMenuOutAction::SEPARATOR_MENU_NO_ACTION;
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
	std::string get_name_multiplier(int index, float value, bool is_fullscreen);
};
#endif

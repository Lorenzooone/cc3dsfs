#ifndef __OUTPUTFRAMERATEMENU_HPP
#define __OUTPUTFRAMERATEMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum OutputFramerateMenuOutAction{
	OUTPUT_FRAMERATE_MENU_NO_ACTION,
	OUTPUT_FRAMERATE_MENU_BACK,
	OUTPUT_FRAMERATE_MENU_MODE_INC,
	OUTPUT_FRAMERATE_MENU_MODE_DEC,
	OUTPUT_FRAMERATE_MENU_RATE_001_INC,
	OUTPUT_FRAMERATE_MENU_RATE_001_DEC,
	OUTPUT_FRAMERATE_MENU_RATE_01_INC,
	OUTPUT_FRAMERATE_MENU_RATE_01_DEC,
	OUTPUT_FRAMERATE_MENU_RATE_1_INC,
	OUTPUT_FRAMERATE_MENU_RATE_1_DEC,
	OUTPUT_FRAMERATE_MENU_RATE_10_INC,
	OUTPUT_FRAMERATE_MENU_RATE_10_DEC,
};

class OutputFramerateMenu : public OptionSelectionMenu {
public:
	OutputFramerateMenu(TextRectanglePool* text_pool);
	~OutputFramerateMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, DisplayData* display_data);
	void insert_data();
	OutputFramerateMenuOutAction selected_index = OutputFramerateMenuOutAction::OUTPUT_FRAMERATE_MENU_NO_ACTION;
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

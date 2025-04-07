#ifndef __SCALINGRATIOMENU_HPP
#define __SCALINGRATIOMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum ScalingRatioMenuOutAction{
	SCALING_RATIO_MENU_NO_ACTION,
	SCALING_RATIO_MENU_BACK,
	SCALING_RATIO_MENU_FULLSCREEN_SCALING_TOP,
	SCALING_RATIO_MENU_FULLSCREEN_SCALING_BOTTOM,
	SCALING_RATIO_MENU_NON_INT_SCALING_TOP,
	SCALING_RATIO_MENU_NON_INT_SCALING_BOTTOM,
	SCALING_RATIO_MENU_ALGO_INC,
	SCALING_RATIO_MENU_ALGO_DEC,
	SCALING_RATIO_MENU_FORCE_SAME_SCALING,
};

class ScalingRatioMenu : public OptionSelectionMenu {
public:
	ScalingRatioMenu(bool font_load_success, sf::Font &text_font);
	~ScalingRatioMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info);
	void insert_data();
	ScalingRatioMenuOutAction selected_index = ScalingRatioMenuOutAction::SCALING_RATIO_MENU_NO_ACTION;
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
	std::string setTextOptionDualPercentage(int index, float value_1, float value_2);
};
#endif

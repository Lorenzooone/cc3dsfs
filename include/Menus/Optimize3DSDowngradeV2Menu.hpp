#ifndef __OPTIMIZE3DSDOWNGRADEV2MENU_HPP
#define __OPTIMIZE3DSDOWNGRADEV2MENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum Optimize3DSDowngradeV2MenuOutAction{
	OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION,
	OPTIMIZE3DS_DOWNGRADEV2_MENU_BACK,
	OPTIMIZE3DS_DOWNGRADEV2_MENU_CONFIRM,
};

class Optimize3DSDowngradeV2Menu : public OptionSelectionMenu {
public:
	Optimize3DSDowngradeV2Menu(TextRectanglePool* text_pool);
	~Optimize3DSDowngradeV2Menu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data();
	Optimize3DSDowngradeV2MenuOutAction selected_index = Optimize3DSDowngradeV2MenuOutAction::OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_selectable(int index, int action);
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

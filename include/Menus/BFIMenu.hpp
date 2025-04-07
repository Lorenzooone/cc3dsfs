#ifndef __BFIMENU_HPP
#define __BFIMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum BFIMenuOutAction{
	BFI_MENU_NO_ACTION,
	BFI_MENU_BACK,
	BFI_MENU_TOGGLE,
	BFI_MENU_DIVIDER_DEC,
	BFI_MENU_DIVIDER_INC,
	BFI_MENU_AMOUNT_DEC,
	BFI_MENU_AMOUNT_INC,
};

class BFIMenu : public OptionSelectionMenu {
public:
	BFIMenu(bool font_load_success, sf::Font &text_font);
	~BFIMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info);
	void insert_data();
	BFIMenuOutAction selected_index = BFIMenuOutAction::BFI_MENU_NO_ACTION;
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

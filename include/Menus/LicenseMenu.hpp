#ifndef __LICENSEMENU_HPP
#define __LICENSEMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum LicenseMenuOutAction{
	LICENSE_MENU_NO_ACTION,
	LICENSE_MENU_BACK,
};

class LicenseMenu : public OptionSelectionMenu {
public:
	LicenseMenu(bool font_load_success, sf::Font &text_font);
	~LicenseMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data();
	LicenseMenuOutAction selected_index = LicenseMenuOutAction::LICENSE_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_selectable(int index, int action);
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

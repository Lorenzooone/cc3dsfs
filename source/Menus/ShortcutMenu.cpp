#include "ShortcutMenu.hpp"

ShortcutMenu::ShortcutMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->initialize(font_load_success, text_font);
}

ShortcutMenu::~ShortcutMenu() {
}

void ShortcutMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Shortcut Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void ShortcutMenu::insert_data(std::vector<std::string> &shortcut_names) {
	this->shortcut_names = &shortcut_names;
	this->prepare_options();
}

void ShortcutMenu::reset_output_option() {
	this->selected_index = SHORTCUT_MENU_NO_ACTION;
}

void ShortcutMenu::set_output_option(int index, int action) {
	if(index >= this->get_num_options())
		this->selected_index = SHORTCUT_MENU_BACK;
	else if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = SHORTCUT_MENU_BACK;
	else
		this->selected_index = index;
}

int ShortcutMenu::get_num_options() {
	return this->shortcut_names->size();
}

std::string ShortcutMenu::get_string_option(int index, int action) {
	return (*this->shortcut_names)[index];
}

void ShortcutMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

#include "ConnectionMenu.hpp"

ConnectionMenu::ConnectionMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->initialize(font_load_success, text_font);
}

ConnectionMenu::~ConnectionMenu() {
}

void ConnectionMenu::class_setup() {
	this->num_options_per_screen = 4;
	this->min_elements_text_scaling_factor = num_options_per_screen;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 10;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Device Selection";
	this->show_back_x = false;
	this->show_x = false;
	this->show_title = true;
}

void ConnectionMenu::insert_data(std::vector<CaptureDevice> *devices_list) {
	this->devices_list = devices_list;
	this->prepare_options();
}

void ConnectionMenu::reset_output_option() {
	this->selected_index = -1;
}

void ConnectionMenu::set_output_option(int index, int action) {
	this->selected_index = index;
}

size_t ConnectionMenu::get_num_options() {
	return this->devices_list->size();
}

std::string ConnectionMenu::get_string_option(int index, int action) {
	return (*this->devices_list)[index].name + " - " + (*this->devices_list)[index].serial_number;
}

void ConnectionMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

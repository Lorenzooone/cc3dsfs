#include "ConnectionMenu.hpp"

ConnectionMenu::ConnectionMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->initialize(font_load_success, text_font);
}

ConnectionMenu::~ConnectionMenu() {
}

void ConnectionMenu::class_setup() {
	this->num_elements_per_screen = 4;
	this->num_page_elements = 3;
	this->num_elements_displayed_per_screen = num_elements_per_screen + num_page_elements;
	this->num_vertical_slices = (num_elements_displayed_per_screen - (num_page_elements - 1));
	this->num_elements_start_id = 0;
	this->num_page_elements_start_id = num_elements_per_screen + num_elements_start_id;
	this->prev_page_id = num_page_elements_start_id;
	this->info_page_id = num_page_elements_start_id + 1;
	this->next_page_id = num_page_elements_start_id + 2;
	this->min_elements_text_scaling_factor = num_elements_per_screen;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 10;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
}

void ConnectionMenu::insert_data(DevicesList *devices_list) {
	this->devices_list = devices_list;
	this->prepare_options();
}

void ConnectionMenu::reset_output_option() {
	this->selected_index = -1;
}

void ConnectionMenu::set_output_option(int index) {
	this->selected_index = index;
}

int ConnectionMenu::get_num_options() {
	return this->devices_list->numValidDevices;
}

std::string ConnectionMenu::get_string_option(int index) {
	return std::string(&this->devices_list->serialNumbers[SERIAL_NUMBER_SIZE * index]);
}

void ConnectionMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

#include "FileConfigMenu.hpp"

FileConfigMenu::FileConfigMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->initialize(font_load_success, text_font);
}

FileConfigMenu::~FileConfigMenu() {
}

void FileConfigMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = this->base_name;
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void FileConfigMenu::setup_title(std::string added_name) {
	this->title = added_name + (added_name != "" ? " " : "") + this->base_name;
}

void FileConfigMenu::insert_data(std::vector<FileData>* possible_files) {
	this->possible_files = possible_files;
	this->prepare_options();
}

void FileConfigMenu::reset_output_option() {
	this->selected_index = FILECONFIG_MENU_NO_ACTION;
}

void FileConfigMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = FILECONFIG_MENU_BACK;
	else
		this->selected_index = (*this->possible_files)[index].index;
}

int FileConfigMenu::get_num_options() {
	return (*this->possible_files).size();
}

std::string FileConfigMenu::get_string_option(int index, int action) {
	FileData file_data = (*this->possible_files)[index];
	if(file_data.index == CREATE_NEW_FILE_INDEX)
		return "Create new...";
	if(file_data.index == STARTUP_FILE_INDEX)
		return "Initial Profile";
	return "Profile " + file_data.name;
}

void FileConfigMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

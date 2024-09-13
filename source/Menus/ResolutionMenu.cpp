#include "ResolutionMenu.hpp"

ResolutionMenu::ResolutionMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->initialize(font_load_success, text_font);
}

ResolutionMenu::~ResolutionMenu() {
}

void ResolutionMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Resolution Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void ResolutionMenu::insert_data(std::vector<sf::VideoMode>* possible_resolutions, const sf::VideoMode &desktop_resolution) {
	this->possible_resolutions = possible_resolutions;
	this->desktop_resolution = desktop_resolution;
	this->prepare_options();
}

void ResolutionMenu::reset_output_option() {
	this->selected_index = RESOLUTION_MENU_NO_ACTION;
}

void ResolutionMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = RESOLUTION_MENU_BACK;
	else
		this->selected_index = index;
}

int ResolutionMenu::get_num_options() {
	return (*this->possible_resolutions).size();
}

sf::VideoMode ResolutionMenu::get_resolution(int index) {
	return (*this->possible_resolutions)[index];
}

std::string ResolutionMenu::get_string_option(int index, int action) {
	sf::VideoMode mode = this->get_resolution(index);
	if((mode.size.x == 0) && (mode.size.y == 0)) {
		return "System Preference (" + std::to_string(this->desktop_resolution.size.x) + " x " + std::to_string(this->desktop_resolution.size.y) + ")";
	}
	return std::to_string(mode.size.x) + " x " + std::to_string(mode.size.y);
}

void ResolutionMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, int fullscreen_mode_width, int fullscreen_mode_height) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int mode_index = start + i;
		sf::VideoMode mode = this->get_resolution(mode_index);
		if((mode.size.x == fullscreen_mode_width) && (mode.size.y == fullscreen_mode_height))
			this->labels[index]->setText("<" + this->get_string_option(mode_index, DEFAULT_ACTION) + ">");
		else
			this->labels[index]->setText(this->get_string_option(mode_index, DEFAULT_ACTION));
	}
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

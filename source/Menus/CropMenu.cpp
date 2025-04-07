#include "CropMenu.hpp"

CropMenu::CropMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->initialize(font_load_success, text_font);
}

CropMenu::~CropMenu() {
}

void CropMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Crop Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void CropMenu::insert_data(std::vector<const CropData*>* possible_crops) {
	this->possible_crops = possible_crops;
	this->prepare_options();
}

void CropMenu::reset_output_option() {
	this->selected_index = CROP_MENU_NO_ACTION;
}

void CropMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = CROP_MENU_BACK;
	else
		this->selected_index = index;
}

size_t CropMenu::get_num_options() {
	return (*this->possible_crops).size();
}

std::string CropMenu::get_string_option(int index, int action) {
	return (*this->possible_crops)[index]->name;
}

void CropMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, int current_crop) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int crop_index = start + i;
		if(crop_index == current_crop)
			this->labels[index]->setText("<" + this->get_string_option(crop_index, DEFAULT_ACTION) + ">");
		else
			this->labels[index]->setText(this->get_string_option(crop_index, DEFAULT_ACTION));
	}
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

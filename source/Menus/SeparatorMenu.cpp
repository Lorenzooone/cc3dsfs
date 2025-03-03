#include "SeparatorMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct SeparatorMenuOptionInfo {
	const std::string base_name;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const SeparatorMenuOutAction inc_out_action;
	const SeparatorMenuOutAction out_action;
	const bool show_windowed;
	const bool show_fullscreen;
};

static const SeparatorMenuOptionInfo separator_size_change_1_option = {
.base_name = "Separator Size:",
.is_inc = true, .dec_str = "-1", .inc_str = "+1", .inc_out_action = SEPARATOR_MENU_SIZE_INC_1,
.out_action = SEPARATOR_MENU_SIZE_DEC_1,
.show_windowed = true, .show_fullscreen = true};

static const SeparatorMenuOptionInfo separator_size_change_10_option = {
.base_name = "",
.is_inc = true, .dec_str = "-10", .inc_str = "+10", .inc_out_action = SEPARATOR_MENU_SIZE_INC_10,
.out_action = SEPARATOR_MENU_SIZE_DEC_10,
.show_windowed = true, .show_fullscreen = true};

static const SeparatorMenuOptionInfo separator_window_mul_change_option = {
.base_name = "Multiplier",
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = SEPARATOR_MENU_WINDOW_MUL_INC,
.out_action = SEPARATOR_MENU_WINDOW_MUL_DEC,
.show_windowed = true, .show_fullscreen = false};

static const SeparatorMenuOptionInfo separator_fullscreen_mul_change_option = {
.base_name = "Multiplier",
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = SEPARATOR_MENU_FULLSCREEN_MUL_INC,
.out_action = SEPARATOR_MENU_FULLSCREEN_MUL_DEC,
.show_windowed = false, .show_fullscreen = true};

static const SeparatorMenuOptionInfo* pollable_options[] = {
&separator_size_change_1_option,
&separator_size_change_10_option,
&separator_window_mul_change_option,
&separator_fullscreen_mul_change_option,
};

SeparatorMenu::SeparatorMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

SeparatorMenu::~SeparatorMenu() {
	delete []this->options_indexes;
}

void SeparatorMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Separator Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void SeparatorMenu::insert_data(bool is_fullscreen) {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		if(is_fullscreen && (!pollable_options[i]->show_fullscreen))
			continue;
		if((!is_fullscreen) && (!pollable_options[i]->show_windowed))
			continue;
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void SeparatorMenu::reset_output_option() {
	this->selected_index = SeparatorMenuOutAction::SEPARATOR_MENU_NO_ACTION;
}

void SeparatorMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = SEPARATOR_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int SeparatorMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string SeparatorMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool SeparatorMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

std::string SeparatorMenu::get_name_multiplier(int index, float value, bool is_fullscreen) {
	if((!is_fullscreen) && (value == SEP_FOLLOW_SCALING_MULTIPLIER))
		return this->get_string_option(index, DEFAULT_ACTION) + ": " + "Scaling";
	return this->setTextOptionFloat(index, value);
}

void SeparatorMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int real_index = start + i;
		int option_index = this->options_indexes[real_index];
		switch(pollable_options[option_index]->out_action) {
			case SEPARATOR_MENU_SIZE_DEC_10:
				this->labels[index]->setText(std::to_string(info->separator_pixel_size));
				break;
			case SEPARATOR_MENU_WINDOW_MUL_DEC:
				this->labels[index]->setText(get_name_multiplier(real_index, info->separator_windowed_multiplier, info->is_fullscreen));
				break;
			case SEPARATOR_MENU_FULLSCREEN_MUL_DEC:
				this->labels[index]->setText(get_name_multiplier(real_index, info->separator_fullscreen_multiplier, info->is_fullscreen));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

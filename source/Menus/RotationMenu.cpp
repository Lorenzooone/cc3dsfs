#include "RotationMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct RotationMenuOptionInfo {
	const std::string base_name;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const RotationMenuOutAction inc_out_action;
	const RotationMenuOutAction out_action;
};

static const RotationMenuOptionInfo top_rotation_option = {
.base_name = "Top Screen",
.is_inc = true, .dec_str = "Left", .inc_str = "Right", .inc_out_action = ROTATION_MENU_TOP_ROTATION_INC,
.out_action = ROTATION_MENU_TOP_ROTATION_DEC};

static const RotationMenuOptionInfo bottom_rotation_option = {
.base_name = "Bottom Screen",
.is_inc = true, .dec_str = "Left", .inc_str = "Right", .inc_out_action = ROTATION_MENU_BOTTOM_ROTATION_INC,
.out_action = ROTATION_MENU_BOTTOM_ROTATION_DEC};

static const RotationMenuOptionInfo both_rotation_option = {
.base_name = "Both Screens",
.is_inc = true, .dec_str = "Left", .inc_str = "Right", .inc_out_action = ROTATION_MENU_BOTH_ROTATION_INC,
.out_action = ROTATION_MENU_BOTH_ROTATION_DEC};

static const RotationMenuOptionInfo* pollable_options[] = {
&top_rotation_option,
&bottom_rotation_option,
&both_rotation_option,
};

RotationMenu::RotationMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

RotationMenu::~RotationMenu() {
	delete []this->options_indexes;
}

void RotationMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Rotation Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void RotationMenu::insert_data() {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void RotationMenu::reset_output_option() {
	this->selected_index = RotationMenuOutAction::ROTATION_MENU_NO_ACTION;
}

void RotationMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = ROTATION_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int RotationMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string RotationMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool RotationMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

static std::string setTextOptionInt(int option_index, int value) {
	return pollable_options[option_index]->base_name + ": " + std::to_string(value);
}

void RotationMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int option_index = this->options_indexes[start + i];
		switch(pollable_options[option_index]->out_action) {
			case ROTATION_MENU_TOP_ROTATION_DEC:
				this->labels[index]->setText(setTextOptionInt(option_index, info->top_rotation));
				break;
			case ROTATION_MENU_BOTTOM_ROTATION_DEC:
				this->labels[index]->setText(setTextOptionInt(option_index, info->bot_rotation));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

#include "OffsetMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct OffsetMenuOptionInfo {
	const std::string base_name;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const OffsetMenuOutAction inc_out_action;
	const OffsetMenuOutAction out_action;
};

static const OffsetMenuOptionInfo small_screen_offset_option = {
.base_name = "Screens Offsets",
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = OFFSET_MENU_SMALL_OFFSET_INC,
.out_action = OFFSET_MENU_SMALL_OFFSET_DEC};

static const OffsetMenuOptionInfo subscreen_distance_option = {
.base_name = "Sub-Screen Distance",
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = OFFSET_MENU_SMALL_SCREEN_DISTANCE_INC,
.out_action = OFFSET_MENU_SMALL_SCREEN_DISTANCE_DEC};

static const OffsetMenuOptionInfo canvas_x_pos_option = {
.base_name = "Canvas X Position",
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = OFFSET_MENU_SCREENS_X_POS_INC,
.out_action = OFFSET_MENU_SCREENS_X_POS_DEC};

static const OffsetMenuOptionInfo canvas_y_pos_option = {
.base_name = "Canvas Y Position",
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = OFFSET_MENU_SCREENS_Y_POS_INC,
.out_action = OFFSET_MENU_SCREENS_Y_POS_DEC};

static const OffsetMenuOptionInfo* pollable_options[] = {
&small_screen_offset_option,
&subscreen_distance_option,
&canvas_x_pos_option,
&canvas_y_pos_option,
};

OffsetMenu::OffsetMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

OffsetMenu::~OffsetMenu() {
	delete []this->options_indexes;
}

void OffsetMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Offset Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void OffsetMenu::insert_data() {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void OffsetMenu::reset_output_option() {
	this->selected_index = OffsetMenuOutAction::OFFSET_MENU_NO_ACTION;
}

void OffsetMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = OFFSET_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int OffsetMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string OffsetMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool OffsetMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

void OffsetMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info) {
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
			case OFFSET_MENU_SMALL_OFFSET_DEC:
				this->labels[index]->setText(this->setTextOptionFloat(real_index, info->subscreen_offset));
				break;
			case OFFSET_MENU_SMALL_SCREEN_DISTANCE_DEC:
				this->labels[index]->setText(this->setTextOptionFloat(real_index, info->subscreen_attached_offset));
				break;
			case OFFSET_MENU_SCREENS_X_POS_DEC:
				this->labels[index]->setText(this->setTextOptionFloat(real_index, info->total_offset_x));
				break;
			case OFFSET_MENU_SCREENS_Y_POS_DEC:
				this->labels[index]->setText(this->setTextOptionFloat(real_index, info->total_offset_y));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

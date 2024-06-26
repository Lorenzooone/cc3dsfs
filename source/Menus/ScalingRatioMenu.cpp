#include "ScalingRatioMenu.hpp"
#include "frontend.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct ScalingRatioMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const ScalingRatioMenuOutAction inc_out_action;
	const ScalingRatioMenuOutAction out_action;
};

static const ScalingRatioMenuOptionInfo ratio_option = {
.base_name = "Screens Ratio", .false_name = "",
.is_inc = true, .dec_str = "+ Top", .inc_str = "+ Bot.", .inc_out_action = SCALING_RATIO_MENU_FULLSCREEN_SCALING_BOTTOM,
.out_action = SCALING_RATIO_MENU_FULLSCREEN_SCALING_TOP};

static const ScalingRatioMenuOptionInfo top_fill_option = {
.base_name = "Disable Top Screen Fill", .false_name = "Enable Top Screen Fill",
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = SCALING_RATIO_MENU_NO_ACTION,
.out_action = SCALING_RATIO_MENU_NON_INT_SCALING_TOP};

static const ScalingRatioMenuOptionInfo bot_fill_option = {
.base_name = "Disable Bot. Screen Fill", .false_name = "Enable Bot. Screen Fill",
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = SCALING_RATIO_MENU_NO_ACTION,
.out_action = SCALING_RATIO_MENU_NON_INT_SCALING_BOTTOM};

static const ScalingRatioMenuOptionInfo ratio_algo_option = {
.base_name = "Ratio Mode", .false_name = "",
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = SCALING_RATIO_MENU_ALGO_INC,
.out_action = SCALING_RATIO_MENU_ALGO_DEC};

static const ScalingRatioMenuOptionInfo* pollable_options[] = {
&ratio_option,
&top_fill_option,
&bot_fill_option,
&ratio_algo_option,
};

ScalingRatioMenu::ScalingRatioMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

ScalingRatioMenu::~ScalingRatioMenu() {
	delete []this->options_indexes;
}

void ScalingRatioMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Scaling & Ratio Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void ScalingRatioMenu::insert_data() {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void ScalingRatioMenu::reset_output_option() {
	this->selected_index = ScalingRatioMenuOutAction::SCALING_RATIO_MENU_NO_ACTION;
}

void ScalingRatioMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = SCALING_RATIO_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int ScalingRatioMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string ScalingRatioMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool ScalingRatioMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

std::string ScalingRatioMenu::setTextOptionDualPercentage(int index, float value_1, float value_2) {
	float sum = value_1 + value_2;
	if(sum > 0) {
		value_1 = (value_1 * 100) / sum;
		value_2 = (value_2 * 100) / sum;
		value_1 += 100 - (value_1 + value_2);
	}
	else {
		value_1 = 0;
		value_2 = 0;
	}
	return this->get_string_option(index, DEFAULT_ACTION) + ": " + get_float_str_decimals(value_1, 1) + " - " + get_float_str_decimals(value_2, 1);
}

void ScalingRatioMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info) {
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
			case SCALING_RATIO_MENU_FULLSCREEN_SCALING_TOP:
				this->labels[index]->setText(this->setTextOptionDualPercentage(real_index, info->non_integer_top_scaling, info->non_integer_bot_scaling));
				break;
			case SCALING_RATIO_MENU_NON_INT_SCALING_TOP:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->use_non_integer_scaling_top));
				break;
			case SCALING_RATIO_MENU_NON_INT_SCALING_BOTTOM:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->use_non_integer_scaling_bottom));
				break;
			case SCALING_RATIO_MENU_ALGO_DEC:
				this->labels[index]->setText(this->setTextOptionString(real_index, get_name_non_int_mode(info->non_integer_mode)));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

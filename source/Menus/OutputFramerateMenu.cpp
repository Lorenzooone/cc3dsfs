#include "OutputFramerateMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct OutputFramerateMenuOptionInfo {
	const std::string base_name;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const OutputFramerateMenuOutAction inc_out_action;
	const OutputFramerateMenuOutAction out_action;
};

static const OutputFramerateMenuOptionInfo output_rate_mode_change_option = {
.base_name = "Framerate",
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = OUTPUT_FRAMERATE_MENU_MODE_INC,
.out_action = OUTPUT_FRAMERATE_MENU_MODE_DEC};

static const OutputFramerateMenuOptionInfo output_rate_fixed_change_10_option = {
.base_name = "",
.is_inc = true, .dec_str = "-10", .inc_str = "+10", .inc_out_action = OUTPUT_FRAMERATE_MENU_RATE_10_INC,
.out_action = OUTPUT_FRAMERATE_MENU_RATE_10_DEC};

static const OutputFramerateMenuOptionInfo output_rate_fixed_change_1_option = {
.base_name = "Fixed Out Framerate:",
.is_inc = true, .dec_str = "-1", .inc_str = "+1", .inc_out_action = OUTPUT_FRAMERATE_MENU_RATE_1_INC,
.out_action = OUTPUT_FRAMERATE_MENU_RATE_1_DEC};

static const OutputFramerateMenuOptionInfo output_rate_fixed_change_01_option = {
.base_name = "",
.is_inc = true, .dec_str = "-0.1", .inc_str = "+0.1", .inc_out_action = OUTPUT_FRAMERATE_MENU_RATE_01_INC,
.out_action = OUTPUT_FRAMERATE_MENU_RATE_01_DEC};

static const OutputFramerateMenuOptionInfo output_rate_fixed_change_001_option = {
.base_name = "",
.is_inc = true, .dec_str = "-0.01", .inc_str = "+0.01", .inc_out_action = OUTPUT_FRAMERATE_MENU_RATE_001_INC,
.out_action = OUTPUT_FRAMERATE_MENU_RATE_001_DEC};

// Wondering if maybe a version with a textfield would be better...
static const OutputFramerateMenuOptionInfo* pollable_options[] = {
&output_rate_mode_change_option,
&output_rate_fixed_change_10_option,
&output_rate_fixed_change_1_option,
&output_rate_fixed_change_01_option,
&output_rate_fixed_change_001_option,
};

OutputFramerateMenu::OutputFramerateMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

OutputFramerateMenu::~OutputFramerateMenu() {
	delete []this->options_indexes;
}

void OutputFramerateMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Out Framerate Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void OutputFramerateMenu::insert_data() {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = (int)i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void OutputFramerateMenu::reset_output_option() {
	this->selected_index = OutputFramerateMenuOutAction::OUTPUT_FRAMERATE_MENU_NO_ACTION;
}

void OutputFramerateMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = OUTPUT_FRAMERATE_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

size_t OutputFramerateMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string OutputFramerateMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool OutputFramerateMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

static std::string get_output_framerate_mode(DisplayData* display_data) {
	return display_data->fixed_output_framerate ? "Fixed" : "Console";
}

void OutputFramerateMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, DisplayData *display_data) {
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
			case OUTPUT_FRAMERATE_MENU_MODE_DEC:
				this->labels[index]->setText(this->setTextOptionString(real_index, get_output_framerate_mode(display_data)));
				break;
			case OUTPUT_FRAMERATE_MENU_RATE_01_DEC:
				this->labels[index]->setText(get_float_str_decimals((float)display_data->target_fixed_output_framerate, 2));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

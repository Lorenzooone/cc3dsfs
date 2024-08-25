#include "ISNitroMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct ISNitroMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_selectable;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const ISNitroMenuOutAction inc_out_action;
	const ISNitroMenuOutAction out_action;
};

static const ISNitroMenuOptionInfo is_nitro_delay_option = {
.base_name = "Delay", .false_name = "", .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = ISN_MENU_NO_ACTION,
.out_action = ISN_MENU_DELAY};

static const ISNitroMenuOptionInfo is_nitro_type_option = {
.base_name = "Capture", .false_name = "", .is_selectable = true,
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = ISN_MENU_TYPE_INC,
.out_action = ISN_MENU_TYPE_DEC};

static const ISNitroMenuOptionInfo* pollable_options[] = {
&is_nitro_delay_option,
&is_nitro_type_option,
};

ISNitroMenu::ISNitroMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

ISNitroMenu::~ISNitroMenu() {
	delete []this->options_indexes;
}

void ISNitroMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "IS Nitro Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void ISNitroMenu::insert_data() {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void ISNitroMenu::reset_output_option() {
	this->selected_index = ISNitroMenuOutAction::ISN_MENU_NO_ACTION;
}

void ISNitroMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = ISN_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int ISNitroMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string ISNitroMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool ISNitroMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool ISNitroMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

static std::string get_capture_type_name(CaptureScreensType capture_type) {
	switch(capture_type) {
		case CAPTURE_SCREENS_TOP:
			return "Top Screen";
		case CAPTURE_SCREENS_BOTTOM:
			return "Bottom Screen";
		default:
			return "Both Screens";
	}
}

void ISNitroMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status) {
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
			case ISN_MENU_DELAY:
				this->labels[index]->setText(this->setTextOptionInt(real_index, capture_status->curr_delay));
				break;
			case ISN_MENU_TYPE_DEC:
				this->labels[index]->setText(this->setTextOptionString(real_index, get_capture_type_name(capture_status->capture_type)));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}
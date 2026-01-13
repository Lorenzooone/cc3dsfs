#include "PartnerCTRMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct PartnerCTRMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_selectable;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const PartnerCTRMenuOutAction inc_out_action;
	const PartnerCTRMenuOutAction out_action;
};

static const PartnerCTRMenuOptionInfo partner_ctr_reset_option = {
.base_name = "Reset Hardware", .false_name = "", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = PCTR_MENU_NO_ACTION,
.out_action = PCTR_MENU_RESET};

static const PartnerCTRMenuOptionInfo partner_ctr_battery_percentage_option = {
.base_name = "Battery Percentage", .false_name = "", .is_selectable = true,
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = PCTR_MENU_BATTERY_INC,
.out_action = PCTR_MENU_BATTERY_DEC};

static const PartnerCTRMenuOptionInfo partner_ctr_ac_adapter_connected_option = {
.base_name = "Disconnect AC Adapter", .false_name = "Connect AC Adapter", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = PCTR_MENU_NO_ACTION,
.out_action = PCTR_MENU_AC_ADAPTER_CONNECTED_TOGGLE};

static const PartnerCTRMenuOptionInfo partner_ctr_ac_adapter_charging_option = {
.base_name = "Disable AC charging", .false_name = "Enable AC charging", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = PCTR_MENU_NO_ACTION,
.out_action = PCTR_MENU_AC_ADAPTER_CHARGING_TOGGLE};

static const PartnerCTRMenuOptionInfo* pollable_options[] = {
&partner_ctr_reset_option,
&partner_ctr_battery_percentage_option,
&partner_ctr_ac_adapter_connected_option,
&partner_ctr_ac_adapter_charging_option,
};

PartnerCTRMenu::PartnerCTRMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

PartnerCTRMenu::~PartnerCTRMenu() {
	delete []this->options_indexes;
}

void PartnerCTRMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Partner CTR Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void PartnerCTRMenu::insert_data(CaptureDevice* device) {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		bool valid = true;
		if(valid) {
			this->options_indexes[this->num_enabled_options] = (int)i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void PartnerCTRMenu::reset_output_option() {
	this->selected_index = PartnerCTRMenuOutAction::PCTR_MENU_NO_ACTION;
}

void PartnerCTRMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = PCTR_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

size_t PartnerCTRMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string PartnerCTRMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool PartnerCTRMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool PartnerCTRMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

void PartnerCTRMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status) {
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
			case PCTR_MENU_BATTERY_DEC:
				this->labels[index]->setText(this->setTextOptionInt(real_index, capture_status->partner_ctr_battery_percentage));
				break;
			case PCTR_MENU_AC_ADAPTER_CONNECTED_TOGGLE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, capture_status->partner_ctr_ac_adapter_connected));
				break;
			case PCTR_MENU_AC_ADAPTER_CHARGING_TOGGLE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, capture_status->partner_ctr_ac_adapter_charging));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

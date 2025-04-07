#include "BFIMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct BFIMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_selectable;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const BFIMenuOutAction inc_out_action;
	const BFIMenuOutAction out_action;
};

static const BFIMenuOptionInfo bfi_warning_option = {
.base_name = "PHOTOSENSITIVITY", .false_name = "", .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = BFI_MENU_NO_ACTION,
.out_action = BFI_MENU_NO_ACTION};

static const BFIMenuOptionInfo bfi_warning2_option = {
.base_name = "WARNING!", .false_name = "", .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = BFI_MENU_NO_ACTION,
.out_action = BFI_MENU_NO_ACTION};

static const BFIMenuOptionInfo bfi_toggle_option = {
.base_name = "Turn BFI Off", .false_name = "Turn BFI On", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = BFI_MENU_NO_ACTION,
.out_action = BFI_MENU_TOGGLE};

static const BFIMenuOptionInfo bfi_divider_option = {
.base_name = "Frequency", .false_name = "", .is_selectable = true,
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = BFI_MENU_DIVIDER_INC,
.out_action = BFI_MENU_DIVIDER_DEC};

static const BFIMenuOptionInfo bfi_amount_option = {
.base_name = "Frames", .false_name = "", .is_selectable = true,
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = BFI_MENU_AMOUNT_INC,
.out_action = BFI_MENU_AMOUNT_DEC};

static const BFIMenuOptionInfo* pollable_options[] = {
&bfi_warning_option,
&bfi_warning2_option,
&bfi_toggle_option,
&bfi_divider_option,
&bfi_amount_option,
};

BFIMenu::BFIMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

BFIMenu::~BFIMenu() {
	delete []this->options_indexes;
}

void BFIMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "BFI Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void BFIMenu::insert_data() {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = (int)i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void BFIMenu::reset_output_option() {
	this->selected_index = BFIMenuOutAction::BFI_MENU_NO_ACTION;
}

void BFIMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = BFI_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

size_t BFIMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string BFIMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool BFIMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool BFIMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

void BFIMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info) {
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
			case BFI_MENU_TOGGLE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->bfi));
				break;
			case BFI_MENU_DIVIDER_DEC:
				this->labels[index]->setText(this->setTextOptionInt(real_index, info->bfi_divider * 60) + " hz");
				break;
			case BFI_MENU_AMOUNT_DEC:
				this->labels[index]->setText(this->setTextOptionInt(real_index, info->bfi_amount));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

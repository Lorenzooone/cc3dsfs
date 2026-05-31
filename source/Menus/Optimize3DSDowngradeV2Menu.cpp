#include "Optimize3DSDowngradeV2Menu.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct Optimize3DSDowngradeV2MenuOptionInfo {
	const std::string base_name;
	const bool is_selectable;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const Optimize3DSDowngradeV2MenuOutAction inc_out_action;
	const Optimize3DSDowngradeV2MenuOutAction out_action;
};

static const Optimize3DSDowngradeV2MenuOptionInfo optimize_n3ds_downgrade_desc1_menu = {
.base_name = "This operation removes the", .is_selectable = false,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION};

static const Optimize3DSDowngradeV2MenuOptionInfo optimize_n3ds_downgrade_desc2_menu = {
.base_name = "Capture Card EEPROM config.", .is_selectable = false,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION};

static const Optimize3DSDowngradeV2MenuOptionInfo optimize_n3ds_downgrade_desc3_menu = {
.base_name = "This will make it incompatible", .is_selectable = false,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION};

static const Optimize3DSDowngradeV2MenuOptionInfo optimize_n3ds_downgrade_desc4_menu = {
.base_name = "with older software, as well", .is_selectable = false,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION};

static const Optimize3DSDowngradeV2MenuOptionInfo optimize_n3ds_downgrade_desc5_menu = {
.base_name = "as other downsides...", .is_selectable = false,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION};

static const Optimize3DSDowngradeV2MenuOptionInfo optimize_n3ds_downgrade_confirm_menu = {
.base_name = "Continue anyway?", .is_selectable = true,
.is_inc = true, .dec_str = "No", .inc_str = "Yes", .inc_out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_CONFIRM,
.out_action = OPTIMIZE3DS_DOWNGRADEV2_MENU_BACK};

static const Optimize3DSDowngradeV2MenuOptionInfo* pollable_options[] = {
&optimize_n3ds_downgrade_desc1_menu,
&optimize_n3ds_downgrade_desc2_menu,
&optimize_n3ds_downgrade_desc3_menu,
&optimize_n3ds_downgrade_desc4_menu,
&optimize_n3ds_downgrade_desc5_menu,
&optimize_n3ds_downgrade_confirm_menu,
};

Optimize3DSDowngradeV2Menu::Optimize3DSDowngradeV2Menu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

Optimize3DSDowngradeV2Menu::~Optimize3DSDowngradeV2Menu() {
	delete []this->options_indexes;
}

void Optimize3DSDowngradeV2Menu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Downgrade V2 Menu";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void Optimize3DSDowngradeV2Menu::insert_data() {
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

void Optimize3DSDowngradeV2Menu::reset_output_option() {
	this->selected_index = Optimize3DSDowngradeV2MenuOutAction::OPTIMIZE3DS_DOWNGRADEV2_MENU_NO_ACTION;
}

void Optimize3DSDowngradeV2Menu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = OPTIMIZE3DS_DOWNGRADEV2_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

size_t Optimize3DSDowngradeV2Menu::get_num_options() {
	return this->num_enabled_options;
}

std::string Optimize3DSDowngradeV2Menu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool Optimize3DSDowngradeV2Menu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool Optimize3DSDowngradeV2Menu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

void Optimize3DSDowngradeV2Menu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
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
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

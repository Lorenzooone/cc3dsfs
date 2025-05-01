#include "Optimize3DSMenu.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct Optimize3DSMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_selectable;
	const bool optimize_o3ds_valid;
	const bool optimize_n3ds_valid;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const Optimize3DSMenuOutAction inc_out_action;
	const Optimize3DSMenuOutAction out_action;
};

static const Optimize3DSMenuOptionInfo optimize_o3ds_hw_option = {
.base_name = "Hardware: Old 3DS", .false_name = "", .is_selectable = false,
.optimize_o3ds_valid = true, .optimize_n3ds_valid = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = OPTIMIZE3DS_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_MENU_NO_ACTION};

static const Optimize3DSMenuOptionInfo optimize_n3ds_hw_option = {
.base_name = "Hardware: New 3DS", .false_name = "", .is_selectable = false,
.optimize_o3ds_valid = false, .optimize_n3ds_valid = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = OPTIMIZE3DS_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_MENU_NO_ACTION};

static const Optimize3DSMenuOptionInfo optimize_3ds_format_explanation_0 = {
.base_name = "Possible Data Types:", .false_name = "", .is_selectable = false,
.optimize_o3ds_valid = true, .optimize_n3ds_valid = true,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_MENU_NO_ACTION};

static const Optimize3DSMenuOptionInfo optimize_3ds_format_explanation_1 = {
.base_name = "RGB888 - Full color Format", .false_name = "", .is_selectable = false,
.optimize_o3ds_valid = true, .optimize_n3ds_valid = true,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_MENU_NO_ACTION};

static const Optimize3DSMenuOptionInfo optimize_3ds_format_explanation_2 = {
.base_name = "more demanding on USB host.", .false_name = "", .is_selectable = false,
.optimize_o3ds_valid = true, .optimize_n3ds_valid = true,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_MENU_NO_ACTION};

static const Optimize3DSMenuOptionInfo optimize_3ds_format_explanation_3 = {
.base_name = "RGB565 - Reduced color Format", .false_name = "", .is_selectable = false,
.optimize_o3ds_valid = true, .optimize_n3ds_valid = true,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_MENU_NO_ACTION};

static const Optimize3DSMenuOptionInfo optimize_3ds_format_explanation_4 = {
.base_name = "less demanding on USB host.", .false_name = "", .is_selectable = false,
.optimize_o3ds_valid = true, .optimize_n3ds_valid = true,
.is_inc = false, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_MENU_NO_ACTION,
.out_action = OPTIMIZE3DS_MENU_NO_ACTION};

static const Optimize3DSMenuOptionInfo optimize_3ds_change_format = {
.base_name = "Data Type", .false_name = "", .is_selectable = true,
.optimize_o3ds_valid = true, .optimize_n3ds_valid = true,
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = OPTIMIZE3DS_MENU_INPUT_VIDEO_FORMAT_INC,
.out_action = OPTIMIZE3DS_MENU_INPUT_VIDEO_FORMAT_DEC};

static const Optimize3DSMenuOptionInfo* pollable_options[] = {
//&optimize_o3ds_hw_option,
//&optimize_n3ds_hw_option,
&optimize_3ds_format_explanation_0,
&optimize_3ds_format_explanation_1,
//&optimize_3ds_format_explanation_2,
&optimize_3ds_format_explanation_3,
//&optimize_3ds_format_explanation_4,
&optimize_3ds_change_format,
};

Optimize3DSMenu::Optimize3DSMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

Optimize3DSMenu::~Optimize3DSMenu() {
	delete []this->options_indexes;
}

void Optimize3DSMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "  Optimize 3DS Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void Optimize3DSMenu::insert_data(CaptureDevice* device) {
	bool is_o3ds = false;
	bool is_n3ds = false;
	#ifdef USE_CYPRESS_OPTIMIZE
	is_o3ds = is_device_optimize_o3ds(device);
	is_n3ds = is_device_optimize_n3ds(device);
	#endif
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		bool valid = true;
		if(is_o3ds)
			valid = valid && pollable_options[i]->optimize_o3ds_valid;
		if(is_n3ds)
			valid = valid && pollable_options[i]->optimize_n3ds_valid;
		if(valid) {
			this->options_indexes[this->num_enabled_options] = (int)i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void Optimize3DSMenu::reset_output_option() {
	this->selected_index = Optimize3DSMenuOutAction::OPTIMIZE3DS_MENU_NO_ACTION;
}

void Optimize3DSMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = OPTIMIZE3DS_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

size_t Optimize3DSMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string Optimize3DSMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool Optimize3DSMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool Optimize3DSMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

static std::string get_data_format_name(bool request_low_bw_format) {
	return request_low_bw_format ? "RGB565" : "RGB888";
}

void Optimize3DSMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status) {
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
			case OPTIMIZE3DS_MENU_INPUT_VIDEO_FORMAT_DEC:
				this->labels[index]->setText(this->setTextOptionString(real_index, get_data_format_name(capture_status->request_low_bw_format)));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

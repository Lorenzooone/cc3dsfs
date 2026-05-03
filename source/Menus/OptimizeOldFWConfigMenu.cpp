#include "OptimizeOldFWConfigMenu.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct OptimizeOldFWConfigMenuOptionInfo {
	const OptimizeOldFWConfigMenuSelectedCase selected_case_menu;
	const OptimizeOldFWConfigMenuSelectedOption selected_option_menu;
};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_2donly_lowscreenclk_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_LOWSCREENCLK};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_2donly_topscreenclk_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENCLK};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_2donly_topscreendata_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENDATA};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_2donly_topscreensync_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENSYNC};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_regular_lowscreenclk_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASEREGULAR, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_LOWSCREENCLK};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_regular_topscreenclk_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASEREGULAR, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENCLK};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_regular_topscreendata_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASEREGULAR, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENDATA};

static const OptimizeOldFWConfigMenuOptionInfo optimize_ofwcfg_regular_topscreensync_option = {
.selected_case_menu = OPTIMIZEOLDFWCONFIG_MENU_CASEREGULAR, .selected_option_menu = OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENSYNC};

static const OptimizeOldFWConfigMenuOptionInfo* pollable_options[] = {
&optimize_ofwcfg_2donly_lowscreenclk_option,
&optimize_ofwcfg_2donly_topscreenclk_option,
&optimize_ofwcfg_2donly_topscreendata_option,
&optimize_ofwcfg_2donly_topscreensync_option,
&optimize_ofwcfg_regular_lowscreenclk_option,
&optimize_ofwcfg_regular_topscreenclk_option,
&optimize_ofwcfg_regular_topscreendata_option,
&optimize_ofwcfg_regular_topscreensync_option,
};

OptimizeOldFWConfigMenu::OptimizeOldFWConfigMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

OptimizeOldFWConfigMenu::~OptimizeOldFWConfigMenu() {
	delete []this->options_indexes;
}

void OptimizeOldFWConfigMenu::class_setup() {
	this->num_options_per_screen = 4;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Opt. Synch Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void OptimizeOldFWConfigMenu::insert_data() {
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

void OptimizeOldFWConfigMenu::reset_output_option() {
	this->selected_index = OptimizeOldFWConfigMenuOutAction::OPTIMIZEOLDFWCONFIG_MENU_NO_ACTION;
}

void OptimizeOldFWConfigMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION) {
		this->selected_index = OPTIMIZEOLDFWCONFIG_MENU_BACK;
		return;
	}
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = OPTIMIZEOLDFWCONFIG_MENU_CHANGE_INC;
	else
		this->selected_index = OPTIMIZEOLDFWCONFIG_MENU_CHANGE_DEC;
	this->selected_case = pollable_options[index]->selected_case_menu;
	this->selected_option = pollable_options[index]->selected_option_menu;
}

size_t OptimizeOldFWConfigMenu::get_num_options() {
	return this->num_enabled_options;
}

static uint8_t* _get_data_to_edit_in_case(CaptureOptimizeOldFirmwareConfigCase* optimize_old_fw_cfg_case, OptimizeOldFWConfigMenuSelectedOption selected_option) {
	switch(selected_option) {
		case OPTIMIZEOLDFWCONFIG_MENU_LOWSCREENCLK:
			return &optimize_old_fw_cfg_case->low_screen_clock;
		case OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENCLK:
			return &optimize_old_fw_cfg_case->top_screen_clock;
		case OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENDATA:
			return &optimize_old_fw_cfg_case->top_screen_data;
		case OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENSYNC:
			return &optimize_old_fw_cfg_case->top_screen_sync;
		default:
			return &optimize_old_fw_cfg_case->low_screen_clock;
	}
	
}

static uint8_t* _get_data_to_edit(CaptureOptimizeOldFirmwareConfig* optimize_old_fw_cfg, OptimizeOldFWConfigMenuSelectedCase selected_case, OptimizeOldFWConfigMenuSelectedOption selected_option) {
	switch(selected_case) {
		case OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY:
			return _get_data_to_edit_in_case(&optimize_old_fw_cfg->mode_2d_only, selected_option);
		case OPTIMIZEOLDFWCONFIG_MENU_CASEREGULAR:
			return _get_data_to_edit_in_case(&optimize_old_fw_cfg->mode_regular, selected_option);
		default:
			return _get_data_to_edit_in_case(&optimize_old_fw_cfg->mode_2d_only, selected_option);
	}
}

static uint8_t* _get_data_to_edit(CaptureOptimizeOldFirmwareConfig* optimize_old_fw_cfg, const OptimizeOldFWConfigMenuOptionInfo* option) {
	return _get_data_to_edit(optimize_old_fw_cfg, option->selected_case_menu, option->selected_option_menu);
}

uint8_t* OptimizeOldFWConfigMenu::get_data_to_edit(CaptureOptimizeOldFirmwareConfig* optimize_old_fw_cfg) {
	return _get_data_to_edit(optimize_old_fw_cfg, this->selected_case, this->selected_option);
}

static std::string _get_string_for_option(const OptimizeOldFWConfigMenuOptionInfo* option) {
	std::string out_str = "";
	switch(option->selected_case_menu) {
		case OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY:
			out_str = "2D Only ";
			break;
		case OPTIMIZEOLDFWCONFIG_MENU_CASEREGULAR:
			out_str = "Regular ";
			break;
		default:
			break;
	}

	switch(option->selected_option_menu) {
		case OPTIMIZEOLDFWCONFIG_MENU_LOWSCREENCLK:
			out_str += "Low Clk";
			break;
		case OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENCLK:
			out_str += "Top Clk";
			break;
		case OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENDATA:
			out_str += "Top Data";
			break;
		case OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENSYNC:
			out_str += "Top Sync";
			break;
		default:
			break;
	}

	return out_str;
}

std::string OptimizeOldFWConfigMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return "+";
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return "-";
	return _get_string_for_option(pollable_options[this->options_indexes[index]]);
}

bool OptimizeOldFWConfigMenu::is_option_selectable(int index, int action) {
	return true;
}

bool OptimizeOldFWConfigMenu::is_option_inc_dec(int index) {
	return true;
}

float OptimizeOldFWConfigMenu::get_option_text_factor(int index) {
	return 0.85f;
}

void OptimizeOldFWConfigMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status) {
	CaptureOptimizeOldFirmwareConfig* to_display_fw_config = &capture_status->device_specific_status.optimize_status.old_fw_config_full_bw_format;
	if(capture_status->device_specific_status.optimize_status.request_low_bw_format_old_2ds)
		to_display_fw_config = &capture_status->device_specific_status.optimize_status.old_fw_config_low_bw_format;

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
		this->labels[index]->setText(this->setTextOptionInt(real_index, *_get_data_to_edit(to_display_fw_config, pollable_options[real_index])));
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

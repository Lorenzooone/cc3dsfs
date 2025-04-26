#include "ExtraSettingsMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct ExtraSettingsMenuOptionInfo {
	const std::string base_name;
	const bool is_selectable;
	const bool active_fullscreen;
	const bool active_windowed_screen;
	const bool active_joint_screen;
	const bool active_top_screen;
	const bool active_bottom_screen;
	const ExtraSettingsMenuOutAction out_action;
};

static const ExtraSettingsMenuOptionInfo warning_option = {
.base_name = "Advanced users only!", .is_selectable = false,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = EXTRA_SETTINGS_MENU_NO_ACTION};

static const ExtraSettingsMenuOptionInfo windowed_option = {
.base_name = "Windowed Mode", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = EXTRA_SETTINGS_MENU_FULLSCREEN};

static const ExtraSettingsMenuOptionInfo fullscreen_option = {
.base_name = "Fullscreen Mode", .is_selectable = true,
.active_fullscreen = false, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = EXTRA_SETTINGS_MENU_FULLSCREEN};

static const ExtraSettingsMenuOptionInfo join_screens_option = {
.base_name = "Join Screens", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = true,
.out_action = EXTRA_SETTINGS_MENU_SPLIT};

static const ExtraSettingsMenuOptionInfo split_screens_option = {
.base_name = "Split Screens", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = EXTRA_SETTINGS_MENU_SPLIT};

static const ExtraSettingsMenuOptionInfo quit_option = {
.base_name = "Quit Application", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = EXTRA_SETTINGS_MENU_QUIT_APPLICATION};

static const ExtraSettingsMenuOptionInfo* pollable_options[] = {
&warning_option,
&windowed_option,
&fullscreen_option,
&join_screens_option,
&split_screens_option,
&quit_option,
};

ExtraSettingsMenu::ExtraSettingsMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

ExtraSettingsMenu::~ExtraSettingsMenu() {
	delete []this->options_indexes;
}

void ExtraSettingsMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Extra Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void ExtraSettingsMenu::insert_data(ScreenType s_type, bool is_fullscreen) {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		bool valid = true;
		if(is_fullscreen)
			valid = valid && pollable_options[i]->active_fullscreen;
		else
			valid = valid && pollable_options[i]->active_windowed_screen;
		if(s_type == ScreenType::TOP)
			valid = valid && pollable_options[i]->active_top_screen;
		else if(s_type == ScreenType::BOTTOM)
			valid = valid && pollable_options[i]->active_bottom_screen;
		else
			valid = valid && pollable_options[i]->active_joint_screen;
		if(valid) {
			this->options_indexes[this->num_enabled_options] = (int)i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void ExtraSettingsMenu::reset_output_option() {
	this->selected_index = ExtraSettingsMenuOutAction::EXTRA_SETTINGS_MENU_NO_ACTION;
}

void ExtraSettingsMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = EXTRA_SETTINGS_MENU_BACK;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

bool ExtraSettingsMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

size_t ExtraSettingsMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string ExtraSettingsMenu::get_string_option(int index, int action) {
	return pollable_options[this->options_indexes[index]]->base_name;
}

void ExtraSettingsMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

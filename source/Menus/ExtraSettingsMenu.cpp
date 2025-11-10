#include "ExtraSettingsMenu.hpp"
#include "USBConflictResolutionMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct ExtraSettingsMenuOptionInfo {
	const std::string base_name;
	const bool is_selectable;
	const bool active_fullscreen;
	const bool active_windowed_screen;
	const bool active_joint_screen;
	const bool active_top_screen;
	const bool active_bottom_screen;
	const bool active_regular;
	const bool active_mono_app;
	const ExtraSettingsMenuOutAction out_action;
};

static const ExtraSettingsMenuOptionInfo warning_option = {
.base_name = "Advanced users only!", .is_selectable = false,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.active_regular = true, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_NO_ACTION};

static const ExtraSettingsMenuOptionInfo reset_to_default_option = {
.base_name = "Reset Settings", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.active_regular = true, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_RESET_SETTINGS};

static const ExtraSettingsMenuOptionInfo windowed_option = {
.base_name = "Windowed Mode", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.active_regular = false, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_FULLSCREEN};

static const ExtraSettingsMenuOptionInfo fullscreen_option = {
.base_name = "Fullscreen Mode", .is_selectable = true,
.active_fullscreen = false, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.active_regular = false, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_FULLSCREEN};

static const ExtraSettingsMenuOptionInfo join_screens_option = {
.base_name = "Join Screens", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = true,
.active_regular = false, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_SPLIT};

static const ExtraSettingsMenuOptionInfo split_screens_option = {
.base_name = "Split Screens", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.active_regular = false, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_SPLIT};

static const ExtraSettingsMenuOptionInfo quit_option = {
.base_name = "Quit Application", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.active_regular = false, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_QUIT_APPLICATION};

static const ExtraSettingsMenuOptionInfo usb_conflict_resolution_menu_option = {
.base_name = "USB Conflict Resolution", .is_selectable = true,
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.active_regular = true, .active_mono_app = true,
.out_action = EXTRA_SETTINGS_MENU_USB_CONFLICT_RESOLUTION};

static const ExtraSettingsMenuOptionInfo* pollable_options[] = {
&warning_option,
&reset_to_default_option,
&windowed_option,
&fullscreen_option,
&join_screens_option,
&split_screens_option,
&usb_conflict_resolution_menu_option,
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

static bool is_option_valid(const ExtraSettingsMenuOptionInfo* option, ScreenType s_type, bool is_fullscreen, bool is_mono_app) {
	bool valid = true;
	if(is_fullscreen)
		valid = valid && option->active_fullscreen;
	else
		valid = valid && option->active_windowed_screen;
	if(s_type == ScreenType::TOP)
		valid = valid && option->active_top_screen;
	else if(s_type == ScreenType::BOTTOM)
		valid = valid && option->active_bottom_screen;
	else
		valid = valid && option->active_joint_screen;
	if(is_mono_app)
		valid = valid && option->active_mono_app;
	else
		valid = valid && option->active_regular;
	if(option->out_action == EXTRA_SETTINGS_MENU_USB_CONFLICT_RESOLUTION)
		valid = valid && (USBConflictResolutionMenu::get_total_possible_selectable_inserted() > 0);
	return valid;
}

int ExtraSettingsMenu::get_total_possible_selectable_inserted(ScreenType s_type, bool is_fullscreen, bool is_mono_app) {
	int num_insertable = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		if(is_option_valid(pollable_options[i], s_type, is_fullscreen, is_mono_app) && pollable_options[i]->is_selectable)
			num_insertable++;
	}
	return num_insertable;
}

void ExtraSettingsMenu::insert_data(ScreenType s_type, bool is_fullscreen, bool is_mono_app) {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		if(is_option_valid(pollable_options[i], s_type, is_fullscreen, is_mono_app)) {
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

#include "MainMenu.hpp"
#include "usb_is_device_acquisition.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct MainMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool active_fullscreen;
	const bool active_windowed_screen;
	const bool active_joint_screen;
	const bool active_top_screen;
	const bool active_bottom_screen;
	const bool enabled_normal_mode;
	const bool enabled_mono_mode;
	const bool is_cc_specific;
	const MainMenuOutAction out_action;
};

static const MainMenuOptionInfo connect_option = {
.base_name = "Disconnect", .false_name = "Connect",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_OPEN};

static const MainMenuOptionInfo windowed_option = {
.base_name = "Windowed Mode", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = false, .is_cc_specific = false,
.out_action = MAIN_MENU_FULLSCREEN};

static const MainMenuOptionInfo fullscreen_option = {
.base_name = "Fullscreen Mode", .false_name = "",
.active_fullscreen = false, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = false, .is_cc_specific = false,
.out_action = MAIN_MENU_FULLSCREEN};

static const MainMenuOptionInfo join_screens_option = {
.base_name = "Join Screens", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = false, .is_cc_specific = false,
.out_action = MAIN_MENU_SPLIT};

static const MainMenuOptionInfo split_screens_option = {
.base_name = "Split Screens", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.enabled_normal_mode = true, .enabled_mono_mode = false, .is_cc_specific = false,
.out_action = MAIN_MENU_SPLIT};

static const MainMenuOptionInfo video_settings_option = {
.base_name = "Video Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_VIDEO_SETTINGS};

static const MainMenuOptionInfo quit_option = {
.base_name = "Quit Application", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = false, .is_cc_specific = false,
.out_action = MAIN_MENU_QUIT_APPLICATION};

static const MainMenuOptionInfo audio_settings_option = {
.base_name = "Audio Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_AUDIO_SETTINGS};

static const MainMenuOptionInfo input_settings_option = {
.base_name = "Input Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_INPUT_SETTINGS};

static const MainMenuOptionInfo save_profiles_option = {
.base_name = "Save Profile", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_SAVE_PROFILES};

static const MainMenuOptionInfo load_profiles_option = {
.base_name = "Load Profile", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_LOAD_PROFILES};

static const MainMenuOptionInfo status_option = {
.base_name = "Status", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_STATUS};

static const MainMenuOptionInfo licenses_option = {
.base_name = "Licenses", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_LICENSES};

static const MainMenuOptionInfo extra_settings_option = {
.base_name = "Extra Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = false, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_EXTRA_SETTINGS};

/*
static const MainMenuOptionInfo shortcut_option = {
.base_name = "Shortcuts", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_SHORTCUT_SETTINGS};
*/

static const MainMenuOptionInfo shutdown_option = {
.base_name = "Shutdown", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = false, .enabled_mono_mode = true, .is_cc_specific = false,
.out_action = MAIN_MENU_SHUTDOWN};

static const MainMenuOptionInfo isn_settings_option = {
.base_name = "IS Nitro Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = true,
.out_action = MAIN_MENU_ISN_SETTINGS};

static const MainMenuOptionInfo ist_settings_option = {
.base_name = "IS TWL Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.enabled_normal_mode = true, .enabled_mono_mode = true, .is_cc_specific = true,
.out_action = MAIN_MENU_IST_SETTINGS};

static const MainMenuOptionInfo* pollable_options[] = {
&connect_option,
&windowed_option,
&fullscreen_option,
&join_screens_option,
&split_screens_option,
&video_settings_option,
&audio_settings_option,
&input_settings_option,
&save_profiles_option,
&load_profiles_option,
//&shortcut_option,
&status_option,
&isn_settings_option,
&ist_settings_option,
&licenses_option,
&extra_settings_option,
&quit_option,
&shutdown_option,
};

MainMenu::MainMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

MainMenu::~MainMenu() {
	delete []this->options_indexes;
}

void MainMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Main Menu";
	this->show_back_x = true;
	this->show_x = true;
	this->show_title = true;
}

static bool check_cc_specific_option(const MainMenuOptionInfo* option, CaptureDevice* device) {
	#ifdef USE_IS_DEVICES_USB
	if((option->out_action == MAIN_MENU_ISN_SETTINGS) && is_device_is_nitro(device))
		return true;
	if((option->out_action == MAIN_MENU_IST_SETTINGS) && is_device_is_twl(device))
		return true;
	#endif
	return false;
}

void MainMenu::insert_data(ScreenType s_type, bool is_fullscreen, bool mono_app_mode, CaptureDevice* device, bool connected) {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
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
		if(mono_app_mode)
			valid = valid && pollable_options[i]->enabled_mono_mode;
		else
			valid = valid && pollable_options[i]->enabled_normal_mode;
		if(pollable_options[i]->is_cc_specific)
			valid = valid && connected && check_cc_specific_option(pollable_options[i], device);
		//if((pollable_options[i]->out_action == MAIN_MENU_SHORTCUT_SETTINGS) && (!enable_shortcut))
		//	valid = false;
		if(valid) {
			this->options_indexes[this->num_enabled_options] = i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void MainMenu::reset_output_option() {
	this->selected_index = MainMenuOutAction::MAIN_MENU_NO_ACTION;
}

void MainMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = MAIN_MENU_CLOSE_MENU;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int MainMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string MainMenu::get_string_option(int index, int action) {
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

void MainMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, bool connected) {
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
			case MAIN_MENU_OPEN:
				this->labels[index]->setText(this->setTextOptionBool(real_index, connected));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

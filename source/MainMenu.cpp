#include "MainMenu.hpp"

#define NUM_TOTAL_MAIN_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

#define OPTION_WINDOWED true
#define OPTION_FULLSCREEN true
#define OPTION_JOIN true
#define OPTION_SPLIT true
#define OPTION_EXTRA false
#define OPTION_QUIT true
#define OPTION_SHUTDOWN false

struct MainMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool active_fullscreen;
	const bool active_windowed_screen;
	const bool active_joint_screen;
	const bool active_top_screen;
	const bool active_bottom_screen;
	const MainMenuOutAction out_action;
};

static const MainMenuOptionInfo connect_option = {
.base_name = "Disconnect", .false_name = "Connect",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_OPEN};

static const MainMenuOptionInfo windowed_option = {
.base_name = "Windowed Mode", .false_name = "",
.active_fullscreen = OPTION_WINDOWED, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_FULLSCREEN};

static const MainMenuOptionInfo fullscreen_option = {
.base_name = "Fullscreen Mode", .false_name = "",
.active_fullscreen = false, .active_windowed_screen = OPTION_FULLSCREEN,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_FULLSCREEN};

static const MainMenuOptionInfo join_screens_option = {
.base_name = "Join Screens", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = OPTION_JOIN, .active_bottom_screen = OPTION_JOIN,
.out_action = MAIN_MENU_SPLIT};

static const MainMenuOptionInfo split_screens_option = {
.base_name = "Split Screens", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = OPTION_SPLIT, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_SPLIT};

static const MainMenuOptionInfo vsync_option = {
.base_name = "Turn VSync Off", .false_name = "Turn VSync On",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_VSYNC};

static const MainMenuOptionInfo async_option = {
.base_name = "Turn Async Off", .false_name = "Turn Async On",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_ASYNC};

static const MainMenuOptionInfo blur_option = {
.base_name = "Turn Blur Off", .false_name = "Turn Blur On",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_BLUR};

static const MainMenuOptionInfo padding_option = {
.base_name = "Turn Padding Off", .false_name = "Turn Padding On",
.active_fullscreen = false, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_PADDING};

static const MainMenuOptionInfo quit_option = {
.base_name = "Quit Application", .false_name = "",
.active_fullscreen = OPTION_QUIT, .active_windowed_screen = OPTION_QUIT,
.active_joint_screen = OPTION_QUIT, .active_top_screen = OPTION_QUIT, .active_bottom_screen = OPTION_QUIT,
.out_action = MAIN_MENU_QUIT_APPLICATION};

static const MainMenuOptionInfo shutdown_option = {
.base_name = "Shutdown", .false_name = "",
.active_fullscreen = OPTION_SHUTDOWN, .active_windowed_screen = OPTION_SHUTDOWN,
.active_joint_screen = OPTION_SHUTDOWN, .active_top_screen = OPTION_SHUTDOWN, .active_bottom_screen = OPTION_SHUTDOWN,
.out_action = MAIN_MENU_SHUTDOWN};

static const MainMenuOptionInfo crop_option = {
.base_name = "Crop Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_CROPPING};

static const MainMenuOptionInfo top_par_option = {
.base_name = "Top Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_TOP_PAR};

static const MainMenuOptionInfo bot_par_option = {
.base_name = "Bottom Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_BOT_PAR};

static const MainMenuOptionInfo one_par_option = {
.base_name = "Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_ONE_PAR};

static const MainMenuOptionInfo bottom_screen_pos_option = {
.base_name = "Bottom Screen Pos.", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_BOTTOM_SCREEN_POS};

static const MainMenuOptionInfo small_screen_offset_option = {
.base_name = "Smaller Screen Offset", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_SMALL_SCREEN_OFFSET};

static const MainMenuOptionInfo subscreen_distance_option = {
.base_name = "Sub-Screen Distance", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_SMALL_SCREEN_DISTANCE};

static const MainMenuOptionInfo canvas_x_pos_option = {
.base_name = "Canvas X Position", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_SCREENS_X_POS};

static const MainMenuOptionInfo canvas_y_pos_option = {
.base_name = "Canvas Y Position", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_SCREENS_Y_POS};

static const MainMenuOptionInfo window_scaling_option = {
.base_name = "Scaling Factor", .false_name = "",
.active_fullscreen = false, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_WINDOW_SCALING};

static const MainMenuOptionInfo fullscreen_scaling_option = {
.base_name = "Screens Scaling", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_FULLSCREEN_SCALING};

static const MainMenuOptionInfo bfi_settings_option = {
.base_name = "BFI Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_BFI_SETTINGS};

static const MainMenuOptionInfo menu_scaling_option = {
.base_name = "Menu Scaling", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_MENU_SCALING};

static const MainMenuOptionInfo resolution_settings_option = {
.base_name = "Resolution Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_RESOLUTION_SETTINGS};

static const MainMenuOptionInfo audio_settings_option = {
.base_name = "Audio Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_AUDIO_SETTINGS};

static const MainMenuOptionInfo save_profiles_option = {
.base_name = "Save Profile", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_SAVE_PROFILES};

static const MainMenuOptionInfo load_profiles_option = {
.base_name = "Load Profile", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_LOAD_PROFILES};

static const MainMenuOptionInfo status_option = {
.base_name = "Status", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_STATUS};

static const MainMenuOptionInfo licenses_option = {
.base_name = "Licenses", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_LICENSES};

static const MainMenuOptionInfo extra_settings_option = {
.base_name = "Extra Settings", .false_name = "",
.active_fullscreen = OPTION_EXTRA, .active_windowed_screen = OPTION_EXTRA,
.active_joint_screen = OPTION_EXTRA, .active_top_screen = OPTION_EXTRA, .active_bottom_screen = OPTION_EXTRA,
.out_action = MAIN_MENU_EXTRA_SETTINGS};

static const MainMenuOptionInfo top_rotation_option = {
.base_name = "Top Screen Rot.", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_TOP_ROTATION};

static const MainMenuOptionInfo bottom_rotation_option = {
.base_name = "Bottom Screen Rot.", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.out_action = MAIN_MENU_BOTTOM_ROTATION};

static const MainMenuOptionInfo top_one_rotation_option = {
.base_name = "Screen Rotation", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = false,
.out_action = MAIN_MENU_TOP_ROTATION};

static const MainMenuOptionInfo bottom_one_rotation_option = {
.base_name = "Screen Rotation", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = false, .active_bottom_screen = true,
.out_action = MAIN_MENU_BOTTOM_ROTATION};

static const MainMenuOptionInfo* pollable_options[] = {
&connect_option,
&windowed_option,
&fullscreen_option,
&join_screens_option,
&split_screens_option,
&crop_option,
&top_par_option,
&bot_par_option,
&one_par_option,
&vsync_option,
&async_option,
&blur_option,
&bottom_screen_pos_option,
&small_screen_offset_option,
&subscreen_distance_option,
&canvas_x_pos_option,
&canvas_y_pos_option,
&top_rotation_option,
&bottom_rotation_option,
&top_one_rotation_option,
&bottom_one_rotation_option,
&window_scaling_option,
&fullscreen_scaling_option,
&menu_scaling_option,
&resolution_settings_option,
&padding_option,
&bfi_settings_option,
&audio_settings_option,
&save_profiles_option,
&load_profiles_option,
&extra_settings_option,
&status_option,
&licenses_option,
&quit_option,
&shutdown_option
};

MainMenu::MainMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MAIN_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

MainMenu::~MainMenu() {
	delete []this->options_indexes;
}

void MainMenu::class_setup() {
	this->num_elements_per_screen = 5;
	this->min_elements_text_scaling_factor = num_elements_per_screen + 2;
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

void MainMenu::insert_data(ScreenType s_type, bool is_fullscreen) {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MAIN_MENU_OPTIONS; i++) {
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
			this->options_indexes[this->num_enabled_options] = i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void MainMenu::reset_output_option() {
	this->selected_index = MainMenuOutAction::MAIN_MENU_NO_ACTION;
}

void MainMenu::set_output_option(int index) {
	if(index == -1)
		this->selected_index = MAIN_MENU_CLOSE_MENU;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int MainMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string MainMenu::get_string_option(int index) {
	return pollable_options[this->options_indexes[index]]->base_name;
}

static std::string setTextOptionBool(int option_index, bool value) {
	if(value)
		return pollable_options[option_index]->base_name;
	return pollable_options[option_index]->false_name;
}

static std::string setTextOptionFloat(int option_index, float value) {
	return pollable_options[option_index]->base_name + ": " + get_float_str_decimals(value, 1);
}

static std::string setTextOptionInt(int option_index, int value) {
	return pollable_options[option_index]->base_name + ": " + std::to_string(value);
}

static std::string setTextOptionDualPercentage(int option_index, int value_1, int value_2) {
	int sum = value_1 + value_2;
	value_1 = ((value_1 * 100) + (sum / 2)) / sum;
	value_2 = ((value_2 * 100) + (sum / 2)) / sum;
	value_1 += 100 - (value_1 + value_2);
	return pollable_options[option_index]->base_name + ": " + std::to_string(value_1) + " - " + std::to_string(value_2);
}

void MainMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, bool connected) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_elements_per_screen;
	for(int i = 0; i < this->num_elements_per_screen; i++) {
		int index = i + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int option_index = this->options_indexes[start + i];
		switch(pollable_options[option_index]->out_action) {
			case MAIN_MENU_OPEN:
				this->labels[index]->setText(setTextOptionBool(option_index, connected));
				break;
			case MAIN_MENU_VSYNC:
				this->labels[index]->setText(setTextOptionBool(option_index, info->v_sync_enabled));
				break;
			case MAIN_MENU_ASYNC:
				this->labels[index]->setText(setTextOptionBool(option_index, info->async));
				break;
			case MAIN_MENU_BLUR:
				this->labels[index]->setText(setTextOptionBool(option_index, info->is_blurred));
				break;
			case MAIN_MENU_PADDING:
				this->labels[index]->setText(setTextOptionBool(option_index, info->rounded_corners_fix));
				break;
			case MAIN_MENU_WINDOW_SCALING:
				this->labels[index]->setText(setTextOptionFloat(option_index, info->scaling));
				break;
			case MAIN_MENU_MENU_SCALING:
				this->labels[index]->setText(setTextOptionFloat(option_index, info->menu_scaling_factor));
				break;
			case MAIN_MENU_FULLSCREEN_SCALING:
				this->labels[index]->setText(setTextOptionDualPercentage(option_index, info->top_scaling, info->bot_scaling));
				break;
			case MAIN_MENU_TOP_ROTATION:
				this->labels[index]->setText(setTextOptionInt(option_index, info->top_rotation));
				break;
			case MAIN_MENU_BOTTOM_ROTATION:
				this->labels[index]->setText(setTextOptionInt(option_index, info->bot_rotation));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

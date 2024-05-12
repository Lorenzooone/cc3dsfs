#include "MainMenu.hpp"

#define NUM_TOTAL_MAIN_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

#define OPTION_WINDOWED true
#define OPTION_FULLSCREEN true
#define OPTION_JOIN true
#define OPTION_SPLIT true

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

static const MainMenuOptionInfo close_option = {
.base_name = "Close Menu", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_CLOSE_MENU};

static const MainMenuOptionInfo quit_option = {
.base_name = "Quit Application", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.out_action = MAIN_MENU_QUIT_APPLICATION};

static const MainMenuOptionInfo* pollable_options[] = {
&connect_option,
&windowed_option,
&fullscreen_option,
&join_screens_option,
&split_screens_option,
&vsync_option,
&async_option,
&blur_option,
&padding_option,
&close_option,
&quit_option
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
	this->num_page_elements = 3;
	this->num_elements_displayed_per_screen = num_elements_per_screen + num_page_elements;
	this->num_vertical_slices = (num_elements_displayed_per_screen - (num_page_elements - 1));
	this->num_elements_start_id = 0;
	this->num_page_elements_start_id = num_elements_per_screen + num_elements_start_id;
	this->prev_page_id = num_page_elements_start_id;
	this->info_page_id = num_page_elements_start_id + 1;
	this->next_page_id = num_page_elements_start_id + 2;
	this->min_elements_text_scaling_factor = this->num_vertical_slices;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
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

void MainMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, bool connected) {
	int num_pages = 1 + ((this->get_num_options() - 1) / this->num_elements_per_screen);
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_elements_per_screen;
	for(int i = 0; i < this->num_elements_per_screen; i++) {
		if(!this->future_enabled_labels[i])
			continue;
		int option_index = this->options_indexes[start + i];
		switch(pollable_options[option_index]->out_action) {
			case MAIN_MENU_OPEN:
				this->labels[i]->setText(setTextOptionBool(option_index, connected));
				break;
			case MAIN_MENU_VSYNC:
				this->labels[i]->setText(setTextOptionBool(option_index, info->v_sync_enabled));
				break;
			case MAIN_MENU_ASYNC:
				this->labels[i]->setText(setTextOptionBool(option_index, info->async));
				break;
			case MAIN_MENU_BLUR:
				this->labels[i]->setText(setTextOptionBool(option_index, info->is_blurred));
				break;
			case MAIN_MENU_PADDING:
				this->labels[i]->setText(setTextOptionBool(option_index, info->rounded_corners_fix));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

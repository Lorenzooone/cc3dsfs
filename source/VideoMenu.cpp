#include "VideoMenu.hpp"

#define NUM_TOTAL_VIDEO_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct VideoMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool active_fullscreen;
	const bool active_windowed_screen;
	const bool active_joint_screen;
	const bool active_top_screen;
	const bool active_bottom_screen;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const VideoMenuOutAction inc_out_action;
	const VideoMenuOutAction out_action;
};

static const VideoMenuOptionInfo vsync_option = {
.base_name = "Turn VSync Off", .false_name = "Turn VSync On",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_VSYNC};

static const VideoMenuOptionInfo async_option = {
.base_name = "Turn Async Off", .false_name = "Turn Async On",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_ASYNC};

static const VideoMenuOptionInfo blur_option = {
.base_name = "Turn Blur Off", .false_name = "Turn Blur On",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BLUR};

static const VideoMenuOptionInfo padding_option = {
.base_name = "Turn Padding Off", .false_name = "Turn Padding On",
.active_fullscreen = false, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_PADDING};

static const VideoMenuOptionInfo crop_option = {
.base_name = "Crop Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_CROPPING};

static const VideoMenuOptionInfo top_par_option = {
.base_name = "Top Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_TOP_PAR};

static const VideoMenuOptionInfo bot_par_option = {
.base_name = "Bottom Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BOT_PAR};

static const VideoMenuOptionInfo one_par_option = {
.base_name = "Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_ONE_PAR};

static const VideoMenuOptionInfo bottom_screen_pos_option = {
.base_name = "Bottom Screen Pos.", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BOTTOM_SCREEN_POS};

static const VideoMenuOptionInfo small_screen_offset_option = {
.base_name = "Smaller Screen Offset", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_SMALL_SCREEN_OFFSET};

static const VideoMenuOptionInfo subscreen_distance_option = {
.base_name = "Sub-Screen Distance", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_SMALL_SCREEN_DISTANCE};

static const VideoMenuOptionInfo canvas_x_pos_option = {
.base_name = "Canvas X Position", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_SCREENS_X_POS};

static const VideoMenuOptionInfo canvas_y_pos_option = {
.base_name = "Canvas Y Position", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_SCREENS_Y_POS};

static const VideoMenuOptionInfo window_scaling_option = {
.base_name = "Scaling Factor", .false_name = "",
.active_fullscreen = false, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = true, .dec_str = "-0.5", .inc_str = "+0.5", .inc_out_action = VIDEO_MENU_WINDOW_SCALING_INC,
.out_action = VIDEO_MENU_WINDOW_SCALING_DEC};

static const VideoMenuOptionInfo fullscreen_scaling_option = {
.base_name = "Screens Scaling", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = true, .dec_str = "+ Top", .inc_str = "+ Bot.", .inc_out_action = VIDEO_MENU_FULLSCREEN_SCALING_BOTTOM,
.out_action = VIDEO_MENU_FULLSCREEN_SCALING_TOP};

static const VideoMenuOptionInfo bfi_settings_option = {
.base_name = "BFI Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BFI_SETTINGS};

static const VideoMenuOptionInfo menu_scaling_option = {
.base_name = "Menu Scaling", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = true, .dec_str = "-0.1", .inc_str = "+0.1", .inc_out_action = VIDEO_MENU_MENU_SCALING_INC,
.out_action = VIDEO_MENU_MENU_SCALING_DEC};

static const VideoMenuOptionInfo resolution_settings_option = {
.base_name = "Resolution Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_RESOLUTION_SETTINGS};

static const VideoMenuOptionInfo top_rotation_option = {
.base_name = "Top Screen Rot.", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_TOP_ROTATION};

static const VideoMenuOptionInfo bottom_rotation_option = {
.base_name = "Bottom Screen Rot.", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BOTTOM_ROTATION};

static const VideoMenuOptionInfo top_one_rotation_option = {
.base_name = "Screen Rotation", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_TOP_ROTATION};

static const VideoMenuOptionInfo bottom_one_rotation_option = {
.base_name = "Screen Rotation", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true,
.active_joint_screen = false, .active_top_screen = false, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BOTTOM_ROTATION};

static const VideoMenuOptionInfo* pollable_options[] = {
&crop_option,
&window_scaling_option,
&fullscreen_scaling_option,
&menu_scaling_option,
&bottom_screen_pos_option,
&vsync_option,
&async_option,
&blur_option,
&small_screen_offset_option,
&subscreen_distance_option,
&canvas_x_pos_option,
&canvas_y_pos_option,
&top_rotation_option,
&bottom_rotation_option,
&top_one_rotation_option,
&bottom_one_rotation_option,
&top_par_option,
&bot_par_option,
&one_par_option,
&resolution_settings_option,
&padding_option,
&bfi_settings_option,
};

VideoMenu::VideoMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_VIDEO_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

VideoMenu::~VideoMenu() {
	delete []this->options_indexes;
}

void VideoMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Video Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void VideoMenu::insert_data(ScreenType s_type, bool is_fullscreen) {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_VIDEO_MENU_OPTIONS; i++) {
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

void VideoMenu::reset_output_option() {
	this->selected_index = VideoMenuOutAction::VIDEO_MENU_NO_ACTION;
}

void VideoMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = VIDEO_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int VideoMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string VideoMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool VideoMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
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

void VideoMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int option_index = this->options_indexes[start + i];
		switch(pollable_options[option_index]->out_action) {
			case VIDEO_MENU_VSYNC:
				this->labels[index]->setText(setTextOptionBool(option_index, info->v_sync_enabled));
				break;
			case VIDEO_MENU_ASYNC:
				this->labels[index]->setText(setTextOptionBool(option_index, info->async));
				break;
			case VIDEO_MENU_BLUR:
				this->labels[index]->setText(setTextOptionBool(option_index, info->is_blurred));
				break;
			case VIDEO_MENU_PADDING:
				this->labels[index]->setText(setTextOptionBool(option_index, info->rounded_corners_fix));
				break;
			case VIDEO_MENU_WINDOW_SCALING_DEC:
				this->labels[index]->setText(setTextOptionFloat(option_index, info->scaling));
				break;
			case VIDEO_MENU_MENU_SCALING_DEC:
				this->labels[index]->setText(setTextOptionFloat(option_index, info->menu_scaling_factor));
				break;
			case VIDEO_MENU_FULLSCREEN_SCALING_TOP:
				this->labels[index]->setText(setTextOptionDualPercentage(option_index, info->top_scaling, info->bot_scaling));
				break;
			case VIDEO_MENU_TOP_ROTATION:
				this->labels[index]->setText(setTextOptionInt(option_index, info->top_rotation));
				break;
			case VIDEO_MENU_BOTTOM_ROTATION:
				this->labels[index]->setText(setTextOptionInt(option_index, info->bot_rotation));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

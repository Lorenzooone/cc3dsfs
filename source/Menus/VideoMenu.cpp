#include "VideoMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct VideoMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool active_fullscreen;
	const bool active_windowed_screen;
	const bool requires_titlebar_possible;
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
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_VSYNC};

static const VideoMenuOptionInfo async_option = {
.base_name = "Turn Async Off", .false_name = "Turn Async On",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_ASYNC};

static const VideoMenuOptionInfo blur_option = {
.base_name = "Turn Blur Off", .false_name = "Turn Blur On",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BLUR};

static const VideoMenuOptionInfo padding_option = {
.base_name = "Turn Padding Off", .false_name = "Turn Padding On",
.active_fullscreen = false, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_PADDING};

static const VideoMenuOptionInfo crop_option = {
.base_name = "Crop Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_CROPPING};

static const VideoMenuOptionInfo top_par_option = {
.base_name = "Top Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_TOP_PAR};

static const VideoMenuOptionInfo bot_par_option = {
.base_name = "Bottom Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BOT_PAR};

static const VideoMenuOptionInfo one_par_option = {
.base_name = "Screen PAR", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_ONE_PAR};

static const VideoMenuOptionInfo bottom_screen_pos_option = {
.base_name = "Screens Relative Positions", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BOTTOM_SCREEN_POS};

static const VideoMenuOptionInfo small_screen_offset_option = {
.base_name = "Screen Offset", .false_name = "",
.active_fullscreen = false, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = VIDEO_MENU_SMALL_SCREEN_OFFSET_INC,
.out_action = VIDEO_MENU_SMALL_SCREEN_OFFSET_DEC};

static const VideoMenuOptionInfo offset_settings_option = {
.base_name = "Offset Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_OFFSET_SETTINGS};

static const VideoMenuOptionInfo window_scaling_option = {
.base_name = "Scaling Factor", .false_name = "",
.active_fullscreen = false, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = VIDEO_MENU_WINDOW_SCALING_INC,
.out_action = VIDEO_MENU_WINDOW_SCALING_DEC};

static const VideoMenuOptionInfo bfi_settings_option = {
.base_name = "BFI Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_BFI_SETTINGS};

static const VideoMenuOptionInfo menu_scaling_option = {
.base_name = "Menu Scaling", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = VIDEO_MENU_MENU_SCALING_INC,
.out_action = VIDEO_MENU_MENU_SCALING_DEC};

static const VideoMenuOptionInfo resolution_settings_option = {
.base_name = "Resolution Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_RESOLUTION_SETTINGS};

static const VideoMenuOptionInfo rotation_settings_option = {
.base_name = "Rotation Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_ROTATION_SETTINGS};

static const VideoMenuOptionInfo top_one_rotation_option = {
.base_name = "Screen Rotation", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = false,
.is_inc = true, .dec_str = "Left", .inc_str = "Right", .inc_out_action = VIDEO_MENU_TOP_ROTATION_INC,
.out_action = VIDEO_MENU_TOP_ROTATION_DEC};

static const VideoMenuOptionInfo bottom_one_rotation_option = {
.base_name = "Screen Rotation", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = false, .active_top_screen = false, .active_bottom_screen = true,
.is_inc = true, .dec_str = "Left", .inc_str = "Right", .inc_out_action = VIDEO_MENU_BOTTOM_ROTATION_INC,
.out_action = VIDEO_MENU_BOTTOM_ROTATION_DEC};

/*
static const VideoMenuOptionInfo fast_poll_option = {
.base_name = "Disable Slow Poll", .false_name = "Enable Slow Poll",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_FAST_POLL};
*/

static const VideoMenuOptionInfo allow_game_crops_option = {
.base_name = "Disable Game Crops", .false_name = "Enable Game Crops",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_GAMES_CROPPING};

static const VideoMenuOptionInfo allow_fill_scaling_option = {
.base_name = "Disable Fill Scaling", .false_name = "Enable Fill Scaling",
.active_fullscreen = true, .active_windowed_screen = false, .requires_titlebar_possible = false,
.active_joint_screen = false, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_NON_INT_SCALING};

static const VideoMenuOptionInfo scaling_ratio_settings_option = {
.base_name = "Scaling & Ratio Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = false, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_SCALING_RATIO_SETTINGS};

static const VideoMenuOptionInfo titlebar_change_option = {
.base_name = "Disable Titlebar", .false_name = "Enable Titlebar",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = true,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_CHANGE_TITLEBAR};

static const VideoMenuOptionInfo video_effects_settings_option = {
.base_name = "Video Effects Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_VIDEO_EFFECTS_SETTINGS};

static const VideoMenuOptionInfo separator_settings_option = {
.base_name = "Separator Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = false, .active_bottom_screen = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_SEPARATOR_SETTINGS};

static const VideoMenuOptionInfo main_menu_3d_settings_option = {
.base_name = "3D Settings", .false_name = "",
.active_fullscreen = true, .active_windowed_screen = true, .requires_titlebar_possible = false,
.active_joint_screen = true, .active_top_screen = true, .active_bottom_screen = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = VIDEO_MENU_NO_ACTION,
.out_action = VIDEO_MENU_3D_SETTINGS};

static const VideoMenuOptionInfo* pollable_options[] = {
&crop_option,
&window_scaling_option,
&allow_fill_scaling_option,
&scaling_ratio_settings_option,
&menu_scaling_option,
&main_menu_3d_settings_option,
&bottom_screen_pos_option,
&separator_settings_option,
&vsync_option,
&async_option,
&blur_option,
//&fast_poll_option,
&small_screen_offset_option,
&offset_settings_option,
&rotation_settings_option,
&top_one_rotation_option,
&bottom_one_rotation_option,
&video_effects_settings_option,
&top_par_option,
&bot_par_option,
&one_par_option,
&allow_game_crops_option,
&titlebar_change_option,
&resolution_settings_option,
&padding_option,
&bfi_settings_option,
};

VideoMenu::VideoMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
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
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Video Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void VideoMenu::insert_data(ScreenType s_type, bool is_fullscreen, bool can_have_titlebar) {
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
		if((!can_have_titlebar) && pollable_options[i]->requires_titlebar_possible)
			valid = false;
		if(valid) {
			this->options_indexes[this->num_enabled_options] = (int)i;
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

size_t VideoMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string VideoMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool VideoMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

void VideoMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, ScreenType screen_type) {
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
			case VIDEO_MENU_VSYNC:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->v_sync_enabled));
				break;
			case VIDEO_MENU_ASYNC:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->async));
				break;
			case VIDEO_MENU_BLUR:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->is_blurred));
				break;
			case VIDEO_MENU_PADDING:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->rounded_corners_fix));
				break;
			case VIDEO_MENU_WINDOW_SCALING_DEC:
				this->labels[index]->setText(this->setTextOptionFloat(real_index, (float)info->scaling));
				break;
			case VIDEO_MENU_MENU_SCALING_DEC:
				this->labels[index]->setText(this->setTextOptionFloat(real_index, (float)info->menu_scaling_factor));
				break;
			case VIDEO_MENU_SMALL_SCREEN_OFFSET_DEC:
				this->labels[index]->setText(this->setTextOptionFloat(real_index, info->subscreen_offset));
				break;
			case VIDEO_MENU_TOP_ROTATION_DEC:
				this->labels[index]->setText(this->setTextOptionInt(real_index, info->top_rotation));
				break;
			case VIDEO_MENU_BOTTOM_ROTATION_DEC:
				this->labels[index]->setText(this->setTextOptionInt(real_index, info->bot_rotation));
				break;
			//case VIDEO_MENU_FAST_POLL:
			//	this->labels[index]->setText(this->setTextOptionBool(real_index, fast_poll));
			//	break;
			case VIDEO_MENU_GAMES_CROPPING:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->allow_games_crops));
				break;
			case VIDEO_MENU_NON_INT_SCALING:
				if(screen_type == ScreenType::TOP)
					this->labels[index]->setText(this->setTextOptionBool(real_index, info->use_non_integer_scaling_top));
				else if(screen_type == ScreenType::BOTTOM)
					this->labels[index]->setText(this->setTextOptionBool(real_index, info->use_non_integer_scaling_bottom));
				break;
			case VIDEO_MENU_CHANGE_TITLEBAR:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->have_titlebar));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

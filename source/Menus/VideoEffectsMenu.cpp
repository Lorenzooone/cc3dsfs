#include "VideoEffectsMenu.hpp"
#include "frontend.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct VideoEffectsMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_selectable;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const VideoEffectsMenuOutAction inc_out_action;
	const VideoEffectsMenuOutAction out_action;
};

static const VideoEffectsMenuOptionInfo input_colorspace_option = {
.base_name = "Input Colors", .false_name = "", .is_selectable = true,
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = VIDEO_EFFECTS_MENU_INPUT_COLORSPACE_INC,
.out_action = VIDEO_EFFECTS_MENU_INPUT_COLORSPACE_DEC};

static const VideoEffectsMenuOptionInfo frame_blending_option = {
.base_name = "Frame Blending", .false_name = "", .is_selectable = true,
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = VIDEO_EFFECTS_MENU_FRAME_BLENDING_INC,
.out_action = VIDEO_EFFECTS_MENU_FRAME_BLENDING_DEC};

static const VideoEffectsMenuOptionInfo* pollable_options[] = {
&input_colorspace_option,
&frame_blending_option,
};

VideoEffectsMenu::VideoEffectsMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

VideoEffectsMenu::~VideoEffectsMenu() {
	delete []this->options_indexes;
}

void VideoEffectsMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Video Effects Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void VideoEffectsMenu::insert_data() {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void VideoEffectsMenu::reset_output_option() {
	this->selected_index = VideoEffectsMenuOutAction::VIDEO_EFFECTS_MENU_NO_ACTION;
}

void VideoEffectsMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = VIDEO_EFFECTS_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int VideoEffectsMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string VideoEffectsMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool VideoEffectsMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool VideoEffectsMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

void VideoEffectsMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info) {
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
			case VIDEO_EFFECTS_MENU_INPUT_COLORSPACE_DEC:
				this->labels[index]->setText(this->setTextOptionString(real_index, get_name_input_colorspace_mode(info->in_colorspace_top)));
				break;
			case VIDEO_EFFECTS_MENU_FRAME_BLENDING_DEC:
				this->labels[index]->setText(this->setTextOptionString(real_index, get_name_frame_blending_mode(info->frame_blending_top)));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

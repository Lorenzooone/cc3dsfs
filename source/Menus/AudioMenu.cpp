#include "AudioMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct AudioMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const AudioMenuOutAction inc_out_action;
	const AudioMenuOutAction out_action;
};

static const AudioMenuOptionInfo audio_volume_option = {
.base_name = "Volume", .false_name = "",
.is_inc = true, .dec_str = "-", .inc_str = "+", .inc_out_action = AUDIO_MENU_VOLUME_INC,
.out_action = AUDIO_MENU_VOLUME_DEC};

static const AudioMenuOptionInfo audio_mute_option = {
.base_name = "Unmute Audio", .false_name = "Mute Audio",
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = AUDIO_MENU_NO_ACTION,
.out_action = AUDIO_MENU_MUTE};

static const AudioMenuOptionInfo audio_restart_option = {
.base_name = "Restart Audio", .false_name = "",
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = AUDIO_MENU_NO_ACTION,
.out_action = AUDIO_MENU_RESTART};

static const AudioMenuOptionInfo* pollable_options[] = {
&audio_volume_option,
&audio_mute_option,
&audio_restart_option,
};

AudioMenu::AudioMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

AudioMenu::~AudioMenu() {
	delete []this->options_indexes;
}

void AudioMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Audio Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void AudioMenu::insert_data() {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void AudioMenu::reset_output_option() {
	this->selected_index = AudioMenuOutAction::AUDIO_MENU_NO_ACTION;
}

void AudioMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = AUDIO_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int AudioMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string AudioMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool AudioMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

static std::string setTextOptionInt(int option_index, int value) {
	return pollable_options[option_index]->base_name + ": " + std::to_string(value);
}

static std::string setTextOptionBool(int option_index, bool value) {
	if(value)
		return pollable_options[option_index]->base_name;
	return pollable_options[option_index]->false_name;
}

void AudioMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, AudioData *audio_data) {
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
			case AUDIO_MENU_VOLUME_DEC:
				this->labels[index]->setText(setTextOptionInt(option_index, audio_data->get_real_volume()));
				break;
			case AUDIO_MENU_MUTE:
				this->labels[index]->setText(setTextOptionBool(option_index, audio_data->get_mute()));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

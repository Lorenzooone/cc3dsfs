#include "AudioDeviceMenu.hpp"

AudioDeviceMenu::AudioDeviceMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->initialize(text_rectangle_pool);
}

AudioDeviceMenu::~AudioDeviceMenu() {
}

void AudioDeviceMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Audio Device Selection";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void AudioDeviceMenu::insert_data(std::vector<std::string>* possible_devices) {
	this->possible_devices = possible_devices;
	this->prepare_options();
}

void AudioDeviceMenu::reset_output_option() {
	this->selected_index = AUDIODEVICE_MENU_NO_ACTION;
}

void AudioDeviceMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = AUDIODEVICE_MENU_BACK;
	else
		this->selected_index = index;
}

size_t AudioDeviceMenu::get_num_options() {
	return this->possible_devices->size() + 1;
}

std::string AudioDeviceMenu::get_string_option(int index, int action) {
	if(index == 0)
		return "System Preference";
	return (*this->possible_devices)[index - 1];
}

void AudioDeviceMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, const audio_output_device_data curr_device_data) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int mode_index = start + i;
		int device_index = -1;
		if(curr_device_data.preference_requested)
			device_index = searchAudioDevice(curr_device_data.preferred, *this->possible_devices);
		if(mode_index == (device_index + 1))
			this->labels[index]->setText("< " + this->get_string_option(mode_index, DEFAULT_ACTION) + " >");
		else
			this->labels[index]->setText(this->get_string_option(mode_index, DEFAULT_ACTION));
	}
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

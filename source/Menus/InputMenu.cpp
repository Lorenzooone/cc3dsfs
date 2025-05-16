#include "InputMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct InputMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const InputMenuOutAction out_action;
};

static const InputMenuOptionInfo input_menu_toggle_buttons_option = {
.base_name = "Disable Buttons", .false_name = "Enable Buttons",
.out_action = INPUT_MENU_TOGGLE_BUTTONS};

static const InputMenuOptionInfo input_menu_toggle_controller_option = {
.base_name = "Disable Controllers", .false_name = "Enable Controllers",
.out_action = INPUT_MENU_TOGGLE_JOYSTICK};

static const InputMenuOptionInfo input_menu_toggle_keyboard_option = {
.base_name = "Disable Keyboard", .false_name = "Enable Keyboard",
.out_action = INPUT_MENU_TOGGLE_KEYBOARD};

static const InputMenuOptionInfo input_menu_toggle_mouse_option = {
.base_name = "Disable Mouse", .false_name = "Enable Mouse",
.out_action = INPUT_MENU_TOGGLE_MOUSE};

static const InputMenuOptionInfo input_menu_toggle_fast_poll_option = {
.base_name = "Speed up Input checks", .false_name = "Slow down Input checks",
.out_action = INPUT_MENU_TOGGLE_FAST_POLL};

static const InputMenuOptionInfo input_menu_shortcut_option = {
.base_name = "Shortcut Settings", .false_name = "",
.out_action = INPUT_MENU_SHORTCUT_SETTINGS};

static const InputMenuOptionInfo* pollable_options[] = {
&input_menu_shortcut_option,
&input_menu_toggle_fast_poll_option,
&input_menu_toggle_controller_option,
&input_menu_toggle_keyboard_option,
&input_menu_toggle_mouse_option,
&input_menu_toggle_buttons_option,
};

InputMenu::InputMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

InputMenu::~InputMenu() {
	delete []this->options_indexes;
}

void InputMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Input Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void InputMenu::insert_data(bool enabled_shortcuts, bool enabled_extra_buttons) {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		bool valid = true;
		InputMenuOutAction out_action = pollable_options[i]->out_action;
		if((out_action == INPUT_MENU_SHORTCUT_SETTINGS) && (!enabled_shortcuts))
			valid = false;
		if((out_action == INPUT_MENU_TOGGLE_BUTTONS) && (!(enabled_shortcuts || enabled_extra_buttons)))
			valid = false;
		if(valid) {
			this->options_indexes[this->num_enabled_options] = (int)i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void InputMenu::reset_output_option() {
	this->selected_index = InputMenuOutAction::INPUT_MENU_NO_ACTION;
}

void InputMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = INPUT_MENU_BACK;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

size_t InputMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string InputMenu::get_string_option(int index, int action) {
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

void InputMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, InputData* input_data) {
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
			case INPUT_MENU_TOGGLE_BUTTONS:
				this->labels[index]->setText(this->setTextOptionBool(real_index, input_data->enable_buttons_input));
				break;
			case INPUT_MENU_TOGGLE_JOYSTICK:
				this->labels[index]->setText(this->setTextOptionBool(real_index, input_data->enable_controller_input));
				break;
			case INPUT_MENU_TOGGLE_KEYBOARD:
				this->labels[index]->setText(this->setTextOptionBool(real_index, input_data->enable_keyboard_input));
				break;
			case INPUT_MENU_TOGGLE_MOUSE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, input_data->enable_mouse_input));
				break;
			case INPUT_MENU_TOGGLE_FAST_POLL:
				this->labels[index]->setText(this->setTextOptionBool(real_index, input_data->fast_poll));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

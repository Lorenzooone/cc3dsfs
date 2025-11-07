#include "OptimizeSerialKeyAddMenu.hpp"
#include "utils.hpp"
#include "cypress_optimize_3ds_acquisition.hpp"
#include "optimize_serial_key_add_table.h"
#include "optimize_serial_key_next_char_table.h"
#include "optimize_serial_key_prev_char_table.h"
#include <cmath>

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct OptimizeSerialKeyAddMenuOptionInfo {
	const std::string base_name;
	const bool is_selectable;
	const bool is_font_mono;
	const bool is_color_transparent;
	const int position_x, position_y, multiplier_y;
	float text_factor_multiplier;
	TextKind text_kind;
	const int divisor_x, size_y, size_x;
	TextPosKind pos_kind;
	OptimizeSerialKeyMenuOutAction out_action;
};

static const OptimizeSerialKeyAddMenuOptionInfo desc_textbox_option = {
.base_name = "Select the textbox, then type the key", .is_selectable = false,
.is_font_mono = false,
.position_x = 0, .position_y = 0, .multiplier_y = 2,
.text_factor_multiplier = 0.8f, .text_kind = TEXT_KIND_NORMAL,
.divisor_x = 1, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_NO_ACTION};

static const OptimizeSerialKeyAddMenuOptionInfo desc2_textbox_option = {
.base_name = "via keyboard or use the key buttons.", .is_selectable = false,
.is_font_mono = false,
.position_x = 0, .position_y = 1, .multiplier_y = 2,
.text_factor_multiplier = 0.8f, .text_kind = TEXT_KIND_NORMAL,
.divisor_x = 1, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_NO_ACTION};

static const OptimizeSerialKeyAddMenuOptionInfo highlight_textbox_option = {
.base_name = "", .is_selectable = true,
.is_font_mono = true,
.position_x = 0, .position_y = 1, .multiplier_y = 1,
.text_factor_multiplier = 0.0f, .text_kind = TEXT_KIND_NORMAL,
.divisor_x = 1, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_SELECT_TEXTBOX};

static const OptimizeSerialKeyAddMenuOptionInfo textbox_option = {
.base_name = "\n____ ____ ____ ____ ____", .is_selectable = false,
.is_font_mono = true,
.position_x = 0, .position_y = 1, .multiplier_y = 1,
.text_factor_multiplier = 0.80f, .text_kind = TEXT_KIND_TEXTBOX,
.divisor_x = 1, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_PRINT};

static const OptimizeSerialKeyAddMenuOptionInfo textbox_button_above_option = {
.base_name = "↑", .is_selectable = false,
.is_font_mono = true,
.position_x = 0, .position_y = 0, .multiplier_y = 1,
.text_factor_multiplier = 0.80f, .text_kind = TEXT_KIND_TEXTBOX,
.divisor_x = 1, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_PRIORITIZE_BOTTOM_LEFT,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_ABOVE_PRINT};

static const OptimizeSerialKeyAddMenuOptionInfo textbox_button_below_option = {
.base_name = "↓", .is_selectable = false,
.is_font_mono = true,
.position_x = 1, .position_y = 0, .multiplier_y = 1,
.text_factor_multiplier = 0.80f, .text_kind = TEXT_KIND_TEXTBOX,
.divisor_x = 2, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_PRIORITIZE_TOP_LEFT,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_BELOW_PRINT};

static const OptimizeSerialKeyAddMenuOptionInfo left_option = {
.base_name = "<", .is_selectable = true,
.is_font_mono = false,
.position_x = 0, .position_y = 2, .multiplier_y = 1,
.text_factor_multiplier = 1.0f, .text_kind = TEXT_KIND_NORMAL,
.divisor_x = 6, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_DEC};

static const OptimizeSerialKeyAddMenuOptionInfo right_option = {
.base_name = ">", .is_selectable = true,
.is_font_mono = false,
.position_x = 5, .position_y = 2, .multiplier_y = 1,
.text_factor_multiplier = 1.0f, .text_kind = TEXT_KIND_NORMAL,
.divisor_x = 6, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_INC};

static const OptimizeSerialKeyAddMenuOptionInfo target_file_option = {
.base_name = "Key for", .is_selectable = false,
.is_font_mono = false,
.position_x = 1, .position_y = 2, .multiplier_y = 1,
.text_factor_multiplier = 1.0f, .text_kind = TEXT_KIND_NORMAL,
.divisor_x = 6, .size_y = 1, .size_x = 4,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_INFO};

static const OptimizeSerialKeyAddMenuOptionInfo add_key_option = {
.base_name = "Add key to list", .is_selectable = true,
.is_font_mono = false,
.position_x = 0, .position_y = 3, .multiplier_y = 1,
.text_factor_multiplier = 1.0f, .text_kind = TEXT_KIND_NORMAL,
.divisor_x = 1, .size_y = 1, .size_x = 1,
.pos_kind = POS_KIND_NORMAL,
.out_action = OPTIMIZE_SERIAL_KEY_ADD_MENU_CONFIRM};

static const OptimizeSerialKeyAddMenuOptionInfo* pollable_options[] = {
&desc_textbox_option,
&desc2_textbox_option,
&highlight_textbox_option,
&target_file_option,
&left_option,
&right_option,
&textbox_option,
&textbox_button_above_option,
&textbox_button_below_option,
&add_key_option,
};

OptimizeSerialKeyAddMenu::OptimizeSerialKeyAddMenu() {
}

OptimizeSerialKeyAddMenu::OptimizeSerialKeyAddMenu(TextRectanglePool* text_rectangle_pool) {
	this->initialize(text_rectangle_pool);
}

OptimizeSerialKeyAddMenu::~OptimizeSerialKeyAddMenu() {
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++)
		delete this->labels[i];
	delete []this->labels;
	delete []this->selectable_labels;
}

void OptimizeSerialKeyAddMenu::on_menu_unloaded() {
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		this->labels[i]->changeFont(FONT_KIND_NORMAL);
		this->labels[i]->setTightAndCentered(false);
		this->labels[i]->setLineSpacing(1.0);
		this->labels[i]->setCharacterSpacing(1.0);
		this->labels[i]->setLocked(false);
	}
	sf::Keyboard::setVirtualKeyboardVisible(false);
}

void OptimizeSerialKeyAddMenu::initialize(TextRectanglePool* text_pool) {
	this->class_setup();
	this->after_class_setup_connected_values();
	this->menu_rectangle.setFillColor(this->menu_color);
	this->menu_rectangle.setPosition({1, 1});
	text_pool->request_num_text_rectangles(this->num_elements_displayed_per_screen);
	this->labels = new TextRectangle*[this->num_elements_displayed_per_screen];
	this->selectable_labels = new bool[this->num_elements_displayed_per_screen];
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		this->labels[i] = text_pool->get_text_rectangle(i);
		this->labels[i]->setProportionalBox(false);
		this->labels[i]->setText(std::to_string(i));
		this->labels[i]->setShowText(true);
		this->selectable_labels[i] = true;
	}
	this->future_data.menu_width = 1;
	this->future_data.menu_height = 1;
	this->future_data.pos_x = 0;
	this->future_data.pos_y = 0;
	this->last_action_time = std::chrono::high_resolution_clock::now();
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	this->selectable_labels[this->back_x_id] = true;
	this->selectable_labels[this->title_id] = false;
}

void OptimizeSerialKeyAddMenu::class_setup() {
	this->num_options_per_screen = NUM_TOTAL_MENU_OPTIONS;
	this->min_elements_text_scaling_factor = 4;
	this->num_vertical_slices = 5;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "New Serial Key";
}

void OptimizeSerialKeyAddMenu::after_class_setup_connected_values() {
	this->num_title_back_x_elements = 2;
	this->num_elements_per_screen = this->num_options_per_screen;
	this->num_elements_displayed_per_screen = num_title_back_x_elements + num_elements_per_screen;
	this->title_back_x_start_id = 0;
	this->elements_start_id = num_title_back_x_elements + title_back_x_start_id;
	this->back_x_id = title_back_x_start_id + 1;
	this->title_id = title_back_x_start_id;
}

bool OptimizeSerialKeyAddMenu::can_execute_action() {
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->last_action_time;
	if(diff.count() > this->action_timeout)
		return true;
	return false;
}

void OptimizeSerialKeyAddMenu::insert_data(CaptureDevice* device) {
	#ifdef USE_CYPRESS_OPTIMIZE
	key_for_new = is_device_optimize_n3ds(device);
	#endif
	this->menu_time = std::chrono::high_resolution_clock::now();
	this->is_inside_textbox = false;
	this->first_pass = true;
	this->prepare_options();
}

void OptimizeSerialKeyAddMenu::reset_data(bool full_reset) {
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	if(full_reset) {
		this->reset_output_option();
		this->future_data.option_selected = -1;
	}
}

bool OptimizeSerialKeyAddMenu::is_option_element(int option) {
	return (option >= this->elements_start_id) && (option < (this->elements_start_id + this->num_elements_per_screen));
}

void OptimizeSerialKeyAddMenu::set_default_cursor_position() {
	this->future_data.option_selected = this->elements_start_id;
	while(!this->selectable_labels[this->future_data.option_selected]){
		this->future_data.option_selected += 1;
		if(this->future_data.option_selected >= this->num_elements_displayed_per_screen)
			this->future_data.option_selected = 0;
		if(this->future_data.option_selected == this->elements_start_id)
			break;
	}
}

// This is reversed, due to it being the position of the bottom screen.
// But we ask the user the position of the top screen.
bool OptimizeSerialKeyAddMenu::is_option_left(int index) {
	return pollable_options[index]->out_action == OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_DEC;
}

bool OptimizeSerialKeyAddMenu::is_option_right(int index) {
	return pollable_options[index]->out_action == OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_INC;
}

void OptimizeSerialKeyAddMenu::decrement_selected_option(bool is_simple) {
	if(this->future_data.option_selected < 0) {
		this->set_default_cursor_position();
		return;
	}
	int elem_index = this->future_data.option_selected - this->elements_start_id;
	if(!is_simple) {
		if(this->is_option_element(this->future_data.option_selected) && this->is_option_right(elem_index))
			this->future_data.option_selected -= 1;
	}
	do {
		this->future_data.option_selected -= 1;
		if(this->future_data.option_selected < 0)
			this->future_data.option_selected = this->num_elements_displayed_per_screen - 1;
	} while(!this->selectable_labels[this->future_data.option_selected]);
}

void OptimizeSerialKeyAddMenu::increment_selected_option(bool is_simple) {
	if(this->future_data.option_selected < 0){
		this->set_default_cursor_position();
		return;
	}
	int elem_index = this->future_data.option_selected - this->elements_start_id;
	if(!is_simple) {
		if(this->is_option_element(this->future_data.option_selected) && this->is_option_left(elem_index))
			this->future_data.option_selected += 1;
	}
	do {
		this->future_data.option_selected += 1;
		if(this->future_data.option_selected >= this->num_elements_displayed_per_screen)
			this->future_data.option_selected = 0;
	} while(!this->selectable_labels[this->future_data.option_selected]);
}

void OptimizeSerialKeyAddMenu::reset_output_option() {
	this->selected_index = OPTIMIZE_SERIAL_KEY_ADD_MENU_NO_ACTION;
}

void OptimizeSerialKeyAddMenu::set_output_option(int index) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = OPTIMIZE_SERIAL_KEY_ADD_MENU_BACK;
	else {
		if(pollable_options[index]->out_action == OPTIMIZE_SERIAL_KEY_ADD_MENU_SELECT_TEXTBOX) {
			this->is_inside_textbox = !this->is_inside_textbox;
			this->pos_key = 0;
			if(this->is_inside_textbox)
				sf::Keyboard::setVirtualKeyboardVisible(true);
			else
				sf::Keyboard::setVirtualKeyboardVisible(false);
		}
		else
			this->selected_index = pollable_options[index]->out_action;
	}
}

bool OptimizeSerialKeyAddMenu::is_option_drawable(int index) {
	return true;
}

bool OptimizeSerialKeyAddMenu::is_option_selectable(int index) {
	return pollable_options[index]->is_selectable;
}

std::string OptimizeSerialKeyAddMenu::get_string_option(int index) {
	return pollable_options[index]->base_name;
}

bool OptimizeSerialKeyAddMenu::is_selected_insert_text_option(int index) {
	if(index < 0)
		return false;
	if(index >= ((int)NUM_TOTAL_MENU_OPTIONS))
		return false;
	return pollable_options[index]->out_action == OPTIMIZE_SERIAL_KEY_ADD_MENU_SELECT_TEXTBOX;
}

static int get_option_index_of_action(OptimizeSerialKeyMenuOutAction action) {
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++)
		if(pollable_options[i]->out_action == action)
			return (int)i;
	return NUM_TOTAL_MENU_OPTIONS - 1;
}

int OptimizeSerialKeyAddMenu::get_next_option_selected_after_key_input() {
	return get_option_index_of_action(OPTIMIZE_SERIAL_KEY_ADD_MENU_CONFIRM);
}

int OptimizeSerialKeyAddMenu::get_option_selected_key_input() {
	return get_option_index_of_action(OPTIMIZE_SERIAL_KEY_ADD_MENU_SELECT_TEXTBOX);
}

int OptimizeSerialKeyAddMenu::get_option_print_key_input() {
	return get_option_index_of_action(OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_PRINT);
}

bool OptimizeSerialKeyAddMenu::handle_click(int index, percentage_pos_text_t percentage, bool started_inside_textbox) {
	if(index < 0)
		return false;
	if(index >= ((int)NUM_TOTAL_MENU_OPTIONS))
		return false;
	bool retval = false;
	int pos_click = 0;
	switch(pollable_options[index]->out_action) {
		case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_PRINT:
			pos_click = (percentage.x * SERIAL_KEY_OPTIMIZE_WITH_DASHES_SIZE) / 100;
			if((pos_click % (SERIAL_KEY_OPTIMIZE_DASHES_REPEATED_POS + 1)) == SERIAL_KEY_OPTIMIZE_DASHES_REPEATED_POS)
				pos_click += 1;
			this->is_inside_textbox = true;
			this->pos_key = pos_click - (pos_click / (SERIAL_KEY_OPTIMIZE_DASHES_REPEATED_POS + 1));
			retval = true;
			break;
		case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_ABOVE_PRINT:
			if(!started_inside_textbox)
				break;
			this->is_inside_textbox = true;
			this->key_update_char(false);
			retval = true;
			break;
		case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_BELOW_PRINT:
			if(!started_inside_textbox)
				break;
			this->is_inside_textbox = true;
			this->key_update_char(true);
			retval = true;
			break;
		default:
			break;
	}
	if((!started_inside_textbox) && this->is_inside_textbox)
		sf::Keyboard::setVirtualKeyboardVisible(true);
	return retval;
}

void OptimizeSerialKeyAddMenu::set_pos_special_label(int index) {
	if(index < 0)
		return;
	if(index >= ((int)NUM_TOTAL_MENU_OPTIONS))
		return;
	TextRectangle* target = this->get_label_for_option(index);
	if(target == NULL)
		return;
	TextRectangle* textbox = NULL;
	size_text_t textbox_size;
	position_text_t textbox_pos;
	int pos_in_key = this->get_pos_in_serial_key();
	int x_pos_key = 0;
	int y_pos_contrib = 0;
	switch(pollable_options[index]->out_action) {
		case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_ABOVE_PRINT:
		case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_BELOW_PRINT:
			if(!this->is_inside_textbox)
				break;
			textbox = this->get_label_for_option(this->get_option_print_key_input());
			if(textbox == NULL)
				break;
			textbox_size = textbox->getFinalSizeNoMultipliers();
			textbox_pos = textbox->getFinalPositionNoMultipliers();
			// For some reason, the 1.01 is needed to make this look right...
			// Maybe because part of the space between 2 letters comes from the "next" letter?
			x_pos_key = (int)std::round(((float)textbox_size.x * pos_in_key * 1.01) / SERIAL_KEY_OPTIMIZE_WITH_DASHES_SIZE);
			if(pollable_options[index]->out_action == OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_BELOW_PRINT)
				y_pos_contrib = textbox_size.y;
			target->setPosition(textbox_pos.x + x_pos_key, textbox_pos.y + y_pos_contrib, pollable_options[index]->pos_kind);
			break;
		default:
			break;
	}
}

void OptimizeSerialKeyAddMenu::prepare_options() {
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		this->labels[i]->setShowText(false);
	}
	this->labels[this->back_x_id]->setText("<--");
	this->labels[this->back_x_id]->setShowText(true);
	this->labels[this->title_id]->setText(this->title);
	this->labels[this->title_id]->setShowText(true);
	for(int i = 0; i < this->num_options_per_screen; i++) {
		int label_start_index = i + this->elements_start_id;
		this->labels[label_start_index]->setText(this->get_string_option(i));
		this->selectable_labels[label_start_index] = this->is_option_selectable(i);
		this->labels[label_start_index]->setShowText(true);
	}
	if(!this->selectable_labels[this->future_data.option_selected]) {
		this->future_data.option_selected = -1;
	}
}

void OptimizeSerialKeyAddMenu::option_selection_handling() {
	if((this->future_data.option_selected == -1) || (!this->selectable_labels[this->future_data.option_selected])) {
		this->future_data.option_selected = -1;
		return;
	}
	if(!this->can_execute_action())
		return;
	if(this->future_data.option_selected == this->back_x_id) {
		this->set_output_option(BACK_X_OUTPUT_OPTION);
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
	else if(this->is_option_element(this->future_data.option_selected)) {
		int elem_index = this->future_data.option_selected - this->elements_start_id;
		this->set_output_option(elem_index);
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
}

void OptimizeSerialKeyAddMenu::up_code(bool is_simple) {
	if(!this->can_execute_action())
		return;
	if(this->is_inside_textbox) {
		this->key_update_char(false);
		return;
	}
	this->decrement_selected_option(is_simple);
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void OptimizeSerialKeyAddMenu::down_code(bool is_simple) {
	if(!this->can_execute_action())
		return;
	if(this->is_inside_textbox) {
		this->key_update_char(true);
		return;
	}
	this->increment_selected_option(is_simple);
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void OptimizeSerialKeyAddMenu::left_code() {
	if(!this->can_execute_action())
		return;
	if(this->is_inside_textbox) {
		this->pos_key -= 1;
		if(this->pos_key < 0)
			this->pos_key = 0;
		return;
	}
	int elem_index = this->future_data.option_selected - this->elements_start_id;
	if(this->is_option_element(this->future_data.option_selected) && this->is_option_left(elem_index))
		this->option_selection_handling();
	else {
		this->future_data.option_selected -= 1;
		for(int i = 0; i < this->num_options_per_screen; i++) {
			if(this->is_option_left(i)) {
				this->future_data.option_selected = i + this->elements_start_id;
				break;
			}
		}
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
}

void OptimizeSerialKeyAddMenu::right_code() {
	if(!this->can_execute_action())
		return;
	if(this->is_inside_textbox) {
		this->pos_key += 1;
		if(this->pos_key >= this->get_key_size())
			this->pos_key = this->get_key_size() - 1;
		return;
	}
	int elem_index = this->future_data.option_selected - this->elements_start_id;
	if(this->is_option_element(this->future_data.option_selected) && this->is_option_right(elem_index))
		this->option_selection_handling();
	else {
		this->future_data.option_selected -= 1;
		for(int i = 0; i < this->num_options_per_screen; i++) {
			if(this->is_option_right(i)) {
				this->future_data.option_selected = i + this->elements_start_id;
				break;
			}
		}
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
}

void OptimizeSerialKeyAddMenu::quit_textbox(bool change_option_selected) {
	this->is_inside_textbox = false;
	if(change_option_selected)
		this->future_data.option_selected = this->get_next_option_selected_after_key_input() + this->elements_start_id;
}

int OptimizeSerialKeyAddMenu::get_pos_in_serial_key() {
	return this->pos_key + (this->pos_key / SERIAL_KEY_OPTIMIZE_DASHES_REPEATED_POS);
}

bool OptimizeSerialKeyAddMenu::add_to_key(uint32_t unicode) {
	int pos_in_serial_key = this->get_pos_in_serial_key();
	if((unicode >= optimize_serial_key_add_table_len) || (optimize_serial_key_add_table[unicode] == 0xFF))
		return false;
	this->key[pos_in_serial_key] = (char)optimize_serial_key_add_table[unicode];
	return true;
}

int OptimizeSerialKeyAddMenu::get_key_size() {
	return SERIAL_KEY_OPTIMIZE_NO_DASHES_SIZE;
}

char OptimizeSerialKeyAddMenu::get_key_update_char(bool increase) {
	int pos_in_serial_key = this->get_pos_in_serial_key();
	uint8_t* char_table = optimize_serial_key_prev_char_table;
	if(increase)
		char_table = optimize_serial_key_next_char_table;
	char base_character = this->key[pos_in_serial_key];
	char target_character = '0';
	if(char_table[(uint8_t)base_character] != 0xFF)
		target_character = (char)char_table[(uint8_t)base_character];
	return target_character;
}

void OptimizeSerialKeyAddMenu::key_update_char(bool increase) {
	int pos_in_serial_key = this->get_pos_in_serial_key();
	this->key[pos_in_serial_key] = this->get_key_update_char(increase);
}

bool OptimizeSerialKeyAddMenu::poll(SFEvent &event_data) {
	bool consumed = true;
	bool started_inside_textbox = this->is_inside_textbox;
	bool is_paste_command = false;
	sf::String pasted_string = "";
	switch (event_data.type) {
	case EVENT_TEXT_ENTERED:
		if(!started_inside_textbox) {
			consumed = false;
			break;
		}
		if(!this->add_to_key(event_data.unicode)) {
			consumed = false;
			break;
		}
		this->pos_key += 1;
		if(this->pos_key >= this->get_key_size())
			this->quit_textbox();
		break;
	case EVENT_KEY_PRESSED:
		switch (event_data.code) {
			case sf::Keyboard::Key::Up:
				this->up_code(false);
				break;
			case sf::Keyboard::Key::Down:
				this->down_code(false);
				break;
			case sf::Keyboard::Key::PageUp:
				this->up_code(true);
				break;
			case sf::Keyboard::Key::PageDown:
				this->down_code(true);
				break;
			case sf::Keyboard::Key::Left:
				this->left_code();
				break;
			case sf::Keyboard::Key::Right:
				this->right_code();
				break;
			case sf::Keyboard::Key::Backspace:
				if(started_inside_textbox)
					this->left_code();
				else
					consumed = false;
				break;
			case sf::Keyboard::Key::V:
				if(event_data.control)
					is_paste_command = true;
				#ifdef __APPLE__
				if(event_data.system)
					is_paste_command = true;
				#endif
				if(!is_paste_command) {
					consumed = false;
					break;
				}
				pasted_string = sf::Clipboard::getString();
				for(size_t i = 0; i < pasted_string.getSize(); i++) {
					if(!this->add_to_key(pasted_string[i]))
						continue;
					this->pos_key += 1;
					if(this->pos_key >= this->get_key_size()) {
						this->quit_textbox();
						break;
					}
				}
				break;
			case sf::Keyboard::Key::Tab:
				if(started_inside_textbox || this->is_selected_insert_text_option(this->future_data.option_selected - this->elements_start_id))
					this->quit_textbox();
				else
					this->future_data.option_selected = this->get_option_selected_key_input() + this->elements_start_id;
				break;
			case sf::Keyboard::Key::Enter:
				if(started_inside_textbox) {
					if(event_data.is_extra) {
						this->pos_key += 1;
						if(this->pos_key >= this->get_key_size())
							this->quit_textbox();
					}
					else
						this->quit_textbox();
					break;
				}
				this->option_selection_handling();
				break;
			default:
				consumed = false;
				break;
		}

		break;
	case EVENT_MOUSE_MOVED:
		this->future_data.option_selected = -1;
		for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
			if(!this->selectable_labels[i])
				continue;
			if(this->labels[i]->isCoordInRectangle(event_data.mouse_x, event_data.mouse_y)) {
				this->future_data.option_selected = i;
				this->last_input_processed_time = std::chrono::high_resolution_clock::now();
				break;
			}
		}
		break;
	case EVENT_MOUSE_BTN_PRESSED:
		if(started_inside_textbox) {
			this->quit_textbox(false);
		}
		this->future_data.option_selected = -1;

		consumed = false;
		for(int i = 0; i < this->num_elements_displayed_per_screen; i++)
			if(this->labels[i]->isCoordInRectangle(event_data.mouse_x, event_data.mouse_y))
				if(this->handle_click(i - this->elements_start_id, this->labels[i]->getCoordInRectanglePercentage(event_data.mouse_x, event_data.mouse_y), started_inside_textbox)) {
					consumed = true;
					break;
				}
		if(consumed)
			break;
		consumed = true;

		for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
			if(!this->selectable_labels[i])
				continue;
			if(this->labels[i]->isCoordInRectangle(event_data.mouse_x, event_data.mouse_y)) {
				this->future_data.option_selected = i;
				break;
			}
		}
		if(started_inside_textbox && (!this->is_inside_textbox))
			sf::Keyboard::setVirtualKeyboardVisible(false);
		if(this->future_data.option_selected == -1)
			break;
		if(event_data.mouse_button == sf::Mouse::Button::Left)
			this->option_selection_handling();
		break;
	case EVENT_JOYSTICK_BTN_PRESSED:
		switch(get_joystick_action(event_data.joystickId, event_data.joy_button)) {
			case JOY_ACTION_CONFIRM:
				if(started_inside_textbox) {
					this->pos_key += 1;
					if(this->pos_key >= this->get_key_size())
						this->quit_textbox();
					break;
				}
				this->option_selection_handling();
				break;
			case JOY_ACTION_NEGATE:
				if(started_inside_textbox) {
					this->pos_key += 1;
					if(this->pos_key >= this->get_key_size())
						this->quit_textbox();
					break;
				}
				this->option_selection_handling();
				break;
			case JOY_ACTION_MENU:
				if(started_inside_textbox) {
					this->quit_textbox();
					break;
				}
				consumed = false;
				break;
			default:
				consumed = false;
				break;
		}
		break;
	case EVENT_JOYSTICK_MOVED:
		switch(get_joystick_direction(event_data.joystickId, event_data.axis, event_data.position)) {
			case JOY_DIR_UP:
				this->up_code(false);
				break;
			case JOY_DIR_DOWN:
				this->down_code(false);
				break;
			case JOY_DIR_LEFT:
				this->left_code();
				break;
			case JOY_DIR_RIGHT:
				this->right_code();
				break;
			default:
				consumed = false;
				break;
		}
		break;
	default:
		consumed = false;
		break;
	}
	return consumed;
}

void OptimizeSerialKeyAddMenu::draw(float scaling_factor, sf::RenderTarget &window) {
	const sf::Vector2f rect_size = this->menu_rectangle.getSize();
	if((this->loaded_data.menu_width != rect_size.x) || (this->loaded_data.menu_height != rect_size.y)) {
		this->menu_rectangle.setSize(sf::Vector2f((float)this->loaded_data.menu_width, (float)this->loaded_data.menu_height));
	}
	this->menu_rectangle.setPosition({(float)this->loaded_data.pos_x, (float)this->loaded_data.pos_y});
	window.draw(this->menu_rectangle);
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		if(this->is_option_drawable(i))
			this->labels[i]->draw(window);
	}
}

void OptimizeSerialKeyAddMenu::prepare_text_slices(int x_multiplier, int x_divisor, int y_multiplier, int y_divisor, int size_x, int size_y, int index, float text_scaling_factor, TextKind text_kind, TextPosKind pos_kind, bool center) {
	this->labels[index]->setTextFactor(text_scaling_factor);
	int x_base_pos = (this->future_data.menu_width * x_multiplier) / x_divisor;
	int y_base_pos = (this->future_data.menu_height * y_multiplier) / y_divisor;
	int x_size = (((this->future_data.menu_width * (x_multiplier + 1)) / x_divisor) - x_base_pos) * size_x;
	int y_size = (((this->future_data.menu_height * (y_multiplier + 1)) / y_divisor) - y_base_pos) * size_y;
	if(center)
		x_size = ((this->future_data.menu_width * (x_divisor - x_multiplier)) / x_divisor) - x_base_pos;
	this->labels[index]->setSize(x_size, y_size);
	if(pos_kind == POS_KIND_NORMAL)
		this->labels[index]->setPosition(this->future_data.pos_x + x_base_pos, this->future_data.pos_y + y_base_pos);
	else
		this->set_pos_special_label(index - this->elements_start_id);
	if(this->is_inside_textbox && this->is_selected_insert_text_option(index - this->elements_start_id))
		this->labels[index]->setRectangleKind(TEXT_KIND_SELECTED);
	else {
		if(index == this->future_data.option_selected)
			this->labels[index]->setRectangleKind(TEXT_KIND_SELECTED);
		else
			this->labels[index]->setRectangleKind(text_kind);
	}
	this->labels[index]->prepareRenderText();
}

std::string OptimizeSerialKeyAddMenu::generate_key_print_string() {
	std::string out_above_string = this->key;
	std::string out_below_string = "____ ____ ____ ____ ____";
	if(this->is_inside_textbox) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->menu_time;
		if(((int)std::round(diff.count() / cursor_blank_timeout)) & 1) {
			out_above_string[this->get_pos_in_serial_key()] = ' ';
			//out_below_string[this->get_pos_in_serial_key()] = ' ';
		}
	}
	return out_above_string + "\n" + out_below_string;
}

TextRectangle* OptimizeSerialKeyAddMenu::get_label_for_option(int option_num) {
	if(option_num < 0)
		return NULL;
	if(option_num >= this->num_options_per_screen)
		return NULL;
	return this->labels[option_num + this->elements_start_id];
}

void OptimizeSerialKeyAddMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	for(int i = 0; i < this->num_options_per_screen; i++) {
		int index = i + this->elements_start_id;
		FontKind font_kind = FONT_KIND_NORMAL;
		if(pollable_options[i]->is_font_mono)
			font_kind = FONT_KIND_MONO;
		this->labels[index]->changeFont(font_kind);
		if((pollable_options[i]->out_action == OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_PRINT) && (!this->first_pass)) {
			this->labels[index]->setLocked(true);
		}
		else
			this->labels[index]->setLocked(false);
		switch(pollable_options[i]->out_action) {
			case OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_INFO:
				if(this->key_for_new)
					this->labels[index]->setText(this->get_string_option(i) + ": New 3DS");
				else
					this->labels[index]->setText(this->get_string_option(i) + ": Old 3DS");
				break;
			case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_ABOVE_PRINT:
				this->labels[index]->setTightAndCentered(true);
				this->labels[index]->setLineSpacing(0.5f);
				if(this->is_inside_textbox) {
					if(!this->labels[index]->getShowText()) {
						this->labels[index]->setLocked(false);
						this->labels[index]->setShowText(true);
					}
					else
						this->labels[index]->setLocked(true);
					this->labels[index]->setText(std::string(1, this->get_key_update_char(false)) + "\n" + this->get_string_option(i));
				}
				else
					this->labels[index]->setShowText(false);
				break;
			case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_BELOW_PRINT:
				this->labels[index]->setTightAndCentered(true);
				this->labels[index]->setLineSpacing(0.5f);
				if(this->is_inside_textbox) {
					if(!this->labels[index]->getShowText()) {
						this->labels[index]->setLocked(false);
						this->labels[index]->setShowText(true);
					}
					else
						this->labels[index]->setLocked(true);
					this->labels[index]->setText(this->get_string_option(i) + "\n" + std::string(1, this->get_key_update_char(true)));
				}
				else
					this->labels[index]->setShowText(false);
				break;
			case OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_PRINT:
				this->labels[index]->setTightAndCentered(true);
				this->labels[index]->setLineSpacing(0.0f);
				this->labels[index]->setCharacterSpacing(1.4f);
				this->labels[index]->setText(this->generate_key_print_string());
				break;
			default:
				break;
		}
	}

	this->first_pass = false;
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

void OptimizeSerialKeyAddMenu::option_slice_prepare(int i, int index, int num_vertical_slices, float text_scaling_factor) {
	this->prepare_text_slices(pollable_options[i]->position_x, pollable_options[i]->divisor_x, pollable_options[i]->position_y + (1 * pollable_options[i]->multiplier_y), num_vertical_slices * pollable_options[i]->multiplier_y, pollable_options[i]->size_x, pollable_options[i]->size_y, index, text_scaling_factor * pollable_options[i]->text_factor_multiplier, pollable_options[i]->text_kind, pollable_options[i]->pos_kind);
}

std::string OptimizeSerialKeyAddMenu::get_key() {
	return this->key;
}

void OptimizeSerialKeyAddMenu::base_prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	const float base_height = (7 * BASE_PIXEL_FONT_HEIGHT * 11) / 9;
	int max_width = (view_size_x * 9) / 10;
	int max_height = (view_size_y * 9) / 10;
	float max_width_corresponding_height = (max_width * this->width_divisor_menu * this->max_width_slack) / this->width_factor_menu;
	float max_scaling_factor = max_height / base_height;
	if(max_width_corresponding_height < max_height)
		max_scaling_factor = max_width_corresponding_height / base_height;
	float final_menu_scaling_factor = (menu_scaling_factor * this->base_height_factor_menu) / this->base_height_divisor_menu;
	if(menu_scaling_factor > max_scaling_factor)
		menu_scaling_factor = max_scaling_factor;
	if(final_menu_scaling_factor > max_scaling_factor)
		final_menu_scaling_factor = max_scaling_factor;
	if(menu_scaling_factor < this->min_text_size)
		menu_scaling_factor = this->min_text_size;
	if(final_menu_scaling_factor < this->min_text_size)
		final_menu_scaling_factor = this->min_text_size;
	int num_elements_text_scaling_factor = this->num_vertical_slices;
	if(num_elements_text_scaling_factor < this->min_elements_text_scaling_factor)
		num_elements_text_scaling_factor = this->min_elements_text_scaling_factor;
	float text_scaling_factor = (menu_scaling_factor * this->num_vertical_slices) / num_elements_text_scaling_factor;
	this->future_data.menu_height = (int)(base_height * final_menu_scaling_factor);
	this->future_data.menu_width = (this->future_data.menu_height * this->width_factor_menu) / this->width_divisor_menu;
	if(this->future_data.menu_width > max_width)
		this->future_data.menu_width = max_width;
	this->future_data.pos_x = (view_size_x - this->future_data.menu_width) / 2;
	this->future_data.pos_y = (view_size_y - this->future_data.menu_height) / 2;
	this->prepare_text_slices(0, 6, 0, this->num_vertical_slices, 1, 1, this->back_x_id, text_scaling_factor, TEXT_KIND_NORMAL);
	this->prepare_text_slices(0, 1, 0, this->num_vertical_slices, 1, 1, this->title_id, text_scaling_factor, TEXT_KIND_TITLE);
	for(int i = 0; i < this->num_options_per_screen; i++) {
		int index = i + this->elements_start_id;
		this->option_slice_prepare(i, index, this->num_vertical_slices, text_scaling_factor);
	}
	this->loaded_data = this->future_data;
}

#include "RelativePositionMenu.hpp"
#include "utils.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct RelativePositionMenuOptionInfo {
	const std::string base_name;
	const bool is_selectable;
	const int position_x, position_y, multiplier_y;
	float text_factor_multiplier;
	const BottomRelativePosition out_action;
	const int divisor_x;
};

static const RelativePositionMenuOptionInfo above_option = {
.base_name = "Above", .is_selectable = true,
.position_x = 1, .position_y = 0, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = UNDER_TOP,
.divisor_x = 3};

static const RelativePositionMenuOptionInfo left_option = {
.base_name = "Left", .is_selectable = true,
.position_x = 0, .position_y = 1, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = RIGHT_TOP,
.divisor_x = 3};

static const RelativePositionMenuOptionInfo right_option = {
.base_name = "Right", .is_selectable = true,
.position_x = 2, .position_y = 1, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = LEFT_TOP,
.divisor_x = 3};

static const RelativePositionMenuOptionInfo below_option = {
.base_name = "Below", .is_selectable = true,
.position_x = 1, .position_y = 2, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = ABOVE_TOP,
.divisor_x = 3};

static const RelativePositionMenuOptionInfo desc_option = {
.base_name = "Top Screen", .is_selectable = false,
.position_x = 1, .position_y = 2, .multiplier_y = 2,
.text_factor_multiplier = 0.95f,
.out_action = BOT_REL_POS_END,
.divisor_x = 3};

static const RelativePositionMenuOptionInfo desc2_option = {
.base_name = "Position", .is_selectable = false,
.position_x = 1, .position_y = 3, .multiplier_y = 2,
.text_factor_multiplier = 0.95f,
.out_action = BOT_REL_POS_END,
.divisor_x = 3};

static const RelativePositionMenuOptionInfo* pollable_options[] = {
&above_option,
&left_option,
&right_option,
&below_option,
&desc_option,
&desc2_option,
};

RelativePositionMenu::RelativePositionMenu() {
}

RelativePositionMenu::RelativePositionMenu(TextRectanglePool* text_rectangle_pool) {
	this->initialize(text_rectangle_pool);
}

RelativePositionMenu::~RelativePositionMenu() {
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++)
		delete this->labels[i];
	delete []this->labels;
	delete []this->selectable_labels;
}

void RelativePositionMenu::initialize(TextRectanglePool* text_pool) {
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

void RelativePositionMenu::class_setup() {
	this->num_options_per_screen = NUM_TOTAL_MENU_OPTIONS;
	this->min_elements_text_scaling_factor = 4;
	this->num_vertical_slices = 4;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Screens Placement";
}

void RelativePositionMenu::after_class_setup_connected_values() {
	this->num_title_back_x_elements = 2;
	this->num_elements_per_screen = this->num_options_per_screen;
	this->num_elements_displayed_per_screen = num_title_back_x_elements + num_elements_per_screen;
	this->title_back_x_start_id = 0;
	this->elements_start_id = num_title_back_x_elements + title_back_x_start_id;
	this->back_x_id = title_back_x_start_id + 1;
	this->title_id = title_back_x_start_id;
}

bool RelativePositionMenu::can_execute_action() {
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->last_action_time;
	if(diff.count() > this->action_timeout)
		return true;
	return false;
}

void RelativePositionMenu::insert_data() {
	this->prepare_options();
}

void RelativePositionMenu::reset_data(bool full_reset) {
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	if(full_reset) {
		this->reset_output_option();
		this->future_data.option_selected = -1;
	}
}

bool RelativePositionMenu::is_option_element(int option) {
	return (option >= this->elements_start_id) && (option < (this->elements_start_id + this->num_elements_per_screen));
}

void RelativePositionMenu::set_default_cursor_position() {
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
bool RelativePositionMenu::is_option_left(int index) {
	return pollable_options[index]->out_action == RIGHT_TOP;
}

bool RelativePositionMenu::is_option_right(int index) {
	return pollable_options[index]->out_action == LEFT_TOP;
}

bool RelativePositionMenu::is_option_above(int index) {
	return pollable_options[index]->out_action == UNDER_TOP;
}

bool RelativePositionMenu::is_option_below(int index) {
	return pollable_options[index]->out_action == ABOVE_TOP;
}

void RelativePositionMenu::decrement_selected_option(bool is_simple) {
	if(this->future_data.option_selected < 0) {
		this->set_default_cursor_position();
		return;
	}
	int elem_index = this->future_data.option_selected - this->elements_start_id;
	if(!is_simple) {
		if(this->is_option_element(this->future_data.option_selected) && (this->is_option_right(elem_index) || this->is_option_below(elem_index)))
			this->future_data.option_selected -= 1;
	}
	do {
		this->future_data.option_selected -= 1;
		if(this->future_data.option_selected < 0)
			this->future_data.option_selected = this->num_elements_displayed_per_screen - 1;
	} while(!this->selectable_labels[this->future_data.option_selected]);
}

void RelativePositionMenu::increment_selected_option(bool is_simple) {
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

void RelativePositionMenu::reset_output_option() {
	this->selected_index = REL_POS_MENU_NO_ACTION;
}

void RelativePositionMenu::set_output_option(int index) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = REL_POS_MENU_BACK;
	else if(pollable_options[index]->out_action != BOT_REL_POS_END){
		this->selected_confirm_value = pollable_options[index]->out_action;
		this->selected_index = REL_POS_MENU_CONFIRM;
	}
}

bool RelativePositionMenu::is_option_drawable(int index) {
	return true;
}

bool RelativePositionMenu::is_option_selectable(int index) {
	return pollable_options[index]->is_selectable;
}

std::string RelativePositionMenu::get_string_option(int index) {
	return pollable_options[index]->base_name;
}

void RelativePositionMenu::prepare_options() {
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

void RelativePositionMenu::option_selection_handling() {
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

void RelativePositionMenu::up_code(bool is_simple) {
	if(!this->can_execute_action())
		return;
	this->decrement_selected_option(is_simple);
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void RelativePositionMenu::down_code(bool is_simple) {
	if(!this->can_execute_action())
		return;
	this->increment_selected_option(is_simple);
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void RelativePositionMenu::left_code() {
	if(!this->can_execute_action())
		return;
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

void RelativePositionMenu::right_code() {
	if(!this->can_execute_action())
		return;
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

bool RelativePositionMenu::poll(SFEvent &event_data) {
	bool consumed = true;
	switch (event_data.type) {
	case EVENT_TEXT_ENTERED:
		consumed = false;
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
		case sf::Keyboard::Key::Enter:
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
		this->future_data.option_selected = -1;
		for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
			if(!this->selectable_labels[i])
				continue;
			if(this->labels[i]->isCoordInRectangle(event_data.mouse_x, event_data.mouse_y)) {
				this->future_data.option_selected = i;
				break;
			}
		}
		if(this->future_data.option_selected == -1)
			break;
		if(event_data.mouse_button == sf::Mouse::Button::Left)
			this->option_selection_handling();
		break;
	case EVENT_JOYSTICK_BTN_PRESSED:
		switch(get_joystick_action(event_data.joystickId, event_data.joy_button)) {
			case JOY_ACTION_CONFIRM:
				this->option_selection_handling();
				break;
			case JOY_ACTION_NEGATE:
				this->option_selection_handling();
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

void RelativePositionMenu::draw(float scaling_factor, sf::RenderTarget &window) {
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

void RelativePositionMenu::prepare_text_slices(int x_multiplier, int x_divisor, int y_multiplier, int y_divisor, int index, float text_scaling_factor, bool center) {
	this->labels[index]->setTextFactor(text_scaling_factor);
	int x_base_pos = (this->future_data.menu_width * x_multiplier) / x_divisor;
	int y_base_pos = (this->future_data.menu_height * y_multiplier) / y_divisor;
	int x_size = ((this->future_data.menu_width * (x_multiplier + 1)) / x_divisor) - x_base_pos;
	int y_size = ((this->future_data.menu_height * (y_multiplier + 1)) / y_divisor) - y_base_pos;
	if(center)
		x_size = ((this->future_data.menu_width * (x_divisor - x_multiplier)) / x_divisor) - x_base_pos;
	this->labels[index]->setSize(x_size, y_size);
	this->labels[index]->setPosition(this->future_data.pos_x + x_base_pos, this->future_data.pos_y + y_base_pos);
	if(index == this->future_data.option_selected) 
		this->labels[index]->setRectangleKind(TEXT_KIND_SELECTED);
	else {
		if(!this->selectable_labels[index])
			this->labels[index]->setRectangleKind(TEXT_KIND_TITLE);
		else
			this->labels[index]->setRectangleKind(TEXT_KIND_NORMAL);
	}
	this->labels[index]->prepareRenderText();
}

void RelativePositionMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, BottomRelativePosition curr_bottom_pos) {
	for(int i = 0; i < this->num_options_per_screen; i++) {
		int index = i + this->elements_start_id;
		switch(pollable_options[i]->out_action) {
			case BOT_REL_POS_END:
				break;
			default:
				if(pollable_options[i]->out_action == curr_bottom_pos)
					this->labels[index]->setText("<" + this->get_string_option(i) + ">");
				else
					this->labels[index]->setText(this->get_string_option(i));
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

void RelativePositionMenu::option_slice_prepare(int i, int index, int num_vertical_slices, float text_scaling_factor) {
	this->prepare_text_slices(pollable_options[i]->position_x, pollable_options[i]->divisor_x, pollable_options[i]->position_y + (1 * pollable_options[i]->multiplier_y), num_vertical_slices * pollable_options[i]->multiplier_y, index, text_scaling_factor * pollable_options[i]->text_factor_multiplier);
}

void RelativePositionMenu::base_prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
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
	this->prepare_text_slices(0, 6, 0, this->num_vertical_slices, this->back_x_id, text_scaling_factor);
	this->prepare_text_slices(0, 1, 0, this->num_vertical_slices, this->title_id, text_scaling_factor);
	for(int i = 0; i < this->num_options_per_screen; i++) {
		int index = i + this->elements_start_id;
		this->option_slice_prepare(i, index, this->num_vertical_slices, text_scaling_factor);
	}
	this->loaded_data = this->future_data;
}

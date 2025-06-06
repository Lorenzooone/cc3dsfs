#include "OptionSelectionMenu.hpp"
#include "utils.hpp"

OptionSelectionMenu::OptionSelectionMenu() {
}

OptionSelectionMenu::~OptionSelectionMenu() {
	delete []this->labels;
	delete []this->selectable_labels;
	delete []this->future_enabled_labels;
	delete []this->loaded_enabled_labels;
}

void OptionSelectionMenu::initialize(TextRectanglePool* text_pool) {
	this->class_setup();
	this->after_class_setup_connected_values();
	this->menu_rectangle.setFillColor(this->menu_color);
	this->menu_rectangle.setPosition({1, 1});
	text_pool->request_num_text_rectangles(this->num_elements_displayed_per_screen);
	this->labels = new TextRectangle*[this->num_elements_displayed_per_screen];
	this->selectable_labels = new bool[this->num_elements_displayed_per_screen];
	this->future_enabled_labels = new bool[this->num_elements_displayed_per_screen];
	this->loaded_enabled_labels = new bool[this->num_elements_displayed_per_screen];
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		this->labels[i] = text_pool->get_text_rectangle(i);
		this->labels[i]->setProportionalBox(false);
		this->labels[i]->setText(std::to_string(i));
		this->labels[i]->setShowText(true);
		this->selectable_labels[i] = true;
		this->future_enabled_labels[i] = true;
	}
	this->future_data.menu_width = 1;
	this->future_data.menu_height = 1;
	this->future_data.pos_x = 0;
	this->future_data.pos_y = 0;
	this->last_action_time = std::chrono::high_resolution_clock::now();
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	this->selectable_labels[this->info_page_id] = false;
	this->selectable_labels[this->back_x_id] = this->show_back_x;
	this->selectable_labels[this->title_id] = false;
	this->future_enabled_labels[this->back_x_id] = this->show_back_x;
	this->future_enabled_labels[this->title_id] = this->show_title;
}

void OptionSelectionMenu::class_setup() {
	this->num_options_per_screen = 1;
	this->min_elements_text_scaling_factor = 3;
	this->width_factor_menu = 1;
	this->width_divisor_menu = 1;
	this->base_height_factor_menu = 1;
	this->base_height_divisor_menu = 1;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Sample Menu";
	this->show_back_x = false;
	this->show_x = false;
	this->show_title = false;
}

void OptionSelectionMenu::after_class_setup_connected_values() {
	this->num_title_back_x_elements = 2;
	this->num_page_elements = 3;
	this->single_option_multiplier = 3;
	this->num_elements_per_screen = single_option_multiplier * (this->num_options_per_screen + 1);
	this->num_elements_displayed_per_screen = num_title_back_x_elements + num_elements_per_screen + num_page_elements;
	this->num_vertical_slices = num_options_per_screen + 1;
	if(show_back_x || show_title)
		this->num_vertical_slices += 1;
	this->title_back_x_start_id = 0;
	this->elements_start_id = num_title_back_x_elements + title_back_x_start_id;
	this->page_elements_start_id = elements_start_id + num_elements_per_screen;
	this->back_x_id = title_back_x_start_id + 1;
	this->title_id = title_back_x_start_id;
	this->prev_page_id = page_elements_start_id;
	this->info_page_id = page_elements_start_id + 1;
	this->next_page_id = page_elements_start_id + 2;
}

bool OptionSelectionMenu::can_execute_action() {
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->last_action_time;
	if(diff.count() > this->action_timeout)
		return true;
	return false;
}

void OptionSelectionMenu::reset_data(bool full_reset) {
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	if(full_reset) {
		this->reset_output_option();
		this->future_data.option_selected = -1;
		this->future_data.page = 0;
	}
}

bool OptionSelectionMenu::is_option_location_left(int option) {
	bool location_left = true;

	if(option == this->next_page_id)
		location_left = false;
	else if((option >= this->elements_start_id) && (option <= (this->elements_start_id + this->num_elements_per_screen)) && (((option - this->elements_start_id) % this->single_option_multiplier) == 2))
		location_left = false;

	return location_left;
}

bool OptionSelectionMenu::is_option_element(int option) {
	return (option >= this->elements_start_id) && (option < (this->elements_start_id + this->num_elements_per_screen));
}

bool OptionSelectionMenu::is_option_location_multichoice(int option) {
	if((option == this->next_page_id) || (option == this->prev_page_id))
		return true;
	if(this->is_option_element(option) && (((option - this->elements_start_id) % this->single_option_multiplier) != 0))
		return true;

	return false;
}

void OptionSelectionMenu::set_default_cursor_position() {
	this->future_data.option_selected = this->elements_start_id;
	while(!(this->selectable_labels[this->future_data.option_selected] && this->future_enabled_labels[this->future_data.option_selected])){
		this->future_data.option_selected += 1;
		if(this->future_data.option_selected >= this->num_elements_displayed_per_screen)
			this->future_data.option_selected = 0;
		if(this->future_data.option_selected == this->elements_start_id)
			break;
	}
}

void OptionSelectionMenu::decrement_selected_option(bool is_simple) {
	if(this->future_data.option_selected < 0) {
		this->set_default_cursor_position();
		return;
	}
	bool location_left = this->is_option_location_left(this->future_data.option_selected);
	bool multichoice = this->is_option_location_multichoice(this->future_data.option_selected);
	bool was_element = this->is_option_element(this->future_data.option_selected);
	if(!is_simple) {
		if(this->future_data.option_selected == this->next_page_id)
			this->future_data.option_selected = this->prev_page_id;
		else {
			if(multichoice && location_left)
				this->future_data.option_selected -= this->single_option_multiplier - 1;
			else
				this->future_data.option_selected -= 1;
			if(was_element && !this->is_option_element(this->future_data.option_selected))
				this->future_data.option_selected = this->elements_start_id;
		}
	}
	do {
		this->future_data.option_selected -= 1;
		if(this->future_data.option_selected < 0)
			this->future_data.option_selected = this->num_elements_displayed_per_screen - 1;
	} while(!(this->selectable_labels[this->future_data.option_selected] && this->future_enabled_labels[this->future_data.option_selected]));
}

void OptionSelectionMenu::increment_selected_option(bool is_simple) {
	if(this->future_data.option_selected < 0){
		this->set_default_cursor_position();
		return;
	}
	bool location_left = this->is_option_location_left(this->future_data.option_selected);
	bool success = false;
	if(!(is_simple || location_left)) {
		do {
			this->future_data.option_selected += this->single_option_multiplier;
			if(this->future_data.option_selected >= this->num_elements_displayed_per_screen) {
				break;
			}
			for(int i = 0; i < this->single_option_multiplier; i++) {
				if(this->selectable_labels[this->future_data.option_selected - i] && this->future_enabled_labels[this->future_data.option_selected - i]) {
					this->future_data.option_selected -= i;
					success = true;
					break;
				}
			}
		} while(!success);
	}
	if(!success) {
		if((!is_simple) && this->is_option_element(this->future_data.option_selected))
			this->future_data.option_selected += 1;
		do {
			this->future_data.option_selected += 1;
			if(this->future_data.option_selected >= this->num_elements_displayed_per_screen)
				this->future_data.option_selected = 0;
		} while(!(this->selectable_labels[this->future_data.option_selected] && this->future_enabled_labels[this->future_data.option_selected]));
	}
}

void OptionSelectionMenu::reset_output_option() {
}

void OptionSelectionMenu::set_output_option(int index, int action) {
}

size_t OptionSelectionMenu::get_num_options() {
	return this->num_options_per_screen;
}

bool OptionSelectionMenu::is_option_selectable(int index, int action) {
	return true;
}

bool OptionSelectionMenu::is_option_inc_dec(int index) {
	return false;
}

std::string OptionSelectionMenu::get_string_option(int index, int action) {
	return "Sample Text";
}

std::string OptionSelectionMenu::setTextOptionInt(int index, int value) {
	return this->get_string_option(index, DEFAULT_ACTION) + ": " + std::to_string(value);
}

std::string OptionSelectionMenu::setTextOptionString(int index, std::string value) {
	return this->get_string_option(index, DEFAULT_ACTION) + ": " + value;
}

std::string OptionSelectionMenu::setTextOptionBool(int index, bool value) {
	if(value)
		return this->get_string_option(index, DEFAULT_ACTION);
	return this->get_string_option(index, FALSE_ACTION);
}

std::string OptionSelectionMenu::setTextOptionFloat(int index, float value, int num_decimals) {
	std::string out = this->get_string_option(index, DEFAULT_ACTION) + ": " + get_float_str_decimals(value, num_decimals);
	return out;
}

int OptionSelectionMenu::get_num_pages() {
	int num_pages = (int)(1 + ((this->get_num_options() - 1) / this->num_options_per_screen));
	if(this->get_num_options() == ((size_t)(this->num_options_per_screen + 1)))
		num_pages = 1;
	return num_pages;
}

void OptionSelectionMenu::prepare_options() {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	int total_elements = (int)(this->get_num_options() - start);
	if((num_pages > 1) && (total_elements > this->num_options_per_screen))
		total_elements = this->num_options_per_screen;
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		this->future_enabled_labels[i] = false;
		this->labels[i]->setShowText(false);
	}
	if(this->show_back_x) {
		this->future_enabled_labels[this->back_x_id] = true;
		if(this->show_x)
			this->labels[this->back_x_id]->setText("X");
		else
			this->labels[this->back_x_id]->setText("<--");
		this->labels[this->back_x_id]->setShowText(true);
	}
	if(this->show_title) {
		this->future_enabled_labels[this->title_id] = true;
		this->labels[this->title_id]->setText(this->title);
		this->labels[this->title_id]->setShowText(true);
	}
	for(int i = 0; i < total_elements; i++) {
		int label_start_index = (i * this->single_option_multiplier) + this->elements_start_id;
		for(int j = 0; j < this->single_option_multiplier; j++)
			this->selectable_labels[label_start_index + j] = false;
		this->future_enabled_labels[label_start_index] = true;
		this->labels[label_start_index]->setText(this->get_string_option(start + i, DEFAULT_ACTION));
		this->labels[label_start_index]->setShowText(true);
		if(!is_option_inc_dec(start + i))
			this->selectable_labels[label_start_index] = this->is_option_selectable(start + i, DEFAULT_ACTION);
		else {
			this->labels[label_start_index + 1]->setText(this->get_string_option(start + i, DEC_ACTION));
			this->labels[label_start_index + 1]->setShowText(true);
			this->future_enabled_labels[label_start_index + 1] = true;
			this->selectable_labels[label_start_index + 1] = this->is_option_selectable(start + i, DEC_ACTION);
			this->labels[label_start_index + 2]->setText(this->get_string_option(start + i, INC_ACTION));
			this->labels[label_start_index + 2]->setShowText(true);
			this->future_enabled_labels[label_start_index + 2] = true;
			this->selectable_labels[label_start_index + 2] = this->is_option_selectable(start + i, INC_ACTION);
		}
	}
	if(num_pages > 1) {
		this->future_enabled_labels[this->info_page_id] = true;
		this->labels[this->info_page_id]->setText(std::to_string(this->future_data.page + 1) + "/" + std::to_string(num_pages));
		this->labels[this->info_page_id]->setShowText(true);
		if(this->future_data.page > 0) {
			this->future_enabled_labels[this->prev_page_id] = true;
			this->labels[this->prev_page_id]->setText("<");
			this->labels[this->prev_page_id]->setShowText(true);
		}
		if(this->future_data.page < (num_pages - 1)) {
			this->future_enabled_labels[this->next_page_id] = true;
			this->labels[this->next_page_id]->setText(">");
			this->labels[this->next_page_id]->setShowText(true);
		}
	}
	if(!(this->selectable_labels[this->future_data.option_selected] && this->future_enabled_labels[this->future_data.option_selected])) {
		this->future_data.option_selected = -1;
	}
}

void OptionSelectionMenu::option_selection_handling() {
	int old_loaded_page = this->future_data.page;
	if((this->future_data.option_selected == -1) || (!(this->selectable_labels[this->future_data.option_selected] && this->future_enabled_labels[this->future_data.option_selected]))) {
		this->future_data.option_selected = -1;
		return;
	}
	if(!this->can_execute_action())
		return;
	if(this->future_data.option_selected == this->back_x_id) {
		this->set_output_option(BACK_X_OUTPUT_OPTION, BAD_ACTION);
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
	else if(this->is_option_element(this->future_data.option_selected)) {
		int elem_index = this->future_data.option_selected - this->elements_start_id;
		int start = this->num_options_per_screen * this->future_data.page;
		this->set_output_option((elem_index / this->single_option_multiplier) + start, elem_index % this->single_option_multiplier);
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
	else if((this->future_data.option_selected == this->prev_page_id) && (this->future_data.page > 0))
		this->future_data.page--;
	else if((this->future_data.option_selected == this->next_page_id) && (this->future_data.page < ((int)((this->get_num_options() - 1) / this->num_options_per_screen))))
		this->future_data.page++;
	if(old_loaded_page != this->future_data.page) {
		this->prepare_options();
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
}

void OptionSelectionMenu::up_code(bool is_simple) {
	if(!this->can_execute_action())
		return;
	this->decrement_selected_option(is_simple);
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void OptionSelectionMenu::down_code(bool is_simple) {
	if(!this->can_execute_action())
		return;
	this->increment_selected_option(is_simple);
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void OptionSelectionMenu::left_code() {
	if(!this->can_execute_action())
		return;
	bool location_left = this->is_option_location_left(this->future_data.option_selected);
	bool multichoice = this->is_option_location_multichoice(this->future_data.option_selected);
	if(multichoice && location_left)
		this->option_selection_handling();
	else if(multichoice && (this->future_data.option_selected != this->next_page_id)) {
		this->future_data.option_selected -= 1;
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
	else if(this->selectable_labels[this->prev_page_id] && this->future_enabled_labels[this->prev_page_id]) {
		this->future_data.option_selected = this->prev_page_id;
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
}

void OptionSelectionMenu::right_code() {
	if(!this->can_execute_action())
		return;
	bool location_left = this->is_option_location_left(this->future_data.option_selected);
	bool multichoice = this->is_option_location_multichoice(this->future_data.option_selected);
	if(multichoice && (!location_left))
		this->option_selection_handling();
	else if(multichoice && (this->future_data.option_selected != this->prev_page_id)) {
		this->future_data.option_selected += 1;
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
	else if(this->selectable_labels[this->next_page_id] && this->future_enabled_labels[this->next_page_id]) {
		this->future_data.option_selected = this->next_page_id;
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
}

bool OptionSelectionMenu::poll(SFEvent &event_data) {
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
			if(!(this->selectable_labels[i] && this->future_enabled_labels[i]))
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
			if(!(this->selectable_labels[i] && this->future_enabled_labels[i]))
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

void OptionSelectionMenu::draw(float scaling_factor, sf::RenderTarget &window) {
	const sf::Vector2f rect_size = this->menu_rectangle.getSize();
	if((this->loaded_data.menu_width != rect_size.x) || (this->loaded_data.menu_height != rect_size.y)) {
		this->menu_rectangle.setSize(sf::Vector2f((float)this->loaded_data.menu_width, (float)this->loaded_data.menu_height));
	}
	this->menu_rectangle.setPosition({(float)this->loaded_data.pos_x, (float)this->loaded_data.pos_y});
	window.draw(this->menu_rectangle);
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		if(this->loaded_enabled_labels[i])
			this->labels[i]->draw(window);
	}
}

void OptionSelectionMenu::prepare_text_slices(int x_multiplier, int x_divisor, int y_multiplier, int y_divisor, int index, float text_scaling_factor, bool center) {
	this->labels[index]->setTextFactor(text_scaling_factor);
	int x_base_pos = (this->future_data.menu_width * x_multiplier) / x_divisor;
	int y_base_pos = (this->future_data.menu_height * y_multiplier) / y_divisor;
	int x_size = ((this->future_data.menu_width * (x_multiplier + 1)) / x_divisor) - x_base_pos;
	int y_size = ((this->future_data.menu_height * (y_multiplier + 1)) / y_divisor) - y_base_pos;
	if(center)
		x_size = ((this->future_data.menu_width * (x_divisor - x_multiplier)) / x_divisor) - x_base_pos;
	this->labels[index]->setSize(x_size, y_size);
	this->labels[index]->setPosition(this->future_data.pos_x + x_base_pos, this->future_data.pos_y + y_base_pos);
	if(index == this->future_data.option_selected) {
		if((index == this->back_x_id) && this->show_x)
			this->labels[index]->setRectangleKind(TEXT_KIND_ERROR);
		else
			this->labels[index]->setRectangleKind(TEXT_KIND_SELECTED);
	}
	else {
		if((index == this->back_x_id) && this->show_x)
			this->labels[index]->setRectangleKind(TEXT_KIND_OPAQUE_ERROR);
		else if(!this->selectable_labels[index])
			this->labels[index]->setRectangleKind(TEXT_KIND_TITLE);
		else
			this->labels[index]->setRectangleKind(TEXT_KIND_NORMAL);
	}
	this->labels[index]->prepareRenderText();
}

void OptionSelectionMenu::base_prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	int num_elements = 0;
	bool has_top = this->future_enabled_labels[this->back_x_id] || this->future_enabled_labels[this->title_id];
	if(has_top)
		num_elements += 1;
	for(int i = 0; i < (this->num_options_per_screen + 1); i++)
		if(this->future_enabled_labels[(i * this->single_option_multiplier) + this->elements_start_id])
			num_elements++;
	bool has_bottom = false;
	for(int i = 0; i < this->num_page_elements; i++)
		if(this->future_enabled_labels[i + this->page_elements_start_id])
			has_bottom = true;
	if(has_bottom)
		num_elements += 1;
	const float base_height = (this->num_vertical_slices * BASE_PIXEL_FONT_HEIGHT * 11.0f) / 9.0f;
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
	int num_elements_text_scaling_factor = num_elements;
	if(num_elements_text_scaling_factor < this->min_elements_text_scaling_factor)
		num_elements_text_scaling_factor = this->min_elements_text_scaling_factor;
	float text_scaling_factor = (menu_scaling_factor * this->num_vertical_slices) / num_elements_text_scaling_factor;
	this->future_data.menu_height = (int)(base_height * final_menu_scaling_factor);
	this->future_data.menu_width = (this->future_data.menu_height * this->width_factor_menu) / this->width_divisor_menu;
	if(this->future_data.menu_width > max_width)
		this->future_data.menu_width = max_width;
	this->future_data.pos_x = (view_size_x - this->future_data.menu_width) / 2;
	this->future_data.pos_y = (view_size_y - this->future_data.menu_height) / 2;
	int num_rendered_y = 0;
	if(this->future_enabled_labels[this->back_x_id]) {
		int top_divisor = 1;
		if(this->future_enabled_labels[this->title_id])
			top_divisor = 6;
		this->prepare_text_slices(0, top_divisor, num_rendered_y, num_elements, this->back_x_id, text_scaling_factor);
	}
	if(this->future_enabled_labels[this->title_id]) {
		this->prepare_text_slices(0, 1, num_rendered_y, num_elements, this->title_id, text_scaling_factor);
	}
	if(has_top)
		++num_rendered_y;
	for(int i = 0; i < (this->num_options_per_screen + 1); i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(this->future_enabled_labels[index]) {
			int elem_divisor = 1;
			if(this->future_enabled_labels[index + 1] || this->future_enabled_labels[index + 2])
				elem_divisor = 6;
			this->prepare_text_slices(0, 1, num_rendered_y, num_elements, index, text_scaling_factor);
			if(this->future_enabled_labels[index + 1])
				this->prepare_text_slices(0, elem_divisor, num_rendered_y, num_elements, index + 1, text_scaling_factor);
			if(this->future_enabled_labels[index + 2])
				this->prepare_text_slices(elem_divisor - 1, elem_divisor, num_rendered_y, num_elements, index + 2, text_scaling_factor);
			++num_rendered_y;
		}
	}
	for(int i = 0; i < this->num_page_elements; i++) {
		int index = i + this->page_elements_start_id;
		if(this->future_enabled_labels[index]){
			this->prepare_text_slices(i, this->num_page_elements, num_rendered_y, num_elements, index, text_scaling_factor);
		}
	}
	if(has_bottom)
		++num_rendered_y;
	this->loaded_data = this->future_data;
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++)
		this->loaded_enabled_labels[i] = this->future_enabled_labels[i];
}

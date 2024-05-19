#include "OptionSelectionMenu.hpp"

OptionSelectionMenu::OptionSelectionMenu() {
}

OptionSelectionMenu::~OptionSelectionMenu() {
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++)
		delete this->labels[i];
	delete []this->labels;
	delete []this->selectable_labels;
	delete []this->future_enabled_labels;
	delete []this->loaded_enabled_labels;
}

void OptionSelectionMenu::initialize(bool font_load_success, sf::Font &text_font) {
	this->class_setup();
	this->after_class_setup_connected_values();
	this->menu_rectangle.setFillColor(this->menu_color);
	this->menu_rectangle.setPosition(1, 1);
	this->labels = new TextRectangle*[this->num_elements_displayed_per_screen];
	this->selectable_labels = new bool[this->num_elements_displayed_per_screen];
	this->future_enabled_labels = new bool[this->num_elements_displayed_per_screen];
	this->loaded_enabled_labels = new bool[this->num_elements_displayed_per_screen];
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++) {
		this->labels[i] = new TextRectangle(font_load_success, text_font);
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
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
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
	this->back_x_id = title_back_x_start_id;
	this->title_id = title_back_x_start_id + 1;
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

void OptionSelectionMenu::reset_data() {
	this->last_input_processed_time = std::chrono::high_resolution_clock::now();
	this->reset_output_option();
	this->future_data.option_selected = -1;
	this->future_data.page = 0;
}

bool OptionSelectionMenu::is_curr_option_location_left() {
	bool location_left = true;

	if(this->future_data.option_selected == this->next_page_id)
		location_left = false;
	else if((this->future_data.option_selected >= this->elements_start_id) && (this->future_data.option_selected <= (this->elements_start_id + this->num_elements_per_screen)) && (((this->future_data.option_selected - this->elements_start_id) % this->single_option_multiplier) == 2))
		location_left = false;

	return location_left;
}

bool OptionSelectionMenu::is_curr_option_location_multichoice() {
	bool location_left = true;

	if((this->future_data.option_selected == this->next_page_id) || (this->future_data.option_selected == this->prev_page_id))
		return true;
	if((this->future_data.option_selected >= this->elements_start_id) && (this->future_data.option_selected <= (this->elements_start_id + this->num_elements_per_screen)) && (((this->future_data.option_selected - this->elements_start_id) % this->single_option_multiplier) != 0))
		return true;

	return false;
}

void OptionSelectionMenu::decrement_selected_option(bool is_simple) {
	if(this->future_data.option_selected < 0) {
		this->future_data.option_selected = this->elements_start_id;
		return;
	}
	bool location_left = this->is_curr_option_location_left();
	bool multichoice = this->is_curr_option_location_multichoice();
	if(!is_simple) {
		if(this->future_data.option_selected == this->next_page_id)
			this->future_data.option_selected = this->prev_page_id;
		else {
			if(multichoice && location_left)
				this->future_data.option_selected -= this->single_option_multiplier - 1;
			else
				this->future_data.option_selected -= 1;
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
		this->future_data.option_selected = this->elements_start_id;
		return;
	}
	bool location_left = this->is_curr_option_location_left();
	if(is_simple || location_left) {
		if((!is_simple) && (this->future_data.option_selected != this->prev_page_id))
			this->future_data.option_selected += 1;
		do {
			this->future_data.option_selected += 1;
			if(this->future_data.option_selected >= this->num_elements_displayed_per_screen)
				this->future_data.option_selected = 0;
		} while(!(this->selectable_labels[this->future_data.option_selected] && this->future_enabled_labels[this->future_data.option_selected]));
	}
	else {
		bool success = false;
		do {
			this->future_data.option_selected += this->single_option_multiplier;
			if(this->future_data.option_selected >= this->num_elements_displayed_per_screen) {
				this->future_data.option_selected = 0;
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
}

void OptionSelectionMenu::reset_output_option() {
}

void OptionSelectionMenu::set_output_option(int index, int action) {
}

int OptionSelectionMenu::get_num_options() {
	return this->num_options_per_screen;
}

bool OptionSelectionMenu::is_option_inc_dec(int index) {
	return false;
}

std::string OptionSelectionMenu::get_string_option(int index, int action) {
	return "Sample Text";
}

int OptionSelectionMenu::get_num_pages() {
	int num_pages = 1 + ((this->get_num_options() - 1) / this->num_options_per_screen);
	if(this->get_num_options() == (this->num_options_per_screen + 1))
		num_pages = 1;
	return num_pages;
}

void OptionSelectionMenu::prepare_options() {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	int total_elements = this->get_num_options() - start;
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
			this->selectable_labels[label_start_index] = true;
		else {
			this->labels[label_start_index + 1]->setText(this->get_string_option(start + i, DEC_ACTION));
			this->labels[label_start_index + 1]->setShowText(true);
			this->future_enabled_labels[label_start_index + 1] = true;
			this->selectable_labels[label_start_index + 1] = true;
			this->labels[label_start_index + 2]->setText(this->get_string_option(start + i, INC_ACTION));
			this->labels[label_start_index + 2]->setShowText(true);
			this->future_enabled_labels[label_start_index + 2] = true;
			this->selectable_labels[label_start_index + 2] = true;
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
	else if(this->future_data.option_selected < (this->elements_start_id + this->num_elements_per_screen)) {
		int elem_index = this->future_data.option_selected - this->elements_start_id;
		int start = this->num_options_per_screen * this->future_data.page;
		this->set_output_option((elem_index / this->single_option_multiplier) + start, elem_index % this->single_option_multiplier);
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
	else if((this->future_data.option_selected == this->prev_page_id) && this->future_data.page > 0)
		this->future_data.page--;
	else if((this->future_data.option_selected == this->next_page_id) && this->future_data.page < ((this->get_num_options() - 1) / this->num_options_per_screen))
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
	bool location_left = this->is_curr_option_location_left();
	bool multichoice = this->is_curr_option_location_multichoice();
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
	bool location_left = this->is_curr_option_location_left();
	bool multichoice = this->is_curr_option_location_multichoice();
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
	case sf::Event::TextEntered:
		switch (event_data.unicode) {
		default:
			consumed = false;
			break;
		}

		break;
		
	case sf::Event::KeyPressed:
		switch (event_data.code) {
		case sf::Keyboard::Up:
			this->up_code(false);
			break;
		case sf::Keyboard::Down:
			this->down_code(false);
			break;
		case sf::Keyboard::PageUp:
			this->up_code(true);
			break;
		case sf::Keyboard::PageDown:
			this->down_code(true);
			break;
		case sf::Keyboard::Left:
			this->left_code();
			break;
		case sf::Keyboard::Right:
			this->right_code();
			break;
		case sf::Keyboard::Enter:
			this->option_selection_handling();
			break;
		default:
			consumed = false;
			break;
		}

		break;
	case sf::Event::MouseMoved:
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
	case sf::Event::MouseButtonPressed:
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
		if(event_data.mouse_button == sf::Mouse::Left)
			this->option_selection_handling();
		break;
	case sf::Event::JoystickButtonPressed:
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
	case sf::Event::JoystickMoved:
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
		this->menu_rectangle.setSize(sf::Vector2f(this->loaded_data.menu_width, this->loaded_data.menu_height));
	}
	this->menu_rectangle.setPosition(this->loaded_data.pos_x, this->loaded_data.pos_y);
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
	const float base_height = (this->num_vertical_slices * BASE_PIXEL_FONT_HEIGHT * 11) / 9;
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
	this->future_data.menu_height = base_height * final_menu_scaling_factor;
	this->future_data.menu_width = (this->future_data.menu_height * this->width_factor_menu) / this->width_divisor_menu;
	if(this->future_data.menu_width > max_width)
		this->future_data.menu_width = max_width;
	this->future_data.pos_x = (view_size_x - this->future_data.menu_width) / 2;
	this->future_data.pos_y = (view_size_y - this->future_data.menu_height) / 2;
	int slice_y_size = this->future_data.menu_height / num_elements;
	int slice_x_size = this->future_data.menu_width / this->num_page_elements;
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

#include "ConnectionMenu.hpp"

#define CONNECTION_PREV_PAGE_ID (CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - 3)
#define CONNECTION_INFO_PAGES_ID (CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - 2)
#define CONNECTION_NEXT_PAGE_ID (CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - 1)

ConnectionMenu::ConnectionMenu(bool font_load_success, sf::Font &text_font) {
	for(int i = 0; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++) {
		this->labels[i] = new TextRectangle(font_load_success, text_font);
		this->labels[i]->setProportionalBox(false);
		this->labels[i]->setText(std::to_string(i));
		this->labels[i]->setShowText(true);
		this->selectable_labels[i] = true;
		this->future_data.enabled_labels[i] = true;
	}
	this->last_action_time = std::chrono::high_resolution_clock::now();
	this->selectable_labels[CONNECTION_INFO_PAGES_ID] = false;
}

ConnectionMenu::~ConnectionMenu() {
	for(int i = 0; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++)
		delete this->labels[i];
}

bool ConnectionMenu::can_execute_action() {
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->last_action_time;
	if(diff.count() > this->action_timeout)
		return true;
	return false;
}

void ConnectionMenu::insert_data(DevicesList *devices_list) {
	this->selected_index = -1;
	this->future_data.option_selected = -1;
	this->future_data.page = 0;
	this->devices_list = devices_list;
	this->prepare_options();
}

void ConnectionMenu::decrement_selected_option() {
	do {
		this->future_data.option_selected -= 1;
		if(this->future_data.option_selected <= 0) {
			this->future_data.option_selected = 0;
			return;
		}
		if(this->future_data.option_selected >= CONNECTION_NUM_ELEMENTS_PER_SCREEN) {
			this->future_data.option_selected = CONNECTION_NUM_ELEMENTS_PER_SCREEN - 1;
		}
	} while(!(this->selectable_labels[this->future_data.option_selected] && this->future_data.enabled_labels[this->future_data.option_selected]));
}

void ConnectionMenu::increment_selected_option() {
	int starting_value = this->future_data.option_selected;
	if(starting_value < 0){
		this->future_data.option_selected = 0;
		return;
	}	
	do {
		this->future_data.option_selected += 1;
		if(this->future_data.option_selected >= CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN) {
			this->future_data.option_selected = starting_value;
			return;
		}
	} while(!(this->selectable_labels[this->future_data.option_selected] && this->future_data.enabled_labels[this->future_data.option_selected]));
}

void ConnectionMenu::prepare_options() {
	int num_pages = 1 + ((this->devices_list->numValidDevices - 1) / CONNECTION_NUM_ELEMENTS_PER_SCREEN);
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * CONNECTION_NUM_ELEMENTS_PER_SCREEN;
	int total_elements = this->devices_list->numValidDevices - start;
	if(total_elements > CONNECTION_NUM_ELEMENTS_PER_SCREEN)
		total_elements = CONNECTION_NUM_ELEMENTS_PER_SCREEN;
	for(int i = 0; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++) {
		this->future_data.enabled_labels[i] = false;
		this->labels[i]->setShowText(false);
	}
	for(int i = 0; i < total_elements; i++) {
		this->future_data.enabled_labels[i] = true;
		this->labels[i]->setText(std::string(&this->devices_list->serialNumbers[SERIAL_NUMBER_SIZE * (i + start)]));
		this->labels[i]->setShowText(true);
	}
	if(num_pages > 1) {
		this->future_data.enabled_labels[CONNECTION_INFO_PAGES_ID] = true;
		this->labels[CONNECTION_INFO_PAGES_ID]->setText(std::to_string(this->future_data.page + 1) + "/" + std::to_string(num_pages));
		this->labels[CONNECTION_INFO_PAGES_ID]->setShowText(true);
		if(this->future_data.page > 0) {
			this->future_data.enabled_labels[CONNECTION_PREV_PAGE_ID] = true;
			this->labels[CONNECTION_PREV_PAGE_ID]->setText("<");
			this->labels[CONNECTION_PREV_PAGE_ID]->setShowText(true);
		}
		if(this->future_data.page < (num_pages - 1)) {
			this->future_data.enabled_labels[CONNECTION_NEXT_PAGE_ID] = true;
			this->labels[CONNECTION_NEXT_PAGE_ID]->setText(">");
			this->labels[CONNECTION_NEXT_PAGE_ID]->setShowText(true);
		}
	}
	if(!(this->selectable_labels[this->future_data.option_selected] && this->future_data.enabled_labels[this->future_data.option_selected])) {
		this->future_data.option_selected = -1;
	}
}

void ConnectionMenu::option_selection_handling() {
	int old_loaded_page = this->future_data.page;
	if(!(this->selectable_labels[this->future_data.option_selected] && this->future_data.enabled_labels[this->future_data.option_selected])) {
		this->future_data.option_selected = -1;
		return;
	}
	if(!this->can_execute_action())
		return;
	if(this->future_data.option_selected < CONNECTION_NUM_ELEMENTS_PER_SCREEN) {
		this->selected_index = this->future_data.option_selected + (CONNECTION_NUM_ELEMENTS_PER_SCREEN * this->future_data.page);
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
	else if((this->future_data.option_selected == CONNECTION_PREV_PAGE_ID) && this->future_data.page > 0)
		this->future_data.page--;
	else if((this->future_data.option_selected == CONNECTION_NEXT_PAGE_ID) && this->future_data.page < ((this->devices_list->numValidDevices - 1) / CONNECTION_NUM_ELEMENTS_PER_SCREEN))
		this->future_data.page++;
	if(old_loaded_page != this->future_data.page) {
		this->prepare_options();
		this->last_action_time = std::chrono::high_resolution_clock::now();
	}
}

void ConnectionMenu::up_code() {
	if(!this->can_execute_action())
		return;
	this->decrement_selected_option();
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void ConnectionMenu::down_code() {
	if(!this->can_execute_action())
		return;
	this->increment_selected_option();
	this->last_action_time = std::chrono::high_resolution_clock::now();
}

void ConnectionMenu::left_code() {
	if(!this->can_execute_action())
		return;
	if(this->selectable_labels[CONNECTION_PREV_PAGE_ID] && this->future_data.enabled_labels[CONNECTION_PREV_PAGE_ID]) {
		if(this->future_data.option_selected == CONNECTION_PREV_PAGE_ID)
			this->option_selection_handling();
		else {
			this->future_data.option_selected = CONNECTION_PREV_PAGE_ID;
			this->last_action_time = std::chrono::high_resolution_clock::now();
		}
	}
}

void ConnectionMenu::right_code() {
	if(!this->can_execute_action())
		return;
	if(this->selectable_labels[CONNECTION_NEXT_PAGE_ID] && this->future_data.enabled_labels[CONNECTION_NEXT_PAGE_ID]) {
		if(this->future_data.option_selected == CONNECTION_NEXT_PAGE_ID)
			this->option_selection_handling();
		else {
			this->future_data.option_selected = CONNECTION_NEXT_PAGE_ID;
			this->last_action_time = std::chrono::high_resolution_clock::now();
		}
	}
}

bool ConnectionMenu::poll(SFEvent &event_data) {
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
			this->up_code();
			break;
		case sf::Keyboard::Down:
			this->down_code();
			break;
		case sf::Keyboard::PageUp:
			this->up_code();
			break;
		case sf::Keyboard::PageDown:
			this->down_code();
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
		for(int i = 0; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++) {
			if(!(this->selectable_labels[i] && this->future_data.enabled_labels[i]))
				continue;
			if(this->labels[i]->isCoordInRectangle(event_data.mouse_x, event_data.mouse_y)) {
				this->future_data.option_selected = i;
				break;
			}
		}
		break;
	case sf::Event::MouseButtonPressed:
		this->future_data.option_selected = -1;
		for(int i = 0; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++) {
			if(!(this->selectable_labels[i] && this->future_data.enabled_labels[i]))
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
			this->up_code();
			break;
		case JOY_DIR_DOWN:
			this->down_code();
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

void ConnectionMenu::draw(float scaling_factor, sf::RenderTarget &window) {
	sf::RectangleShape menu_rectangle(sf::Vector2f(this->loaded_data.menu_width, this->loaded_data.menu_height));
	menu_rectangle.setFillColor(sf::Color(30, 30, 60, 192));
	menu_rectangle.setPosition(this->loaded_data.pos_x, this->loaded_data.pos_y);
	window.draw(menu_rectangle);
	for(int i = 0; i < CONNECTION_NUM_ELEMENTS_PER_SCREEN; i++) {
		if(this->loaded_data.enabled_labels[i])
			this->labels[i]->draw(window);
	}
	for(int i = CONNECTION_NUM_ELEMENTS_PER_SCREEN; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++) {
		if(this->loaded_data.enabled_labels[i])
			this->labels[i]->draw(window);
	}
}

void ConnectionMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	int num_elements = 1;
	for(int i = 1; i < CONNECTION_NUM_ELEMENTS_PER_SCREEN; i++)
		if(this->future_data.enabled_labels[i])
			num_elements++;
	bool has_bottom = false;
	for(int i = CONNECTION_NUM_ELEMENTS_PER_SCREEN; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++)
		if(this->future_data.enabled_labels[i])
			has_bottom = true;
	if(has_bottom)
		num_elements += 1;
	const float base_height = ((CONNECTION_NUM_ELEMENTS_PER_SCREEN + 1) * (BASE_PIXEL_FONT_HEIGHT * 8)) / 6;
	int max_width = (view_size_x * 9) / 10;
	int max_height = (view_size_y * 9) / 10;
	float max_scaling_factor = max_height / base_height;
	if(menu_scaling_factor > max_scaling_factor)
		menu_scaling_factor = max_scaling_factor;
	if(menu_scaling_factor < 0.3)
		menu_scaling_factor = 0.3;
	int num_elements_text_scaling_factor = num_elements;
	if(num_elements_text_scaling_factor < 3)
		num_elements_text_scaling_factor = 3;
	float text_scaling_factor = (menu_scaling_factor * (CONNECTION_NUM_ELEMENTS_PER_SCREEN + 1)) / num_elements_text_scaling_factor;
	int menu_height = base_height * menu_scaling_factor;
	int menu_width = (menu_height * 16) / 9;
	if(menu_width > max_width)
		menu_width = max_width;
	int pos_x = (view_size_x - menu_width) / 2;
	int pos_y = (view_size_y - menu_height) / 2;
	int slice_y_size = menu_height / num_elements;
	int slice_x_size = menu_width / (CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - CONNECTION_NUM_ELEMENTS_PER_SCREEN);
	for(int i = 0; i < CONNECTION_NUM_ELEMENTS_PER_SCREEN; i++) {
		if(!this->future_data.enabled_labels[i])
			continue;
		this->labels[i]->setTextFactor(text_scaling_factor);
		this->labels[i]->setSize(menu_width, slice_y_size);
		this->labels[i]->setPosition(pos_x, pos_y + (slice_y_size * i));
		if(i == this->future_data.option_selected)
			this->labels[i]->setRectangleKind(TEXT_KIND_SELECTED);
		else
			this->labels[i]->setRectangleKind(TEXT_KIND_NORMAL);
		this->labels[i]->prepareRenderText();
	}
	for(int i = CONNECTION_NUM_ELEMENTS_PER_SCREEN; i < CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN; i++) {
		if(!this->future_data.enabled_labels[i])
			continue;
		this->labels[i]->setTextFactor(text_scaling_factor);
		int x_size = slice_x_size;
		if(i == (CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - 1))
			x_size = menu_width - (slice_x_size * (CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - CONNECTION_NUM_ELEMENTS_PER_SCREEN - 1));
		this->labels[i]->setSize(x_size, menu_height - (slice_y_size * (num_elements - 1)));
		this->labels[i]->setPosition(pos_x + (slice_x_size * (i - CONNECTION_NUM_ELEMENTS_PER_SCREEN)), pos_y + (slice_y_size * (num_elements - 1)));
		if(i == this->future_data.option_selected)
			this->labels[i]->setRectangleKind(TEXT_KIND_SELECTED);
		else
			this->labels[i]->setRectangleKind(TEXT_KIND_NORMAL);
		this->labels[i]->prepareRenderText();
	}
	this->loaded_data = this->future_data;
}

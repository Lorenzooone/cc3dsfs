#include "MainMenu.hpp"

static std::string main_menu_option_strings[NUM_TOTAL_MAIN_MENU_OPTIONS] = {"Disconnect", "Connect", "Close Menu", "Quit Application"};
static bool usable_fullscreen[NUM_TOTAL_MAIN_MENU_OPTIONS] = {true, false, true, true};
static bool usable_normal_screen[NUM_TOTAL_MAIN_MENU_OPTIONS] = {true, false, true, true};
static bool usable_joint_screen[NUM_TOTAL_MAIN_MENU_OPTIONS] = {true, false, true, true};
static bool usable_top_screen[NUM_TOTAL_MAIN_MENU_OPTIONS] = {true, false, true, true};
static bool usable_bottom_screen[NUM_TOTAL_MAIN_MENU_OPTIONS] = {true, false, true, true};
static MainMenuOutAction main_menu_out_actions[NUM_TOTAL_MAIN_MENU_OPTIONS] = {MAIN_MENU_OPEN, MAIN_MENU_NO_ACTION, MAIN_MENU_CLOSE_MENU, MAIN_MENU_QUIT_APPLICATION};

MainMenu::MainMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

MainMenu::~MainMenu() {
}

void MainMenu::class_setup() {
	this->num_elements_per_screen = 5;
	this->num_page_elements = 3;
	this->num_elements_displayed_per_screen = num_elements_per_screen + num_page_elements;
	this->num_vertical_slices = (num_elements_displayed_per_screen - (num_page_elements - 1));
	this->num_elements_start_id = 0;
	this->num_page_elements_start_id = num_elements_per_screen + num_elements_start_id;
	this->prev_page_id = num_page_elements_start_id;
	this->info_page_id = num_page_elements_start_id + 1;
	this->next_page_id = num_page_elements_start_id + 2;
	this->min_elements_text_scaling_factor = this->num_vertical_slices;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
}

void MainMenu::insert_data(ScreenType s_type, bool is_fullscreen) {
	bool *fullscreen_enabled_ptr = usable_fullscreen;
	if(!is_fullscreen)
		fullscreen_enabled_ptr = usable_normal_screen;
	bool *screen_type_enabled_ptr = usable_joint_screen;
	if(s_type == ScreenType::TOP)
		screen_type_enabled_ptr = usable_top_screen;
	if(s_type == ScreenType::BOTTOM)
		screen_type_enabled_ptr = usable_bottom_screen;
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MAIN_MENU_OPTIONS; i++) {
		if(fullscreen_enabled_ptr[i] && screen_type_enabled_ptr[i]) {
			this->options_indexes[this->num_enabled_options] = i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void MainMenu::reset_output_option() {
	this->selected_index = MainMenuOutAction::MAIN_MENU_NO_ACTION;
}

void MainMenu::set_output_option(int index) {
	this->selected_index = main_menu_out_actions[this->options_indexes[index]];
}

int MainMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string MainMenu::get_string_option(int index) {
	return main_menu_option_strings[this->options_indexes[index]];
}

static std::string setTextOptionBool(int option_index, bool value) {
	if(value)
		return main_menu_option_strings[option_index];
	return main_menu_option_strings[option_index + 1];
}

void MainMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, bool connected) {
	int num_pages = 1 + ((this->get_num_options() - 1) / this->num_elements_per_screen);
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_elements_per_screen;
	for(int i = 0; i < this->num_elements_per_screen; i++) {
		if(!this->future_enabled_labels[i])
			continue;
		int option_index = this->options_indexes[start + i];
		switch(main_menu_out_actions[option_index]) {
			case MAIN_MENU_OPEN:
				this->labels[i]->setText(setTextOptionBool(option_index, connected));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

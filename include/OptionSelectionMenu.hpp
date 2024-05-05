#ifndef __OPTIONSELECTIONMENU_HPP
#define __OPTIONSELECTIONMENU_HPP

#include <SFML/Graphics.hpp>
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"

class OptionSelectionMenu {
public:
	OptionSelectionMenu();
	~OptionSelectionMenu();
	bool poll(SFEvent &event_data);
	void draw(float scaling_factor, sf::RenderTarget &window);
	void reset_data();
	std::chrono::time_point<std::chrono::high_resolution_clock> last_input_processed_time;
protected:
	int num_elements_per_screen = 1;
	int num_page_elements = 3;
	int num_elements_displayed_per_screen = num_elements_per_screen + num_page_elements;
	int num_vertical_slices = (num_elements_displayed_per_screen - (num_page_elements - 1));
	int num_elements_start_id = 0;
	int num_page_elements_start_id = num_elements_per_screen + num_elements_start_id;
	int prev_page_id = num_page_elements_start_id;
	int info_page_id = num_page_elements_start_id + 1;
	int next_page_id = num_page_elements_start_id + 2;
	int min_elements_text_scaling_factor = 3;
	int width_factor_menu = 1;
	int width_divisor_menu = 1;
	int base_height_factor_menu = 1;
	int base_height_divisor_menu = 1;
	float min_text_size = 0.3;
	virtual void reset_output_option();
	virtual void set_output_option(int index);
	virtual int get_num_options();
	virtual std::string get_string_option(int index);
	virtual void class_setup();
	void initialize(bool font_load_success, sf::Font &text_font);
	void prepare_options();
	void base_prepare(float menu_scaling_factor, int view_size_x, int view_size_y);
	struct MenuData {
		int page = 0;
		int option_selected = -1;
		int menu_width;
		int menu_height;
		int pos_x;
		int pos_y;
	};
	MenuData future_data;
	TextRectangle **labels;
	bool *future_enabled_labels;
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> last_action_time;
	const float action_timeout = 0.1;
	int loaded_page = 0;
	int option_selected = -1;
	bool *selectable_labels;
	bool *loaded_enabled_labels;
	MenuData loaded_data;

	void prepare_text_slices(int x_multiplier, int x_divisor, int y_multiplier, int y_divisor, int index, float text_scaling_factor);
	bool can_execute_action();
	void up_code();
	void down_code();
	void left_code();
	void right_code();
	void option_selection_handling();
	void decrement_selected_option();
	void increment_selected_option();
};
#endif

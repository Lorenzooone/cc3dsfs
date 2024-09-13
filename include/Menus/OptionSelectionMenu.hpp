#ifndef __OPTIONSELECTIONMENU_HPP
#define __OPTIONSELECTIONMENU_HPP

#include <SFML/Graphics.hpp>
#include <chrono>

#include "TextRectangle.hpp"
#include "event_structs.hpp"

#define BACK_X_OUTPUT_OPTION -1

#define BAD_ACTION -1
#define DEFAULT_ACTION 0
#define DEC_ACTION 1
#define INC_ACTION 2
#define FALSE_ACTION 3

class OptionSelectionMenu {
public:
	OptionSelectionMenu();
	~OptionSelectionMenu();
	bool poll(SFEvent &event_data);
	void draw(float scaling_factor, sf::RenderTarget &window);
	void reset_data();
	std::chrono::time_point<std::chrono::high_resolution_clock> last_input_processed_time;
	virtual void reset_output_option();
protected:
	int num_options_per_screen;
	int elements_start_id;
	int min_elements_text_scaling_factor;
	int width_factor_menu;
	int width_divisor_menu;
	int base_height_factor_menu;
	int base_height_divisor_menu;
	float min_text_size;
	float max_width_slack;
	sf::Color menu_color;
	std::string title;
	bool show_back_x;
	bool show_x;
	bool show_title;

	virtual bool is_option_selectable(int index, int action);
	virtual bool is_option_inc_dec(int index);
	virtual void set_output_option(int index, int action);
	virtual int get_num_options();
	virtual std::string get_string_option(int index, int action);
	virtual void class_setup();

	int single_option_multiplier;

	int get_num_pages();
	void initialize(bool font_load_success, sf::Font &text_font);
	void prepare_options();
	void base_prepare(float menu_scaling_factor, int view_size_x, int view_size_y);
	std::string setTextOptionInt(int index, int value);
	std::string setTextOptionString(int index, std::string value);
	std::string setTextOptionBool(int index, bool value);
	std::string setTextOptionFloat(int index, float value, int num_decimals = 1);

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
	int num_elements_per_screen;
	int num_title_back_x_elements;
	int num_page_elements;
	int num_elements_displayed_per_screen;
	int num_vertical_slices;
	int title_back_x_start_id;
	int page_elements_start_id;
	int back_x_id;
	int title_id;
	int prev_page_id;
	int info_page_id;
	int next_page_id;

	std::chrono::time_point<std::chrono::high_resolution_clock> last_action_time;
	const float action_timeout = 0.1;
	int loaded_page = 0;
	int option_selected = -1;
	bool *selectable_labels;
	bool *loaded_enabled_labels;
	MenuData loaded_data;
	sf::RectangleShape menu_rectangle = sf::RectangleShape(sf::Vector2f(1, 1));

	void after_class_setup_connected_values();
	void prepare_text_slices(int x_multiplier, int x_divisor, int y_multiplier, int y_divisor, int index, float text_scaling_factor, bool center = false);
	bool can_execute_action();
	void up_code(bool is_simple);
	void down_code(bool is_simple);
	void left_code();
	void right_code();
	void option_selection_handling();
	bool is_option_location_left(int option);
	bool is_option_element(int option);
	bool is_option_location_multichoice(int option);
	void set_default_cursor_position();
	void decrement_selected_option(bool is_simple);
	void increment_selected_option(bool is_simple);
};
#endif

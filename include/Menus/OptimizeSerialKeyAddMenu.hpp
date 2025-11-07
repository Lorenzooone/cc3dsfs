#ifndef __OPTIMIZESERIALKEYADDMENU_HPP
#define __OPTIMIZESERIALKEYADDMENU_HPP

#include <chrono>

#include "GenericMenu.hpp"

#include "TextRectangle.hpp"
#include "TextRectanglePool.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

#define BACK_X_OUTPUT_OPTION -1

enum OptimizeSerialKeyMenuOutAction{
	OPTIMIZE_SERIAL_KEY_ADD_MENU_NO_ACTION,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_BACK,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_DEC,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_INC,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_TARGET_INFO,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_SELECT_TEXTBOX,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_CONFIRM,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_PRINT,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_ABOVE_PRINT,
	OPTIMIZE_SERIAL_KEY_ADD_MENU_KEY_BUTTON_BELOW_PRINT,
};

class OptimizeSerialKeyAddMenu : public GenericMenu {
public:
	OptimizeSerialKeyAddMenu();
	OptimizeSerialKeyAddMenu(TextRectanglePool* text_pool);
	virtual ~OptimizeSerialKeyAddMenu();
	bool poll(SFEvent &event_data);
	void draw(float scaling_factor, sf::RenderTarget &window);
	void reset_data(bool full_reset);
	std::chrono::time_point<std::chrono::high_resolution_clock> last_input_processed_time;
	void reset_output_option();
	void on_menu_unloaded();
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data(CaptureDevice* device);
	std::string get_key();

	OptimizeSerialKeyMenuOutAction selected_index = OptimizeSerialKeyMenuOutAction::OPTIMIZE_SERIAL_KEY_ADD_MENU_NO_ACTION;
	bool key_for_new = false;
protected:
	struct TextFieldInputFixedData {
		int option_selected = -1;
		int menu_width;
		int menu_height;
		int pos_x;
		int pos_y;
	};
	TextFieldInputFixedData future_data;
	TextRectangle **labels;
	bool *selectable_labels;

	int num_vertical_slices;
	int num_elements_per_screen;
	int num_elements_displayed_per_screen;
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

	void initialize(TextRectanglePool* text_pool);
	void prepare_options();
	void base_prepare(float menu_scaling_factor, int view_size_x, int view_size_y);
	void prepare_text_slices(int x_multiplier, int x_divisor, int y_multiplier, int y_divisor, int x_size, int y_size, int index, float text_scaling_factor, TextKind text_kind, TextPosKind pos_kind = POS_KIND_NORMAL, bool center = false);
	TextRectangle* get_label_for_option(int option_num);

	virtual bool is_option_selectable(int index);
	virtual bool is_option_drawable(int index);
	virtual void set_output_option(int index);
	virtual std::string get_string_option(int index);
	virtual void class_setup();
	virtual void option_slice_prepare(int i, int index, int num_vertical_slices, float text_scaling_factor);
	virtual bool is_option_element(int option);
	virtual bool is_option_left(int index);
	virtual bool is_option_right(int index);
	virtual bool is_selected_insert_text_option(int index);
	virtual int get_next_option_selected_after_key_input();
	virtual int get_option_selected_key_input();
	virtual int get_option_print_key_input();
	virtual bool add_to_key(uint32_t unicode);
	virtual int get_pos_in_serial_key();
	virtual int get_key_size();
	virtual void key_update_char(bool increase);
	virtual bool handle_click(int index, percentage_pos_text_t percentage, bool started_inside_textbox);
	virtual void set_pos_special_label(int index);
private:
	int num_title_back_x_elements;
	int title_back_x_start_id;
	int back_x_id;
	int title_id;
	int pos_key = 0;
	std::string key = "WWWW-WWWW-WWWW-WWWW-WWWW";
	const float cursor_blank_timeout = 0.2f;
	std::chrono::time_point<std::chrono::high_resolution_clock> menu_time;
	bool first_pass;

	std::chrono::time_point<std::chrono::high_resolution_clock> last_action_time;
	const float action_timeout = 0.1f;
	TextFieldInputFixedData loaded_data;
	sf::RectangleShape menu_rectangle = sf::RectangleShape(sf::Vector2f(1, 1));

	void after_class_setup_connected_values();
	bool can_execute_action();
	void up_code(bool is_simple);
	void down_code(bool is_simple);
	void left_code();
	void right_code();
	void option_selection_handling();
	void set_default_cursor_position();
	void decrement_selected_option(bool is_simple);
	void increment_selected_option(bool is_simple);
	void quit_textbox(bool change_option_selected = true);
	char get_key_update_char(bool increase);
	std::string generate_key_print_string();
};
#endif

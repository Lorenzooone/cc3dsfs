#ifndef __RELPOSMENU_HPP
#define __RELPOSMENU_HPP

#include <chrono>

#include "GenericMenu.hpp"

#include "TextRectangle.hpp"
#include "TextRectanglePool.hpp"
#include "display_structs.hpp"

#define BACK_X_OUTPUT_OPTION -1

enum RelPosMenuOutAction{
	REL_POS_MENU_NO_ACTION,
	REL_POS_MENU_BACK,
	REL_POS_MENU_CONFIRM,
};

class RelativePositionMenu : public GenericMenu {
public:
	RelativePositionMenu();
	RelativePositionMenu(TextRectanglePool* text_pool);
	virtual ~RelativePositionMenu();
	bool poll(SFEvent &event_data);
	void draw(float scaling_factor, sf::RenderTarget &window);
	void reset_data(bool full_reset);
	std::chrono::time_point<std::chrono::high_resolution_clock> last_input_processed_time;
	void reset_output_option();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, BottomRelativePosition curr_bottom_pos);
	void insert_data();
	RelPosMenuOutAction selected_index = RelPosMenuOutAction::REL_POS_MENU_NO_ACTION;
	BottomRelativePosition selected_confirm_value = BOT_REL_POS_END;
protected:
	struct RelPosMenuData {
		int option_selected = -1;
		int menu_width;
		int menu_height;
		int pos_x;
		int pos_y;
	};
	RelPosMenuData future_data;
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
	void prepare_text_slices(int x_multiplier, int x_divisor, int y_multiplier, int y_divisor, int index, float text_scaling_factor, bool center = false);

	virtual bool is_option_selectable(int index);
	virtual bool is_option_drawable(int index);
	virtual void set_output_option(int index);
	virtual std::string get_string_option(int index);
	virtual void class_setup();
	virtual void option_slice_prepare(int i, int index, int num_vertical_slices, float text_scaling_factor);
	virtual bool is_option_element(int option);
	virtual bool is_option_left(int index);
	virtual bool is_option_right(int index);
	virtual bool is_option_above(int index);
	virtual bool is_option_below(int index);
private:
	int num_title_back_x_elements;
	int title_back_x_start_id;
	int back_x_id;
	int title_id;

	std::chrono::time_point<std::chrono::high_resolution_clock> last_action_time;
	const float action_timeout = 0.1f;
	RelPosMenuData loaded_data;
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
};
#endif

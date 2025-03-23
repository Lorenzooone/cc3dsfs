#ifndef __SECOND_SCREEN_3D_RELPOSMENU_HPP
#define __SECOND_SCREEN_3D_RELPOSMENU_HPP

#include <SFML/Graphics.hpp>
#include <chrono>

#include "TextRectangle.hpp"
#include "display_structs.hpp"
#include "event_structs.hpp"
#include "RelativePositionMenu.hpp"

#define BACK_X_OUTPUT_OPTION -1

enum SecondScreen3DRelPosMenuOutAction{
	SECOND_SCREEN_3D_REL_POS_MENU_NO_ACTION,
	SECOND_SCREEN_3D_REL_POS_MENU_BACK,
	SECOND_SCREEN_3D_REL_POS_MENU_TOGGLE_MATCH,
	SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM,
};

class SecondScreen3DRelativePositionMenu : public RelativePositionMenu {
public:
	SecondScreen3DRelativePositionMenu(bool font_load_success, sf::Font &text_font);
	~SecondScreen3DRelativePositionMenu();
	std::chrono::time_point<std::chrono::high_resolution_clock> last_input_processed_time;
	void reset_output_option();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo* info);
	void insert_data(ScreenType stype);
	SecondScreen3DRelPosMenuOutAction selected_index = SecondScreen3DRelPosMenuOutAction::SECOND_SCREEN_3D_REL_POS_MENU_NO_ACTION;
	SecondScreen3DRelativePosition selected_confirm_value = SECOND_SCREEN_3D_REL_POS_END;
protected:
	bool is_option_selectable(int index);
	void set_output_option(int index);
	std::string get_string_option(int index);
	void class_setup();

	void option_slice_prepare(int i, int index, int num_vertical_slices, float text_scaling_factor);
	bool is_option_element(int option);
	bool is_option_left(int index);
	bool is_option_right(int index);
	bool is_option_above(int index);
	bool is_option_below(int index);
private:
	std::string setTextOptionBool(int index, bool value);
	std::string get_false_option(int index);
	int pos_y_subtractor;
	ScreenType stype;
};
#endif

#ifndef __INPUTMENU_HPP
#define __INPUTMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum InputMenuOutAction{
	INPUT_MENU_NO_ACTION,
	INPUT_MENU_BACK,
	INPUT_MENU_SHORTCUT_SETTINGS,
	INPUT_MENU_TOGGLE_BUTTONS,
	INPUT_MENU_TOGGLE_JOYSTICK,
	INPUT_MENU_TOGGLE_KEYBOARD,
	INPUT_MENU_TOGGLE_MOUSE,
	INPUT_MENU_TOGGLE_FAST_POLL,
};

class InputMenu : public OptionSelectionMenu {
public:
	InputMenu(bool font_load_success, sf::Font &text_font);
	~InputMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, InputData* input_data);
	void insert_data(bool enabled_shortcuts, bool enabled_extra_buttons);
	InputMenuOutAction selected_index = InputMenuOutAction::INPUT_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

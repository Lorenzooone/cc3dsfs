#ifndef __ACTIONSELECTIONMENU_HPP
#define __ACTIONSELECTIONMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "WindowCommands.hpp"

#define ACTION_SELECTION_MENU_NO_ACTION -1
#define ACTION_SELECTION_MENU_BACK -2

class ActionSelectionMenu : public OptionSelectionMenu {
public:
	ActionSelectionMenu(TextRectanglePool* text_pool);
	~ActionSelectionMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, PossibleWindowCommands curr_cmd);
	void insert_data(std::vector<const WindowCommand*> &possible_actions);
	int selected_index = ACTION_SELECTION_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	std::vector<const WindowCommand*> *possible_actions;
};
#endif

#ifndef __SHORTCUTMENU_HPP
#define __SHORTCUTMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

#define SHORTCUT_MENU_NO_ACTION -1
#define SHORTCUT_MENU_BACK -2

class ShortcutMenu : public OptionSelectionMenu {
public:
	ShortcutMenu(bool font_load_success, sf::Font &text_font);
	~ShortcutMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data(std::vector<std::string> &shortcut_names);
	int selected_index = SHORTCUT_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	std::vector<std::string> *shortcut_names;
};
#endif

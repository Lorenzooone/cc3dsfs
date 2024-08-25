#ifndef __ISNMENU_HPP
#define __ISNMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum ISNitroMenuOutAction{
	ISN_MENU_NO_ACTION,
	ISN_MENU_BACK,
	ISN_MENU_DELAY,
	ISN_MENU_TYPE_DEC,
	ISN_MENU_TYPE_INC,
};

class ISNitroMenu : public OptionSelectionMenu {
public:
	ISNitroMenu(bool font_load_success, sf::Font &text_font);
	~ISNitroMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status);
	void insert_data();
	ISNitroMenuOutAction selected_index = ISNitroMenuOutAction::ISN_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_selectable(int index, int action);
	bool is_option_inc_dec(int index);
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	int num_enabled_options;
};
#endif

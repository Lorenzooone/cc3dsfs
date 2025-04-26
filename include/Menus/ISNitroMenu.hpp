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
	ISN_MENU_RESET,
	ISN_MENU_SPEED_INC,
	ISN_MENU_SPEED_DEC,
	ISN_MENU_BATTERY_INC,
	ISN_MENU_BATTERY_DEC,
	ISN_MENU_AC_ADAPTER_TOGGLE,
};

class ISNitroMenu : public OptionSelectionMenu {
public:
	ISNitroMenu(TextRectanglePool* text_pool);
	~ISNitroMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status);
	void insert_data(CaptureDevice* device);
	ISNitroMenuOutAction selected_index = ISNitroMenuOutAction::ISN_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_selectable(int index, int action);
	bool is_option_inc_dec(int index);
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

#ifndef __MAIN3DMENU_HPP
#define __MAIN3DMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum Main3DMenuOutAction{
	MAIN_3D_MENU_NO_ACTION,
	MAIN_3D_MENU_BACK,
	MAIN_3D_MENU_REQUEST_3D_TOGGLE,
	MAIN_3D_MENU_INTERLEAVED_TOGGLE,
	MAIN_3D_MENU_SQUISH_TOP_TOGGLE,
	MAIN_3D_MENU_SQUISH_BOTTOM_TOGGLE,
	MAIN_3D_MENU_USB_SPEED_INFO,
	MAIN_3D_MENU_DEVICE_INFO,
	MAIN_3D_MENU_IMPLEMENTATION_INFO,
	MAIN_3D_MENU_SECOND_SCREEN_POSITION_SETTINGS,
};

class Main3DMenu : public OptionSelectionMenu {
public:
	Main3DMenu(bool font_load_success, sf::Font &text_font);
	~Main3DMenu();
	void prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, DisplayData* display_data, CaptureStatus* status);
	void insert_data(ScreenType stype);
	Main3DMenuOutAction selected_index = Main3DMenuOutAction::MAIN_3D_MENU_NO_ACTION;
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

#ifndef __EXTRASETTINGSMENU_HPP
#define __EXTRASETTINGSMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum ExtraSettingsMenuOutAction{
	EXTRA_SETTINGS_MENU_NO_ACTION,
	EXTRA_SETTINGS_MENU_BACK,
	EXTRA_SETTINGS_MENU_QUIT_APPLICATION,
	EXTRA_SETTINGS_MENU_FULLSCREEN,
	EXTRA_SETTINGS_MENU_SPLIT,
	EXTRA_SETTINGS_MENU_USB_CONFLICT_RESOLUTION,
};

class ExtraSettingsMenu : public OptionSelectionMenu {
public:
	ExtraSettingsMenu(TextRectanglePool* text_pool);
	~ExtraSettingsMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	static int get_total_possible_selectable_inserted(ScreenType s_type, bool is_fullscreen, bool is_mono_app);
	void insert_data(ScreenType s_type, bool is_fullscreen, bool is_mono_app);
	ExtraSettingsMenuOutAction selected_index = ExtraSettingsMenuOutAction::EXTRA_SETTINGS_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	bool is_option_selectable(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

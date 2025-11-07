#ifndef __OPTIMIZE3DSMENU_HPP
#define __OPTIMIZE3DSMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum Optimize3DSMenuOutAction{
	OPTIMIZE3DS_MENU_NO_ACTION,
	OPTIMIZE3DS_MENU_BACK,
	OPTIMIZE3DS_MENU_INPUT_VIDEO_FORMAT_INC,
	OPTIMIZE3DS_MENU_INPUT_VIDEO_FORMAT_DEC,
	OPTIMIZE3DS_MENU_INFO_DEVICE_ID,
	OPTIMIZE3DS_MENU_COPY_DEVICE_ID,
	OPTIMIZE3DS_MENU_OPTIMIZE_SERIAL_KEY,
	OPTIMIZE3DS_MENU_OPTIMIZE_SERIAL_KEY_MENU,
};

class Optimize3DSMenu : public OptionSelectionMenu {
public:
	Optimize3DSMenu(TextRectanglePool* text_pool);
	~Optimize3DSMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status);
	void insert_data(CaptureDevice* device);
	Optimize3DSMenuOutAction selected_index = Optimize3DSMenuOutAction::OPTIMIZE3DS_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_selectable(int index, int action);
	bool is_option_inc_dec(int index);
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	float get_option_text_factor(int index);
	void class_setup();
private:
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

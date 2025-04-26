#ifndef __STATUSMENU_HPP
#define __STATUSMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum StatusMenuOutAction{
	STATUS_MENU_NO_ACTION,
	STATUS_MENU_BACK,
};

class StatusMenu : public OptionSelectionMenu {
public:
	StatusMenu(TextRectanglePool* text_pool);
	~StatusMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, double in_fps, double poll_fps, double draw_fps, CaptureStatus* capture_status);
	void insert_data();
	StatusMenuOutAction selected_index = StatusMenuOutAction::STATUS_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_inc_dec(int index);
	bool is_option_selectable(int index, int action);
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	const float update_timeout = 1.0;
	bool do_update;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_update_time;
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

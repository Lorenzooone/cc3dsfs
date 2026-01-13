#ifndef __PARTCTRMENU_HPP
#define __PARTCTRMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum PartnerCTRMenuOutAction{
	PCTR_MENU_NO_ACTION,
	PCTR_MENU_BACK,
	PCTR_MENU_RESET,
	PCTR_MENU_BATTERY_INC,
	PCTR_MENU_BATTERY_DEC,
	PCTR_MENU_AC_ADAPTER_CONNECTED_TOGGLE,
	PCTR_MENU_AC_ADAPTER_CHARGING_TOGGLE,
};

class PartnerCTRMenu : public OptionSelectionMenu {
public:
	PartnerCTRMenu(TextRectanglePool* text_pool);
	~PartnerCTRMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status);
	void insert_data(CaptureDevice* device);
	PartnerCTRMenuOutAction selected_index = PartnerCTRMenuOutAction::PCTR_MENU_NO_ACTION;
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

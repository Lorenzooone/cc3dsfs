#ifndef __USBCONRESOMENU_HPP
#define __USBCONRESOMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum USBConflictResolutionMenuOutAction{
	USBCONRESO_MENU_NO_ACTION,
	USBCONRESO_MENU_BACK,
	USBCONRESO_MENU_NISE_DS,
	USBCONRESO_MENU_OPTI_O3DS,
};

class USBConflictResolutionMenu : public OptionSelectionMenu {
public:
	USBConflictResolutionMenu(TextRectanglePool* text_pool);
	~USBConflictResolutionMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status);
	static int get_total_possible_selectable_inserted();
	void insert_data();
	USBConflictResolutionMenuOutAction selected_index = USBConflictResolutionMenuOutAction::USBCONRESO_MENU_NO_ACTION;
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

#ifndef __OPTIMIZEOLDFWCONFIGMENU_HPP
#define __OPTIMIZEOLDFWCONFIGMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "capture_structs.hpp"

enum OptimizeOldFWConfigMenuOutAction{
	OPTIMIZEOLDFWCONFIG_MENU_NO_ACTION,
	OPTIMIZEOLDFWCONFIG_MENU_BACK,
	OPTIMIZEOLDFWCONFIG_MENU_CHANGE_INC,
	OPTIMIZEOLDFWCONFIG_MENU_CHANGE_DEC,
};

enum OptimizeOldFWConfigMenuSelectedOption {
	OPTIMIZEOLDFWCONFIG_MENU_LOWSCREENCLK,
	OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENCLK,
	OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENDATA,
	OPTIMIZEOLDFWCONFIG_MENU_TOPSCREENSYNC,
};

enum OptimizeOldFWConfigMenuSelectedCase {
	OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY,
	OPTIMIZEOLDFWCONFIG_MENU_CASEREGULAR,
};

class OptimizeOldFWConfigMenu : public OptionSelectionMenu {
public:
	OptimizeOldFWConfigMenu(TextRectanglePool* text_pool);
	~OptimizeOldFWConfigMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status);
	void insert_data();
	uint8_t* get_data_to_edit(CaptureOptimizeOldFirmwareConfig* optimize_old_fw_cfg);
	OptimizeOldFWConfigMenuOutAction selected_index = OptimizeOldFWConfigMenuOutAction::OPTIMIZEOLDFWCONFIG_MENU_NO_ACTION;
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
	OptimizeOldFWConfigMenuSelectedOption selected_option = OptimizeOldFWConfigMenuSelectedOption::OPTIMIZEOLDFWCONFIG_MENU_LOWSCREENCLK;
	OptimizeOldFWConfigMenuSelectedCase selected_case = OptimizeOldFWConfigMenuSelectedCase::OPTIMIZEOLDFWCONFIG_MENU_CASE2DONLY;
};
#endif

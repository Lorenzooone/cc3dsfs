#ifndef __PARMENU_HPP
#define __PARMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

#define PAR_MENU_NO_ACTION -1
#define PAR_MENU_BACK -2

class PARMenu : public OptionSelectionMenu {
public:
	PARMenu(bool font_load_success, sf::Font &text_font);
	~PARMenu();
	void setup_title(std::string added_name);
	void prepare(float scaling_factor, int view_size_x, int view_size_y, int current_crop);
	void insert_data(std::vector<const PARData*>* possible_pars);
	int selected_index = PAR_MENU_NO_ACTION;
protected:
	void reset_output_option();
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	const std::string base_name = "PAR Settings";
	std::vector<const PARData*>* possible_pars;
};
#endif

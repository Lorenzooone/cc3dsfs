#ifndef __COLORCORRECTIONMENU_HPP
#define __COLORCORRECTIONMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

#define COLORCORRECTION_MENU_NO_ACTION -1
#define COLORCORRECTION_MENU_BACK -2

class ColorCorrectionMenu : public OptionSelectionMenu {
public:
	ColorCorrectionMenu(bool font_load_success, sf::Font &text_font);
	~ColorCorrectionMenu();
	void setup_title(std::string added_name);
	void prepare(float scaling_factor, int view_size_x, int view_size_y, int current_crop);
	void insert_data(std::vector<const ShaderColorEmulationData*>* possible_correction_data);
	int selected_index = COLORCORRECTION_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	const std::string base_name = "Color Correction";
	std::vector<const ShaderColorEmulationData*>* possible_correction_data;
};
#endif

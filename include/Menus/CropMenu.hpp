#ifndef __CROPMENU_HPP
#define __CROPMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

#define CROP_MENU_NO_ACTION -1
#define CROP_MENU_BACK -2

class CropMenu : public OptionSelectionMenu {
public:
	CropMenu(bool font_load_success, sf::Font &text_font);
	~CropMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, int current_crop);
	void insert_data(std::vector<const CropData*>* possible_crops);
	int selected_index = CROP_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	std::vector<const CropData*>* possible_crops;
};
#endif

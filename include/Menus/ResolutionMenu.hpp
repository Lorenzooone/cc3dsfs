#ifndef __RESOLUTIONMENU_HPP
#define __RESOLUTIONMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

#define RESOLUTION_MENU_NO_ACTION -1
#define RESOLUTION_MENU_BACK -2

class ResolutionMenu : public OptionSelectionMenu {
public:
	ResolutionMenu(bool font_load_success, sf::Font &text_font);
	~ResolutionMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, int fullscreen_mode_width, int fullscreen_mode_height);
	void insert_data(std::vector<sf::VideoMode>* possible_resolutions, const sf::VideoMode &desktop_resolution);
	int selected_index = RESOLUTION_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	std::vector<sf::VideoMode>* possible_resolutions;
	sf::VideoMode get_resolution(int index);
	sf::VideoMode desktop_resolution;
};
#endif

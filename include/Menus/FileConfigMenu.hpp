#ifndef __FILECONFIGMENU_HPP
#define __FILECONFIGMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "utils.hpp"

#define FILECONFIG_MENU_NO_ACTION -20
#define FILECONFIG_MENU_BACK -21

struct FileData {
	int index;
	std::string name;
};

class FileConfigMenu : public OptionSelectionMenu {
public:
	FileConfigMenu(bool font_load_success, sf::Font &text_font);
	~FileConfigMenu();
	void setup_title(std::string added_name);
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data(std::vector<FileData>* possible_files);
	int selected_index = FILECONFIG_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	const std::string base_name = "Profiles";
	std::vector<FileData>* possible_files;
};
#endif

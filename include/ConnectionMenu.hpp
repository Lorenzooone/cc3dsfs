#ifndef __CONNECTIONMENU_HPP
#define __CONNECTIONMENU_HPP

#include "OptionSelectionMenu.hpp"

#include "capture_structs.hpp"

class ConnectionMenu : public OptionSelectionMenu {
public:
	ConnectionMenu(bool font_load_success, sf::Font &text_font);
	~ConnectionMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data(DevicesList *devices_list);
	int selected_index = -1;
protected:
	void reset_output_option();
	void set_output_option(int index);
	int get_num_options();
	std::string get_string_option(int index);
	void class_setup();
private:
	DevicesList *devices_list;
};
#endif

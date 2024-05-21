#ifndef __CONNECTIONMENU_HPP
#define __CONNECTIONMENU_HPP

#include "OptionSelectionMenu.hpp"

#include "capture_structs.hpp"

#define CONNECTION_MENU_NO_ACTION (-1)

class ConnectionMenu : public OptionSelectionMenu {
public:
	ConnectionMenu(bool font_load_success, sf::Font &text_font);
	~ConnectionMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data(DevicesList *devices_list);
	int selected_index = CONNECTION_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	int get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	DevicesList *devices_list;
};
#endif

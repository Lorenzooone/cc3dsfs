#ifndef __CONNECTIONMENU_HPP
#define __CONNECTIONMENU_HPP

#include <SFML/Graphics.hpp>
#include <chrono>

#include "TextRectangle.hpp"
#include "capture_structs.hpp"
#include "sfml_gfx_structs.hpp"

#define CONNECTION_NUM_ELEMENTS_PER_SCREEN 4
#define CONNECTION_NUM_PAGE_ELEMENTS 3
#define CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN (CONNECTION_NUM_ELEMENTS_PER_SCREEN + CONNECTION_NUM_PAGE_ELEMENTS)
#define CONNECTION_NUM_VERTICAL_SLICES (CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN - (CONNECTION_NUM_PAGE_ELEMENTS - 1))

class ConnectionMenu {
public:
	ConnectionMenu(bool font_load_success, sf::Font &text_font);
	~ConnectionMenu();
	bool poll(SFEvent &event_data);
	void draw(float scaling_factor, sf::RenderTarget &window);
	void prepare(float scaling_factor, int view_size_x, int view_size_y);
	void insert_data(DevicesList *devices_list);
	int selected_index = -1;
private:
	struct ConnectionMenuData {
		int page = 0;
		int option_selected = -1;
		bool enabled_labels[CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN];
		int menu_width;
		int menu_height;
		int pos_x;
		int pos_y;
	};
	std::chrono::time_point<std::chrono::high_resolution_clock> last_action_time;
	const float action_timeout = 0.1;
	int loaded_page = 0;
	int option_selected = -1;
	TextRectangle *labels[CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN];
	bool selectable_labels[CONNECTION_NUM_ELEMENTS_DISPLAYED_PER_SCREEN];
	ConnectionMenuData future_data;
	ConnectionMenuData loaded_data;
	DevicesList *devices_list;

	bool can_execute_action();
	void up_code();
	void down_code();
	void left_code();
	void right_code();
	void prepare_options();
	void option_selection_handling();
	void decrement_selected_option();
	void increment_selected_option();
};
#endif

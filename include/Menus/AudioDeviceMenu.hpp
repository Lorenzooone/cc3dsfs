#ifndef __AUDIODEVICEMENU_HPP
#define __AUDIODEVICEMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"
#include "audio_data.hpp"

#define AUDIODEVICE_MENU_NO_ACTION -1
#define AUDIODEVICE_MENU_BACK -2

class AudioDeviceMenu : public OptionSelectionMenu {
public:
	AudioDeviceMenu(TextRectanglePool* text_pool);
	~AudioDeviceMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, const audio_output_device_data curr_device_data);
	void insert_data(std::vector<std::string>* possible_devices);
	int selected_index = AUDIODEVICE_MENU_NO_ACTION;
	void reset_output_option();
protected:
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	std::vector<std::string>* possible_devices;
};
#endif

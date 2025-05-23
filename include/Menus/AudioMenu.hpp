#ifndef __AUDIOMENU_HPP
#define __AUDIOMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "audio_data.hpp"

enum AudioMenuOutAction{
	AUDIO_MENU_NO_ACTION,
	AUDIO_MENU_BACK,
	AUDIO_MENU_MUTE,
	AUDIO_MENU_VOLUME_DEC,
	AUDIO_MENU_VOLUME_INC,
	AUDIO_MENU_MAX_LATENCY_DEC,
	AUDIO_MENU_MAX_LATENCY_INC,
	AUDIO_MENU_OUTPUT_DEC,
	AUDIO_MENU_OUTPUT_INC,
	AUDIO_MENU_RESTART,
	AUDIO_MENU_CHANGE_DEVICE,
	AUDIO_MENU_MODE_DEC,
	AUDIO_MENU_MODE_INC,
	AUDIO_MENU_AUTO_SCAN,
};

class AudioMenu : public OptionSelectionMenu {
public:
	AudioMenu(TextRectanglePool* text_pool);
	~AudioMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, AudioData *audio_data);
	void insert_data();
	AudioMenuOutAction selected_index = AudioMenuOutAction::AUDIO_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_inc_dec(int index);
	void set_output_option(int index, int action);
	size_t get_num_options();
	std::string get_string_option(int index, int action);
	void class_setup();
private:
	int *options_indexes;
	size_t num_enabled_options;
};
#endif

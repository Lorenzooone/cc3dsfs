#ifndef __VIDEOEFFECTSMENU_HPP
#define __VIDEOEFFECTSMENU_HPP

#include "OptionSelectionMenu.hpp"
#include <chrono>

#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "display_structs.hpp"

enum VideoEffectsMenuOutAction{
	VIDEO_EFFECTS_MENU_NO_ACTION,
	VIDEO_EFFECTS_MENU_BACK,
	VIDEO_EFFECTS_MENU_INPUT_COLORSPACE_INC,
	VIDEO_EFFECTS_MENU_INPUT_COLORSPACE_DEC,
	VIDEO_EFFECTS_MENU_FRAME_BLENDING_INC,
	VIDEO_EFFECTS_MENU_FRAME_BLENDING_DEC,
	VIDEO_EFFECTS_MENU_COLOR_CORRECTION_MENU,
};

class VideoEffectsMenu : public OptionSelectionMenu {
public:
	VideoEffectsMenu(TextRectanglePool* text_pool);
	~VideoEffectsMenu();
	void prepare(float scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info);
	void insert_data();
	VideoEffectsMenuOutAction selected_index = VideoEffectsMenuOutAction::VIDEO_EFFECTS_MENU_NO_ACTION;
	void reset_output_option();
protected:
	bool is_option_selectable(int index, int action);
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

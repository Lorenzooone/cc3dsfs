#ifndef __SHADERS_LIST_HPP
#define __SHADERS_LIST_HPP

#include<string>

enum shader_list_enum {
	NO_EFFECT_FRAGMENT_SHADER,
	FRAME_BLENDING_FRAGMENT_SHADER,
	BIT_CRUSHER_FRAGMENT_SHADER_1,
	BIT_CRUSHER_FRAGMENT_SHADER_2,
	BIT_CRUSHER_FRAGMENT_SHADER_3,
	BIT_CRUSHER_FRAGMENT_SHADER_4,
	BIT_CRUSHER_FRAGMENT_SHADER_5,
	BIT_CRUSHER_FRAGMENT_SHADER_6,
	BIT_CRUSHER_FRAGMENT_SHADER_7,
	BIT_MERGER_FRAGMENT_SHADER_0,
	BIT_MERGER_FRAGMENT_SHADER_1,
	BIT_MERGER_FRAGMENT_SHADER_2,
	BIT_MERGER_FRAGMENT_SHADER_3,
	BIT_MERGER_FRAGMENT_SHADER_4,
	BIT_MERGER_FRAGMENT_SHADER_5,
	BIT_MERGER_FRAGMENT_SHADER_6,
	BIT_MERGER_FRAGMENT_SHADER_7,
	BIT_MERGER_CRUSHER_FRAGMENT_SHADER_1,
	BIT_MERGER_CRUSHER_FRAGMENT_SHADER_2,
	BIT_MERGER_CRUSHER_FRAGMENT_SHADER_3,
	BIT_MERGER_CRUSHER_FRAGMENT_SHADER_4,
	BIT_MERGER_CRUSHER_FRAGMENT_SHADER_5,
	BIT_MERGER_CRUSHER_FRAGMENT_SHADER_6,
	BIT_MERGER_CRUSHER_FRAGMENT_SHADER_7,
	FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_1,
	FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_2,
	FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_3,
	FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_4,
	FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_5,
	FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_6,
	FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_7,

	TOTAL_NUM_SHADERS
};

std::string get_shader_string(shader_list_enum requested_shader);
void shader_strings_init();

#endif
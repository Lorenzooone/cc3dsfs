#include "shaders_list.hpp"

static std::string shader_strings[TOTAL_NUM_SHADERS];

std::string get_shader_string(shader_list_enum requested_shader) {
	if((requested_shader < 0) || (requested_shader >= TOTAL_NUM_SHADERS))
		return "";
	return shader_strings[requested_shader];
}

// This is a template
void shader_strings_init() {

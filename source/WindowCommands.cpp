#include "WindowCommands.hpp"

#define NUM_SELECTABLE_WINDOW_COMMANDS (sizeof(possible_cmds) / sizeof(possible_cmds[0]))

static const WindowCommand none_cmd = {
.cmd = WINDOW_COMMAND_NONE,
.name = "Nothing",
.usable_always = true,
.available_mono_extra = true,
};

static const WindowCommand connect_cmd = {
.cmd = WINDOW_COMMAND_CONNECT,
.name = "Connection",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand split_cmd = {
.cmd = WINDOW_COMMAND_SPLIT,
.name = "Split",
.usable_always = true,
.available_mono_extra = false,
};

static const WindowCommand fullscreen_cmd = {
.cmd = WINDOW_COMMAND_FULLSCREEN,
.name = "Fullscreen",
.usable_always = true,
.available_mono_extra = false,
};

static const WindowCommand crop_cmd = {
.cmd = WINDOW_COMMAND_CROP,
.name = "Crop",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand async_cmd = {
.cmd = WINDOW_COMMAND_ASYNC,
.name = "Async",
.usable_always = true,
.available_mono_extra = true,
};

static const WindowCommand vsync_cmd = {
.cmd = WINDOW_COMMAND_VSYNC,
.name = "VSync",
.usable_always = true,
.available_mono_extra = true,
};

static const WindowCommand menu_scaling_inc_cmd = {
.cmd = WINDOW_COMMAND_MENU_SCALING_INC,
.name = "Inc. Menu Scaling",
.usable_always = true,
.available_mono_extra = true,
};

static const WindowCommand menu_scaling_dec_cmd = {
.cmd = WINDOW_COMMAND_MENU_SCALING_DEC,
.name = "Dec. Menu Scaling",
.usable_always = true,
.available_mono_extra = true,
};

static const WindowCommand window_scaling_inc_cmd = {
.cmd = WINDOW_COMMAND_WINDOW_SCALING_INC,
.name = "Inc. Window Scaling",
.usable_always = true,
.available_mono_extra = true,
};

static const WindowCommand window_scaling_dec_cmd = {
.cmd = WINDOW_COMMAND_WINDOW_SCALING_DEC,
.name = "Dec. Window Scaling",
.usable_always = true,
.available_mono_extra = true,
};

static const WindowCommand ratio_cycle_cmd = {
.cmd = WINDOW_COMMAND_RATIO_CYCLE,
.name = "Cycle Screens Ratio",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand ratio_top_cmd = {
.cmd = WINDOW_COMMAND_RATIO_TOP,
.name = "Inc. Top Ratio",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand ratio_bot_cmd = {
.cmd = WINDOW_COMMAND_RATIO_BOT,
.name = "Inc. Bottom Ratio",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand blur_cmd = {
.cmd = WINDOW_COMMAND_BLUR,
.name = "Blur",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand transpose_cmd = {
.cmd = WINDOW_COMMAND_TRANSPOSE,
.name = "Screens Rel. Pos.",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand screen_offset_cmd = {
.cmd = WINDOW_COMMAND_SCREEN_OFFSET,
.name = "Screen Offset",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand sub_screen_distance_cmd = {
.cmd = WINDOW_COMMAND_SUB_SCREEN_DISTANCE,
.name = "Sub-Screen Distance",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand canvas_x_cmd = {
.cmd = WINDOW_COMMAND_CANVAS_X,
.name = "Canvas X Pos.",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand canvas_y_cmd = {
.cmd = WINDOW_COMMAND_CANVAS_Y,
.name = "Canvas Y Pos.",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand rot_inc_cmd = {
.cmd = WINDOW_COMMAND_ROT_INC,
.name = "Rotate Screens Right",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand rot_dec_cmd = {
.cmd = WINDOW_COMMAND_ROT_DEC,
.name = "Rotate Screens Left",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand rot_top_inc_cmd = {
.cmd = WINDOW_COMMAND_ROT_TOP_INC,
.name = "Rot. Top Screen Right",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand rot_top_dec_cmd = {
.cmd = WINDOW_COMMAND_ROT_TOP_DEC,
.name = "Rot. Top Screen Left",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand rot_bot_inc_cmd = {
.cmd = WINDOW_COMMAND_ROT_BOT_INC,
.name = "Rot. Bot. Screen Right",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand rot_bot_dec_cmd = {
.cmd = WINDOW_COMMAND_ROT_BOT_DEC,
.name = "Rot. Bot. Screen Left",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand padding_cmd = {
.cmd = WINDOW_COMMAND_PADDING,
.name = "Extra Padding",
.usable_always = false,
.available_mono_extra = false,
};

static const WindowCommand top_par_cmd = {
.cmd = WINDOW_COMMAND_TOP_PAR,
.name = "Top Screen PAR",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand bot_par_cmd = {
.cmd = WINDOW_COMMAND_BOT_PAR,
.name = "Bottom Screen PAR",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand audio_mute_cmd = {
.cmd = WINDOW_COMMAND_AUDIO_MUTE,
.name = "Mute Audio",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand volume_inc_cmd = {
.cmd = WINDOW_COMMAND_VOLUME_INC,
.name = "Increase Volume",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand volume_dec_cmd = {
.cmd = WINDOW_COMMAND_VOLUME_DEC,
.name = "Decrease Volume",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand audio_restart_cmd = {
.cmd = WINDOW_COMMAND_AUDIO_RESTART,
.name = "Restart Audio",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand load_startup_cmd = {
.cmd = WINDOW_COMMAND_LOAD_PROFILE_STARTUP,
.name = "Load Initial Profile",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand load_profile_1_cmd = {
.cmd = WINDOW_COMMAND_LOAD_PROFILE_1,
.name = "Load Profile 1",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand load_profile_2_cmd = {
.cmd = WINDOW_COMMAND_LOAD_PROFILE_2,
.name = "Load Profile 2",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand load_profile_3_cmd = {
.cmd = WINDOW_COMMAND_LOAD_PROFILE_3,
.name = "Load Profile 3",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand load_profile_4_cmd = {
.cmd = WINDOW_COMMAND_LOAD_PROFILE_4,
.name = "Load Profile 4",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand save_startup_cmd = {
.cmd = WINDOW_COMMAND_SAVE_PROFILE_STARTUP,
.name = "Save Initial Profile",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand save_profile_1_cmd = {
.cmd = WINDOW_COMMAND_SAVE_PROFILE_1,
.name = "Save Profile 1",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand save_profile_2_cmd = {
.cmd = WINDOW_COMMAND_SAVE_PROFILE_2,
.name = "Save Profile 2",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand save_profile_3_cmd = {
.cmd = WINDOW_COMMAND_SAVE_PROFILE_3,
.name = "Save Profile 3",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand save_profile_4_cmd = {
.cmd = WINDOW_COMMAND_SAVE_PROFILE_4,
.name = "Save Profile 4",
.usable_always = false,
.available_mono_extra = true,
};

static const WindowCommand* possible_cmds[] = {
&none_cmd,
&connect_cmd,
&split_cmd,
&fullscreen_cmd,
&crop_cmd,
&async_cmd,
&vsync_cmd,
&menu_scaling_inc_cmd,
&menu_scaling_dec_cmd,
&window_scaling_inc_cmd,
&window_scaling_dec_cmd,
&ratio_cycle_cmd,
&ratio_top_cmd,
&ratio_bot_cmd,
&blur_cmd,
&transpose_cmd,
&screen_offset_cmd,
&sub_screen_distance_cmd,
&canvas_x_cmd,
&canvas_y_cmd,
&rot_inc_cmd,
&rot_dec_cmd,
&rot_top_inc_cmd,
&rot_top_dec_cmd,
&rot_bot_inc_cmd,
&rot_bot_dec_cmd,
&padding_cmd,
&top_par_cmd,
&bot_par_cmd,
&audio_mute_cmd,
&volume_inc_cmd,
&volume_dec_cmd,
&audio_restart_cmd,
&load_startup_cmd,
&load_profile_1_cmd,
&load_profile_2_cmd,
&load_profile_3_cmd,
&load_profile_4_cmd,
&save_startup_cmd,
&save_profile_1_cmd,
&save_profile_2_cmd,
&save_profile_3_cmd,
&save_profile_4_cmd,
};

void create_window_commands_list(std::vector<const WindowCommand*> &list_to_fill, bool is_mono_extra) {
	for(size_t i = 0; i < NUM_SELECTABLE_WINDOW_COMMANDS; i++)
		if(possible_cmds[i]->available_mono_extra || (!is_mono_extra))
			list_to_fill.push_back(possible_cmds[i]);
}

const WindowCommand* get_window_command(PossibleWindowCommands cmd) {
	for(size_t i = 0; i < NUM_SELECTABLE_WINDOW_COMMANDS; i++)
		if(possible_cmds[i]->cmd == cmd)
			return possible_cmds[i];
	return &none_cmd;
}

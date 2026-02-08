#include "frontend.hpp"
#include "utils.hpp"
#include <cmath>
#include <thread>
#include <SFML/System.hpp>

const CropData default_3ds_crop = {
.top_width = TOP_WIDTH_3DS, .top_height = HEIGHT_3DS,
.top_x = 0, .top_y = 0,
.bot_width = BOT_WIDTH_3DS, .bot_height = HEIGHT_3DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "3DS"};

const CropData special_ds_crop = {
.top_width = TOP_SPECIAL_DS_WIDTH_3DS, .top_height = HEIGHT_3DS,
.top_x = (TOP_WIDTH_3DS - TOP_SPECIAL_DS_WIDTH_3DS) / 2, .top_y = 0,
.bot_width = BOT_WIDTH_3DS, .bot_height = HEIGHT_3DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "16:10"};

const CropData scaled_ds_crop = {
.top_width = TOP_SCALED_DS_WIDTH_3DS, .top_height = HEIGHT_3DS,
.top_x = (TOP_WIDTH_3DS - TOP_SCALED_DS_WIDTH_3DS) / 2, .top_y = 0,
.bot_width = BOT_WIDTH_3DS, .bot_height = HEIGHT_3DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "Scaled DS"};

const CropData native_ds_crop = {
.top_width = WIDTH_DS, .top_height = HEIGHT_DS,
.top_x = (TOP_WIDTH_3DS - WIDTH_DS) / 2, .top_y = 0,
.bot_width = WIDTH_DS, .bot_height = HEIGHT_DS,
.bot_x = (BOT_WIDTH_3DS - WIDTH_DS) / 2, .bot_y = HEIGHT_3DS - HEIGHT_DS,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "Native DS"};

const CropData scaled_gba_crop = {
.top_width = WIDTH_SCALED_GBA, .top_height = HEIGHT_SCALED_GBA,
.top_x = (TOP_WIDTH_3DS - WIDTH_SCALED_GBA) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SCALED_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "Scaled GBA"};

const CropData native_gba_crop = {
.top_width = WIDTH_GBA, .top_height = HEIGHT_GBA,
.top_x = (TOP_WIDTH_3DS - WIDTH_GBA) / 2, .top_y = (HEIGHT_3DS - HEIGHT_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "Native GBA"};

const CropData scaled_vc_gb_crop = {
.top_width = WIDTH_SCALED_GB, .top_height = HEIGHT_SCALED_GB,
.top_x = (TOP_WIDTH_3DS - WIDTH_SCALED_GB) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SCALED_GB) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "Scaled VC GB"};

const CropData vc_gb_crop = {
.top_width = WIDTH_GB, .top_height = HEIGHT_GB,
.top_x = (TOP_WIDTH_3DS - WIDTH_GB) / 2, .top_y = (HEIGHT_3DS - HEIGHT_GB) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "VC GB"};

const CropData scaled_snes_crop = {
.top_width = WIDTH_SCALED_SNES, .top_height = HEIGHT_SCALED_SNES,
.top_x = (TOP_WIDTH_3DS - WIDTH_SCALED_SNES) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SCALED_SNES) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "Scaled SNES"};

const CropData vc_snes_crop = {
.top_width = WIDTH_SNES, .top_height = HEIGHT_SNES,
.top_x = (TOP_WIDTH_3DS - WIDTH_SNES) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SNES) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "VC SNES"};

const CropData vc_nes_crop = {
.top_width = WIDTH_NES, .top_height = HEIGHT_NES,
.top_x = (TOP_WIDTH_3DS - WIDTH_NES) / 2, .top_y = (HEIGHT_3DS - HEIGHT_NES) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = false,
.name = "VC NES"};

const CropData scaled_ds_pokemon_crop = {
.top_width = TOP_SCALED_DS_WIDTH_3DS - 6, .top_height = HEIGHT_3DS - 6,
.top_x = (TOP_WIDTH_3DS - TOP_SCALED_DS_WIDTH_3DS + 6) / 2, .top_y = 3,
.bot_width = BOT_WIDTH_3DS, .bot_height = HEIGHT_3DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = true,
.name = "Scaled Pokémon DS"};

const CropData scaled_ds_pmd_brt_crop = {
.top_width = TOP_SCALED_DS_WIDTH_3DS, .top_height = HEIGHT_3DS,
.top_x = (TOP_WIDTH_3DS - TOP_SCALED_DS_WIDTH_3DS) / 2, .top_y = 0,
.bot_width = BOT_WIDTH_3DS - 3, .bot_height = HEIGHT_3DS,
.bot_x = 3, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = true,
.name = "Scaled PMD:BRT"};

const CropData native_ds_pokemon_crop = {
.top_width = WIDTH_DS - 2, .top_height = HEIGHT_DS - 2,
.top_x = (TOP_WIDTH_3DS - WIDTH_DS + 2) / 2, .top_y = 1,
.bot_width = WIDTH_DS, .bot_height = HEIGHT_DS,
.bot_x = (BOT_WIDTH_3DS - WIDTH_DS) / 2, .bot_y = HEIGHT_3DS - HEIGHT_DS,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = true,
.name = "Native Pokémon DS"};

const CropData native_ds_pmd_brt_crop = {
.top_width = WIDTH_DS, .top_height = HEIGHT_DS,
.top_x = (TOP_WIDTH_3DS - WIDTH_DS) / 2, .top_y = 0,
.bot_width = WIDTH_DS - 1, .bot_height = HEIGHT_DS,
.bot_x = (BOT_WIDTH_3DS - WIDTH_DS + 2) / 2, .bot_y = HEIGHT_3DS - HEIGHT_DS,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = true,
.name = "Native PMD:BRT"};

const CropData scaled_gba_pmd_rrt_crop = {
.top_width = WIDTH_SCALED_GBA - 3, .top_height = HEIGHT_SCALED_GBA,
.top_x = (TOP_WIDTH_3DS - WIDTH_SCALED_GBA + 6) / 2, .top_y = (HEIGHT_3DS - HEIGHT_SCALED_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = true,
.name = "Scaled PMD:RRT"};

const CropData native_gba_pmd_rrt_crop = {
.top_width = WIDTH_GBA - 1, .top_height = HEIGHT_GBA,
.top_x = (TOP_WIDTH_3DS - WIDTH_GBA + 2) / 2, .top_y = (HEIGHT_3DS - HEIGHT_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = true, .allowed_ds = false, .is_game_specific = true,
.name = "Native PMD:RRT"};

const CropData default_ds_crop = {
.top_width = WIDTH_DS, .top_height = HEIGHT_DS,
.top_x = 0, .top_y = 0,
.bot_width = WIDTH_DS, .bot_height = HEIGHT_DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.allowed_3ds = false, .allowed_ds = true, .is_game_specific = false,
.name = "DS"};

const CropData top_gba_ds_crop = {
.top_width = WIDTH_GBA, .top_height = HEIGHT_GBA,
.top_x = (WIDTH_DS - WIDTH_GBA) / 2, .top_y = (HEIGHT_DS - HEIGHT_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = false, .allowed_ds = true, .is_game_specific = false,
.name = "Top GBA"};

const CropData bottom_gba_ds_crop = {
.top_width = 0, .top_height = 0,
.top_x = 0, .top_y = 0,
.bot_width = WIDTH_GBA, .bot_height = HEIGHT_GBA,
.bot_x = (WIDTH_DS - WIDTH_GBA) / 2, .bot_y = (HEIGHT_DS - HEIGHT_GBA) / 2,
.allowed_joint = true, .allowed_top = false, .allowed_bottom = true,
.allowed_3ds = false, .allowed_ds = true, .is_game_specific = false,
.name = "Bottom GBA"};

const CropData ds_pokemon_crop = {
.top_width = WIDTH_DS - 2, .top_height = HEIGHT_DS - 2,
.top_x = 1, .top_y = 1,
.bot_width = WIDTH_DS, .bot_height = HEIGHT_DS,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.allowed_3ds = false, .allowed_ds = true, .is_game_specific = true,
.name = "Pokémon DS"};

const CropData ds_pmd_brt_crop = {
.top_width = WIDTH_DS, .top_height = HEIGHT_DS,
.top_x = 0, .top_y = 0,
.bot_width = WIDTH_DS - 1, .bot_height = HEIGHT_DS,
.bot_x = 1, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = true,
.allowed_3ds = false, .allowed_ds = true, .is_game_specific = true,
.name = "PMD:BRT"};

const CropData top_gba_ds_pmd_rrt_crop = {
.top_width = WIDTH_GBA - 1, .top_height = HEIGHT_GBA,
.top_x = (WIDTH_DS - WIDTH_GBA + 2) / 2, .top_y = (HEIGHT_DS - HEIGHT_GBA) / 2,
.bot_width = 0, .bot_height = 0,
.bot_x = 0, .bot_y = 0,
.allowed_joint = true, .allowed_top = true, .allowed_bottom = false,
.allowed_3ds = false, .allowed_ds = true, .is_game_specific = true,
.name = "Top PMD:RRT"};

const CropData bottom_gba_ds_pmd_rrt_crop = {
.top_width = 0, .top_height = 0,
.top_x = 0, .top_y = 0,
.bot_width = WIDTH_GBA - 1, .bot_height = HEIGHT_GBA,
.bot_x = (WIDTH_DS - WIDTH_GBA + 2) / 2, .bot_y = (HEIGHT_DS - HEIGHT_GBA) / 2,
.allowed_joint = true, .allowed_top = false, .allowed_bottom = true,
.allowed_3ds = false, .allowed_ds = true, .is_game_specific = true,
.name = "Bottom PMD:RRT"};

static const CropData* basic_possible_crops[] = {
&default_3ds_crop,
&special_ds_crop,
&scaled_ds_crop,
&native_ds_crop,
&scaled_gba_crop,
&native_gba_crop,
&scaled_vc_gb_crop,
&vc_gb_crop,
&scaled_snes_crop,
&vc_snes_crop,
&vc_nes_crop,
&default_ds_crop,
&top_gba_ds_crop,
&bottom_gba_ds_crop,
&scaled_ds_pokemon_crop,
&native_ds_pokemon_crop,
&ds_pokemon_crop,
&scaled_ds_pmd_brt_crop,
&native_ds_pmd_brt_crop,
&ds_pmd_brt_crop,
&scaled_gba_pmd_rrt_crop,
&native_gba_pmd_rrt_crop,
&top_gba_ds_pmd_rrt_crop,
&bottom_gba_ds_pmd_rrt_crop,
};

const PARData base_par = {
.width_multiplier = 1.0, .width_divisor = 1.0,
.is_width_main = true, .is_fit = false,
.name = "1:1"};

const PARData snes_horizontal_par = {
.width_multiplier = 8.0, .width_divisor = 7.0,
.is_width_main = true, .is_fit = false,
.name = "SNES Horizontal"};

const PARData snes_vertical_par = {
.width_multiplier = 8.0, .width_divisor = 7.0,
.is_width_main = false, .is_fit = false,
.name = "SNES Vertical"};

const PARData lb_horizontal_par = {
.width_multiplier = 4.0, .width_divisor = 3.0,
.is_width_main = true, .is_fit = false,
.name = "4:3 Horizontal"};

const PARData lb_vertical_par = {
.width_multiplier = 4.0, .width_divisor = 3.0,
.is_width_main = false, .is_fit = false,
.name = "4:3 Vertical"};

const PARData top_3ds_horizontal_par = {
.width_multiplier = 5.0, .width_divisor = 3.0,
.is_width_main = true, .is_fit = false,
.name = "5:3 Horizontal"};

const PARData top_3ds_vertical_par = {
.width_multiplier = 5.0, .width_divisor = 3.0,
.is_width_main = false, .is_fit = false,
.name = "5:3 Vertical"};

const PARData wide_horizontal_par = {
.width_multiplier = 16.0, .width_divisor = 9.0,
.is_width_main = true, .is_fit = false,
.name = "16:9 Horizontal"};

const PARData wide_vertical_par = {
.width_multiplier = 16.0, .width_divisor = 9.0,
.is_width_main = false, .is_fit = false,
.name = "16:9 Vertical"};

const PARData fit_square_horizontal_par = {
.width_multiplier = 1.0, .width_divisor = 1.0,
.is_width_main = true, .is_fit = true,
.name = "Fit to 1:1 H."};

const PARData fit_square_vertical_par = {
.width_multiplier = 1.0, .width_divisor = 1.0,
.is_width_main = false, .is_fit = true,
.name = "Fit to 1:1 V."};

const PARData fit_lb_horizontal_par = {
.width_multiplier = 4.0, .width_divisor = 3.0,
.is_width_main = true, .is_fit = true,
.name = "Fit to 4:3 H."};

const PARData fit_lb_vertical_par = {
.width_multiplier = 4.0, .width_divisor = 3.0,
.is_width_main = false, .is_fit = true,
.name = "Fit to 4:3 V."};

const PARData fit_top_3ds_horizontal_par = {
.width_multiplier = 5.0, .width_divisor = 3.0,
.is_width_main = true, .is_fit = true,
.name = "Fit to 5:3 H."};

const PARData fit_top_3ds_vertical_par = {
.width_multiplier = 5.0, .width_divisor = 3.0,
.is_width_main = false, .is_fit = true,
.name = "Fit to 5:3 V."};

const PARData fit_wide_horizontal_par = {
.width_multiplier = 16.0, .width_divisor = 9.0,
.is_width_main = true, .is_fit = true,
.name = "Fit to 16:9 H."};

const PARData fit_wide_vertical_par = {
.width_multiplier = 16.0, .width_divisor = 9.0,
.is_width_main = false, .is_fit = true,
.name = "Fit to 16:9 V."};

static const PARData* basic_possible_pars[] = {
&base_par,
&snes_horizontal_par,
&snes_vertical_par,
&lb_horizontal_par,
&lb_vertical_par,
&top_3ds_horizontal_par,
&top_3ds_vertical_par,
&wide_horizontal_par,
&wide_vertical_par,
&fit_square_horizontal_par,
&fit_square_vertical_par,
&fit_lb_horizontal_par,
&fit_lb_vertical_par,
&fit_top_3ds_horizontal_par,
&fit_top_3ds_vertical_par,
&fit_wide_horizontal_par,
&fit_wide_vertical_par,
};

// libretro shader values. Credits: hunterk and Pokefan531.
// Based on profi200's open_agb_firm implementation.
// https://github.com/libretro/slang-shaders/tree/master/handheld/shaders/color
const ShaderColorEmulationData color_profile_identity = {
.targetGamma = 1.f, .lum = 1.f,
.rgb_mod = {
{1.f, 0.f, 0.f},
{0.f, 1.f, 0.f},
{0.f, 0.f, 1.f}
},
.displayGamma = 1.f / 1.f,
.is_valid = false,
.name = "No Correction"
};

const ShaderColorEmulationData color_profile_libretro_gba = {
.targetGamma = 2.2f + (0.28125f * 1.6f), .lum = 0.91f,
.rgb_mod = {
{0.905f,  0.195f,  -0.1f},
{0.1f,    0.65f,    0.25f},
{0.1575f, 0.1425f,  0.7f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "GBA"
};

const ShaderColorEmulationData color_profile_libretro_gb_micro = {
.targetGamma = 2.2f, .lum = 0.9f,
.rgb_mod = {
{0.8025f, 0.31f,   -0.1125f},
{0.1f,    0.6875f,  0.2125f},
{0.1225f, 0.1125f,  0.765f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "GB Micro"
};

const ShaderColorEmulationData color_profile_libretro_gba_sp_101 = {
.targetGamma = 2.2f, .lum = 0.935f,
.rgb_mod = {
{0.96f,    0.11f, -0.07f},
{0.0325f,  0.89f,  0.0775f},
{0.001f,  -0.03f,  1.029f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "GBA SP 101"
};

const ShaderColorEmulationData color_profile_libretro_nds = {
.targetGamma = 2.2f, .lum = 0.905f,
.rgb_mod = {
{0.835f, 0.27f,   -0.105f},
{0.1f,   0.6375f,  0.2625f},
{0.105f, 0.175f,   0.72f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "Nintendo DS"
};

const ShaderColorEmulationData color_profile_libretro_nds_lite = {
.targetGamma = 2.2f, .lum = 0.935f,
.rgb_mod = {
{0.93f,   0.14f, -0.07f},
{0.025f,  0.9f,   0.075f},
{0.008f, -0.03f,  1.022f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "Nintendo DS Lite"
};

const ShaderColorEmulationData color_profile_libretro_nso_gba = {
.targetGamma = 2.2f, .lum = 1.f,
.rgb_mod = {
{0.865f,  0.1225f, 0.0125f},
{0.0575f, 0.925f,  0.0125f},
{0.0575f, 0.1225f, 0.82f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "NSO GBA"
};

const ShaderColorEmulationData color_profile_libretro_emulators_gba = {
.targetGamma = 1.45f, .lum = 1.f,
.rgb_mod = {
{0.73f,   0.27f,   0.f},
{0.0825f, 0.6775f, 0.24f},
{0.0825f, 0.24f,   0.6775f}
},
.displayGamma = 1.f / 1.45f,
.is_valid = true,
.name = "Emulators GBA"
};

const ShaderColorEmulationData color_profile_libretro_gbc = {
.targetGamma = 2.2f + (0.5f * (-1.0f)), .lum = 0.91f,
.rgb_mod = {
{0.905f,   0.195f,   -0.1f},
{0.1f,      0.65f,   0.25f},
{0.1575f, 0.1425f,    0.7f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "GBC"
};

const ShaderColorEmulationData color_profile_nso_gbc = {
.targetGamma = 2.2f, .lum = 0.85f,
.rgb_mod = {
{0.84f,  0.265f, 0.f},
{0.105f, 0.67f,  0.24f},
{0.15f,  0.30f,  0.525f}
},
.displayGamma = 1.f / 2.2f,
.is_valid = true,
.name = "NSO GBC"
};

static const ShaderColorEmulationData* basic_possible_color_profiles[] = {
&color_profile_identity,
&color_profile_libretro_nds,
&color_profile_libretro_nds_lite,
&color_profile_libretro_gba,
&color_profile_libretro_gb_micro,
&color_profile_libretro_gba_sp_101,
&color_profile_libretro_nso_gba,
&color_profile_libretro_emulators_gba,
&color_profile_libretro_gbc,
&color_profile_nso_gbc,
};

static bool is_allowed_crop(const CropData* crop_data, ScreenType s_type, bool is_ds, bool allow_game_specific) {
	if(is_ds && (!crop_data->allowed_ds))
		return false;
	if((!is_ds) && (!crop_data->allowed_3ds))
		return false;
	if((!allow_game_specific) && (crop_data->is_game_specific))
		return false;
	if((s_type == ScreenType::JOINT) && (!crop_data->allowed_joint))
		return false;
	if((s_type == ScreenType::TOP) && (!crop_data->allowed_top))
		return false;
	if((s_type == ScreenType::BOTTOM) && (!crop_data->allowed_bottom))
		return false;
	return true;
}

void insert_basic_crops(std::vector<const CropData*> &crop_vector, ScreenType s_type, bool is_ds, bool allow_game_specific) {
	for(size_t i = 0; i < (sizeof(basic_possible_crops) / sizeof(basic_possible_crops[0])); i++) {
		if(is_allowed_crop(basic_possible_crops[i], s_type, is_ds, allow_game_specific))
			crop_vector.push_back(basic_possible_crops[i]);
	}
}

void insert_basic_pars(std::vector<const PARData*> &par_vector) {
	for(size_t i = 0; i < (sizeof(basic_possible_pars) / sizeof(basic_possible_pars[0])); i++) {
		par_vector.push_back(basic_possible_pars[i]);
	}
}

void insert_basic_color_profiles(std::vector<const ShaderColorEmulationData*> &color_profiles_vector) {
	for(size_t i = 0; i < (sizeof(basic_possible_color_profiles) / sizeof(basic_possible_color_profiles[0])); i++) {
		color_profiles_vector.push_back(basic_possible_color_profiles[i]);
	}
}

void reset_display_data(DisplayData* display_data) {
	display_data->last_connected_ds = false;
	display_data->interleaved_3d = false;
}

void reset_input_data(InputData* input_data) {
	input_data->fast_poll = false;
	input_data->enable_controller_input = true;
	input_data->enable_keyboard_input = true;
	input_data->enable_mouse_input = true;
	input_data->enable_buttons_input = true;
	input_data->extra_button_shortcuts.enter_shortcut = get_window_command(WINDOW_COMMAND_RATIO_CYCLE);
	input_data->extra_button_shortcuts.page_up_shortcut = get_window_command(WINDOW_COMMAND_CROP);
}

void reset_shared_data(SharedData* shared_data) {
	reset_input_data(&shared_data->input_data);
	shared_data->periodic_connection_try = false;
}

void reset_fullscreen_info(ScreenInfo &info) {
	info.fullscreen_mode_width = 0;
	info.fullscreen_mode_height = 0;
	info.fullscreen_mode_bpp = 0;
}

void sanitize_enabled_info(ScreenInfo &top_bot_info, ScreenInfo &top_info, ScreenInfo &bot_info) {
	if((top_bot_info.window_enabled) || (top_info.window_enabled) || (bot_info.window_enabled))
		return;
	top_bot_info.window_enabled = true;
}

void reset_screen_info(ScreenInfo &info) {
	info.is_blurred = false;
	info.crop_kind = 0;
	info.crop_kind_ds = 0;
	info.allow_games_crops = false;
	info.scaling = DEFAULT_WINDOW_SCALING_VALUE;
	info.is_fullscreen = false;
	info.bottom_pos = UNDER_TOP;
	info.second_screen_pos = RIGHT_FIRST;
	info.subscreen_offset = 0.5;
	info.subscreen_attached_offset = 0.0;
	info.total_offset_x = 0.5;
	info.total_offset_y = 0.5;
	info.top_rotation = 0;
	info.bot_rotation = 0;
	info.show_mouse = true;
	info.v_sync_enabled = false;
	#if (defined(RASPI) || defined(ANDROID_COMPILATION))
	info.async = false;
	#else
	info.async = true;
	#endif
	info.top_scaling = DEFAULT_NO_SCALING_VALUE;
	info.bot_scaling = DEFAULT_NO_SCALING_VALUE;
	info.force_same_scaling = false;
	info.non_integer_top_scaling = DEFAULT_NO_SCALING_VALUE;
	info.non_integer_bot_scaling = DEFAULT_NO_SCALING_VALUE;
	info.bfi = false;
	info.bfi_divider = 2;
	info.bfi_amount = 1;
	info.menu_scaling_factor = 1.0;
	info.rounded_corners_fix = false;
	info.top_par = 0;
	info.bot_par = 0;
	reset_fullscreen_info(info);
	info.non_integer_mode = SMALLER_PRIORITY;
	info.use_non_integer_scaling_top = false;
	info.use_non_integer_scaling_bottom = false;
	info.failed_fullscreen = false;
	info.have_titlebar = true;
	info.top_color_correction = 0;
	info.bot_color_correction = 0;
	info.in_colorspace_top = FULL_COLORSPACE;
	info.in_colorspace_bot = FULL_COLORSPACE;
	info.frame_blending_top = NO_FRAME_BLENDING;
	info.frame_blending_bot = NO_FRAME_BLENDING;
	info.window_enabled = false;
	info.initial_pos_x = DEFAULT_NO_POS_WINDOW_VALUE;
	info.initial_pos_y = DEFAULT_NO_POS_WINDOW_VALUE;
	info.separator_pixel_size = DEFAULT_SEP_SIZE;
	info.separator_windowed_multiplier = SEP_FOLLOW_SCALING_MULTIPLIER;
	info.separator_fullscreen_multiplier = DEFAULT_WINDOW_SCALING_VALUE;
	info.squish_3d_top = false;
	info.squish_3d_bot = false;
	info.match_bottom_pos_and_second_screen_pos = false;
}

void override_set_data_to_screen_info(override_win_data &override_win, ScreenInfo &info) {
	info.initial_pos_x = override_win.pos_x;
	info.initial_pos_y = override_win.pos_y;
	if(override_win.enabled != DEFAULT_NO_ENABLED_VALUE)
		info.window_enabled = override_win.enabled;
	if((override_win.scaling != DEFAULT_NO_SCALING_VALUE) && (override_win.scaling >= MIN_WINDOW_SCALING_VALUE) && (override_win.scaling <= MAX_WINDOW_SCALING_VALUE))
		info.scaling = override_win.scaling;
}

static InputColorspaceMode input_colorspace_sanitization(int value) {
	if((value < 0) || (value >= INPUT_COLORSPACE_END))
		return FULL_COLORSPACE;
	return static_cast<InputColorspaceMode>(value);
}

static FrameBlendingMode frame_blending_sanitization(int value) {
	if((value < 0) || (value >= FRAME_BLENDING_END))
		return NO_FRAME_BLENDING;
	return static_cast<FrameBlendingMode>(value);
}

static float offset_sanitization(float value) {
	if(value <= 0.0)
		return 0.0;
	if(value >= 1.0)
		return 1.0;
	return value;
}

bool load_screen_info(std::string key, std::string value, std::string base, ScreenInfo &info) {
	if(key == (base + "blur")) {
		info.is_blurred = std::stoi(value);
		return true;
	}
	if(key == (base + "crop")) {
		info.crop_kind = std::stoi(value);
		return true;
	}
	if(key == (base + "crop_ds")) {
		info.crop_kind_ds = std::stoi(value);
		return true;
	}
	if(key == (base + "allow_games_crops")) {
		info.allow_games_crops = std::stoi(value);
		return true;
	}
	if(key == (base + "scale")) {
		info.scaling = std::stod(value);
		if(info.scaling < MIN_WINDOW_SCALING_VALUE)
			info.scaling = MIN_WINDOW_SCALING_VALUE;
		if(info.scaling > MAX_WINDOW_SCALING_VALUE)
			info.scaling = MAX_WINDOW_SCALING_VALUE;
		return true;
	}
	if(key == (base + "fullscreen")) {
		info.is_fullscreen = std::stoi(value);
		return true;
	}
	if(key == (base + "bot_pos")) {
		info.bottom_pos = static_cast<BottomRelativePosition>(std::stoi(value) % BottomRelativePosition::BOT_REL_POS_END);
		return true;
	}
	if(key == (base + "sub_off")) {
		info.subscreen_offset = offset_sanitization(std::stof(value));
		return true;
	}
	if(key == (base + "sub_att_off")) {
		info.subscreen_attached_offset = offset_sanitization(std::stof(value));
		return true;
	}
	if(key == (base + "off_x")) {
		info.total_offset_x = offset_sanitization(std::stof(value));
		return true;
	}
	if(key == (base + "off_y")) {
		info.total_offset_y = offset_sanitization(std::stof(value));
		return true;
	}
	if(key == (base + "top_rot")) {
		info.top_rotation = std::stoi(value);
		info.top_rotation %= 360;
		info.top_rotation += (info.top_rotation < 0) ? 360 : 0;
		return true;
	}
	if(key == (base + "bot_rot")) {
		info.bot_rotation = std::stoi(value);
		info.bot_rotation %= 360;
		info.bot_rotation += (info.bot_rotation < 0) ? 360 : 0;
		return true;
	}
	if(key == (base + "vsync")) {
		info.v_sync_enabled = std::stoi(value);
		return true;
	}
	if(key == (base + "async")) {
		info.async = std::stoi(value);
		return true;
	}
	if(key == (base + "top_scaling")) {
		info.top_scaling = std::stoi(value);
		info.non_integer_top_scaling = (float)info.top_scaling;
		return true;
	}
	if(key == (base + "bot_scaling")) {
		info.bot_scaling = std::stoi(value);
		info.non_integer_bot_scaling = (float)info.bot_scaling;
		return true;
	}
	if(key == (base + "bfi")) {
		info.bfi = std::stoi(value);
		return true;
	}
	if(key == (base + "bfi_divider")) {
		info.bfi_divider = std::stoi(value);
		if(info.bfi_divider < 2)
			info.bfi_divider = 2;
		if(info.bfi_divider > 10)
			info.bfi_divider = 10;
		if(info.bfi_amount > (info.bfi_divider - 1))
			info.bfi_amount = info.bfi_divider - 1;
		return true;
	}
	if(key == (base + "bfi_amount")) {
		info.bfi_amount = std::stoi(value);
		if(info.bfi_amount < 1)
			info.bfi_amount = 1;
		if(info.bfi_amount > (info.bfi_divider - 1))
			info.bfi_amount = info.bfi_divider - 1;
		return true;
	}
	if(key == (base + "menu_scaling_factor")) {
		info.menu_scaling_factor = std::stod(value);
		if(info.menu_scaling_factor < 0.3)
			info.menu_scaling_factor = 0.3;
		if(info.menu_scaling_factor > 10.0)
			info.menu_scaling_factor = 10.0;
		return true;
	}
	if(key == (base + "rounded_corners_fix")) {
		info.rounded_corners_fix = std::stoi(value);
		return true;
	}
	if(key == (base + "force_same_scaling")) {
		info.force_same_scaling = std::stoi(value);
		return true;
	}
	if(key == (base + "separator_pixel_size")) {
		info.separator_pixel_size = std::stoi(value);
		if(info.separator_pixel_size > MAX_SEP_SIZE)
			info.separator_pixel_size = MAX_SEP_SIZE;
		if(info.separator_pixel_size < 0)
			info.separator_pixel_size = 0;
		return true;
	}
	if(key == (base + "separator_windowed_multiplier")) {
		info.separator_windowed_multiplier = std::stof(value);
		if(info.separator_windowed_multiplier < SEP_WINDOW_SCALING_MIN_MULTIPLIER)
			info.separator_windowed_multiplier = SEP_WINDOW_SCALING_MIN_MULTIPLIER;
		if(info.separator_windowed_multiplier > MAX_WINDOW_SCALING_VALUE)
			info.separator_windowed_multiplier = MAX_WINDOW_SCALING_VALUE;
		info.separator_windowed_multiplier = (float)(std::round(info.separator_windowed_multiplier / WINDOW_SCALING_CHANGE) * WINDOW_SCALING_CHANGE);
		return true;
	}
	if(key == (base + "separator_fullscreen_multiplier")) {
		info.separator_fullscreen_multiplier = std::stof(value);
		if(info.separator_fullscreen_multiplier < SEP_FULLSCREEN_SCALING_MIN_MULTIPLIER)
			info.separator_fullscreen_multiplier = SEP_FULLSCREEN_SCALING_MIN_MULTIPLIER;
		if(info.separator_fullscreen_multiplier > MAX_WINDOW_SCALING_VALUE)
			info.separator_fullscreen_multiplier = MAX_WINDOW_SCALING_VALUE;
		info.separator_fullscreen_multiplier = (float)(std::round(info.separator_fullscreen_multiplier / WINDOW_SCALING_CHANGE) * WINDOW_SCALING_CHANGE);
		return true;
	}
	if(key == (base + "top_par")) {
		info.top_par = std::stoi(value);
		return true;
	}
	if(key == (base + "bot_par")) {
		info.bot_par = std::stoi(value);
		return true;
	}
	if(key == (base + "fullscreen_mode_width")) {
		info.fullscreen_mode_width = std::stoi(value);
		return true;
	}
	if(key == (base + "fullscreen_mode_height")) {
		info.fullscreen_mode_height = std::stoi(value);
		return true;
	}
	if(key == (base + "fullscreen_mode_bpp")) {
		info.fullscreen_mode_bpp = std::stoi(value);
		return true;
	}
	if(key == (base + "non_integer_mode")) {
		int read_value = std::stoi(value);
		if((read_value < 0) || (read_value >= END_NONINT_SCALE_MODES))
			read_value = 0;
		info.non_integer_mode = static_cast<NonIntegerScalingModes>(read_value);
		return true;
	}
	if(key == (base + "use_non_integer_scaling_top")) {
		info.use_non_integer_scaling_top = std::stoi(value);
		return true;
	}
	if(key == (base + "use_non_integer_scaling_bottom")) {
		info.use_non_integer_scaling_bottom = std::stoi(value);
		return true;
	}
	if(key == (base + "have_titlebar")) {
		info.have_titlebar = std::stoi(value);
		return true;
	}
	if(key == (base + "top_color_correction")) {
		info.top_color_correction = std::stoi(value);
		return true;
	}
	if(key == (base + "bot_color_correction")) {
		info.bot_color_correction = std::stoi(value);
		return true;
	}
	if(key == (base + "in_colorspace_top")) {
		info.in_colorspace_top = input_colorspace_sanitization(std::stoi(value));
		return true;
	}
	if(key == (base + "in_colorspace_bot")) {
		info.in_colorspace_bot = input_colorspace_sanitization(std::stoi(value));
		return true;
	}
	if(key == (base + "frame_blending_top")) {
		info.frame_blending_top = frame_blending_sanitization(std::stoi(value));
		return true;
	}
	if(key == (base + "frame_blending_bot")) {
		info.frame_blending_bot = frame_blending_sanitization(std::stoi(value));
		return true;
	}
	if(key == (base + "window_enabled")) {
		info.window_enabled = std::stoi(value);
		return true;
	}
	if(key == (base + "second_screen_pos")) {
		info.second_screen_pos = static_cast<SecondScreen3DRelativePosition>(std::stoi(value) % SecondScreen3DRelativePosition::SECOND_SCREEN_3D_REL_POS_END);
		return true;
	}
	if(key == (base + "squish_3d_top")) {
		info.squish_3d_top = std::stoi(value);
		return true;
	}
	if(key == (base + "squish_3d_bot")) {
		info.squish_3d_bot = std::stoi(value);
		return true;
	}
	if(key == (base + "match_bottom_pos_and_second_screen_pos")) {
		info.match_bottom_pos_and_second_screen_pos = std::stoi(value);
		return true;
	}
	return false;
}

std::string save_screen_info(std::string base, const ScreenInfo &info) {
	std::string out = "";
	out += base + "blur=" + std::to_string(info.is_blurred) + "\n";
	out += base + "crop=" + std::to_string(info.crop_kind) + "\n";
	out += base + "crop_ds=" + std::to_string(info.crop_kind_ds) + "\n";
	out += base + "allow_games_crops=" + std::to_string(info.allow_games_crops) + "\n";
	out += base + "scale=" + std::to_string(info.scaling) + "\n";
	out += base + "fullscreen=" + std::to_string(info.is_fullscreen) + "\n";
	out += base + "bot_pos=" + std::to_string(info.bottom_pos) + "\n";
	out += base + "sub_off=" + std::to_string(info.subscreen_offset) + "\n";
	out += base + "sub_att_off=" + std::to_string(info.subscreen_attached_offset) + "\n";
	out += base + "off_x=" + std::to_string(info.total_offset_x) + "\n";
	out += base + "off_y=" + std::to_string(info.total_offset_y) + "\n";
	out += base + "top_rot=" + std::to_string(info.top_rotation) + "\n";
	out += base + "bot_rot=" + std::to_string(info.bot_rotation) + "\n";
	out += base + "vsync=" + std::to_string(info.v_sync_enabled) + "\n";
	out += base + "async=" + std::to_string(info.async) + "\n";
	out += base + "top_scaling=" + std::to_string(info.top_scaling) + "\n";
	out += base + "bot_scaling=" + std::to_string(info.bot_scaling) + "\n";
	out += base + "bfi=" + std::to_string(info.bfi) + "\n";
	out += base + "bfi_divider=" + std::to_string(info.bfi_divider) + "\n";
	out += base + "bfi_amount=" + std::to_string(info.bfi_amount) + "\n";
	out += base + "menu_scaling_factor=" + std::to_string(info.menu_scaling_factor) + "\n";
	out += base + "rounded_corners_fix=" + std::to_string(info.rounded_corners_fix) + "\n";
	out += base + "top_par=" + std::to_string(info.top_par) + "\n";
	out += base + "bot_par=" + std::to_string(info.bot_par) + "\n";
	out += base + "fullscreen_mode_width=" + std::to_string(info.fullscreen_mode_width) + "\n";
	out += base + "fullscreen_mode_height=" + std::to_string(info.fullscreen_mode_height) + "\n";
	out += base + "fullscreen_mode_bpp=" + std::to_string(info.fullscreen_mode_bpp) + "\n";
	out += base + "non_integer_mode=" + std::to_string(info.non_integer_mode) + "\n";
	out += base + "use_non_integer_scaling_top=" + std::to_string(info.use_non_integer_scaling_top) + "\n";
	out += base + "use_non_integer_scaling_bottom=" + std::to_string(info.use_non_integer_scaling_bottom) + "\n";
	out += base + "have_titlebar=" + std::to_string(info.have_titlebar) + "\n";
	out += base + "top_color_correction=" + std::to_string(info.top_color_correction) + "\n";
	out += base + "bot_color_correction=" + std::to_string(info.bot_color_correction) + "\n";
	out += base + "in_colorspace_top=" + std::to_string(info.in_colorspace_top) + "\n";
	out += base + "in_colorspace_bot=" + std::to_string(info.in_colorspace_bot) + "\n";
	out += base + "frame_blending_top=" + std::to_string(info.frame_blending_top) + "\n";
	out += base + "frame_blending_bot=" + std::to_string(info.frame_blending_bot) + "\n";
	out += base + "window_enabled=" + std::to_string(info.window_enabled) + "\n";
	out += base + "force_same_scaling=" + std::to_string(info.force_same_scaling) + "\n";
	out += base + "separator_pixel_size=" + std::to_string(info.separator_pixel_size) + "\n";
	out += base + "separator_windowed_multiplier=" + std::to_string(info.separator_windowed_multiplier) + "\n";
	out += base + "separator_fullscreen_multiplier=" + std::to_string(info.separator_fullscreen_multiplier) + "\n";
	out += base + "second_screen_pos=" + std::to_string(info.second_screen_pos) + "\n";
	out += base + "squish_3d_top=" + std::to_string(info.squish_3d_top) + "\n";
	out += base + "squish_3d_bot=" + std::to_string(info.squish_3d_bot) + "\n";
	out += base + "match_bottom_pos_and_second_screen_pos=" + std::to_string(info.match_bottom_pos_and_second_screen_pos) + "\n";
	return out;
}

void joystick_axis_poll(std::queue<SFEvent> &events_queue) {
	for(unsigned int i = 0; i < sf::Joystick::Count; i++) {
		if(!sf::Joystick::isConnected(i))
			continue;

		for(unsigned int j = 0; j < sf::Joystick::AxisCount; j++) {
			sf::Joystick::Axis axis = sf::Joystick::Axis((unsigned int)sf::Joystick::Axis::X + j);
			if(sf::Joystick::hasAxis(i, axis))
				events_queue.emplace(i, axis, sf::Joystick::getAxisPosition(i, axis));
		}
	}
}

void joystick_print_all() {
	int num_connected = 0;

	for(unsigned int i = 0; i < sf::Joystick::Count; i++) {
		if(!sf::Joystick::isConnected(i))
			continue;
		num_connected += 1;
	}
	
	ActualConsoleOutText("Connected Joysticks: " + std::to_string(num_connected));

	for(unsigned int i = 0; i < sf::Joystick::Count; i++) {
		if(!sf::Joystick::isConnected(i))
			continue;

		ActualConsoleOutText("Joystick " + std::to_string(i + 1) + ":");
		sf::Joystick::Identification joy_details = sf::Joystick::getIdentification(i);
		ActualConsoleOutText("\tName: " + joy_details.name);
		ActualConsoleOutText("\tVID: " + std::to_string(joy_details.vendorId));
		ActualConsoleOutText("\tPID: " + std::to_string(joy_details.productId));
	}
}

const PARData* get_base_par() {
	return basic_possible_pars[0];
}

void get_par_size(int &width, int &height, float multiplier_factor, const PARData *correction_factor, bool divide_3d_par) {
	width = (int)(width * multiplier_factor);
	height = (int)(height * multiplier_factor);
	float correction_factor_3d = 1.0;
	if(divide_3d_par)
		correction_factor_3d = 2.0;
	float correction_factor_divisor = correction_factor->width_multiplier;
	if(correction_factor->is_width_main)
		correction_factor_divisor = correction_factor->width_divisor;
	float correction_factor_approx_contribute = correction_factor_divisor / 2;
	if(correction_factor->is_fit) {
		if(correction_factor->is_width_main)
			width = (int)(((height * correction_factor->width_multiplier) + correction_factor_approx_contribute) / correction_factor_divisor);
		else {
			width = (int)(width / correction_factor_3d);
			height = (int)(((width * correction_factor->width_divisor) + correction_factor_approx_contribute) / correction_factor_divisor);
		}
	}
	else {
		if(correction_factor->is_width_main)
			width = (int)(((width * correction_factor->width_multiplier) + correction_factor_approx_contribute) / correction_factor_divisor);
		else
			height = (int)(((height * correction_factor->width_divisor) + correction_factor_approx_contribute) / correction_factor_divisor);
		width = (int)(width / correction_factor_3d);
	}
}

void get_par_size(float &width, float &height, float multiplier_factor, const PARData *correction_factor, bool divide_3d_par) {
	width *= multiplier_factor;
	height *= multiplier_factor;
	float correction_factor_3d = 1.0;
	if(divide_3d_par)
		correction_factor_3d = 2.0;
	float correction_factor_divisor = correction_factor->width_multiplier;
	if(correction_factor->is_width_main)
		correction_factor_divisor = correction_factor->width_divisor;
	if(correction_factor->is_fit) {
		if(correction_factor->is_width_main)
			width = (height * correction_factor->width_multiplier) / correction_factor_divisor;
		else {
			width /= correction_factor_3d;
			height = (width * correction_factor->width_divisor) / correction_factor_divisor;
		}
	}
	else {
		if(correction_factor->is_width_main)
			width = (width * correction_factor->width_multiplier) / correction_factor_divisor;
		else
			height = (height * correction_factor->width_divisor) / correction_factor_divisor;
		width /= correction_factor_3d;
	}
}

SecondScreen3DRelativePosition get_second_screen_pos(ScreenInfo* info, ScreenType stype) {
	if(info->match_bottom_pos_and_second_screen_pos && (stype == ScreenType::JOINT)) {
		switch(info->bottom_pos) {
			case UNDER_TOP:
				return RIGHT_FIRST;
			case ABOVE_TOP:
				return LEFT_FIRST;
			case RIGHT_TOP:
				return UNDER_FIRST;
			case LEFT_TOP:
				return ABOVE_FIRST;
			default:
				return RIGHT_FIRST;
		}
	}
	return info->second_screen_pos;
}

JoystickDirection get_joystick_direction(uint32_t joystickId, sf::Joystick::Axis axis, float position) {
	bool is_horizontal = false;
	if((axis == sf::Joystick::Axis::Z) || (axis == sf::Joystick::Axis::R))
		return JOY_DIR_NONE;
	if((axis == sf::Joystick::Axis::X) || (axis == sf::Joystick::Axis::U) || (axis == sf::Joystick::Axis::PovX))
		is_horizontal = true;
	int direction = 0;
	if(position >= 90.0)
		direction = 1;
	if(position <= -90.0)
		direction = -1;
	if(direction == 0)
		return JOY_DIR_NONE;
	if(direction > 0) {
		if(is_horizontal)
			return JOY_DIR_RIGHT;
		return JOY_DIR_DOWN;
	}
	if(is_horizontal)
		return JOY_DIR_LEFT;
	return JOY_DIR_UP;
}

JoystickAction get_joystick_action(uint32_t joystickId, uint32_t joy_button) {
	if((joy_button == 0) || (joy_button == 1))
		return JOY_ACTION_CONFIRM;
	if((joy_button == 2) || (joy_button == 3))
		return JOY_ACTION_NEGATE;
	if((joy_button == 6) || (joy_button == 7))
		return JOY_ACTION_MENU;
	return JOY_ACTION_NONE;
}

bool should_do_output(FrontendData* frontend_data) {
	#ifndef ANDROID_COMPILATION
	return (frontend_data->joint_screen->m_info.window_enabled) || (frontend_data->bot_screen->m_info.window_enabled) || (frontend_data->top_screen->m_info.window_enabled);
	#else
	if(frontend_data->joint_screen->m_info.window_enabled)
		return (!frontend_data->joint_screen->is_open()) || frontend_data->joint_screen->has_focus();
	if(frontend_data->bot_screen->m_info.window_enabled)
		return (!frontend_data->bot_screen->is_open()) || frontend_data->bot_screen->has_focus();
	if(frontend_data->top_screen->m_info.window_enabled)
		return (!frontend_data->top_screen->is_open()) || frontend_data->top_screen->has_focus();
	return false;
	#endif
}

void update_output(FrontendData* frontend_data, double frame_time, VideoOutputData *out_buf, InputVideoDataType video_data_type, bool update_rendered_buffer) {
	if(frontend_data->reload) {
		frontend_data->top_screen->reload();
		frontend_data->bot_screen->reload();
		frontend_data->joint_screen->reload();
		frontend_data->reload = false;
	}
	// Make sure the window is closed before showing split/non-split
	if(!frontend_data->joint_screen->m_info.window_enabled)
		frontend_data->joint_screen->draw(frame_time, out_buf, video_data_type, update_rendered_buffer);
	if(!frontend_data->top_screen->m_info.window_enabled)
		frontend_data->top_screen->draw(frame_time, out_buf, video_data_type, update_rendered_buffer);
	if(!frontend_data->bot_screen->m_info.window_enabled)
		frontend_data->bot_screen->draw(frame_time, out_buf, video_data_type, update_rendered_buffer);
	if(frontend_data->joint_screen->m_info.window_enabled)
		frontend_data->joint_screen->draw(frame_time, out_buf, video_data_type, update_rendered_buffer);
	if(frontend_data->top_screen->m_info.window_enabled)
		frontend_data->top_screen->draw(frame_time, out_buf, video_data_type, update_rendered_buffer);
	if(frontend_data->bot_screen->m_info.window_enabled)
		frontend_data->bot_screen->draw(frame_time, out_buf, video_data_type, update_rendered_buffer);
}

static bool are_cc_device_screens_same(const CaptureDevice &old_cc_device, const CaptureDevice &new_cc_device) {
	if(old_cc_device.width != new_cc_device.width)
		return false;
	if(old_cc_device.height != new_cc_device.height)
		return false;
	if(old_cc_device.base_rotation != new_cc_device.base_rotation)
		return false;
	if(old_cc_device.top_screen_x != new_cc_device.top_screen_x)
		return false;
	if(old_cc_device.top_screen_y != new_cc_device.top_screen_y)
		return false;
	if(old_cc_device.bot_screen_x != new_cc_device.bot_screen_x)
		return false;
	if(old_cc_device.bot_screen_y != new_cc_device.bot_screen_y)
		return false;
	return true;
}

void update_connected_3ds_ds(FrontendData* frontend_data, const CaptureDevice &old_cc_device, const CaptureDevice &new_cc_device) {
	bool old_connected = frontend_data->display_data.last_connected_ds;
	frontend_data->display_data.last_connected_ds = !new_cc_device.is_3ds;
	bool changed_type = old_connected != frontend_data->display_data.last_connected_ds;
	bool display_data_updated = !are_cc_device_screens_same(old_cc_device, new_cc_device);
	if(display_data_updated || changed_type) {
		frontend_data->top_screen->update_ds_3ds_connection(changed_type);
		frontend_data->bot_screen->update_ds_3ds_connection(changed_type);
		frontend_data->joint_screen->update_ds_3ds_connection(changed_type);
	}
}

void update_connected_specific_settings(FrontendData* frontend_data, const CaptureDevice &cc_device) {
	if((cc_device.cc_type == CAPTURE_CONN_IS_NITRO) || (cc_device.cc_type == CAPTURE_CONN_CYPRESS_OPTIMIZE) || (cc_device.cc_type == CAPTURE_CONN_PARTNER_CTR)) {
		frontend_data->top_screen->update_capture_specific_settings();
		frontend_data->bot_screen->update_capture_specific_settings();
		frontend_data->joint_screen->update_capture_specific_settings();
	}
}

void default_sleep(float wanted_ms) {
	if(wanted_ms < 0)
		wanted_ms = 1000.0f/USB_CHECKS_PER_SECOND;
	sf::sleep(sf::microseconds((int)(wanted_ms * 1000)));
}

void screen_display_thread(WindowScreen *screen) {
	screen->display_thread();
}

bool is_input_data_valid(InputData* input_data, bool consider_buttons) {
	bool valid_input_mode_found = false;
	valid_input_mode_found = valid_input_mode_found || input_data->enable_controller_input;
	valid_input_mode_found = valid_input_mode_found || input_data->enable_keyboard_input;
	valid_input_mode_found = valid_input_mode_found || input_data->enable_mouse_input;
	valid_input_mode_found = valid_input_mode_found || (consider_buttons && input_data->enable_buttons_input);
	return valid_input_mode_found;
}

std::string get_name_non_int_mode(NonIntegerScalingModes input) {
	std::string output = "";
	switch(input) {
		case SMALLER_PRIORITY:
			output = "Smaller Priority";
			break;
		case EQUAL_PRIORITY:
			output = "Same Priority";
			break;
		case PROPORTIONAL_PRIORITY:
			output = "Prop. Priority";
			break;
		case INVERSE_PROPORTIONAL_PRIORITY:
			output = "Inv. Prop. Priority";
			break;
		case BIGGER_PRIORITY:
			output = "Bigger Priority";
			break;
		default:
			break;
	}
	return output;
}

std::string get_name_frame_blending_mode(FrameBlendingMode input) {
	std::string output = "";
	switch(input) {
		case NO_FRAME_BLENDING:
			output = "Off";
			break;
		case FULL_FRAME_BLENDING:
			output = "On";
			break;
		case DS_3D_BOTH_SCREENS_FRAME_BLENDING:
			output = "3D on DS";
			break;
		default:
			break;
	}
	return output;
}

std::string get_name_input_colorspace_mode(InputColorspaceMode input) {
	std::string output = "";
	switch(input) {
		case FULL_COLORSPACE:
			output = "Full";
			break;
		case DS_COLORSPACE:
			output = "DS";
			break;
		case GBA_COLORSPACE:
			output = "GBA";
			break;
		default:
			break;
	}
	return output;
}

static void ConsoleOutText(std::string full_text, std::string preamble_name) {
	if(full_text != "")
		ActualConsoleOutText("[" + preamble_name + "] " + full_text);
}

void ConsumeOutText(OutTextData &out_text_data, bool update_consumed) {
	if(!out_text_data.consumed)
		ConsoleOutText(out_text_data.full_text, out_text_data.preamble_name);
	if(update_consumed)
		out_text_data.consumed = true;
}

void UpdateOutText(OutTextData &out_text_data, std::string full_text, std::string small_text, TextKind kind) {
	if(!out_text_data.consumed)
		ConsumeOutText(out_text_data);
	out_text_data.full_text = full_text;
	out_text_data.small_text = small_text;
	out_text_data.kind = kind;
	out_text_data.consumed = false;
}

#include "LicenseMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct LicenseMenuOptionInfo {
	const std::string base_name;
};

static const LicenseMenuOptionInfo sfml_license_0_option = {
.base_name = "This software makes use of"};

static const LicenseMenuOptionInfo sfml_license_1_option = {
.base_name = "SFML."};

static const LicenseMenuOptionInfo sfml_license_2_option = {
.base_name = "For its license, and that of the"};

static const LicenseMenuOptionInfo sfml_license_3_option = {
.base_name = "libraries it uses, check:"};

static const LicenseMenuOptionInfo sfml_license_4_option = {
.base_name = "www.sfml-dev.org/license.php"};

static const LicenseMenuOptionInfo ftd3xx_license_0_option = {
.base_name = "This software makes use of"};

#if defined(USE_FTD3) && defined(USE_FTD2_DRIVER)
static const LicenseMenuOptionInfo ftd3xx_license_1_option = {
.base_name = "FTD3XX and FTD2XX."};
#else
#ifdef USE_FTD3
static const LicenseMenuOptionInfo ftd3xx_license_1_option = {
.base_name = "FTD3XX."};
#else
static const LicenseMenuOptionInfo ftd3xx_license_1_option = {
.base_name = "FTD2XX."};
#endif
#endif

static const LicenseMenuOptionInfo ftd3xx_license_2_option = {
.base_name = "For its license, check:"};

static const LicenseMenuOptionInfo ftd3xx_license_3_option = {
.base_name = "ftdichip.com/"};

static const LicenseMenuOptionInfo ftd3xx_license_4_option = {
.base_name = "driver-licence-terms-details/"};

static const LicenseMenuOptionInfo libusb_license_0_option = {
.base_name = "This software makes use of"};

static const LicenseMenuOptionInfo libusb_license_1_option = {
.base_name = "libusb."};

static const LicenseMenuOptionInfo libusb_license_2_option = {
.base_name = "For its license, check:"};

static const LicenseMenuOptionInfo libusb_license_3_option = {
.base_name = "github.com/libusb/"};

static const LicenseMenuOptionInfo libusb_license_4_option = {
.base_name = "libusb?tab=LGPL-2.1-1-ov-file"};

static const LicenseMenuOptionInfo freetype_license_0_option = {
.base_name = "Portions of this software are"};

static const LicenseMenuOptionInfo freetype_license_1_option = {
.base_name = "copyright (c) 2022 The FreeType"};

static const LicenseMenuOptionInfo freetype_license_2_option = {
.base_name = "Project (www.freetype.org)."};

static const LicenseMenuOptionInfo freetype_license_3_option = {
.base_name = "All rights reserved."};

static const LicenseMenuOptionInfo freetype_license_4_option = {
.base_name = ""};

static const LicenseMenuOptionInfo font_license_0_option = {
.base_name = "This software uses the font"};

static const LicenseMenuOptionInfo font_license_1_option = {
.base_name = "OFL Sorts Mill Goudy TT."};

static const LicenseMenuOptionInfo font_license_2_option = {
.base_name = "For its license, check:"};

static const LicenseMenuOptionInfo font_license_3_option = {
.base_name = "openfontlicense.org/"};

static const LicenseMenuOptionInfo font_license_4_option = {
.base_name = "open-font-license-official-text/"};

static const LicenseMenuOptionInfo isng_license_0_option = {
.base_name = "Portions of this software are"};

static const LicenseMenuOptionInfo isng_license_1_option = {
.base_name = "based off Gericom's sample"};

static const LicenseMenuOptionInfo isng_license_2_option = {
.base_name = "IS Nitro Emulator code."};

static const LicenseMenuOptionInfo isng_license_3_option = {
.base_name = "MIT License."};

static const LicenseMenuOptionInfo isng_license_4_option = {
.base_name = "Copyright (c) 2024 Gericom"};

static const LicenseMenuOptionInfo cc3dsfs_license_0_option = {
.base_name = NAME " is distributed under"};

static const LicenseMenuOptionInfo cc3dsfs_license_1_option = {
.base_name = "the MIT License."};

static const LicenseMenuOptionInfo cc3dsfs_license_2_option = {
.base_name = "Copyright (c)"};

static const LicenseMenuOptionInfo cc3dsfs_license_3_option = {
.base_name = "2024 Lorenzooone"};

static const LicenseMenuOptionInfo cc3dsfs_license_4_option = {
.base_name = ""};

static const LicenseMenuOptionInfo* pollable_options[] = {
&sfml_license_0_option,
&sfml_license_1_option,
&sfml_license_2_option,
&sfml_license_3_option,
&sfml_license_4_option,
#if defined(USE_FTD3XX) || defined(USE_FTD2_DRIVER)
&ftd3xx_license_0_option,
&ftd3xx_license_1_option,
&ftd3xx_license_2_option,
&ftd3xx_license_3_option,
&ftd3xx_license_4_option,
#endif
#if defined(USE_LIBUSB) || defined(USE_FTD3XX) || defined(USE_FTD2_DRIVER)
&libusb_license_0_option,
&libusb_license_1_option,
&libusb_license_2_option,
&libusb_license_3_option,
&libusb_license_4_option,
#endif
#if defined(USE_IS_DEVICES_USB)
&isng_license_0_option,
&isng_license_1_option,
&isng_license_2_option,
&isng_license_3_option,
&isng_license_4_option,
#endif
&freetype_license_0_option,
&freetype_license_1_option,
&freetype_license_2_option,
&freetype_license_3_option,
&freetype_license_4_option,
&font_license_0_option,
&font_license_1_option,
&font_license_2_option,
&font_license_3_option,
&font_license_4_option,
&cc3dsfs_license_0_option,
&cc3dsfs_license_1_option,
&cc3dsfs_license_2_option,
&cc3dsfs_license_3_option,
&cc3dsfs_license_4_option,
};

LicenseMenu::LicenseMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

LicenseMenu::~LicenseMenu() {
	delete []this->options_indexes;
}

void LicenseMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Licenses";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void LicenseMenu::insert_data() {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = (int)i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void LicenseMenu::reset_output_option() {
	this->selected_index = LicenseMenuOutAction::LICENSE_MENU_NO_ACTION;
}

void LicenseMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = LICENSE_MENU_BACK;
}

size_t LicenseMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string LicenseMenu::get_string_option(int index, int action) {
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool LicenseMenu::is_option_selectable(int index, int action) {
	return false;
}

void LicenseMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y) {
	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

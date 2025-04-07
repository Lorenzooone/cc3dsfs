#include "SecondScreen3DRelativePositionMenu.hpp"
#include "utils.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct SecondScreen3DRelativePositionMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_selectable;
	const int position_x, position_y, multiplier_y;
	float text_factor_multiplier;
	const SecondScreen3DRelPosMenuOutAction out_action;
	const SecondScreen3DRelativePosition out_position;
	int divisor_x;
	const bool exists_non_joint;
};

static const SecondScreen3DRelativePositionMenuOptionInfo above_option = {
.base_name = "Above", .false_name = "",
.is_selectable = true,
.position_x = 1, .position_y = 1, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM,
.out_position = ABOVE_FIRST,
.divisor_x = 3, .exists_non_joint = true};

static const SecondScreen3DRelativePositionMenuOptionInfo left_option = {
.base_name = "Left", .false_name = "",
.is_selectable = true,
.position_x = 0, .position_y = 2, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM,
.out_position = LEFT_FIRST,
.divisor_x = 3, .exists_non_joint = true};

static const SecondScreen3DRelativePositionMenuOptionInfo right_option = {
.base_name = "Right", .false_name = "",
.is_selectable = true,
.position_x = 2, .position_y = 2, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM,
.out_position = RIGHT_FIRST,
.divisor_x = 3, .exists_non_joint = true};

static const SecondScreen3DRelativePositionMenuOptionInfo below_option = {
.base_name = "Below", .false_name = "",
.is_selectable = true,
.position_x = 1, .position_y = 3, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM,
.out_position = UNDER_FIRST,
.divisor_x = 3, .exists_non_joint = true};

static const SecondScreen3DRelativePositionMenuOptionInfo desc_option = {
.base_name = "3D Screen", .false_name = "",
.is_selectable = false,
.position_x = 1, .position_y = 4, .multiplier_y = 2,
.text_factor_multiplier = 0.95f,
.out_action = SECOND_SCREEN_3D_REL_POS_MENU_NO_ACTION,
.out_position = SECOND_SCREEN_3D_REL_POS_END,
.divisor_x = 3, .exists_non_joint = true};

static const SecondScreen3DRelativePositionMenuOptionInfo desc2_option = {
.base_name = "Position", .false_name = "",
.is_selectable = false,
.position_x = 1, .position_y = 5, .multiplier_y = 2,
.text_factor_multiplier = 0.95f,
.out_action = SECOND_SCREEN_3D_REL_POS_MENU_NO_ACTION,
.out_position = SECOND_SCREEN_3D_REL_POS_END,
.divisor_x = 3, .exists_non_joint = true};

static const SecondScreen3DRelativePositionMenuOptionInfo match_option = {
.base_name = "Set Manual 3D Screen Pos.", .false_name = "Set Automatic 3D Screen Pos.",
.is_selectable = true,
.position_x = 0, .position_y = 0, .multiplier_y = 1,
.text_factor_multiplier = 1.0,
.out_action = SECOND_SCREEN_3D_REL_POS_MENU_TOGGLE_MATCH,
.out_position = SECOND_SCREEN_3D_REL_POS_END,
.divisor_x = 1, .exists_non_joint = false};

static const SecondScreen3DRelativePositionMenuOptionInfo* pollable_options[] = {
&match_option,
&above_option,
&left_option,
&right_option,
&below_option,
&desc_option,
&desc2_option,
};

SecondScreen3DRelativePositionMenu::SecondScreen3DRelativePositionMenu(bool font_load_success, sf::Font &text_font) {	
	this->initialize(font_load_success, text_font);
}

SecondScreen3DRelativePositionMenu::~SecondScreen3DRelativePositionMenu() {
	for(int i = 0; i < this->num_elements_displayed_per_screen; i++)
		delete this->labels[i];
	delete []this->labels;
	delete []this->selectable_labels;
}

void SecondScreen3DRelativePositionMenu::class_setup() {
	this->num_options_per_screen = NUM_TOTAL_MENU_OPTIONS;
	this->min_elements_text_scaling_factor = 4;
	this->num_vertical_slices = 5;
	this->pos_y_subtractor = 0;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "3D Screen Placement";
}

void SecondScreen3DRelativePositionMenu::insert_data(ScreenType stype_out) {
	this->stype = stype_out;
	this->pos_y_subtractor = 0;
	this->num_vertical_slices = 5;
	if(this->stype != ScreenType::JOINT) {
		this->pos_y_subtractor += 1;
		this->num_vertical_slices -= 1;
	}
	this->prepare_options();
}

void SecondScreen3DRelativePositionMenu::reset_output_option() {
	this->selected_index = SECOND_SCREEN_3D_REL_POS_MENU_NO_ACTION;
}

void SecondScreen3DRelativePositionMenu::set_output_option(int index) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = SECOND_SCREEN_3D_REL_POS_MENU_BACK;
	else {
		this->selected_index = pollable_options[index]->out_action;
		this->selected_confirm_value = pollable_options[index]->out_position;
	}
}

bool SecondScreen3DRelativePositionMenu::is_option_selectable(int index) {
	if((this->stype != ScreenType::JOINT) && (!pollable_options[index]->exists_non_joint))
		return false;
	return pollable_options[index]->is_selectable;
}

std::string SecondScreen3DRelativePositionMenu::get_string_option(int index) {
	return pollable_options[index]->base_name;
}

std::string SecondScreen3DRelativePositionMenu::get_false_option(int index) {
	return pollable_options[index]->false_name;
}

void SecondScreen3DRelativePositionMenu::option_slice_prepare(int i, int index, int num_vertical_slices, float text_scaling_factor) {
	if((this->stype != ScreenType::JOINT) && (!pollable_options[i]->exists_non_joint))
		return;
	int pos_y = pollable_options[i]->position_y - (this->pos_y_subtractor * pollable_options[i]->multiplier_y);
	this->prepare_text_slices(pollable_options[i]->position_x, pollable_options[i]->divisor_x, pos_y + (1 * pollable_options[i]->multiplier_y), num_vertical_slices * pollable_options[i]->multiplier_y, index, text_scaling_factor * pollable_options[i]->text_factor_multiplier);
}

bool SecondScreen3DRelativePositionMenu::is_option_element(int option) {
	return (option >= this->elements_start_id) && (option < (this->elements_start_id + this->num_elements_per_screen));
}

bool SecondScreen3DRelativePositionMenu::is_option_left(int index) {
	return (pollable_options[index]->out_action == SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM) && (pollable_options[index]->out_position == LEFT_FIRST);
}

bool SecondScreen3DRelativePositionMenu::is_option_right(int index) {
	return (pollable_options[index]->out_action == SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM) && (pollable_options[index]->out_position == RIGHT_FIRST);
}

bool SecondScreen3DRelativePositionMenu::is_option_above(int index) {
	return (pollable_options[index]->out_action == SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM) && (pollable_options[index]->out_position == ABOVE_FIRST);
}

bool SecondScreen3DRelativePositionMenu::is_option_below(int index) {
	return (pollable_options[index]->out_action == SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM) && (pollable_options[index]->out_position == UNDER_FIRST);
}

std::string SecondScreen3DRelativePositionMenu::setTextOptionBool(int index, bool value) {
	if(value)
		return this->get_string_option(index);
	return this->get_false_option(index);
}

void SecondScreen3DRelativePositionMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo* info) {
	SecondScreen3DRelativePosition curr_2nd_screen_3d_pos = info->second_screen_pos;
	for(int i = 0; i < this->num_options_per_screen; i++) {
		int index = i + this->elements_start_id;
		switch(pollable_options[i]->out_action) {
			case SECOND_SCREEN_3D_REL_POS_MENU_CONFIRM:
				if(pollable_options[i]->out_position == curr_2nd_screen_3d_pos)
					this->labels[index]->setText("<" + this->get_string_option(i) + ">");
				else
					this->labels[index]->setText(this->get_string_option(i));
				break;
			case SECOND_SCREEN_3D_REL_POS_MENU_TOGGLE_MATCH:
				this->labels[index]->setText(this->setTextOptionBool(i, info->match_bottom_pos_and_second_screen_pos));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

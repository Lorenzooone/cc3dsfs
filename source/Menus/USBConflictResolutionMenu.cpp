#include "USBConflictResolutionMenu.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

#ifdef EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS
	#undef EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS
#endif

#if (defined(USE_CYNI_USB) && defined(USE_CYPRESS_OPTIMIZE))
	#define EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS true
#else
	#define EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS false
#endif

struct USBConflictResolutionMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool exists;
	const bool is_selectable;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const USBConflictResolutionMenuOutAction inc_out_action;
	const USBConflictResolutionMenuOutAction out_action;
};

static const USBConflictResolutionMenuOptionInfo warning_nise_opti_conflict_0 = {
.base_name = "There are two capture cards",
.false_name = "", .exists = EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS, .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = USBCONRESO_MENU_NO_ACTION,
.out_action = USBCONRESO_MENU_NO_ACTION};

static const USBConflictResolutionMenuOptionInfo warning_nise_opti_conflict_1 = {
.base_name = "which use the FX2LP board.",
.false_name = "", .exists = EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS, .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = USBCONRESO_MENU_NO_ACTION,
.out_action = USBCONRESO_MENU_NO_ACTION};

static const USBConflictResolutionMenuOptionInfo warning_nise_opti_conflict_2 = {
.base_name = "You may disable one to make",
.false_name = "", .exists = EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS, .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = USBCONRESO_MENU_NO_ACTION,
.out_action = USBCONRESO_MENU_NO_ACTION};

static const USBConflictResolutionMenuOptionInfo warning_nise_opti_conflict_3 = {
.base_name = "accessing the other faster.",
.false_name = "", .exists = EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS, .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = USBCONRESO_MENU_NO_ACTION,
.out_action = USBCONRESO_MENU_NO_ACTION};

static const USBConflictResolutionMenuOptionInfo nisetro_ds_disable_option = {
.base_name = "Disable Nisetro DS(i)",
.false_name = "Enable Nisetro DS(i)", .exists = EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS, .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = USBCONRESO_MENU_NO_ACTION,
.out_action = USBCONRESO_MENU_NISE_DS};

static const USBConflictResolutionMenuOptionInfo optimize_o3ds_disable_option = {
.base_name = "Disable Optimize Old 3DS",
.false_name = "Enable Optimize Old 3DS", .exists = EXISTENCE_VALUE_CONFLICT_NISE_OPTI_O3DS, .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = USBCONRESO_MENU_NO_ACTION,
.out_action = USBCONRESO_MENU_OPTI_O3DS};

static const USBConflictResolutionMenuOptionInfo* pollable_options[] = {
&warning_nise_opti_conflict_0,
&warning_nise_opti_conflict_1,
&warning_nise_opti_conflict_2,
&warning_nise_opti_conflict_3,
&nisetro_ds_disable_option,
&optimize_o3ds_disable_option,
};

USBConflictResolutionMenu::USBConflictResolutionMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
}

USBConflictResolutionMenu::~USBConflictResolutionMenu() {
	delete []this->options_indexes;
}

void USBConflictResolutionMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "USB Conflicts";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

static bool is_option_valid(const USBConflictResolutionMenuOptionInfo* option) {
	bool valid = true;
	valid = valid && option->exists;
	return valid;
}

int USBConflictResolutionMenu::get_total_possible_selectable_inserted() {
	int num_insertable = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		if(is_option_valid(pollable_options[i]) && pollable_options[i]->is_selectable)
			num_insertable++;
	}
	return num_insertable;
}

void USBConflictResolutionMenu::insert_data() {
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		if(is_option_valid(pollable_options[i])) {
			this->options_indexes[this->num_enabled_options] = (int)i;
			this->num_enabled_options++;
		}
	}
	this->prepare_options();
}

void USBConflictResolutionMenu::reset_output_option() {
	this->selected_index = USBConflictResolutionMenuOutAction::USBCONRESO_MENU_NO_ACTION;
}

void USBConflictResolutionMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = USBCONRESO_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

size_t USBConflictResolutionMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string USBConflictResolutionMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool USBConflictResolutionMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool USBConflictResolutionMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

void USBConflictResolutionMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, CaptureStatus* capture_status) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int real_index = start + i;
		int option_index = this->options_indexes[real_index];
		switch(pollable_options[option_index]->out_action) {
			case USBCONRESO_MENU_NISE_DS:
				this->labels[index]->setText(this->setTextOptionBool(real_index, capture_status->devices_allowed_scan[CC_NISETRO_DS]));
				break;
			case USBCONRESO_MENU_OPTI_O3DS:
				this->labels[index]->setText(this->setTextOptionBool(real_index, capture_status->devices_allowed_scan[CC_OPTIMIZE_O3DS]));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

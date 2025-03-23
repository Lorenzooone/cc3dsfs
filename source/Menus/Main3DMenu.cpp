#include "Main3DMenu.hpp"
#include "devicecapture.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

struct Main3DMenuOptionInfo {
	const std::string base_name;
	const std::string false_name;
	const bool is_selectable;
	const bool is_inc;
	const std::string dec_str;
	const std::string inc_str;
	const Main3DMenuOutAction inc_out_action;
	const Main3DMenuOutAction out_action;
	const bool selectable_joint;
	const bool selectable_top;
	const bool selectable_bottom;
};

static const Main3DMenuOptionInfo main_3d_menu_device_option = {
.base_name = "", .false_name = "", .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = MAIN_3D_MENU_NO_ACTION,
.out_action = MAIN_3D_MENU_DEVICE_INFO,
.selectable_joint = true, .selectable_top = true, .selectable_bottom = true};

static const Main3DMenuOptionInfo main_3d_menu_usb_speed_option = {
.base_name = "", .false_name = "", .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = MAIN_3D_MENU_NO_ACTION,
.out_action = MAIN_3D_MENU_USB_SPEED_INFO,
.selectable_joint = true, .selectable_top = true, .selectable_bottom = true};

static const Main3DMenuOptionInfo main_3d_menu_implemented_option = {
.base_name = "", .false_name = "", .is_selectable = false,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = MAIN_3D_MENU_NO_ACTION,
.out_action = MAIN_3D_MENU_IMPLEMENTATION_INFO,
.selectable_joint = true, .selectable_top = true, .selectable_bottom = true};

static const Main3DMenuOptionInfo main_3d_menu_request_3d_option = {
.base_name = "Preference: 3D", .false_name = "Preference: 2D", .is_selectable = true,
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = MAIN_3D_MENU_REQUEST_3D_TOGGLE,
.out_action = MAIN_3D_MENU_REQUEST_3D_TOGGLE,
.selectable_joint = true, .selectable_top = true, .selectable_bottom = true};

static const Main3DMenuOptionInfo main_3d_menu_3d_interleaved_option = {
.base_name = "3D Style: Interleaved", .false_name = "3D Style: Separate", .is_selectable = true,
.is_inc = true, .dec_str = "<", .inc_str = ">", .inc_out_action = MAIN_3D_MENU_INTERLEAVED_TOGGLE,
.out_action = MAIN_3D_MENU_INTERLEAVED_TOGGLE,
.selectable_joint = true, .selectable_top = true, .selectable_bottom = true};

static const Main3DMenuOptionInfo main_3d_menu_squish_top_option = {
.base_name = "Stretch Top Screen", .false_name = "Squish Top Screen", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = MAIN_3D_MENU_NO_ACTION,
.out_action = MAIN_3D_MENU_SQUISH_TOP_TOGGLE,
.selectable_joint = true, .selectable_top = false, .selectable_bottom = false};

static const Main3DMenuOptionInfo main_3d_menu_squish_bottom_option = {
.base_name = "Stretch Bottom Screen", .false_name = "Squish Bottom Screen", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = MAIN_3D_MENU_NO_ACTION,
.out_action = MAIN_3D_MENU_SQUISH_BOTTOM_TOGGLE,
.selectable_joint = true, .selectable_top = false, .selectable_bottom = false};

static const Main3DMenuOptionInfo main_3d_menu_squish_mono_option = {
.base_name = "Stretch Screen", .false_name = "Squish Screen", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = MAIN_3D_MENU_NO_ACTION,
.out_action = MAIN_3D_MENU_SQUISH_TOP_TOGGLE,
.selectable_joint = false, .selectable_top = true, .selectable_bottom = false};

static const Main3DMenuOptionInfo main_3d_menu_second_screen_position_settings_option = {
.base_name = "3D Screen Placement", .false_name = "", .is_selectable = true,
.is_inc = false, .dec_str = "", .inc_str = "", .inc_out_action = MAIN_3D_MENU_NO_ACTION,
.out_action = MAIN_3D_MENU_SECOND_SCREEN_POSITION_SETTINGS,
.selectable_joint = true, .selectable_top = true, .selectable_bottom = false};

static const Main3DMenuOptionInfo* pollable_options[] = {
&main_3d_menu_device_option,
&main_3d_menu_usb_speed_option,
&main_3d_menu_implemented_option,
&main_3d_menu_request_3d_option,
&main_3d_menu_3d_interleaved_option,
&main_3d_menu_squish_top_option,
&main_3d_menu_squish_bottom_option,
&main_3d_menu_squish_mono_option,
&main_3d_menu_second_screen_position_settings_option,
};

Main3DMenu::Main3DMenu(bool font_load_success, sf::Font &text_font) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(font_load_success, text_font);
	this->num_enabled_options = 0;
}

Main3DMenu::~Main3DMenu() {
	delete []this->options_indexes;
}

void Main3DMenu::class_setup() {
	this->num_options_per_screen = 5;
	this->min_elements_text_scaling_factor = num_options_per_screen + 2;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3;
	this->max_width_slack = 1.1;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "3D Settings";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void Main3DMenu::insert_data(ScreenType stype) {
	this->num_enabled_options = 0;
	for(int i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		if((stype == ScreenType::BOTTOM) && (!pollable_options[i]->selectable_bottom))
			continue;
		if((stype == ScreenType::TOP) && (!pollable_options[i]->selectable_top))
			continue;
		if((stype == ScreenType::JOINT) && (!pollable_options[i]->selectable_joint))
			continue;
		this->options_indexes[this->num_enabled_options] = i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void Main3DMenu::reset_output_option() {
	this->selected_index = Main3DMenuOutAction::MAIN_3D_MENU_NO_ACTION;
}

void Main3DMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = MAIN_3D_MENU_BACK;
	else if((action == INC_ACTION) && this->is_option_inc_dec(index))
		this->selected_index = pollable_options[this->options_indexes[index]]->inc_out_action;
	else
		this->selected_index = pollable_options[this->options_indexes[index]]->out_action;
}

int Main3DMenu::get_num_options() {
	return this->num_enabled_options;
}

std::string Main3DMenu::get_string_option(int index, int action) {
	if((action == INC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->inc_str;
	if((action == DEC_ACTION) && this->is_option_inc_dec(index))
		return pollable_options[this->options_indexes[index]]->dec_str;
	if(action == FALSE_ACTION)
		return pollable_options[this->options_indexes[index]]->false_name;
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool Main3DMenu::is_option_selectable(int index, int action) {
	return pollable_options[this->options_indexes[index]]->is_selectable;
}

bool Main3DMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

static std::string get_device_connected_firmware_info(bool is_device_connected, bool is_device_3ds, bool is_firmware_3d) {
	if(!is_device_connected)
		return "No device connected";
	if(!is_device_3ds)
		return "No 3DS connected";
	if(!is_firmware_3d)
		return "FW: No 3D";
	return "FW: 3D";
}

static std::string get_usb_speed_3d_info(bool is_device_3ds, bool is_usb_speed_enough, int speed_device_connected) {
	if(!is_device_3ds)
		return "";
	if(!is_usb_speed_enough)
		return "USB " + std::to_string(speed_device_connected) + ": Slow";
	return "USB " + std::to_string(speed_device_connected) + ": Ok";
}

static std::string get_implementation_info(bool is_device_3ds, bool is_implemented_3d) {
	if(!is_device_3ds)
		return "";
	if(!is_implemented_3d)
		return "3D not implemented";
	return "3D implemented";
}

void Main3DMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, ScreenInfo *info, DisplayData* display_data, CaptureStatus* status) {
	int num_pages = this->get_num_pages();
	if(this->future_data.page >= num_pages)
		this->future_data.page = num_pages - 1;
	int start = this->future_data.page * this->num_options_per_screen;
	bool is_device_connected = status->connected;
	bool is_device_3ds = is_device_connected && status->device.is_3ds;
	int speed_device_connected = get_usb_speed_of_device(status);
	bool is_usb_speed_enough = is_device_3ds && is_usb_speed_of_device_enough_3d(status);
	bool is_implemented_3d = is_device_3ds && get_device_3d_implemented(status);
	bool is_firmware_3d = is_device_3ds && get_device_can_do_3d(status);
	for(int i = 0; i < this->num_options_per_screen + 1; i++) {
		int index = (i * this->single_option_multiplier) + this->elements_start_id;
		if(!this->future_enabled_labels[index])
			continue;
		int real_index = start + i;
		int option_index = this->options_indexes[real_index];
		switch(pollable_options[option_index]->out_action) {
			case MAIN_3D_MENU_DEVICE_INFO:
				this->labels[index]->setText(get_device_connected_firmware_info(is_device_connected, is_device_3ds, is_firmware_3d));
				break;
			case MAIN_3D_MENU_USB_SPEED_INFO:
				this->labels[index]->setText(get_usb_speed_3d_info(is_device_3ds, is_usb_speed_enough, speed_device_connected));
				break;
			case MAIN_3D_MENU_IMPLEMENTATION_INFO:
				this->labels[index]->setText(get_implementation_info(is_device_3ds, is_implemented_3d));
				break;
			case MAIN_3D_MENU_REQUEST_3D_TOGGLE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, status->requested_3d));
				break;
			case MAIN_3D_MENU_INTERLEAVED_TOGGLE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, display_data->interleaved_3d));
				break;
			case MAIN_3D_MENU_SQUISH_TOP_TOGGLE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->squish_3d_top));
				break;
			case MAIN_3D_MENU_SQUISH_BOTTOM_TOGGLE:
				this->labels[index]->setText(this->setTextOptionBool(real_index, info->squish_3d_bot));
				break;
			default:
				break;
		}
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

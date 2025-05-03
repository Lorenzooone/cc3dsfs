#include "StatusMenu.hpp"
#include "devicecapture.hpp"

#define NUM_TOTAL_MENU_OPTIONS (sizeof(pollable_options)/sizeof(pollable_options[0]))

enum StatusMenuID {
	STATUS_MENU_NAME_VERSION,
	STATUS_MENU_URL,
	STATUS_MENU_FPS_IN,
	STATUS_MENU_FPS_POLL,
	STATUS_MENU_FPS_DRAW,
	STATUS_MENU_CONNECTION,
	STATUS_MENU_USB_CONNECTION,
};

struct StatusMenuOptionInfo {
	const std::string base_name;
	bool is_inc;
	const StatusMenuID id;
};

static const StatusMenuOptionInfo status_name_version_option = {
.base_name = NAME, .is_inc = false,
.id = STATUS_MENU_NAME_VERSION};

static const StatusMenuOptionInfo status_url_option = {
.base_name = "github.com/Lorenzooone/cc3dsfs", .is_inc = false,
.id = STATUS_MENU_URL};

static const StatusMenuOptionInfo status_fps_in_option = {
.base_name = "Input FPS:", .is_inc = true,
.id = STATUS_MENU_FPS_IN};

static const StatusMenuOptionInfo status_fps_poll_option = {
.base_name = "Poll FPS:", .is_inc = true,
.id = STATUS_MENU_FPS_POLL};

static const StatusMenuOptionInfo status_fps_draw_option = {
.base_name = "Output FPS:", .is_inc = true,
.id = STATUS_MENU_FPS_DRAW};

static const StatusMenuOptionInfo status_curr_device_option = {
.base_name = "", .is_inc = false,
.id = STATUS_MENU_CONNECTION};

static const StatusMenuOptionInfo status_usb_connection_option = {
.base_name = "", .is_inc = false,
.id = STATUS_MENU_USB_CONNECTION};

static const StatusMenuOptionInfo* pollable_options[] = {
&status_name_version_option,
&status_curr_device_option,
&status_usb_connection_option,
&status_url_option,
&status_fps_in_option,
&status_fps_poll_option,
&status_fps_draw_option,
};

StatusMenu::StatusMenu(TextRectanglePool* text_rectangle_pool) : OptionSelectionMenu(){
	this->options_indexes = new int[NUM_TOTAL_MENU_OPTIONS];
	this->initialize(text_rectangle_pool);
	this->num_enabled_options = 0;
	this->last_update_time = std::chrono::high_resolution_clock::now();
	this->do_update = true;
}

StatusMenu::~StatusMenu() {
	delete []this->options_indexes;
}

void StatusMenu::class_setup() {
	this->num_options_per_screen = 6;
	this->min_elements_text_scaling_factor = num_options_per_screen + 1;
	this->width_factor_menu = 16;
	this->width_divisor_menu = 9;
	this->base_height_factor_menu = 12;
	this->base_height_divisor_menu = 6;
	this->min_text_size = 0.3f;
	this->max_width_slack = 1.1f;
	this->menu_color = sf::Color(30, 30, 60, 192);
	this->title = "Status";
	this->show_back_x = true;
	this->show_x = false;
	this->show_title = true;
}

void StatusMenu::insert_data() {
	this->last_update_time = std::chrono::high_resolution_clock::now();
	this->do_update = true;
	this->num_enabled_options = 0;
	for(size_t i = 0; i < NUM_TOTAL_MENU_OPTIONS; i++) {
		this->options_indexes[this->num_enabled_options] = (int)i;
		this->num_enabled_options++;
	}
	this->prepare_options();
}

void StatusMenu::reset_output_option() {
	this->selected_index = StatusMenuOutAction::STATUS_MENU_NO_ACTION;
}

void StatusMenu::set_output_option(int index, int action) {
	if(index == BACK_X_OUTPUT_OPTION)
		this->selected_index = STATUS_MENU_BACK;
}

size_t StatusMenu::get_num_options() {
	return this->num_enabled_options;
}

bool StatusMenu::is_option_inc_dec(int index) {
	return pollable_options[this->options_indexes[index]]->is_inc;
}

std::string StatusMenu::get_string_option(int index, int action) {
	if(action == DEC_ACTION)
		return "";
	if(action == INC_ACTION)
		return "";
	return pollable_options[this->options_indexes[index]]->base_name;
}

bool StatusMenu::is_option_selectable(int index, int action) {
	return false;
}

static std::string get_usb_speed_text(int usb_speed) {
	if(usb_speed <= 0)
		return "";
	return "Connection: USB " + std::to_string(usb_speed);
}

void StatusMenu::prepare(float menu_scaling_factor, int view_size_x, int view_size_y, double in_fps, double poll_fps, double draw_fps, CaptureStatus* capture_status) {
	if(!this->do_update) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_update_time;
		if(diff.count() > this->update_timeout)
			this->do_update = true;
	}

	if(this->do_update) {
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
			switch(pollable_options[option_index]->id) {
				case STATUS_MENU_NAME_VERSION:
					this->labels[index]->setText(this->get_string_option(real_index, DEFAULT_ACTION) + " - V." + get_version_string());
					break;
				case STATUS_MENU_CONNECTION:
					this->labels[index]->setText(get_name_of_device(capture_status, true));
					break;
				case STATUS_MENU_USB_CONNECTION:
					this->labels[index]->setText(get_usb_speed_text(get_usb_speed_of_device(capture_status)));
					break;
				case STATUS_MENU_FPS_IN:
					this->labels[index + INC_ACTION]->setText(get_float_str_decimals((float)in_fps * get_framerate_multiplier(capture_status), 2));
					break;
				case STATUS_MENU_FPS_POLL:
					this->labels[index + INC_ACTION]->setText(get_float_str_decimals((float)poll_fps, 2));
					break;
				case STATUS_MENU_FPS_DRAW:
					this->labels[index + INC_ACTION]->setText(get_float_str_decimals((float)draw_fps * get_framerate_multiplier(capture_status), 2));
					break;
				default:
					break;
			}
		}
		this->do_update = false;
		this->last_update_time = std::chrono::high_resolution_clock::now();
	}

	this->base_prepare(menu_scaling_factor, view_size_x, view_size_y);
}

#include "frontend.hpp"

#define FPS_WINDOW_SIZE 64

static const sf::VideoMode default_fs_mode_4_5k_macos = sf::VideoMode({4480, 2520});
static const sf::VideoMode default_fs_mode_4k_macos = sf::VideoMode({4096, 2304});
static const sf::VideoMode default_fs_mode_4k = sf::VideoMode({3840, 2160});
static const sf::VideoMode default_fs_mode_1440p = sf::VideoMode({2560, 1440});
static const sf::VideoMode default_fs_mode_4_5k_half_macos = sf::VideoMode({2240, 1260});
static const sf::VideoMode default_fs_mode_4k_half_macos = sf::VideoMode({2048, 1152});
static const sf::VideoMode default_fs_mode_1080p = sf::VideoMode({1920, 1080});
static const sf::VideoMode default_fs_mode_1050p = sf::VideoMode({1680, 1050});
static const sf::VideoMode default_fs_mode_900p = sf::VideoMode({1600, 900});
static const sf::VideoMode default_fs_mode_720p = sf::VideoMode({1280, 720});
static const sf::VideoMode default_fs_mode_600p = sf::VideoMode({800, 600});
static const sf::VideoMode default_fs_mode_480p = sf::VideoMode({720, 480});
static const sf::VideoMode default_fs_mode_240p = sf::VideoMode({400, 240});
static const sf::VideoMode default_fs_mode_240p_2 = sf::VideoMode({320, 240});

static const sf::VideoMode* default_fs_modes[] = {
&default_fs_mode_4_5k_macos,
&default_fs_mode_4k_macos,
&default_fs_mode_4k,
&default_fs_mode_1440p,
&default_fs_mode_4_5k_half_macos,
&default_fs_mode_4k_half_macos,
&default_fs_mode_1080p,
&default_fs_mode_1050p,
&default_fs_mode_900p,
&default_fs_mode_720p,
&default_fs_mode_600p,
&default_fs_mode_480p,
&default_fs_mode_240p,
&default_fs_mode_240p_2,
};

static void check_held_reset(bool value, HeldTime &action_time) {
	if(value) {
		if(!action_time.started) {
			action_time.start_time = std::chrono::high_resolution_clock::now();
			action_time.started = true;
		}
	}
	else
		action_time.started = false;
}

static float check_held_diff(std::chrono::time_point<std::chrono::high_resolution_clock> &curr_time, HeldTime &action_time) {
	if(!action_time.started)
		return 0.0;
	const std::chrono::duration<double> diff = curr_time - action_time.start_time;
	return diff.count();
}

static bool is_shortcut_valid() {
	return (get_extra_button_name(sf::Keyboard::Key::Enter) != "") || (get_extra_button_name(sf::Keyboard::Key::PageUp) != "");
}

void FPSArrayInit(FPSArray *array) {
	array->data = new double[FPS_WINDOW_SIZE];
	array->index = 0;
}

void FPSArrayDestroy(FPSArray *array) {
	delete []array->data;
}

void FPSArrayInsertElement(FPSArray *array, double frame_time) {
	if(array->index < 0)
		array->index = 0;
	double rate = 0.0;
	if(frame_time != 0.0)
		rate = 1.0 / frame_time;
	array->data[array->index % FPS_WINDOW_SIZE] = rate;
	array->index++;
	if(array->index == (2 * FPS_WINDOW_SIZE))
		array->index = FPS_WINDOW_SIZE;
}

static double FPSArrayGetAverage(FPSArray *array) {
	int available_fps = FPS_WINDOW_SIZE;
	if(array->index < available_fps)
		available_fps = array->index;
	if(available_fps == 0)
		return 0.0;
	double fps_sum = 0;
	for(int i = 0; i < available_fps; i++)
		fps_sum += array->data[i];
	return fps_sum / available_fps;
}

void WindowScreen::init_menus() {
	this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	this->connection_menu = new ConnectionMenu(this->font_load_success, this->text_font);
	this->main_menu = new MainMenu(this->font_load_success, this->text_font);
	this->video_menu = new VideoMenu(this->font_load_success, this->text_font);
	this->crop_menu = new CropMenu(this->font_load_success, this->text_font);
	this->par_menu = new PARMenu(this->font_load_success, this->text_font);
	this->offset_menu = new OffsetMenu(this->font_load_success, this->text_font);
	this->rotation_menu = new RotationMenu(this->font_load_success, this->text_font);
	this->audio_menu = new AudioMenu(this->font_load_success, this->text_font);
	this->bfi_menu = new BFIMenu(this->font_load_success, this->text_font);
	this->relpos_menu = new RelativePositionMenu(this->font_load_success, this->text_font);
	this->resolution_menu = new ResolutionMenu(this->font_load_success, this->text_font);
	this->fileconfig_menu = new FileConfigMenu(this->font_load_success, this->text_font);
	this->extra_menu = new ExtraSettingsMenu(this->font_load_success, this->text_font);
	this->status_menu = new StatusMenu(this->font_load_success, this->text_font);
	this->license_menu = new LicenseMenu(this->font_load_success, this->text_font);
	this->shortcut_menu = new ShortcutMenu(this->font_load_success, this->text_font);
	this->action_selection_menu = new ActionSelectionMenu(this->font_load_success, this->text_font);
	this->scaling_ratio_menu = new ScalingRatioMenu(this->font_load_success, this->text_font);
	this->is_nitro_menu = new ISNitroMenu(this->font_load_success, this->text_font);
}

void WindowScreen::destroy_menus() {
	delete this->connection_menu;
	delete this->main_menu;
	delete this->video_menu;
	delete this->crop_menu;
	delete this->par_menu;
	delete this->offset_menu;
	delete this->rotation_menu;
	delete this->audio_menu;
	delete this->bfi_menu;
	delete this->relpos_menu;
	delete this->resolution_menu;
	delete this->fileconfig_menu;
	delete this->extra_menu;
	delete this->status_menu;
	delete this->license_menu;
	delete this->shortcut_menu;
	delete this->action_selection_menu;
	delete this->scaling_ratio_menu;
	delete this->is_nitro_menu;
}

void WindowScreen::set_close(int ret_val) {
	this->m_prepare_quit = true;
	this->ret_val = ret_val;
}

void WindowScreen::split_change() {
	if(this->curr_menu != CONNECT_MENU_TYPE)
		this->setup_no_menu();
	this->m_info.is_fullscreen = false;
	this->display_data->split = !this->display_data->split;
}

void WindowScreen::fullscreen_change() {
	if(this->curr_menu != CONNECT_MENU_TYPE)
		this->setup_no_menu();
	this->m_info.is_fullscreen = !this->m_info.is_fullscreen;
	this->create_window(true);
	if(this->m_info.is_fullscreen && this->m_info.failed_fullscreen)
		this->print_notification("Fullscreen impossible", TEXT_KIND_ERROR);
}

void WindowScreen::async_change() {
	this->m_info.async = !this->m_info.async;
	this->print_notification_on_off("Async", this->m_info.async);
}

void WindowScreen::vsync_change() {
	this->m_info.v_sync_enabled = !this->m_info.v_sync_enabled;
	this->print_notification_on_off("VSync", this->m_info.v_sync_enabled);
}

void WindowScreen::blur_change() {
	this->m_info.is_blurred = !this->m_info.is_blurred;
	this->future_operations.call_blur = true;
	this->print_notification_on_off("Blur", this->m_info.is_blurred);
}

void WindowScreen::fast_poll_change() {
	this->display_data->fast_poll = !this->display_data->fast_poll;
	this->print_notification_on_off("Slow Poll", this->display_data->fast_poll);
}

void WindowScreen::is_nitro_capture_type_change(bool positive) {
	int new_value = (int)(this->capture_status->capture_type);
	if(positive)
		new_value += 1;
	else
		new_value += CAPTURE_SCREENS_ENUM_END - 1;
	this->capture_status->capture_type = static_cast<CaptureScreensType>(new_value % CAPTURE_SCREENS_ENUM_END);
}

void WindowScreen::is_nitro_capture_reset_hw() {
	this->capture_status->reset_hardware = true;
}

void WindowScreen::padding_change() {
	if(this->m_info.is_fullscreen)
		return;
	this->m_info.rounded_corners_fix = !this->m_info.rounded_corners_fix;
	this->future_operations.call_screen_settings_update = true;
	this->print_notification_on_off("Extra Padding", this->m_info.rounded_corners_fix);
}

void WindowScreen::game_crop_enable_change() {
	this->m_info.allow_games_crops = !this->m_info.allow_games_crops;
	this->print_notification_on_off("Game Specific Crops", this->m_info.allow_games_crops);
	this->crop_value_change(*this->get_crop_index_ptr(&this->m_info), false, false);
}

void WindowScreen::crop_value_change(int new_crop_value, bool do_print_notification, bool do_cycle) {
	std::vector<const CropData*> *crops = this->get_crop_data_vector(&this->m_info);
	int *crop_value = this->get_crop_index_ptr(&this->m_info);
	int new_value = new_crop_value % crops->size();
	if((!do_cycle) && ((new_crop_value < 0) || (new_crop_value >= crops->size())))
		new_value = 0;
	if(new_value < 0)
		new_value = 0;
	if((*crop_value) != new_value) {
		*crop_value = new_value;
		if(do_print_notification)
			this->print_notification("Crop: " + (*crops)[*crop_value]->name);
		this->prepare_size_ratios(false, false);
		this->future_operations.call_crop = true;
	}
}

void WindowScreen::par_value_change(int new_par_value, bool is_top) {
	if(((!is_top) && (this->m_stype == ScreenType::TOP)) || ((is_top) && (this->m_stype == ScreenType::BOTTOM)))
		return;
	int par_index = new_par_value % this->possible_pars.size();
	if(par_index < 0)
		par_index = 0;
	std::string setting_name = "PAR: ";
	bool updated = false;
	if(is_top) {
		if(this->m_info.top_par != par_index)
			updated = true;
		this->m_info.top_par = par_index;
		if(this->m_stype == ScreenType::JOINT)
			setting_name = "Top " + setting_name;
	}
	else {
		if(this->m_info.bot_par != par_index)
			updated = true;
		this->m_info.bot_par = par_index;
		if(this->m_stype == ScreenType::JOINT)
			setting_name = "Bottom " + setting_name;
	}
	if(updated) {
		this->prepare_size_ratios(false, false);
		this->future_operations.call_screen_settings_update = true;
		this->print_notification(setting_name + this->possible_pars[par_index]->name);
	}
}

void WindowScreen::offset_change(float &value, float change) {
	if(change >= 1.0)
		return;
	if(change <= -1.0)
		return;
	float absolute_change = std::abs(change);
	if(absolute_change == 0.0)
		return;
	float close_slice = ((int)(value / absolute_change)) * absolute_change;
	if((value - close_slice) > (absolute_change / 2.0))
		close_slice += absolute_change;
	close_slice += change;
	if(close_slice > (1.0 + (absolute_change / 2.0)))
		close_slice = 0.0;
	else if(close_slice > 1.0)
		close_slice = 1.0;
	else if(close_slice < (0.0 - (absolute_change / 2.0)))
		close_slice = 1.0;
	else if (close_slice < 0.0)
		close_slice = 0.0;
	value = close_slice;
	this->future_operations.call_screen_settings_update = true;
}

void WindowScreen::menu_scaling_change(bool positive) {
	double old_scaling = this->m_info.menu_scaling_factor;
	if(positive)
		this->m_info.menu_scaling_factor += 0.1;
	else
		this->m_info.menu_scaling_factor -= 0.1;
	if(this->m_info.menu_scaling_factor < 0.35)
		this->m_info.menu_scaling_factor = 0.3;
	if(this->m_info.menu_scaling_factor > 9.95)
		this->m_info.menu_scaling_factor = 10.0;
	if(old_scaling != this->m_info.menu_scaling_factor) {
		this->print_notification_float("Menu Scaling", this->m_info.menu_scaling_factor, 1);
	}
}

void WindowScreen::window_scaling_change(bool positive) {
	if(this->m_info.is_fullscreen)
		return;
	double old_scaling = this->m_info.scaling;
	if(positive)
		this->m_info.scaling += 0.5;
	else
		this->m_info.scaling -= 0.5;
	if (this->m_info.scaling < 1.25)
		this->m_info.scaling = 1.0;
	if (this->m_info.scaling > 44.75)
		this->m_info.scaling = 45.0;
	if(old_scaling != this->m_info.scaling) {
		this->print_notification_float("Scaling", this->m_info.scaling, 1);
		this->future_operations.call_screen_settings_update = true;
	}
}

void WindowScreen::rotation_change(int &value, bool right) {
	int add_value = 90;
	if(!right)
		add_value = 270;
	value = (value + add_value) % 360;
	this->prepare_size_ratios(false, false);
	this->future_operations.call_rotate = true;
}

void WindowScreen::ratio_change(bool top_priority, bool cycle) {
	this->prepare_size_ratios(top_priority, !top_priority, cycle);
	this->future_operations.call_screen_settings_update = true;
}

void WindowScreen::bfi_change() {
	this->m_info.bfi = !this->m_info.bfi;
	this->print_notification_on_off("BFI", this->m_info.bfi);
}

void WindowScreen::bottom_pos_change(int new_bottom_pos) {
	BottomRelativePosition cast_new_bottom_pos = static_cast<BottomRelativePosition>(new_bottom_pos % BottomRelativePosition::BOT_REL_POS_END);
	if(cast_new_bottom_pos != this->m_info.bottom_pos) {
		this->m_info.bottom_pos = cast_new_bottom_pos;
		this->prepare_size_ratios(false, false);
		this->future_operations.call_screen_settings_update = true;
	}
}

void WindowScreen::non_int_scaling_change(bool target_top) {
	if(this->m_stype == ScreenType::TOP)
		target_top = true;
	else if(this->m_stype == ScreenType::BOTTOM)
		target_top = false;
	if(target_top)
		this->m_info.use_non_integer_scaling_top = !this->m_info.use_non_integer_scaling_top;
	else
		this->m_info.use_non_integer_scaling_bottom = !this->m_info.use_non_integer_scaling_bottom;
	this->prepare_size_ratios(true, true);
	this->future_operations.call_screen_settings_update = true;
}

void WindowScreen::non_int_mode_change(bool positive) {
	int change = 1;
	if(!positive)
		change = END_NONINT_SCALE_MODES - 1;
	this->m_info.non_integer_mode = static_cast<NonIntegerScalingModes>((this->m_info.non_integer_mode + change) % END_NONINT_SCALE_MODES);
	this->prepare_size_ratios(true, true);
	this->future_operations.call_screen_settings_update = true;
}

void WindowScreen::titlebar_change() {
	this->m_info.have_titlebar = !this->m_info.have_titlebar;
	this->create_window(true, false);
	this->future_operations.call_titlebar = true;
}

bool WindowScreen::can_execute_cmd(const WindowCommand* window_cmd, bool is_extra, bool is_always) {
	if((!window_cmd->usable_always) && is_always)
		return false;
	if((!window_cmd->available_mono_extra) && is_extra && this->display_data->mono_app_mode)
		return false;
	return true;
}

bool WindowScreen::execute_cmd(PossibleWindowCommands id) {
	bool consumed = true;
	switch(id) {
		case WINDOW_COMMAND_CONNECT:
			this->m_prepare_open = true;
			break;
		case WINDOW_COMMAND_SPLIT:
			this->split_change();
			break;
		case WINDOW_COMMAND_FULLSCREEN:
			this->fullscreen_change();
			break;
		case WINDOW_COMMAND_CROP:
			this->crop_value_change(*this->get_crop_index_ptr(&this->m_info) + 1);
			break;
		case WINDOW_COMMAND_ASYNC:
			this->async_change();
			break;
		case WINDOW_COMMAND_VSYNC:
			this->vsync_change();
			break;
		case WINDOW_COMMAND_MENU_SCALING_INC:
			this->menu_scaling_change(true);
			break;
		case WINDOW_COMMAND_MENU_SCALING_DEC:
			this->menu_scaling_change(false);
			break;
		case WINDOW_COMMAND_WINDOW_SCALING_INC:
			this->window_scaling_change(true);
			break;
		case WINDOW_COMMAND_WINDOW_SCALING_DEC:
			this->window_scaling_change(false);
			break;
		case WINDOW_COMMAND_RATIO_CYCLE:
			this->ratio_change(true, true);
			break;
		case WINDOW_COMMAND_RATIO_TOP:
			this->ratio_change(true);
			break;
		case WINDOW_COMMAND_RATIO_BOT:
			this->ratio_change(false);
			break;
		case WINDOW_COMMAND_BLUR:
			this->blur_change();
			break;
		case WINDOW_COMMAND_TRANSPOSE:
			this->bottom_pos_change(this->m_info.bottom_pos + 1);
			break;
		case WINDOW_COMMAND_SCREEN_OFFSET:
			this->offset_change(this->m_info.subscreen_offset, 0.5);
			break;
		case WINDOW_COMMAND_SUB_SCREEN_DISTANCE:
			this->offset_change(this->m_info.subscreen_attached_offset, 0.5);
			break;
		case WINDOW_COMMAND_CANVAS_X:
			this->offset_change(this->m_info.total_offset_x, 0.5);
			break;
		case WINDOW_COMMAND_CANVAS_Y:
			this->offset_change(this->m_info.total_offset_y, 0.5);
			break;
		case WINDOW_COMMAND_ROT_INC:
			this->rotation_change(this->m_info.top_rotation, true);
			this->rotation_change(this->m_info.bot_rotation, true);
			break;
		case WINDOW_COMMAND_ROT_DEC:
			this->rotation_change(this->m_info.top_rotation, false);
			this->rotation_change(this->m_info.bot_rotation, false);
			break;
		case WINDOW_COMMAND_ROT_TOP_INC:
			this->rotation_change(this->m_info.top_rotation, true);
			break;
		case WINDOW_COMMAND_ROT_TOP_DEC:
			this->rotation_change(this->m_info.top_rotation, false);
			break;
		case WINDOW_COMMAND_ROT_BOT_INC:
			this->rotation_change(this->m_info.bot_rotation, true);
			break;
		case WINDOW_COMMAND_ROT_BOT_DEC:
			this->rotation_change(this->m_info.bot_rotation, false);
			break;
		case WINDOW_COMMAND_PADDING:
			this->padding_change();
			break;
		case WINDOW_COMMAND_TOP_PAR:
			this->par_value_change(this->m_info.top_par + 1, true);
			break;
		case WINDOW_COMMAND_BOT_PAR:
			this->par_value_change(this->m_info.bot_par + 1, false);
			break;
		case WINDOW_COMMAND_AUDIO_MUTE:
			audio_data->change_audio_mute();
			break;
		case WINDOW_COMMAND_VOLUME_DEC:
			audio_data->change_audio_volume(false);
			break;
		case WINDOW_COMMAND_VOLUME_INC:
			audio_data->change_audio_volume(true);
			break;
		case WINDOW_COMMAND_AUDIO_RESTART:
			audio_data->request_audio_restart();
			break;
		case WINDOW_COMMAND_LOAD_PROFILE_STARTUP:
			this->m_prepare_load = STARTUP_FILE_INDEX;
			break;
		case WINDOW_COMMAND_LOAD_PROFILE_1:
		case WINDOW_COMMAND_LOAD_PROFILE_2:
		case WINDOW_COMMAND_LOAD_PROFILE_3:
		case WINDOW_COMMAND_LOAD_PROFILE_4:
			this->m_prepare_load = id - WINDOW_COMMAND_LOAD_PROFILE_1 + 1;
			break;
		case WINDOW_COMMAND_SAVE_PROFILE_STARTUP:
			this->m_prepare_save = STARTUP_FILE_INDEX;
			break;
		case WINDOW_COMMAND_SAVE_PROFILE_1:
		case WINDOW_COMMAND_SAVE_PROFILE_2:
		case WINDOW_COMMAND_SAVE_PROFILE_3:
		case WINDOW_COMMAND_SAVE_PROFILE_4:
			this->m_prepare_save = id - WINDOW_COMMAND_SAVE_PROFILE_1 + 1;
			break;
		default:
			consumed = false;
			break;
	}
	return consumed;
}

bool WindowScreen::common_poll(SFEvent &event_data) {
	bool consumed = true;
	switch(event_data.type) {
		case EVENT_CLOSED:
			this->set_close(0);
			break;

		case EVENT_TEXT_ENTERED:
			switch(event_data.unicode) {
				case 's':
					this->execute_cmd(WINDOW_COMMAND_SPLIT);
					break;

				case 'f':
					this->execute_cmd(WINDOW_COMMAND_FULLSCREEN);
					break;

				case 'a':
					this->execute_cmd(WINDOW_COMMAND_ASYNC);
					break;

				case 'v':
					this->execute_cmd(WINDOW_COMMAND_VSYNC);
					break;

				case 'z':
					this->execute_cmd(WINDOW_COMMAND_MENU_SCALING_DEC);
					break;

				case 'x':
					this->execute_cmd(WINDOW_COMMAND_MENU_SCALING_INC);
					break;

				case '-':
					this->execute_cmd(WINDOW_COMMAND_WINDOW_SCALING_DEC);
					break;

				case '0':
					this->execute_cmd(WINDOW_COMMAND_WINDOW_SCALING_INC);
					break;

				default:
					consumed = false;
					break;
			}

			break;
			
		case EVENT_KEY_PRESSED:
			switch(event_data.code) {
				case sf::Keyboard::Key::Escape:
					this->set_close(0);
					break;
				case sf::Keyboard::Key::Enter:
					if(event_data.is_extra)
						check_held_reset(true, this->extra_enter_action);
					else
						check_held_reset(true, this->enter_action);
					consumed = false;
					break;
				case sf::Keyboard::Key::PageDown:
					if(event_data.is_extra)
						check_held_reset(true, this->extra_pgdown_action);
					else
						check_held_reset(true, this->pgdown_action);
					consumed = false;
					break;
				default:
					consumed = false;
					break;
			}

			break;
		case EVENT_KEY_RELEASED:
			switch(event_data.code) {
				case sf::Keyboard::Key::Enter:
					if(event_data.is_extra)
						check_held_reset(false, this->extra_enter_action);
					else
						check_held_reset(false, this->enter_action);
					consumed = false;
					break;
				case sf::Keyboard::Key::PageDown:
					if(event_data.is_extra)
						check_held_reset(false, this->extra_pgdown_action);
					else
						check_held_reset(false, this->pgdown_action);
					consumed = false;
					break;
				default:
					consumed = false;
					break;
			}

			break;
		case EVENT_MOUSE_MOVED:
			this->m_info.show_mouse = true;
			this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			consumed = false;
			break;
		case EVENT_MOUSE_BTN_PRESSED:
			this->m_info.show_mouse = true;
			this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			consumed = false;
			break;
		case EVENT_MOUSE_BTN_RELEASED:
			this->m_info.show_mouse = true;
			this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			consumed = false;
			break;
		case EVENT_JOYSTICK_BTN_PRESSED:
			consumed = false;
			break;
		case EVENT_JOYSTICK_MOVED:
			consumed = false;
			break;
		default:
			consumed = false;
			break;
	}
	return consumed;
}

bool WindowScreen::can_setup_menu() {
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->last_menu_change_time;
	return diff.count() > this->menu_change_timeout;
}

void WindowScreen::setup_no_menu() {
	this->curr_menu = DEFAULT_MENU_TYPE;
	this->last_menu_change_time = std::chrono::high_resolution_clock::now();
}

void WindowScreen::setup_main_menu(bool reset_data, bool skip_setup_check) {
	if((!skip_setup_check) && (!this->can_setup_menu()))
		return;
	if(this->curr_menu != MAIN_MENU_TYPE) {
		this->curr_menu = MAIN_MENU_TYPE;
		if(reset_data)
			this->main_menu->reset_data();
		this->main_menu->insert_data(this->m_stype, this->m_info.is_fullscreen, this->display_data->mono_app_mode, is_shortcut_valid(), this->capture_status->device.cc_type, this->capture_status->connected);
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_video_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != VIDEO_MENU_TYPE) {
		this->curr_menu = VIDEO_MENU_TYPE;
		if(reset_data)
			this->video_menu->reset_data();
		this->video_menu->insert_data(this->m_stype, this->m_info.is_fullscreen, (!this->m_info.is_fullscreen) || this->m_info.failed_fullscreen);
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_crop_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != CROP_MENU_TYPE) {
		this->curr_menu = CROP_MENU_TYPE;
		if(reset_data)
			this->crop_menu->reset_data();
		this->crop_menu->insert_data(this->get_crop_data_vector(&this->m_info));
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_par_menu(bool is_top, bool reset_data) {
	if(!this->can_setup_menu())
		return;
	CurrMenuType wanted_type = TOP_PAR_MENU_TYPE;
	if(!is_top)
		wanted_type = BOTTOM_PAR_MENU_TYPE;
	if(this->curr_menu != wanted_type) {
		this->curr_menu = wanted_type;
		if(reset_data)
			this->par_menu->reset_data();
		std::string title_piece = "";
		if((is_top) && (this->m_stype == ScreenType::JOINT))
			title_piece = "Top";
		else if((!is_top) && (this->m_stype == ScreenType::JOINT))
			title_piece = "Bot.";
		this->par_menu->setup_title(title_piece);
		this->par_menu->insert_data(&this->possible_pars);
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_offset_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != OFFSET_MENU_TYPE) {
		this->curr_menu = OFFSET_MENU_TYPE;
		if(reset_data)
			this->offset_menu->reset_data();
		this->offset_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_rotation_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != ROTATION_MENU_TYPE) {
		this->curr_menu = ROTATION_MENU_TYPE;
		if(reset_data)
			this->rotation_menu->reset_data();
		this->rotation_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_audio_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != AUDIO_MENU_TYPE) {
		this->curr_menu = AUDIO_MENU_TYPE;
		if(reset_data)
			this->audio_menu->reset_data();
		this->audio_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_bfi_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != BFI_MENU_TYPE) {
		this->curr_menu = BFI_MENU_TYPE;
		if(reset_data)
			this->bfi_menu->reset_data();
		this->bfi_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_fileconfig_menu(bool is_save, bool reset_data, bool skip_setup_check) {
	if((!skip_setup_check) && (!this->can_setup_menu()))
		return;
	CurrMenuType wanted_type = LOAD_MENU_TYPE;
	if(is_save)
		wanted_type = SAVE_MENU_TYPE;
	if(this->curr_menu != wanted_type) {
		this->possible_files.clear();
		FileData file_data;
		bool success = false;
		if(is_save) {
			file_data.index = CREATE_NEW_FILE_INDEX;
			file_data.name = "";
			this->possible_files.push_back(file_data);
		}
		file_data.index = STARTUP_FILE_INDEX;
		file_data.name = load_layout_name(file_data.index, success);
		this->possible_files.push_back(file_data);
		for(int i = 1; i <= 4; i++) {
			file_data.index = i;
			file_data.name = load_layout_name(file_data.index, success);
			this->possible_files.push_back(file_data);
		}
		int first_free;
		bool failed = false;
		for(first_free = 5; first_free <= 100; first_free++) {
			file_data.index = first_free;
			file_data.name = load_layout_name(file_data.index, success);
			if(!success) {
				failed = true;
				break;
			}
			this->possible_files.push_back(file_data);
		}
		if(is_save && (!failed)) {
			this->possible_files.erase(this->possible_files.begin());
		}
		this->curr_menu = wanted_type;
		if(reset_data)
			this->fileconfig_menu->reset_data();
		std::string title_piece = "Load";
		if(is_save)
			title_piece = "Save";
		this->fileconfig_menu->setup_title(title_piece);
		this->fileconfig_menu->insert_data(&this->possible_files);
		if(!skip_setup_check)
			this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_resolution_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != RESOLUTION_MENU_TYPE) {
		this->possible_resolutions.clear();
		std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
		if(modes.size() > 0) {
			this->possible_resolutions.push_back(sf::VideoMode({0, 0}, 0));
			this->possible_resolutions.push_back(modes[0]);
			for(int i = 1; i < modes.size(); ++i)
				if(modes[0].bitsPerPixel == modes[i].bitsPerPixel)
					this->possible_resolutions.push_back(modes[i]);
		}
		else {
			this->print_notification("No Fullscreen resolution found", TEXT_KIND_WARNING);
			for(int i = 0; i < (sizeof(default_fs_modes) / sizeof(default_fs_modes[0])); i++) {
				this->possible_resolutions.push_back(*default_fs_modes[i]);
			}
		}
		if(this->possible_resolutions.size() > 0) {
			this->curr_menu = RESOLUTION_MENU_TYPE;
			if(reset_data)
				this->resolution_menu->reset_data();
			this->resolution_menu->insert_data(&this->possible_resolutions, sf::VideoMode::getDesktopMode());
			this->last_menu_change_time = std::chrono::high_resolution_clock::now();
		}
	}
}

void WindowScreen::setup_extra_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != EXTRA_MENU_TYPE) {
		this->curr_menu = EXTRA_MENU_TYPE;
		if(reset_data)
			this->extra_menu->reset_data();
		this->extra_menu->insert_data(this->m_stype, this->m_info.is_fullscreen);
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_action_selection_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != ACTION_SELECTION_MENU_TYPE) {
		this->curr_menu = ACTION_SELECTION_MENU_TYPE;
		if(reset_data)
			this->action_selection_menu->reset_data();
		this->possible_actions.clear();
		create_window_commands_list(this->possible_actions, this->possible_buttons_extras[this->chosen_button] && this->display_data->mono_app_mode);
		this->action_selection_menu->insert_data(this->possible_actions);
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_shortcuts_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(!is_shortcut_valid())
		return;
	if(this->curr_menu != SHORTCUTS_MENU_TYPE) {
		this->curr_menu = SHORTCUTS_MENU_TYPE;
		if(reset_data)
			this->shortcut_menu->reset_data();
		this->possible_buttons_names.clear();
		this->possible_buttons_ptrs.clear();
		this->possible_buttons_extras.clear();
		if(get_extra_button_name(sf::Keyboard::Key::Enter) != "") {
			this->possible_buttons_ptrs.push_back(&extra_button_shortcuts->enter_shortcut);
			this->possible_buttons_names.push_back(get_extra_button_name(sf::Keyboard::Key::Enter) + " Button: " + extra_button_shortcuts->enter_shortcut->name);
			this->possible_buttons_extras.push_back(true);
		}
		if(get_extra_button_name(sf::Keyboard::Key::PageUp) != "") {
			this->possible_buttons_ptrs.push_back(&extra_button_shortcuts->page_up_shortcut);
			this->possible_buttons_names.push_back(get_extra_button_name(sf::Keyboard::Key::PageUp) + " Button: " + extra_button_shortcuts->page_up_shortcut->name);
			this->possible_buttons_extras.push_back(true);
		}
		this->shortcut_menu->insert_data(this->possible_buttons_names);
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_status_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != STATUS_MENU_TYPE) {
		this->curr_menu = STATUS_MENU_TYPE;
		if(reset_data)
			this->status_menu->reset_data();
		this->status_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_licenses_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != LICENSES_MENU_TYPE) {
		this->curr_menu = LICENSES_MENU_TYPE;
		if(reset_data)
			this->license_menu->reset_data();
		this->license_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_relative_pos_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != RELATIVE_POS_MENU_TYPE) {
		this->curr_menu = RELATIVE_POS_MENU_TYPE;
		if(reset_data)
			this->relpos_menu->reset_data();
		this->relpos_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_scaling_ratio_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != SCALING_RATIO_MENU_TYPE) {
		this->curr_menu = SCALING_RATIO_MENU_TYPE;
		if(reset_data)
			this->scaling_ratio_menu->reset_data();
		this->scaling_ratio_menu->insert_data();
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::setup_is_nitro_menu(bool reset_data) {
	if(!this->can_setup_menu())
		return;
	if(this->curr_menu != ISN_MENU_TYPE) {
		this->curr_menu = ISN_MENU_TYPE;
		if(reset_data)
			this->is_nitro_menu->reset_data();
		this->is_nitro_menu->insert_data(&this->capture_status->device);
		this->last_menu_change_time = std::chrono::high_resolution_clock::now();
	}
}

void WindowScreen::update_save_menu() {
	if(this->curr_menu == SAVE_MENU_TYPE) {
		this->curr_menu = DEFAULT_MENU_TYPE;
		this->setup_fileconfig_menu(true, false, true);
	}
}

bool WindowScreen::no_menu_poll(SFEvent &event_data) {
	bool consumed = true;
	switch(event_data.type) {
		case EVENT_TEXT_ENTERED:
			switch(event_data.unicode) {
				default:
					consumed = false;
					break;
			}
			break;
		case EVENT_KEY_PRESSED:
			switch(event_data.code) {
				case sf::Keyboard::Key::Enter:
					if(event_data.is_extra) {
						consumed = this->can_execute_cmd(extra_button_shortcuts->enter_shortcut, true, false);
						if(consumed)
							this->execute_cmd(extra_button_shortcuts->enter_shortcut->cmd);
					}
					else
						this->setup_main_menu();
					break;
				case sf::Keyboard::Key::PageDown:
					if(event_data.is_extra)
						this->setup_main_menu();
					else
						consumed = false;
					break;
				case sf::Keyboard::Key::PageUp:
					if(event_data.is_extra){
						consumed = this->can_execute_cmd(extra_button_shortcuts->page_up_shortcut, true, false);
						if(consumed)
							this->execute_cmd(extra_button_shortcuts->page_up_shortcut->cmd);
					}
					else
						consumed = false;
					break;
				default:
					consumed = false;
					break;
			}
			break;
		case EVENT_MOUSE_MOVED:
			consumed = false;
			break;
		case EVENT_MOUSE_BTN_PRESSED:
			if(event_data.mouse_button == sf::Mouse::Button::Right)
				this->setup_main_menu();
			else
				consumed = false;
			break;
		case EVENT_MOUSE_BTN_RELEASED:
			consumed = false;
			break;
		case EVENT_JOYSTICK_BTN_PRESSED:
			switch(get_joystick_action(event_data.joystickId, event_data.joy_button)) {
				case JOY_ACTION_MENU:
					this->setup_main_menu();
					break;
				default:
					consumed = false;
					break;
			}
			break;
		case EVENT_JOYSTICK_MOVED:
			consumed = false;
			break;
		default:
			consumed = false;
			break;
	}
	return consumed;
}

bool WindowScreen::main_poll(SFEvent &event_data) {
	bool consumed = true;
	switch(event_data.type) {
		case EVENT_TEXT_ENTERED:
			switch(event_data.unicode) {
				case 'c':
					this->execute_cmd(WINDOW_COMMAND_CROP);
					break;

				case 'b':
					this->execute_cmd(WINDOW_COMMAND_BLUR);
					break;

				//case 'i':
				//	this->bfi_change();
				//	break;

				case 'o':
					this->execute_cmd(WINDOW_COMMAND_CONNECT);
					break;

				case 't':
					this->execute_cmd(WINDOW_COMMAND_TRANSPOSE);
					break;

				case '6':
					this->execute_cmd(WINDOW_COMMAND_SCREEN_OFFSET);
					break;

				case '7':
					this->execute_cmd(WINDOW_COMMAND_SUB_SCREEN_DISTANCE);
					break;

				case '4':
					this->execute_cmd(WINDOW_COMMAND_CANVAS_X);
					break;

				case '5':
					this->execute_cmd(WINDOW_COMMAND_CANVAS_Y);
					break;

				case 'y':
					this->execute_cmd(WINDOW_COMMAND_RATIO_TOP);
					break;

				case 'u':
					this->execute_cmd(WINDOW_COMMAND_RATIO_BOT);
					break;

				case '8':
					this->execute_cmd(WINDOW_COMMAND_ROT_DEC);
					break;

				case '9':
					this->execute_cmd(WINDOW_COMMAND_ROT_INC);
					break;

				case 'h':
					this->execute_cmd(WINDOW_COMMAND_ROT_TOP_DEC);
					break;

				case 'j':
					this->execute_cmd(WINDOW_COMMAND_ROT_TOP_INC);
					break;

				case 'k':
					this->execute_cmd(WINDOW_COMMAND_ROT_BOT_DEC);
					break;

				case 'l':
					this->execute_cmd(WINDOW_COMMAND_ROT_BOT_INC);
					break;

				case 'r':
					this->execute_cmd(WINDOW_COMMAND_PADDING);
					break;

				case '2':
					this->execute_cmd(WINDOW_COMMAND_TOP_PAR);
					break;

				case '3':
					this->execute_cmd(WINDOW_COMMAND_BOT_PAR);
					break;

				case 'm':
					this->execute_cmd(WINDOW_COMMAND_AUDIO_MUTE);
					break;

				case ',':
					this->execute_cmd(WINDOW_COMMAND_VOLUME_DEC);
					break;

				case '.':
					this->execute_cmd(WINDOW_COMMAND_VOLUME_INC);
					break;

				case 'n':
					this->execute_cmd(WINDOW_COMMAND_AUDIO_RESTART);
					break;

				default:
					consumed = false;
					break;
			}

			break;
			
		case EVENT_KEY_PRESSED:
			switch(event_data.code) {
				case sf::Keyboard::Key::F1:
				case sf::Keyboard::Key::F2:
				case sf::Keyboard::Key::F3:
				case sf::Keyboard::Key::F4:
					this->execute_cmd(static_cast<PossibleWindowCommands>(WINDOW_COMMAND_LOAD_PROFILE_1 + ((int)event_data.code - (int)sf::Keyboard::Key::F1)));
					break;

				case sf::Keyboard::Key::F5:
				case sf::Keyboard::Key::F6:
				case sf::Keyboard::Key::F7:
				case sf::Keyboard::Key::F8:
					this->execute_cmd(static_cast<PossibleWindowCommands>(WINDOW_COMMAND_SAVE_PROFILE_1 + ((int)event_data.code - (int)sf::Keyboard::Key::F5)));
					break;
				default:
					consumed = false;
					break;
			}

			break;
		case EVENT_MOUSE_MOVED:
			consumed = false;
			break;
		case EVENT_MOUSE_BTN_PRESSED:
			consumed = false;
			break;
		case EVENT_MOUSE_BTN_RELEASED:
			consumed = false;
			break;
		case EVENT_JOYSTICK_BTN_PRESSED:
			consumed = false;
			break;
		case EVENT_JOYSTICK_MOVED:
			consumed = false;
			break;
		default:
			consumed = false;
			break;
	}
	return consumed;
}

void WindowScreen::poll(bool do_everything) {
	if(this->close_capture())
		return;
	if((this->m_info.is_fullscreen || this->display_data->mono_app_mode) && this->m_info.show_mouse) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_mouse_action_time;
		if(diff.count() > this->mouse_timeout)
			this->m_info.show_mouse = false;
	}
	this->poll_window(do_everything);
	bool done = false;
	while(!events_queue.empty()) {
		if(done)
			break;
		SFEvent event_data = events_queue.front();
		events_queue.pop();
		if((event_data.type == EVENT_KEY_PRESSED) && event_data.poweroff_cmd) {
			this->set_close(1);
			done = true;
			continue;
		}
		if(this->query_reset_request()) {
			this->reset_held_times();
			this->m_prepare_load = SIMPLE_RESET_DATA_INDEX;
			done = true;
			continue;
		}
		if(this->common_poll(event_data)) {
			if(this->close_capture())
				done = true;
			continue;
		}
		if(this->loaded_menu != CONNECT_MENU_TYPE) {
			if(this->main_poll(event_data))
				continue;
		}
		switch(this->loaded_menu) {
			case DEFAULT_MENU_TYPE:
				if(this->no_menu_poll(event_data))
					continue;
				break;
			case CONNECT_MENU_TYPE:
				if(this->connection_menu->poll(event_data)) {
					if(this->check_connection_menu_result() != CONNECTION_MENU_NO_ACTION)
						done = true;
					continue;
				}
				break;
			case MAIN_MENU_TYPE:
				if(this->main_menu->poll(event_data)) {
					switch(this->main_menu->selected_index) {
						case MAIN_MENU_OPEN:
							this->m_prepare_open = true;
							break;
						case MAIN_MENU_CLOSE_MENU:
							this->setup_no_menu();
							done = true;
							break;
						case MAIN_MENU_QUIT_APPLICATION:
							this->set_close(0);
							this->setup_no_menu();
							done = true;
							break;
						case MAIN_MENU_FULLSCREEN:
							this->fullscreen_change();
							done = true;
							break;
						case MAIN_MENU_SPLIT:
							this->split_change();
							done = true;
							break;
						case MAIN_MENU_VIDEO_SETTINGS:
							this->setup_video_menu();
							done = true;
							break;
						case MAIN_MENU_AUDIO_SETTINGS:
							this->setup_audio_menu();
							done = true;
							break;
						case MAIN_MENU_LOAD_PROFILES:
							this->setup_fileconfig_menu(false);
							done = true;
							break;
						case MAIN_MENU_SAVE_PROFILES:
							this->setup_fileconfig_menu(true);
							done = true;
							break;
						case MAIN_MENU_STATUS:
							this->setup_status_menu();
							done = true;
							break;
						case MAIN_MENU_LICENSES:
							this->setup_licenses_menu();
							done = true;
							break;
						case MAIN_MENU_EXTRA_SETTINGS:
							this->setup_extra_menu();
							done = true;
							break;
						case MAIN_MENU_SHORTCUT_SETTINGS:
							this->setup_shortcuts_menu();
							done = true;
							break;
						case MAIN_MENU_ISN_SETTINGS:
							this->setup_is_nitro_menu();
							done = true;
							break;
						case MAIN_MENU_SHUTDOWN:
							this->set_close(1);
							this->setup_no_menu();
							done = true;
							break;
						default:
							break;
					}
					this->main_menu->reset_output_option();
					continue;
				}
				break;
			case AUDIO_MENU_TYPE:
				if(this->audio_menu->poll(event_data)) {
					switch(this->audio_menu->selected_index) {
						case AUDIO_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case AUDIO_MENU_NO_ACTION:
							break;
						case AUDIO_MENU_VOLUME_DEC:
							this->audio_data->change_audio_volume(false);
							break;
						case AUDIO_MENU_VOLUME_INC:
							this->audio_data->change_audio_volume(true);
							break;
						case AUDIO_MENU_MAX_LATENCY_DEC:
							this->audio_data->change_max_audio_latency(false);
							break;
						case AUDIO_MENU_MAX_LATENCY_INC:
							this->audio_data->change_max_audio_latency(true);
							break;
						case AUDIO_MENU_OUTPUT_DEC:
							this->audio_data->change_audio_output_type(false);
							break;
						case AUDIO_MENU_OUTPUT_INC:
							this->audio_data->change_audio_output_type(true);
							break;
						case AUDIO_MENU_MUTE:
							this->audio_data->change_audio_mute();
							break;
						case AUDIO_MENU_RESTART:
							this->audio_data->request_audio_restart();
							break;
						default:
							break;
					}
					this->audio_menu->reset_output_option();
					continue;
				}
				break;
			case SAVE_MENU_TYPE:
				if(this->fileconfig_menu->poll(event_data)) {
					switch(this->fileconfig_menu->selected_index) {
						case FILECONFIG_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case FILECONFIG_MENU_NO_ACTION:
							break;
						case CREATE_NEW_FILE_INDEX:
							this->m_prepare_save = this->possible_files[this->possible_files.size() - 1].index + 1;
							done = true;
							break;
						default:
							this->m_prepare_save = this->fileconfig_menu->selected_index;
							done = true;
							break;
					}
					this->fileconfig_menu->reset_output_option();
					continue;
				}
				break;
			case LOAD_MENU_TYPE:
				if(this->fileconfig_menu->poll(event_data)) {
					switch(this->fileconfig_menu->selected_index) {
						case FILECONFIG_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case FILECONFIG_MENU_NO_ACTION:
							break;
						default:
							this->m_prepare_load = this->fileconfig_menu->selected_index;
							done = true;
							break;
					}
					this->fileconfig_menu->reset_output_option();
					continue;
				}
				break;
			case VIDEO_MENU_TYPE:
				if(this->video_menu->poll(event_data)) {
					switch(this->video_menu->selected_index) {
						case VIDEO_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case VIDEO_MENU_VSYNC:
							this->vsync_change();
							break;
						case VIDEO_MENU_ASYNC:
							this->async_change();
							break;
						case VIDEO_MENU_BLUR:
							this->blur_change();
							break;
						case VIDEO_MENU_FAST_POLL:
							this->fast_poll_change();
							break;
						case VIDEO_MENU_PADDING:
							this->padding_change();
							break;
						case VIDEO_MENU_CROPPING:
							this->setup_crop_menu();
							done = true;
							break;
						case VIDEO_MENU_TOP_PAR:
							this->setup_par_menu(true);
							done = true;
							break;
						case VIDEO_MENU_BOT_PAR:
							this->setup_par_menu(false);
							done = true;
							break;
						case VIDEO_MENU_ONE_PAR:
							this->setup_par_menu(this->m_stype == ScreenType::TOP);
							done = true;
							break;
						case VIDEO_MENU_MENU_SCALING_DEC:
							this->menu_scaling_change(false);
							break;
						case VIDEO_MENU_MENU_SCALING_INC:
							this->menu_scaling_change(true);
							break;
						case VIDEO_MENU_WINDOW_SCALING_DEC:
							this->window_scaling_change(false);
							break;
						case VIDEO_MENU_WINDOW_SCALING_INC:
							this->window_scaling_change(true);
							break;
						case VIDEO_MENU_TOP_ROTATION_DEC:
							this->rotation_change(this->m_info.top_rotation, false);
							break;
						case VIDEO_MENU_TOP_ROTATION_INC:
							this->rotation_change(this->m_info.top_rotation, true);
							break;
						case VIDEO_MENU_BOTTOM_ROTATION_DEC:
							this->rotation_change(this->m_info.bot_rotation, false);
							break;
						case VIDEO_MENU_BOTTOM_ROTATION_INC:
							this->rotation_change(this->m_info.bot_rotation, true);
							break;
						case VIDEO_MENU_SMALL_SCREEN_OFFSET_DEC:
							this->offset_change(this->m_info.subscreen_offset, -0.1);
							break;
						case VIDEO_MENU_SMALL_SCREEN_OFFSET_INC:
							this->offset_change(this->m_info.subscreen_offset, 0.1);
							break;
						case VIDEO_MENU_ROTATION_SETTINGS:
							this->setup_rotation_menu();
							done = true;
							break;
						case VIDEO_MENU_OFFSET_SETTINGS:
							this->setup_offset_menu();
							done = true;
							break;
						case VIDEO_MENU_BFI_SETTINGS:
							this->setup_bfi_menu();
							done = true;
							break;
						case VIDEO_MENU_RESOLUTION_SETTINGS:
							this->setup_resolution_menu();
							done = true;
							break;
						case VIDEO_MENU_BOTTOM_SCREEN_POS:
							this->setup_relative_pos_menu();
							done = true;
							break;
						case VIDEO_MENU_GAMES_CROPPING:
							this->game_crop_enable_change();
							break;
						case VIDEO_MENU_NON_INT_SCALING:
							this->non_int_scaling_change(false);
							break;
						case VIDEO_MENU_SCALING_RATIO_SETTINGS:
							this->setup_scaling_ratio_menu();
							break;
						case VIDEO_MENU_CHANGE_TITLEBAR:
							this->titlebar_change();
							break;
						default:
							break;
					}
					this->video_menu->reset_output_option();
					continue;
				}
				break;
			case CROP_MENU_TYPE:
				if(this->crop_menu->poll(event_data)) {
					switch(this->crop_menu->selected_index) {
						case CROP_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case CROP_MENU_NO_ACTION:
							break;
						default:
							this->crop_value_change(this->crop_menu->selected_index);
							break;
					}
					this->crop_menu->reset_output_option();
					continue;
				}
				break;
			case TOP_PAR_MENU_TYPE:
				if(this->par_menu->poll(event_data)) {
					switch(this->par_menu->selected_index) {
						case PAR_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case PAR_MENU_NO_ACTION:
							break;
						default:
							this->par_value_change(this->par_menu->selected_index, true);
							break;
					}
					this->par_menu->reset_output_option();
					continue;
				}
				break;
			case BOTTOM_PAR_MENU_TYPE:
				if(this->par_menu->poll(event_data)) {
					switch(this->par_menu->selected_index) {
						case PAR_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case PAR_MENU_NO_ACTION:
							break;
						default:
							this->par_value_change(this->par_menu->selected_index, false);
							break;
					}
					this->par_menu->reset_output_option();
					continue;
				}
				break;
			case ROTATION_MENU_TYPE:
				if(this->rotation_menu->poll(event_data)) {
					switch(this->rotation_menu->selected_index) {
						case ROTATION_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case ROTATION_MENU_NO_ACTION:
							break;
						case ROTATION_MENU_TOP_ROTATION_DEC:
							this->rotation_change(this->m_info.top_rotation, false);
							break;
						case ROTATION_MENU_TOP_ROTATION_INC:
							this->rotation_change(this->m_info.top_rotation, true);
							break;
						case ROTATION_MENU_BOTTOM_ROTATION_DEC:
							this->rotation_change(this->m_info.bot_rotation, false);
							break;
						case ROTATION_MENU_BOTTOM_ROTATION_INC:
							this->rotation_change(this->m_info.bot_rotation, true);
							break;
						case ROTATION_MENU_BOTH_ROTATION_DEC:
							this->rotation_change(this->m_info.top_rotation, false);
							this->rotation_change(this->m_info.bot_rotation, false);
							break;
						case ROTATION_MENU_BOTH_ROTATION_INC:
							this->rotation_change(this->m_info.top_rotation, true);
							this->rotation_change(this->m_info.bot_rotation, true);
							break;
						default:
							break;
					}
					this->rotation_menu->reset_output_option();
					continue;
				}
				break;
			case OFFSET_MENU_TYPE:
				if(this->offset_menu->poll(event_data)) {
					switch(this->offset_menu->selected_index) {
						case OFFSET_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case OFFSET_MENU_NO_ACTION:
							break;
						case OFFSET_MENU_SMALL_OFFSET_DEC:
							this->offset_change(this->m_info.subscreen_offset, -0.1);
							break;
						case OFFSET_MENU_SMALL_OFFSET_INC:
							this->offset_change(this->m_info.subscreen_offset, 0.1);
							break;
						case OFFSET_MENU_SMALL_SCREEN_DISTANCE_DEC:
							this->offset_change(this->m_info.subscreen_attached_offset, -0.1);
							break;
						case OFFSET_MENU_SMALL_SCREEN_DISTANCE_INC:
							this->offset_change(this->m_info.subscreen_attached_offset, 0.1);
							break;
						case OFFSET_MENU_SCREENS_X_POS_DEC:
							this->offset_change(this->m_info.total_offset_x, -0.1);
							break;
						case OFFSET_MENU_SCREENS_X_POS_INC:
							this->offset_change(this->m_info.total_offset_x, 0.1);
							break;
						case OFFSET_MENU_SCREENS_Y_POS_DEC:
							this->offset_change(this->m_info.total_offset_y, -0.1);
							break;
						case OFFSET_MENU_SCREENS_Y_POS_INC:
							this->offset_change(this->m_info.total_offset_y, 0.1);
							break;
						default:
							break;
					}
					this->offset_menu->reset_output_option();
					continue;
				}
				break;
			case BFI_MENU_TYPE:
				if(this->bfi_menu->poll(event_data)) {
					switch(this->bfi_menu->selected_index) {
						case BFI_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case BFI_MENU_NO_ACTION:
							break;
						case BFI_MENU_TOGGLE:
							this->bfi_change();
							break;
						case BFI_MENU_DIVIDER_DEC:
							this->m_info.bfi_divider -= 1;
							if(this->m_info.bfi_divider < 2)
								this->m_info.bfi_divider = 2;
							if(this->m_info.bfi_amount > (this->m_info.bfi_divider - 1))
								this->m_info.bfi_amount = this->m_info.bfi_divider - 1;
							break;
						case BFI_MENU_DIVIDER_INC:
							this->m_info.bfi_divider += 1;
							if(this->m_info.bfi_divider > 10)
								this->m_info.bfi_divider = 10;
							break;
						case BFI_MENU_AMOUNT_DEC:
							this->m_info.bfi_amount -= 1;
							if(this->m_info.bfi_amount < 1)
								this->m_info.bfi_amount = 1;
							break;
						case BFI_MENU_AMOUNT_INC:
							this->m_info.bfi_amount += 1;
							if(this->m_info.bfi_amount > (this->m_info.bfi_divider - 1))
								this->m_info.bfi_amount = this->m_info.bfi_divider - 1;
							break;
						default:
							break;
					}
					this->bfi_menu->reset_output_option();
					continue;
				}
				break;
			case RELATIVE_POS_MENU_TYPE:
				if(this->relpos_menu->poll(event_data)) {
					switch(this->relpos_menu->selected_index) {
						case REL_POS_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case REL_POS_MENU_NO_ACTION:
							break;
						case REL_POS_MENU_CONFIRM:
							this->bottom_pos_change(this->relpos_menu->selected_confirm_value);
							break;
						default:
							break;
					}
					this->relpos_menu->reset_output_option();
					continue;
				}
				break;
			case RESOLUTION_MENU_TYPE:
				if(this->resolution_menu->poll(event_data)) {
					switch(this->resolution_menu->selected_index) {
						case RESOLUTION_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case RESOLUTION_MENU_NO_ACTION:
							break;
						default:
							this->m_info.fullscreen_mode_width = possible_resolutions[this->resolution_menu->selected_index].size.x;
							this->m_info.fullscreen_mode_height = possible_resolutions[this->resolution_menu->selected_index].size.y;
							this->m_info.fullscreen_mode_bpp = possible_resolutions[this->resolution_menu->selected_index].bitsPerPixel;
							this->create_window(true);
							break;
					}
					this->resolution_menu->reset_output_option();
					continue;
				}
				break;
			case EXTRA_MENU_TYPE:
				if(this->extra_menu->poll(event_data)) {
					switch(this->extra_menu->selected_index) {
						case EXTRA_SETTINGS_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case EXTRA_SETTINGS_MENU_NO_ACTION:
							break;
						case EXTRA_SETTINGS_MENU_QUIT_APPLICATION:
							this->set_close(0);
							this->setup_no_menu();
							done = true;
							break;
						case EXTRA_SETTINGS_MENU_FULLSCREEN:
							this->fullscreen_change();
							done = true;
							break;
						case EXTRA_SETTINGS_MENU_SPLIT:
							this->split_change();
							done = true;
							break;
						default:
							break;
					}
					this->extra_menu->reset_output_option();
					continue;
				}
				break;
			case SHORTCUTS_MENU_TYPE:
				if(this->shortcut_menu->poll(event_data)) {
					switch(this->shortcut_menu->selected_index) {
						case SHORTCUT_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case SHORTCUT_MENU_NO_ACTION:
							break;
						default:
							this->chosen_button = this->shortcut_menu->selected_index;
							this->setup_action_selection_menu();
							done = true;
							break;
					}
					this->shortcut_menu->reset_output_option();
					continue;
				}
				break;
			case ACTION_SELECTION_MENU_TYPE:
				if(this->action_selection_menu->poll(event_data)) {
					switch(this->action_selection_menu->selected_index) {
						case ACTION_SELECTION_MENU_BACK:
							this->setup_shortcuts_menu(false);
							done = true;
							break;
						case ACTION_SELECTION_MENU_NO_ACTION:
							break;
						default:
							(*this->possible_buttons_ptrs[this->chosen_button]) = this->possible_actions[this->action_selection_menu->selected_index];
							break;
					}
					this->action_selection_menu->reset_output_option();
					continue;
				}
				break;
			case STATUS_MENU_TYPE:
				if(this->status_menu->poll(event_data)) {
					switch(this->status_menu->selected_index) {
						case STATUS_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case STATUS_MENU_NO_ACTION:
							break;
						default:
							break;
					}
					this->status_menu->reset_output_option();
					continue;
				}
				break;
			case LICENSES_MENU_TYPE:
				if(this->license_menu->poll(event_data)) {
					switch(this->license_menu->selected_index) {
						case LICENSE_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case LICENSE_MENU_NO_ACTION:
							break;
						default:
							break;
					}
					this->license_menu->reset_output_option();
					continue;
				}
				break;
			case SCALING_RATIO_MENU_TYPE:
				if(this->scaling_ratio_menu->poll(event_data)) {
					switch(this->scaling_ratio_menu->selected_index) {
						case SCALING_RATIO_MENU_BACK:
							this->setup_video_menu(false);
							done = true;
							break;
						case SCALING_RATIO_MENU_NO_ACTION:
							break;
						case SCALING_RATIO_MENU_FULLSCREEN_SCALING_TOP:
							this->ratio_change(true);
							break;
						case SCALING_RATIO_MENU_FULLSCREEN_SCALING_BOTTOM:
							this->ratio_change(false);
							break;
						case SCALING_RATIO_MENU_NON_INT_SCALING_TOP:
							this->non_int_scaling_change(true);
							break;
						case SCALING_RATIO_MENU_NON_INT_SCALING_BOTTOM:
							this->non_int_scaling_change(false);
							break;
						case SCALING_RATIO_MENU_ALGO_DEC:
							this->non_int_mode_change(false);
							break;
						case SCALING_RATIO_MENU_ALGO_INC:
							this->non_int_mode_change(true);
							break;
						default:
							break;
					}
					this->scaling_ratio_menu->reset_output_option();
					continue;
				}
				break;
			case ISN_MENU_TYPE:
				if(this->is_nitro_menu->poll(event_data)) {
					switch(this->is_nitro_menu->selected_index) {
						case ISN_MENU_BACK:
							this->setup_main_menu(false);
							done = true;
							break;
						case ISN_MENU_NO_ACTION:
							break;
						case ISN_MENU_DELAY:
							break;
						case ISN_MENU_TYPE_DEC:
							this->is_nitro_capture_type_change(false);
							break;
						case ISN_MENU_TYPE_INC:
							this->is_nitro_capture_type_change(true);
							break;
						case ISN_MENU_RESET:
							this->is_nitro_capture_reset_hw();
							break;
						default:
							break;
					}
					this->is_nitro_menu->reset_output_option();
					continue;
				}
				break;
			default:
				break;
		}
	}
}

void WindowScreen::setup_connection_menu(std::vector<CaptureDevice> *devices_list, bool reset_data) {
	// Skip the check here. It's special.
	this->curr_menu = CONNECT_MENU_TYPE;
	if(reset_data)
		this->connection_menu->reset_data();
	this->connection_menu->insert_data(devices_list);
}

int WindowScreen::check_connection_menu_result() {
	return this->connection_menu->selected_index;
}

void WindowScreen::end_connection_menu() {
	this->setup_no_menu();
}

void WindowScreen::update_ds_3ds_connection(bool changed_type) {
	if(changed_type && (this->curr_menu == CROP_MENU_TYPE))
		this->setup_no_menu();
	this->prepare_size_ratios(false, false);
	this->future_operations.call_crop = true;
}

void WindowScreen::update_capture_specific_settings() {
	if(this->curr_menu == MAIN_MENU_TYPE) {
		this->setup_no_menu();
		this->setup_main_menu(true, true);
	}
	if(this->curr_menu == ISN_MENU_TYPE)
		this->setup_no_menu();
}

int WindowScreen::load_data() {
	int ret_val = this->m_prepare_load;
	this->m_prepare_load = 0;
	return ret_val;
}

int WindowScreen::save_data() {
	int ret_val = this->m_prepare_save;
	this->m_prepare_save = 0;
	return ret_val;
}

bool WindowScreen::open_capture() {
	bool ret_val = this->m_prepare_open;
	this->m_prepare_open = false;
	return ret_val;
}

bool WindowScreen::close_capture() {
	return this->m_prepare_quit;
}

int WindowScreen::get_ret_val() {
	if(!this->m_prepare_quit)
		return -1;
	return this->ret_val;
}

bool WindowScreen::query_reset_request() {
	std::chrono::time_point<std::chrono::high_resolution_clock> curr_time = std::chrono::high_resolution_clock::now();
	if(check_held_diff(curr_time, this->right_click_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->enter_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->pgdown_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->extra_enter_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->extra_pgdown_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->controller_button_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->touch_action) > this->bad_resolution_timeout)
		return true;
	return false;
}

void WindowScreen::reset_held_times() {
	check_held_reset(false, this->right_click_action);
	check_held_reset(false, this->enter_action);
	check_held_reset(false, this->pgdown_action);
	check_held_reset(false, this->extra_enter_action);
	check_held_reset(false, this->extra_pgdown_action);
	check_held_reset(false, this->controller_button_action);
	check_held_reset(false, this->touch_action);
}

void WindowScreen::poll_window(bool do_everything) {
	if(this->m_win.isOpen()) {
		if(do_everything) {
			auto curr_time = std::chrono::high_resolution_clock::now();
			const std::chrono::duration<double> diff = curr_time - this->last_poll_time;
			FPSArrayInsertElement(&poll_fps, diff.count());
			this->last_poll_time = curr_time;
			while(const std::optional event = this->m_win.pollEvent()) {
				if(event->is<sf::Event::Closed>())
					events_queue.emplace(EVENT_CLOSED);
				else if(const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
					events_queue.emplace(true, keyPressed->code);
				else if(const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
					events_queue.emplace(false, keyReleased->code);
				else if(const auto* textEntered = event->getIf<sf::Event::TextEntered>())
					events_queue.emplace(textEntered->unicode);
				else if(const auto* joystick_press = event->getIf<sf::Event::JoystickButtonPressed>())
					events_queue.emplace(true, joystick_press->joystickId, joystick_press->button);
				else if(const auto* joystick_release = event->getIf<sf::Event::JoystickButtonReleased>())
					events_queue.emplace(false, joystick_release->joystickId, joystick_release->button);
				else if(const auto* joystick_movement = event->getIf<sf::Event::JoystickMoved>())
					events_queue.emplace(joystick_movement->joystickId, joystick_movement->axis, joystick_movement->position);
				else if(const auto* mouse_pressed = event->getIf<sf::Event::MouseButtonPressed>())
					events_queue.emplace(true, mouse_pressed->button, mouse_pressed->position);
				else if(const auto* mouse_released = event->getIf<sf::Event::MouseButtonReleased>())
					events_queue.emplace(false, mouse_released->button, mouse_released->position);
				else if(const auto* mouse_movement = event->getIf<sf::Event::MouseMoved>())
					events_queue.emplace(mouse_movement->position);
			}
		}
		if(this->m_win.hasFocus()) {
			if(do_everything) {
				check_held_reset(sf::Mouse::isButtonPressed(sf::Mouse::Button::Right), this->right_click_action);
				bool found = false;
				for(int i = 0; i < sf::Joystick::Count; i++) {
					if(!sf::Joystick::isConnected(i))
						continue;
					if(sf::Joystick::getButtonCount(i) <= 0)
						continue;
					found = true;
					check_held_reset(sf::Joystick::isButtonPressed(i, 0), this->controller_button_action);
					break;
				}
				if(!found)
					check_held_reset(false, this->controller_button_action);
				bool touch_active = sf::Touch::isDown(0);
				check_held_reset(touch_active, this->touch_action);
				check_held_reset(touch_active, this->touch_right_click_action);
				if(touch_active) {
					sf::Vector2i touch_pos = sf::Touch::getPosition(0, this->m_win);
					auto curr_time = std::chrono::high_resolution_clock::now();
					const std::chrono::duration<double> diff = curr_time - this->touch_right_click_action.start_time;
					if(diff.count() > this->touch_long_press_timer) {
						events_queue.emplace(true, sf::Mouse::Button::Right, touch_pos);
						this->touch_right_click_action.start_time = std::chrono::high_resolution_clock::now();
					}
					events_queue.emplace(true, sf::Mouse::Button::Left, touch_pos);
				}
				joystick_axis_poll(this->events_queue);
			}
			extra_buttons_poll(this->events_queue);
		}
		else {
			this->reset_held_times();
			check_held_reset(false, this->touch_right_click_action);
		}
	}
	else {
		this->reset_held_times();
		check_held_reset(false, this->touch_right_click_action);
	}
}

void WindowScreen::prepare_menu_draws(int view_size_x, int view_size_y) {
	switch(this->loaded_menu) {
		case CONNECT_MENU_TYPE:
			this->connection_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
			break;
		case MAIN_MENU_TYPE:
			this->main_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->capture_status->connected);
			break;
		case VIDEO_MENU_TYPE:
			this->video_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->loaded_info, this->display_data->fast_poll, this->m_stype);
			break;
		case CROP_MENU_TYPE:
			this->crop_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, *this->get_crop_index_ptr(&this->loaded_info));
			break;
		case TOP_PAR_MENU_TYPE:
			this->par_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->loaded_info.top_par);
			break;
		case BOTTOM_PAR_MENU_TYPE:
			this->par_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->loaded_info.bot_par);
			break;
		case ROTATION_MENU_TYPE:
			this->rotation_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->loaded_info);
			break;
		case OFFSET_MENU_TYPE:
			this->offset_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->loaded_info);
			break;
		case AUDIO_MENU_TYPE:
			this->audio_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->audio_data);
			break;
		case BFI_MENU_TYPE:
			this->bfi_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->loaded_info);
			break;
		case RELATIVE_POS_MENU_TYPE:
			this->relpos_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->loaded_info.bottom_pos);
			break;
		case RESOLUTION_MENU_TYPE:
			this->resolution_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->loaded_info.fullscreen_mode_width, this->loaded_info.fullscreen_mode_height);
			break;
		case SAVE_MENU_TYPE:
			this->fileconfig_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
			break;
		case LOAD_MENU_TYPE:
			this->fileconfig_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
			break;
		case EXTRA_MENU_TYPE:
			this->extra_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
			break;
		case SHORTCUTS_MENU_TYPE:
			this->shortcut_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
			break;
		case ACTION_SELECTION_MENU_TYPE:
			this->action_selection_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, (*this->possible_buttons_ptrs[this->chosen_button])->cmd);
			break;
		case STATUS_MENU_TYPE:
			this->status_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, FPSArrayGetAverage(&in_fps), FPSArrayGetAverage(&poll_fps), FPSArrayGetAverage(&draw_fps), this->capture_status);
			break;
		case LICENSES_MENU_TYPE:
			this->license_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
			break;
		case SCALING_RATIO_MENU_TYPE:
			this->scaling_ratio_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->loaded_info);
			break;
		case ISN_MENU_TYPE:
			this->is_nitro_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->capture_status);
			break;
		default:
			break;
	}
}

void WindowScreen::execute_menu_draws() {
	switch(this->loaded_menu) {
		case CONNECT_MENU_TYPE:
			this->connection_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case MAIN_MENU_TYPE:
			this->main_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case VIDEO_MENU_TYPE:
			this->video_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case CROP_MENU_TYPE:
			this->crop_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case TOP_PAR_MENU_TYPE:
			this->par_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case BOTTOM_PAR_MENU_TYPE:
			this->par_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case ROTATION_MENU_TYPE:
			this->rotation_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case OFFSET_MENU_TYPE:
			this->offset_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case AUDIO_MENU_TYPE:
			this->audio_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case BFI_MENU_TYPE:
			this->bfi_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case RELATIVE_POS_MENU_TYPE:
			this->relpos_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case RESOLUTION_MENU_TYPE:
			this->resolution_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case SAVE_MENU_TYPE:
			this->fileconfig_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case LOAD_MENU_TYPE:
			this->fileconfig_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case EXTRA_MENU_TYPE:
			this->extra_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case SHORTCUTS_MENU_TYPE:
			this->shortcut_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case ACTION_SELECTION_MENU_TYPE:
			this->action_selection_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case STATUS_MENU_TYPE:
			this->status_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case LICENSES_MENU_TYPE:
			this->license_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case SCALING_RATIO_MENU_TYPE:
			this->scaling_ratio_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case ISN_MENU_TYPE:
			this->is_nitro_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		default:
			break;
	}
}

#include "frontend.hpp"

#define FPS_WINDOW_SIZE 64

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
}

void WindowScreen::set_close(int ret_val) {
	this->m_prepare_quit = true;
	this->ret_val = ret_val;
}

void WindowScreen::split_change() {
	if(this->curr_menu != CONNECT_MENU_TYPE)
		this->curr_menu = DEFAULT_MENU_TYPE;
	this->m_info.is_fullscreen = false;
	this->display_data->split = !this->display_data->split;
}

void WindowScreen::fullscreen_change() {
	if(this->curr_menu != CONNECT_MENU_TYPE)
		this->curr_menu = DEFAULT_MENU_TYPE;
	this->m_info.is_fullscreen = !this->m_info.is_fullscreen;
	this->create_window(true);
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
	this->print_notification_on_off("Fast Poll", this->display_data->fast_poll);
}

void WindowScreen::padding_change() {
	if(this->m_info.is_fullscreen)
		return;
	this->m_info.rounded_corners_fix = !this->m_info.rounded_corners_fix;
	this->future_operations.call_screen_settings_update = true;
	this->print_notification_on_off("Extra Padding", this->m_info.rounded_corners_fix);
}

void WindowScreen::crop_value_change(int new_crop_value) {
	int new_value = new_crop_value % this->possible_crops.size();
	if(this->m_info.crop_kind != new_value) {
		this->m_info.crop_kind = new_value;
		this->print_notification("Crop: " + this->possible_crops[this->m_info.crop_kind]->name);
		this->prepare_size_ratios(false, false);
		this->future_operations.call_crop = true;
	}
}

void WindowScreen::par_value_change(int new_par_value, bool is_top) {
	if(((!is_top) && (this->m_stype == ScreenType::TOP)) || ((is_top) && (this->m_stype == ScreenType::BOTTOM)))
		return;
	int par_index = new_par_value % this->possible_pars.size();
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

void WindowScreen::ratio_change(bool top_priority) {
	this->prepare_size_ratios(top_priority, !top_priority);
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

bool WindowScreen::common_poll(SFEvent &event_data) {
	bool consumed = true;
	switch(event_data.type) {
		case sf::Event::Closed:
			this->set_close(0);
			break;

		case sf::Event::TextEntered:
			switch(event_data.unicode) {
				case 's':
					this->split_change();
					break;

				case 'f':
					this->fullscreen_change();
					break;

				case 'a':
					this->async_change();
					break;

				case 'v':
					this->vsync_change();
					break;

				case 'z':
					this->menu_scaling_change(false);
					break;

				case 'x':
					this->menu_scaling_change(true);
					break;

				case '-':
					this->window_scaling_change(false);
					break;

				case '0':
					this->window_scaling_change(true);
					break;

				default:
					consumed = false;
					break;
			}

			break;
			
		case sf::Event::KeyPressed:
			switch(event_data.code) {
				case sf::Keyboard::Escape:
					this->set_close(0);
					break;
				case sf::Keyboard::Enter:
					if(event_data.is_extra)
						check_held_reset(true, this->extra_enter_action);
					else
						check_held_reset(true, this->enter_action);
					consumed = false;
					break;
				case sf::Keyboard::PageDown:
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
		case sf::Event::KeyReleased:
			switch(event_data.code) {
				case sf::Keyboard::Enter:
					if(event_data.is_extra)
						check_held_reset(false, this->extra_enter_action);
					else
						check_held_reset(false, this->enter_action);
					consumed = false;
					break;
				case sf::Keyboard::PageDown:
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
		case sf::Event::MouseMoved:
			this->m_info.show_mouse = true;
			this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			consumed = false;
			break;
		case sf::Event::MouseButtonPressed:
			this->m_info.show_mouse = true;
			this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			consumed = false;
			break;
		case sf::Event::MouseButtonReleased:
			this->m_info.show_mouse = true;
			this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			consumed = false;
			break;
		case sf::Event::JoystickButtonPressed:
			consumed = false;
			break;
		case sf::Event::JoystickMoved:
			consumed = false;
			break;
		default:
			consumed = false;
			break;
	}
	return consumed;
}

void WindowScreen::setup_main_menu(bool reset_data) {
	if(this->curr_menu != MAIN_MENU_TYPE) {
		this->curr_menu = MAIN_MENU_TYPE;
		if(reset_data)
			this->main_menu->reset_data();
		this->main_menu->insert_data(this->m_stype, this->m_info.is_fullscreen, this->display_data->mono_app_mode);
	}
}

void WindowScreen::setup_video_menu(bool reset_data) {
	if(this->curr_menu != VIDEO_MENU_TYPE) {
		this->curr_menu = VIDEO_MENU_TYPE;
		if(reset_data)
			this->video_menu->reset_data();
		this->video_menu->insert_data(this->m_stype, this->m_info.is_fullscreen);
	}
}

void WindowScreen::setup_crop_menu(bool reset_data) {
	if(this->curr_menu != CROP_MENU_TYPE) {
		this->curr_menu = CROP_MENU_TYPE;
		if(reset_data)
			this->crop_menu->reset_data();
		this->crop_menu->insert_data(&this->possible_crops);
	}
}

void WindowScreen::setup_par_menu(bool is_top, bool reset_data) {
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
	}
}

void WindowScreen::setup_offset_menu(bool reset_data) {
	if(this->curr_menu != OFFSET_MENU_TYPE) {
		this->curr_menu = OFFSET_MENU_TYPE;
		if(reset_data)
			this->offset_menu->reset_data();
		this->offset_menu->insert_data();
	}
}

void WindowScreen::setup_rotation_menu(bool reset_data) {
	if(this->curr_menu != ROTATION_MENU_TYPE) {
		this->curr_menu = ROTATION_MENU_TYPE;
		if(reset_data)
			this->rotation_menu->reset_data();
		this->rotation_menu->insert_data();
	}
}

void WindowScreen::setup_audio_menu(bool reset_data) {
	if(this->curr_menu != AUDIO_MENU_TYPE) {
		this->curr_menu = AUDIO_MENU_TYPE;
		if(reset_data)
			this->audio_menu->reset_data();
		this->audio_menu->insert_data();
	}
}

void WindowScreen::setup_bfi_menu(bool reset_data) {
	if(this->curr_menu != BFI_MENU_TYPE) {
		this->curr_menu = BFI_MENU_TYPE;
		if(reset_data)
			this->bfi_menu->reset_data();
		this->bfi_menu->insert_data();
	}
}

void WindowScreen::setup_fileconfig_menu(bool is_save, bool reset_data) {
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
	}
}

void WindowScreen::setup_resolution_menu(bool reset_data) {
	if(this->curr_menu != RESOLUTION_MENU_TYPE) {
		this->possible_resolutions.clear();
		std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
		if(modes.size() > 0)
			this->possible_resolutions.push_back(modes[0]);
		for(int i = 1; i < modes.size(); ++i)
			if(this->possible_resolutions[0].bitsPerPixel == modes[i].bitsPerPixel)
				this->possible_resolutions.push_back(modes[i]);
		if(this->possible_resolutions.size() > 0) {
			this->curr_menu = RESOLUTION_MENU_TYPE;
			if(reset_data)
				this->resolution_menu->reset_data();
			this->resolution_menu->insert_data(&this->possible_resolutions);
		}
		else
			this->print_notification("No Resolution found", TEXT_KIND_WARNING);
	}
}

void WindowScreen::setup_extra_menu(bool reset_data) {
	if(this->curr_menu != EXTRA_MENU_TYPE) {
		this->curr_menu = EXTRA_MENU_TYPE;
		if(reset_data)
			this->extra_menu->reset_data();
		this->extra_menu->insert_data(this->m_stype, this->m_info.is_fullscreen);
	}
}

void WindowScreen::setup_status_menu(bool reset_data) {
	if(this->curr_menu != STATUS_MENU_TYPE) {
		this->curr_menu = STATUS_MENU_TYPE;
		if(reset_data)
			this->status_menu->reset_data();
		this->status_menu->insert_data();
	}
}

void WindowScreen::setup_licenses_menu(bool reset_data) {
	if(this->curr_menu != LICENSES_MENU_TYPE) {
		this->curr_menu = LICENSES_MENU_TYPE;
		if(reset_data)
			this->license_menu->reset_data();
		this->license_menu->insert_data();
	}
}

void WindowScreen::setup_relative_pos_menu(bool reset_data) {
	if(this->curr_menu != RELATIVE_POS_MENU_TYPE) {
		this->curr_menu = RELATIVE_POS_MENU_TYPE;
		if(reset_data)
			this->relpos_menu->reset_data();
		this->relpos_menu->insert_data();
	}
}

void WindowScreen::update_save_menu() {
	if(this->curr_menu == SAVE_MENU_TYPE) {
		this->curr_menu = DEFAULT_MENU_TYPE;
		this->setup_fileconfig_menu(true, false);
	}
}

bool WindowScreen::no_menu_poll(SFEvent &event_data) {
	bool consumed = true;
	switch(event_data.type) {
		case sf::Event::TextEntered:
			switch(event_data.unicode) {
				default:
					consumed = false;
					break;
			}
			break;
		case sf::Event::KeyPressed:
			switch(event_data.code) {
				case sf::Keyboard::Enter:
					this->setup_main_menu();
					break;
				case sf::Keyboard::PageDown:
					this->setup_main_menu();
					break;
				default:
					consumed = false;
					break;
			}
			break;
		case sf::Event::MouseMoved:
			consumed = false;
			break;
		case sf::Event::MouseButtonPressed:
			if(event_data.mouse_button == sf::Mouse::Right)
				this->setup_main_menu();
			else
				consumed = false;
			break;
		case sf::Event::MouseButtonReleased:
			consumed = false;
			break;
		case sf::Event::JoystickButtonPressed:switch(get_joystick_action(event_data.joystickId, event_data.joy_button)) {
			case JOY_ACTION_MENU:
				this->setup_main_menu();
				break;
			default:
				consumed = false;
				break;
			}
			break;
		case sf::Event::JoystickMoved:
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
		case sf::Event::TextEntered:
			switch(event_data.unicode) {
				case 'c':
					this->crop_value_change(this->m_info.crop_kind + 1);
					break;

				case 'b':
					this->blur_change();
					break;

				//case 'i':
				//	this->bfi_change();
				//	break;

				case 'o':
					this->m_prepare_open = true;
					break;

				case 't':
					this->bottom_pos_change(this->m_info.bottom_pos + 1);
					break;

				case '6':
					this->offset_change(this->m_info.subscreen_offset, 0.5);
					break;

				case '7':
					this->offset_change(this->m_info.subscreen_attached_offset, 0.5);
					break;

				case '4':
					this->offset_change(this->m_info.total_offset_x, 0.5);
					break;

				case '5':
					this->offset_change(this->m_info.total_offset_y, 0.5);
					break;

				case 'y':
					this->ratio_change(true);
					break;

				case 'u':
					this->ratio_change(false);
					break;

				case '8':
					this->rotation_change(this->m_info.top_rotation, false);
					this->rotation_change(this->m_info.bot_rotation, false);
					break;

				case '9':
					this->rotation_change(this->m_info.top_rotation, true);
					this->rotation_change(this->m_info.bot_rotation, true);
					break;

				case 'h':
					this->rotation_change(this->m_info.top_rotation, false);
					break;

				case 'j':
					this->rotation_change(this->m_info.top_rotation, true);
					break;

				case 'k':
					this->rotation_change(this->m_info.bot_rotation, false);
					break;

				case 'l':
					this->rotation_change(this->m_info.bot_rotation, true);
					break;

				case 'r':
					this->padding_change();
					break;

				case '2':
					this->par_value_change(this->m_info.top_par + 1, true);
					break;

				case '3':
					this->par_value_change(this->m_info.bot_par + 1, false);
					break;

				case 'm':
					audio_data->change_audio_mute();
					break;

				case ',':
					audio_data->change_audio_volume(false);
					break;

				case '.':
					audio_data->change_audio_volume(true);
					break;

				case 'n':
					audio_data->request_audio_restart();
					break;

				default:
					consumed = false;
					break;
			}

			break;
			
		case sf::Event::KeyPressed:
			switch(event_data.code) {
				case sf::Keyboard::F1:
				case sf::Keyboard::F2:
				case sf::Keyboard::F3:
				case sf::Keyboard::F4:
					this->m_prepare_load = event_data.code - sf::Keyboard::F1 + 1;
					break;

				case sf::Keyboard::F5:
				case sf::Keyboard::F6:
				case sf::Keyboard::F7:
				case sf::Keyboard::F8:
					this->m_prepare_save = event_data.code - sf::Keyboard::F5 + 1;
					break;
				default:
					consumed = false;
					break;
			}

			break;
		case sf::Event::MouseMoved:
			consumed = false;
			break;
		case sf::Event::MouseButtonPressed:
			consumed = false;
			break;
		case sf::Event::MouseButtonReleased:
			consumed = false;
			break;
		case sf::Event::JoystickButtonPressed:
			consumed = false;
			break;
		case sf::Event::JoystickMoved:
			consumed = false;
			break;
		default:
			consumed = false;
			break;
	}
	return consumed;
}

void WindowScreen::poll() {
	if(this->close_capture())
		return;
	if((this->m_info.is_fullscreen || this->display_data->mono_app_mode) && this->m_info.show_mouse) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_mouse_action_time;
		if(diff.count() > this->mouse_timeout)
			this->m_info.show_mouse = false;
	}
	this->poll_window();
	bool done = false;
	while(!events_queue.empty()) {
		if(done)
			break;
		SFEvent event_data = events_queue.front();
		events_queue.pop();
		if((event_data.type == sf::Event::KeyPressed) && event_data.poweroff_cmd) {
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
							this->curr_menu = DEFAULT_MENU_TYPE;
							done = true;
							break;
						case MAIN_MENU_QUIT_APPLICATION:
							this->set_close(0);
							this->curr_menu = DEFAULT_MENU_TYPE;
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
						case MAIN_MENU_SHUTDOWN:
							this->set_close(1);
							this->curr_menu = DEFAULT_MENU_TYPE;
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
						case VIDEO_MENU_FULLSCREEN_SCALING_TOP:
							this->ratio_change(true);
							break;
						case VIDEO_MENU_FULLSCREEN_SCALING_BOTTOM:
							this->ratio_change(false);
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
							this->m_info.fullscreen_mode_width = possible_resolutions[this->resolution_menu->selected_index].width;
							this->m_info.fullscreen_mode_height = possible_resolutions[this->resolution_menu->selected_index].height;
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
							this->curr_menu = DEFAULT_MENU_TYPE;
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
			default:
				break;
		}
	}
}

void WindowScreen::setup_connection_menu(DevicesList *devices_list, bool reset_data) {
	this->curr_menu = CONNECT_MENU_TYPE;
	if(reset_data)
		this->connection_menu->reset_data();
	this->connection_menu->insert_data(devices_list);
}

int WindowScreen::check_connection_menu_result() {
	return this->connection_menu->selected_index;
}

void WindowScreen::end_connection_menu() {
	this->curr_menu = DEFAULT_MENU_TYPE;
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
	auto curr_time = std::chrono::high_resolution_clock::now();
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

void WindowScreen::poll_window() {
	if(this->m_win.isOpen()) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_poll_time;
		FPSArrayInsertElement(&poll_fps, diff.count());
		this->last_poll_time = curr_time;
		sf::Event event;
		while(this->m_win.pollEvent(event)) {
			int joystickId = event.joystickConnect.joystickId;
			if(event.type == sf::Event::JoystickButtonPressed)
				joystickId = event.joystickButton.joystickId;
			else if(event.type == sf::Event::JoystickMoved)
				joystickId = event.joystickMove.joystickId;
			int mouse_x = event.mouseButton.x;
			int mouse_y = event.mouseButton.y;
			if(event.type == sf::Event::MouseMoved) {
				mouse_x = event.mouseMove.x;
				mouse_y = event.mouseMove.y;
			}
			events_queue.emplace(event.type, event.key.code, event.text.unicode, joystickId, event.joystickButton.button, event.joystickMove.axis, 0.0, event.mouseButton.button, mouse_x, mouse_y, false, false);
		}
		if(this->m_win.hasFocus()) {
			check_held_reset(sf::Mouse::isButtonPressed(sf::Mouse::Right), this->right_click_action);
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
					events_queue.emplace(sf::Event::MouseButtonPressed, sf::Keyboard::Backspace, 0, 0, 0, sf::Joystick::Axis::X, 0.0, sf::Mouse::Right, touch_pos.x, touch_pos.y, false, false);
					this->touch_right_click_action.start_time = std::chrono::high_resolution_clock::now();
				}
				events_queue.emplace(sf::Event::MouseButtonPressed, sf::Keyboard::Backspace, 0, 0, 0, sf::Joystick::Axis::X, 0.0, sf::Mouse::Left, touch_pos.x, touch_pos.y, false, false);
			}
			joystick_axis_poll(this->events_queue);
			extra_buttons_poll(this->events_queue);
		}
		else {
			this->reset_held_times();
			this->touch_right_click_action.started = false;
		}
	}
	else {
		this->reset_held_times();
		this->touch_right_click_action.started = false;
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
			this->video_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->loaded_info, this->display_data->fast_poll);
			break;
		case CROP_MENU_TYPE:
			this->crop_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->loaded_info.crop_kind);
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
			this->resolution_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->curr_desk_mode);
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
		case STATUS_MENU_TYPE:
			this->status_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, FPSArrayGetAverage(&in_fps), FPSArrayGetAverage(&poll_fps), FPSArrayGetAverage(&draw_fps));
			break;
		case LICENSES_MENU_TYPE:
			this->license_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
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
		case STATUS_MENU_TYPE:
			this->status_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		case LICENSES_MENU_TYPE:
			this->license_menu->draw(this->loaded_info.menu_scaling_factor, this->m_win);
			break;
		default:
			break;
	}
}
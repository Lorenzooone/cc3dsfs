#include "frontend.hpp"

#include <cstring>
#include "font_ttf.h"

#define LEFT_ROUNDED_PADDING 5
#define RIGHT_ROUNDED_PADDING 5
#define TOP_ROUNDED_PADDING 0
#define BOTTOM_ROUNDED_PADDING 5

WindowScreen::WindowScreen(ScreenType stype, CaptureStatus* capture_status, DisplayData* display_data, AudioData* audio_data, std::mutex* events_access) {
	this->m_stype = stype;
	insert_basic_crops(this->possible_crops, this->m_stype);
	insert_basic_pars(this->possible_pars);
	this->events_access = events_access;
	this->m_prepare_save = 0;
	this->m_prepare_load = 0;
	this->m_prepare_open = false;
	this->m_prepare_quit = false;
	reset_screen_info(this->m_info);
	this->font_load_success = this->text_font.loadFromMemory(font_ttf, font_ttf_len);
	this->notification = new TextRectangle(this->font_load_success, this->text_font);
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
	this->in_tex.create(IN_VIDEO_WIDTH, IN_VIDEO_HEIGHT);
	this->m_in_rect_top.setTexture(&this->in_tex);
	this->m_in_rect_bot.setTexture(&this->in_tex);
	this->display_data = display_data;
	this->audio_data = audio_data;
	this->last_window_creation_time = std::chrono::high_resolution_clock::now();
	this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
	this->touch_right_click_action.started = false;
	this->reset_held_times();
	WindowScreen::reset_operations(future_operations);
	this->done_display = true;
	this->saved_buf = new VideoOutputData;
	this->win_title = NAME;
	if(this->m_stype == ScreenType::TOP)
		this->win_title += "_top";
	if(this->m_stype == ScreenType::BOTTOM)
		this->win_title += "_bot";
	this->last_connected_status = false;
	this->capture_status = capture_status;
}

WindowScreen::~WindowScreen() {
	this->possible_files.clear();
	this->possible_resolutions.clear();
	delete this->saved_buf;
	delete this->notification;
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
}

void WindowScreen::build() {
	sf::Vector2f top_screen_size = sf::Vector2f(TOP_WIDTH_3DS, HEIGHT_3DS);
	sf::Vector2f bot_screen_size = sf::Vector2f(BOT_WIDTH_3DS, HEIGHT_3DS);

	int width = TOP_WIDTH_3DS;
	if(this->m_stype == ScreenType::BOTTOM)
		width = BOT_WIDTH_3DS;

	this->reload();

	this->m_out_rect_top.out_tex.create(top_screen_size.x, top_screen_size.y);
	this->m_out_rect_top.out_rect.setSize(top_screen_size);
	this->m_out_rect_top.out_rect.setTexture(&this->m_out_rect_top.out_tex.getTexture());

	this->m_out_rect_bot.out_tex.create(bot_screen_size.x, bot_screen_size.y);
	this->m_out_rect_bot.out_rect.setSize(bot_screen_size);
	this->m_out_rect_bot.out_rect.setTexture(&this->m_out_rect_bot.out_tex.getTexture());

	this->m_view.setRotation(0);
}

void WindowScreen::reload() {
	this->curr_menu = DEFAULT_MENU_TYPE;
	this->future_operations.call_crop = true;
	this->future_operations.call_rotate = true;
	this->future_operations.call_blur = true;
	this->future_operations.call_screen_settings_update = true;
	if(this->m_win.isOpen())
		this->create_window(false);
	this->prepare_size_ratios(true, true);
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
	reset_fullscreen_info(this->m_info);
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
			this->m_prepare_quit = true;
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
					this->m_prepare_quit = true;
					break;
				default:
					consumed = false;
					break;
			}

			break;
		case sf::Event::MouseMoved:
			if(this->m_info.is_fullscreen) {
				this->m_info.show_mouse = true;
				this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			}
			consumed = false;
			break;
		case sf::Event::MouseButtonPressed:
			if(this->m_info.is_fullscreen) {
				this->m_info.show_mouse = true;
				this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			}
			consumed = false;
			break;
		case sf::Event::MouseButtonReleased:
			if(this->m_info.is_fullscreen) {
				this->m_info.show_mouse = true;
				this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
			}
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
		this->main_menu->insert_data(this->m_stype, this->m_info.is_fullscreen);
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
}

void WindowScreen::setup_licenses_menu(bool reset_data) {
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
	if(this->m_info.is_fullscreen && this->m_info.show_mouse) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_mouse_action_time;
		if(diff.count() > this->mouse_timeout)
			this->m_info.show_mouse = false;
	}
	this->poll_window();
	bool done = false;
	while(!events_queue.empty()) {
		if(done)
			return;
		if(this->query_reset_request()) {
			this->reset_held_times();
			this->m_prepare_load = SIMPLE_RESET_DATA_INDEX;
			done = true;
			continue;
		}
		SFEvent event_data = events_queue.front();
		events_queue.pop();
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
							this->m_prepare_quit = true;
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
							this->setup_main_menu();
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
							this->setup_main_menu();
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
							this->setup_main_menu();
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
							this->setup_main_menu();
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
			case CROP_MENU_TYPE:
				if(this->crop_menu->poll(event_data)) {
					switch(this->crop_menu->selected_index) {
						case CROP_MENU_BACK:
							this->setup_video_menu();
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
							this->setup_video_menu();
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
							this->setup_video_menu();
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
							this->setup_video_menu();
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
							this->setup_video_menu();
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
							this->setup_video_menu();
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
							this->setup_video_menu();
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
							this->setup_video_menu();
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
							this->setup_main_menu();
							done = true;
							break;
						case EXTRA_SETTINGS_MENU_NO_ACTION:
							break;
						case EXTRA_SETTINGS_MENU_QUIT_APPLICATION:
							this->m_prepare_quit = true;
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
			default:
				break;
		}
	}
}

void WindowScreen::close() {
	this->future_operations.call_close = true;
	this->notification->setShowText(false);
}

void WindowScreen::display_call(bool is_main_thread) {
	while(!this->is_window_factory_done);
	this->prepare_screen_rendering();
	if(this->m_win.isOpen()) {
		if(this->main_thread_owns_window != is_main_thread) {
			this->m_win.setActive(true);
			this->main_thread_owns_window = is_main_thread;
		}
		this->window_render_call();
	}
	this->done_display = true;
}

void WindowScreen::display_thread() {
	while(this->capture_status->running) {
		this->display_lock.lock();
		if(!this->capture_status->running)
			break;
		this->free_ownership_of_window(false);
		this->is_thread_done = true;
		if(this->loaded_info.async) {
			this->display_call(false);
		}
	}
	if(!this->main_thread_owns_window)
		this->m_win.setActive(false);
	this->done_display = true;
}

void WindowScreen::end() {
	if(this->main_thread_owns_window)
		this->m_win.setActive(false);
	this->display_lock.unlock();
}

void WindowScreen::after_thread_join() {
	if(this->m_win.isOpen()) {
		this->m_win.setActive(true);
		this->m_win.close();
	}
}

void WindowScreen::draw(double frame_time, VideoOutputData* out_buf) {
	if(!this->done_display)
		return;

	bool should_be_open = this->display_data->split;
	if(this->m_stype == ScreenType::JOINT)
		should_be_open = !should_be_open;
	if(this->m_win.isOpen() ^ should_be_open) {
		if(this->m_win.isOpen())
			this->close();
		else
			this->open();
	}
	this->loaded_menu = this->curr_menu;
	loaded_operations = future_operations;
	if(this->m_win.isOpen() || this->loaded_operations.call_create) {
		WindowScreen::reset_operations(future_operations);
		if(out_buf != NULL)
			memcpy(this->saved_buf, out_buf, sizeof(VideoOutputData));
		else
			memset(this->saved_buf, 0, sizeof(VideoOutputData));
		loaded_info = m_info;
		this->notification->setTextFactor(this->loaded_info.menu_scaling_factor);
		this->notification->prepareRenderText();
		this->frame_time = frame_time;
		this->scheduled_work_on_window = this->window_needs_work();
		int view_size_x = this->m_window_width;
		int view_size_y = this->m_window_height;
		if(!this->loaded_info.is_fullscreen) {
			view_size_x = this->m_width;
			view_size_y = this->m_height;
		}
		switch(this->loaded_menu) {
			case CONNECT_MENU_TYPE:
				this->connection_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y);
				break;
			case MAIN_MENU_TYPE:
				this->main_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, this->capture_status->connected);
				break;
			case VIDEO_MENU_TYPE:
				this->video_menu->prepare(this->loaded_info.menu_scaling_factor, view_size_x, view_size_y, &this->loaded_info);
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
			default:
				break;
		}
		this->is_window_factory_done = false;
		this->is_thread_done = false;
		this->done_display = false;
		if((!this->main_thread_owns_window) || (this->loaded_info.async))
			this->display_lock.unlock();
		if((!this->main_thread_owns_window) && ((this->scheduled_work_on_window) || (!this->loaded_info.async)))
			while(!this->is_thread_done);
		this->window_factory(true);
		if(!this->loaded_info.async)
			this->display_call(true);
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

void WindowScreen::update_connection() {
	if(this->last_connected_status == this->capture_status->connected)
		return;
	this->last_connected_status = this->capture_status->connected;
	if(this->m_win.isOpen()) {
		this->m_win.setTitle(this->title_factory());
	}
}

void WindowScreen::print_notification(std::string text, TextKind kind) {
	this->notification->setText(text);
	this->notification->setRectangleKind(kind);
	this->notification->startTimer(true);
	this->notification->setShowText(true);
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

void WindowScreen::reset_operations(ScreenOperations &operations) {
	operations.call_create = false;
	operations.call_close = false;
	operations.call_crop = false;
	operations.call_rotate = false;
	operations.call_screen_settings_update = false;
	operations.call_blur = false;
}

void WindowScreen::free_ownership_of_window(bool is_main_thread) {
	if(is_main_thread == this->main_thread_owns_window) {
		if(this->scheduled_work_on_window)
			this->m_win.setActive(false);
		else if(is_main_thread == this->loaded_info.async)
			this->m_win.setActive(false);
	}
}

void WindowScreen::resize_in_rect(sf::RectangleShape &in_rect, int start_x, int start_y, int width, int height) {
	in_rect.setTextureRect(sf::IntRect(start_y, start_x, height, width));

	in_rect.setSize(sf::Vector2f(height, width));
	in_rect.setOrigin(height / 2, width / 2);

	in_rect.setRotation(-90);
	in_rect.setPosition(width / 2, height / 2);
}

int WindowScreen::get_screen_corner_modifier_x(int rotation, int width) {
	if(((rotation / 10) % 4) == 1) {
		return width;
	}
	if(((rotation / 10) % 4) == 2) {
		return width;
	}
	return 0;
}

int WindowScreen::get_screen_corner_modifier_y(int rotation, int height) {
	if(((rotation / 10) % 4) == 2) {
		return height;
	}
	if(((rotation / 10) % 4) == 3) {
		return height;
	}
	return 0;
}

void WindowScreen::print_notification_on_off(std::string base_text, bool value) {
	std::string status_text = "On";
	if(!value)
		status_text = "Off";
	this->print_notification(base_text + ": " + status_text);
}

void WindowScreen::print_notification_float(std::string base_text, float value, int decimals) {
	this->print_notification(base_text + ": " + get_float_str_decimals(value, decimals));
}

static void check_held_reset(bool value, HeldTime &action_time) {
	if(value) {
		if(!action_time.started) {
			action_time.start_time = std::chrono::high_resolution_clock::now();
			action_time.started = true;
		}
	}
	else {
		action_time.started = false;
	}
}

static float check_held_diff(std::chrono::time_point<std::chrono::high_resolution_clock> &curr_time, HeldTime &action_time) {
	if(!action_time.started)
		return 0.0;
	const std::chrono::duration<double> diff = curr_time - action_time.start_time;
	return diff.count();
}

bool WindowScreen::query_reset_request() {
	auto curr_time = std::chrono::high_resolution_clock::now();
	if(check_held_diff(curr_time, this->right_click_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->enter_action) > this->bad_resolution_timeout)
		return true;
	if(check_held_diff(curr_time, this->pgdown_action) > this->bad_resolution_timeout)
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
	check_held_reset(false, this->controller_button_action);
	check_held_reset(false, this->touch_action);
}

void WindowScreen::poll_window() {
	if(this->m_win.isOpen()) {
		sf::Event event;
		this->events_access->lock();
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
			events_queue.emplace(event.type, event.key.code, event.text.unicode, joystickId, event.joystickButton.button, event.joystickMove.axis, 0.0, event.mouseButton.button, mouse_x, mouse_y);
		}
		if(this->m_win.hasFocus()) {
			check_held_reset(sf::Mouse::isButtonPressed(sf::Mouse::Right), this->right_click_action);
			check_held_reset(sf::Keyboard::isKeyPressed(sf::Keyboard::Enter), this->enter_action);
			check_held_reset(sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown), this->pgdown_action);
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
					events_queue.emplace(sf::Event::MouseButtonPressed, sf::Keyboard::Backspace, 0, 0, 0, sf::Joystick::Axis::X, 0.0, sf::Mouse::Right, touch_pos.x, touch_pos.y);
					this->touch_right_click_action.start_time = std::chrono::high_resolution_clock::now();
				}
				events_queue.emplace(sf::Event::MouseButtonPressed, sf::Keyboard::Backspace, 0, 0, 0, sf::Joystick::Axis::X, 0.0, sf::Mouse::Left, touch_pos.x, touch_pos.y);
			}
			joystick_axis_poll(this->events_queue);
		}
		else {
			this->reset_held_times();
			this->touch_right_click_action.started = false;
		}
		this->events_access->unlock();
	}
	else {
		this->reset_held_times();
		this->touch_right_click_action.started = false;
	}
}

void WindowScreen::prepare_screen_rendering() {
	if(loaded_operations.call_blur) {
		this->m_out_rect_top.out_tex.setSmooth(this->loaded_info.is_blurred);
		this->m_out_rect_bot.out_tex.setSmooth(this->loaded_info.is_blurred);
	}
	if(loaded_operations.call_crop) {
		this->crop();
	}
	if(loaded_operations.call_rotate) {
		this->rotate();
	}
	if(loaded_operations.call_screen_settings_update) {
		this->update_screen_settings();
	}
}

bool WindowScreen::window_needs_work() {
	this->resize_window_and_out_rects();
	if(this->loaded_operations.call_close)
		return true;
	if(this->loaded_operations.call_create)
		return true;
	int width = this->m_width;
	int height = this->m_height;
	if(this->loaded_info.is_fullscreen) {
		width = this->m_window_width;
		height = this->m_window_height;
	}
	int win_width = this->m_win.getSize().x;
	int win_height = this->m_win.getSize().y;
	if((win_width != width) || (win_height != height))
		return true;
	if(this->last_connected_status != this->capture_status->connected)
		return true;
	return false;
}

void WindowScreen::window_factory(bool is_main_thread) {
	if(this->loaded_operations.call_close) {
		this->events_access->lock();
		this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		this->m_win.close();
		while(!events_queue.empty())
			events_queue.pop();
		this->events_access->unlock();
		this->loaded_operations.call_close = false;
		this->loaded_operations.call_create = false;
	}
	if(this->loaded_operations.call_create) {
		this->notification->setShowText(false);
		this->events_access->lock();
		this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		if(!this->loaded_info.is_fullscreen) {
			this->update_screen_settings();
			this->m_win.create(sf::VideoMode(this->m_width, this->m_height), this->title_factory());
			this->update_view_size();
		}
		else
			this->m_win.create(this->curr_desk_mode, this->title_factory(), sf::Style::Fullscreen);
		this->last_window_creation_time = std::chrono::high_resolution_clock::now();
		this->update_screen_settings();
		this->events_access->unlock();
		this->loaded_operations.call_create = false;
	}
	if(this->m_win.isOpen()) {
		this->setWinSize(is_main_thread);
	}
	if((is_main_thread == this->main_thread_owns_window) && (this->main_thread_owns_window == this->loaded_info.async)) {
		this->m_win.setActive(false);
	}
	this->update_connection();
	this->is_window_factory_done = true;
}

std::string WindowScreen::title_factory() {
	std::string title = this->win_title;
	if(this->capture_status->connected)
		title += " - " + this->capture_status->serial_number;
	return title;
}

void WindowScreen::pre_texture_conversion_processing() {
	if(this->loaded_menu == CONNECT_MENU_TYPE)
		return;
	//Place preprocessing window-specific effects here
	this->in_tex.update((uint8_t*)this->saved_buf, IN_VIDEO_WIDTH, IN_VIDEO_HEIGHT, 0, 0);
}

void WindowScreen::post_texture_conversion_processing(out_rect_data &rect_data, const sf::RectangleShape &in_rect, bool actually_draw, bool is_top, bool is_debug) {
	if((is_top && this->m_stype == ScreenType::BOTTOM) || ((!is_top) && this->m_stype == ScreenType::TOP))
		return;
	if(this->loaded_menu == CONNECT_MENU_TYPE)
		return;

	if(is_debug) {
		if(is_top)
			rect_data.out_tex.clear(sf::Color::Red);
		else
			rect_data.out_tex.clear(sf::Color::Blue);
	}
	else {
		rect_data.out_tex.clear();
		if(actually_draw) {
			rect_data.out_tex.draw(in_rect);
			//Place postprocessing effects here
		}
	}
	rect_data.out_tex.display();
}

void WindowScreen::window_bg_processing() {
	//Place BG processing here
}

void WindowScreen::display_data_to_window(bool actually_draw, bool is_debug) {
	this->post_texture_conversion_processing(this->m_out_rect_top, this->m_in_rect_top, actually_draw, true, is_debug);
	this->post_texture_conversion_processing(this->m_out_rect_bot, this->m_in_rect_bot, actually_draw, false, is_debug);

	if(is_debug)
		this->m_win.clear(sf::Color::Green);
	else
		this->m_win.clear();
	this->window_bg_processing();
	if(this->loaded_menu != CONNECT_MENU_TYPE) {
		if(this->m_stype != ScreenType::BOTTOM)
			this->m_win.draw(this->m_out_rect_top.out_rect);
		if(this->m_stype != ScreenType::TOP)
			this->m_win.draw(this->m_out_rect_bot.out_rect);
	}
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
		default:
			break;
	}
	this->notification->draw(this->m_win);
	this->m_win.display();
}

void WindowScreen::window_render_call() {
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->last_window_creation_time;
	if(diff.count() > this->v_sync_timeout)
		this->m_win.setVerticalSyncEnabled(this->loaded_info.v_sync_enabled);
	else
		this->m_win.setVerticalSyncEnabled(false);
	this->m_win.setMouseCursorVisible(this->loaded_info.show_mouse);
	this->pre_texture_conversion_processing();

	this->display_data_to_window(true);

	if(this->loaded_info.bfi) {
		if(this->frame_time > 0.0) {
			if(this->loaded_info.bfi_divider < 2)
				this->loaded_info.bfi_divider = 2;
			if(this->loaded_info.bfi_divider > 10)
				this->loaded_info.bfi_divider = 10;
			if(this->loaded_info.bfi_amount < 1)
				this->loaded_info.bfi_amount = 1;
			if(this->loaded_info.bfi_amount > (this->loaded_info.bfi_divider - 1))
				this->loaded_info.bfi_amount = this->loaded_info.bfi_divider - 1;
			for(int i = 0; i < (this->loaded_info.bfi_divider - this->loaded_info.bfi_amount - 1); i++) {
				sf::sleep(sf::seconds(frame_time / (this->loaded_info.bfi_divider * 1.1)));
			}
			for(int i = 0; i < this->loaded_info.bfi_amount; i++) {
				sf::sleep(sf::seconds(frame_time / (this->loaded_info.bfi_divider * 1.1)));
				this->display_data_to_window(false);
			}
		}
	}
}

void WindowScreen::set_position_screens(sf::Vector2f &curr_top_screen_size, sf::Vector2f &curr_bot_screen_size, int offset_x, int offset_y, int max_x, int max_y, bool do_work) {
	int top_screen_x = 0, top_screen_y = 0;
	int bot_screen_x = 0, bot_screen_y = 0;
	int top_screen_width = curr_top_screen_size.x;
	int top_screen_height = curr_top_screen_size.y;
	int bot_screen_width = curr_bot_screen_size.x;
	int bot_screen_height = curr_bot_screen_size.y;

	if((this->loaded_info.top_rotation / 10) % 2) {
		top_screen_width = curr_top_screen_size.y;
		top_screen_height = curr_top_screen_size.x;
	}
	if((this->loaded_info.bot_rotation / 10) % 2) {
		bot_screen_width = curr_bot_screen_size.y;
		bot_screen_height = curr_bot_screen_size.x;
	}

	if(this->m_stype == ScreenType::TOP)
		bot_screen_width = bot_screen_height = 0;
	if(this->m_stype == ScreenType::BOTTOM)
		top_screen_width = top_screen_height = 0;

	int greatest_width = top_screen_width;
	if(greatest_width < bot_screen_width)
		greatest_width = bot_screen_width;
	int greatest_height = top_screen_height;
	if(greatest_height < bot_screen_height)
		greatest_height = bot_screen_height;

	if(this->m_stype == ScreenType::JOINT) {
		switch(this->loaded_info.bottom_pos) {
			case UNDER_TOP:
				bot_screen_x = (greatest_width - bot_screen_width) * this->loaded_info.subscreen_offset;
				top_screen_x = (greatest_width - top_screen_width) * this->loaded_info.subscreen_offset;
				bot_screen_y = top_screen_height;
				if(max_y > 0)
					bot_screen_y += (max_y - bot_screen_height - top_screen_height - offset_y) * this->loaded_info.subscreen_attached_offset;
				break;
			case RIGHT_TOP:
				bot_screen_y = (greatest_height - bot_screen_height) * this->loaded_info.subscreen_offset;
				top_screen_y = (greatest_height - top_screen_height) * this->loaded_info.subscreen_offset;
				bot_screen_x = top_screen_width;
				if(max_x > 0)
					bot_screen_x += (max_x - bot_screen_width - top_screen_width - offset_x) * this->loaded_info.subscreen_attached_offset;
				break;
			case ABOVE_TOP:
				bot_screen_x = (greatest_width - bot_screen_width) * this->loaded_info.subscreen_offset;
				top_screen_x = (greatest_width - top_screen_width) * this->loaded_info.subscreen_offset;
				top_screen_y = bot_screen_height;
				if(max_y > 0)
					top_screen_y += (max_y - bot_screen_height - top_screen_height - offset_y) * this->loaded_info.subscreen_attached_offset;
				break;
			case LEFT_TOP:
				bot_screen_y = (greatest_height - bot_screen_height) * this->loaded_info.subscreen_offset;
				top_screen_y = (greatest_height - top_screen_height) * this->loaded_info.subscreen_offset;
				top_screen_x = bot_screen_width;
				if(max_x > 0)
					top_screen_x += (max_x - bot_screen_width - top_screen_width - offset_x) * this->loaded_info.subscreen_attached_offset;
				break;
			default:
				break;
		}
	}
	if(do_work) {
		this->m_out_rect_top.out_rect.setPosition(top_screen_x + offset_x + get_screen_corner_modifier_x(this->loaded_info.top_rotation, top_screen_width), top_screen_y + offset_y + get_screen_corner_modifier_y(this->loaded_info.top_rotation, top_screen_height));
		this->m_out_rect_bot.out_rect.setPosition(bot_screen_x + offset_x + get_screen_corner_modifier_x(this->loaded_info.bot_rotation, bot_screen_width), bot_screen_y + offset_y + get_screen_corner_modifier_y(this->loaded_info.bot_rotation, bot_screen_height));
	}

	int top_end_x = top_screen_x + offset_x + top_screen_width;
	int top_end_y = top_screen_y + offset_y + top_screen_height;
	int bot_end_x = bot_screen_x + offset_x + bot_screen_width;
	int bot_end_y = bot_screen_y + offset_y + bot_screen_height;
	this->m_width = top_end_x;
	this->m_height = top_end_y;
	if(bot_end_x > this->m_width)
		this->m_width = bot_end_x;
	if(bot_end_y > this->m_height)
		this->m_height = bot_end_y;
}

int WindowScreen::prepare_screen_ratio(sf::Vector2f &screen_size, int own_rotation, int width_limit, int height_limit, int other_rotation, const PARData* own_par) {
	float own_width = screen_size.x;
	float own_height = screen_size.y;
	if((own_width < 1.0) || (own_height < 1.0))
		return 0;

	get_par_size(own_width, own_height, 1, own_par);

	if((own_rotation / 10) % 2)
		std::swap(own_width, own_height);
	if((other_rotation / 10) % 2)
		std::swap(width_limit, height_limit);

	int desk_height = curr_desk_mode.height;
	int desk_width = curr_desk_mode.width;
	int height_ratio = desk_height / own_height;
	int width_ratio = desk_width / own_width;
	switch(this->m_info.bottom_pos) {
		case UNDER_TOP:
		case ABOVE_TOP:
			height_ratio = (desk_height - height_limit) / own_height;
			break;
		case LEFT_TOP:
		case RIGHT_TOP:
			width_ratio = (desk_width - width_limit) / own_width;
			break;
		default:
			break;
	}
	if(width_ratio < height_ratio)
		return width_ratio;
	return height_ratio;
}

void WindowScreen::calc_scaling_resize_screens(sf::Vector2f &own_screen_size, sf::Vector2f &other_screen_size, int &own_scaling, int &other_scaling, int own_rotation, int other_rotation, bool increase, bool mantain, bool set_to_zero, const PARData* own_par, const PARData* other_par) {
	int min_other_width = other_screen_size.x;
	int min_other_height = other_screen_size.y;
	get_par_size(min_other_width, min_other_height, 1, other_par);
	int chosen_ratio = prepare_screen_ratio(own_screen_size, own_rotation, min_other_width, min_other_height, other_rotation, own_par);
	if(chosen_ratio <= 0) {
		chosen_ratio = prepare_screen_ratio(own_screen_size, own_rotation, 0, 0, other_rotation, own_par);
		if(chosen_ratio <= 0)
			chosen_ratio = 0;
	}
	int old_scaling = own_scaling;
	if(increase && (chosen_ratio > own_scaling) && (chosen_ratio > 0))
		own_scaling += 1;
	else if(mantain && (chosen_ratio >= own_scaling) && (chosen_ratio > 0))
		own_scaling = own_scaling;
	else
		own_scaling = chosen_ratio;
	if(set_to_zero)
		own_scaling = 0;
	int own_height = own_screen_size.y;
	int own_width = own_screen_size.x;
	get_par_size(own_width, own_height, own_scaling, own_par);
	if((increase && ((old_scaling == own_scaling) || (other_scaling == 0)) || (mantain && (other_scaling == 0))) && (own_scaling > 0))
		other_scaling = 0;
	else
		other_scaling = prepare_screen_ratio(other_screen_size, other_rotation, own_width, own_height, own_rotation, other_par);
	if(other_scaling < 0)
		other_scaling = 0;
	if((this->m_stype == ScreenType::JOINT) && ((own_scaling > 0) || ((other_scaling == 0) && (own_scaling == 0)))) {
		// Due to size differences, it may be possible that
		// the chosen screen might be able to increase its
		// scaling even more without compromising the other one...
		int other_height = other_screen_size.y;
		int other_width = other_screen_size.x;
		get_par_size(other_width, other_height, other_scaling, other_par);
		own_scaling = prepare_screen_ratio(own_screen_size, own_rotation, other_width, other_height, other_rotation, own_par);
		if(own_scaling < 0)
			own_scaling = 0;
	}
}

void WindowScreen::prepare_size_ratios(bool top_increase, bool bot_increase) {
	if(!this->m_info.is_fullscreen)
		return;

	sf::Vector2f top_screen_size = getShownScreenSize(true, this->m_info.crop_kind);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, this->m_info.crop_kind);

	if(this->m_stype == ScreenType::TOP) {
		bot_increase = true;
		top_increase = false;
	}
	if(this->m_stype == ScreenType::BOTTOM) {
		bot_increase = false;
		top_increase = true;
	}
	bool try_mantain_ratio = top_increase && bot_increase;
	if(!(top_increase ^ bot_increase)) {
		top_increase = false;
		bot_increase = false;
	}
	bool prioritize_top = (!bot_increase) && (top_increase || (this->m_info.bottom_pos == UNDER_TOP) || (this->m_info.bottom_pos == RIGHT_TOP));
	if(prioritize_top)
		calc_scaling_resize_screens(top_screen_size, bot_screen_size, this->m_info.top_scaling, this->m_info.bot_scaling, this->m_info.top_rotation, this->m_info.bot_rotation, top_increase, try_mantain_ratio, this->m_stype == ScreenType::BOTTOM, this->possible_pars[this->m_info.top_par], this->possible_pars[this->m_info.bot_par]);
	else
		calc_scaling_resize_screens(bot_screen_size, top_screen_size, this->m_info.bot_scaling, this->m_info.top_scaling, this->m_info.bot_rotation, this->m_info.top_rotation, bot_increase, try_mantain_ratio, this->m_stype == ScreenType::TOP, this->possible_pars[this->m_info.bot_par], this->possible_pars[this->m_info.top_par]);
}

int WindowScreen::get_fullscreen_offset_x(int top_width, int top_height, int bot_width, int bot_height) {
	if((this->loaded_info.top_rotation / 10) % 2)
		std::swap(top_width, top_height);
	if((this->loaded_info.bot_rotation / 10) % 2)
		std::swap(bot_width, bot_height);
	int greatest_width = top_width;
	if(bot_width > top_width)
		greatest_width = bot_width;
	int offset_contribute = 0;
	switch(this->loaded_info.bottom_pos) {
		case UNDER_TOP:
		case ABOVE_TOP:
			offset_contribute = greatest_width;
			break;
		case LEFT_TOP:
		case RIGHT_TOP:
			offset_contribute = top_width + bot_width;
			break;
		default:
			return 0;
	}
	return (curr_desk_mode.width - offset_contribute) * this->loaded_info.total_offset_x;
}

int WindowScreen::get_fullscreen_offset_y(int top_width, int top_height, int bot_width, int bot_height) {
	if((this->loaded_info.top_rotation / 10) % 2)
		std::swap(top_width, top_height);
	if((this->loaded_info.bot_rotation / 10) % 2)
		std::swap(bot_width, bot_height);
	int greatest_height = top_height;
	if(bot_height > top_height)
		greatest_height = bot_height;
	int offset_contribute = 0;
	switch(this->loaded_info.bottom_pos) {
		case UNDER_TOP:
		case ABOVE_TOP:
			offset_contribute = top_height + bot_height;
			break;
		case LEFT_TOP:
		case RIGHT_TOP:
			offset_contribute = greatest_height;
			break;
		default:
			return 0;
	}
	return (curr_desk_mode.height - offset_contribute) * this->loaded_info.total_offset_y;
}

void WindowScreen::resize_window_and_out_rects(bool do_work) {
	sf::Vector2f top_screen_size = getShownScreenSize(true, this->loaded_info.crop_kind);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, this->loaded_info.crop_kind);
	int top_width = top_screen_size.x;
	int top_height = top_screen_size.y;
	float top_scaling = this->loaded_info.scaling;
	int bot_width = bot_screen_size.x;
	int bot_height = bot_screen_size.y;
	float bot_scaling = this->loaded_info.scaling;
	int offset_x = 0;
	int offset_y = 0;
	int max_x = 0;
	int max_y = 0;

	if(this->loaded_info.is_fullscreen) {
		top_scaling = this->loaded_info.top_scaling;
		bot_scaling = this->loaded_info.bot_scaling;
	}
	get_par_size(top_width, top_height, top_scaling, this->possible_pars[this->loaded_info.top_par]);
	get_par_size(bot_width, bot_height, bot_scaling, this->possible_pars[this->loaded_info.bot_par]);

	if((!this->loaded_info.is_fullscreen) && this->loaded_info.rounded_corners_fix) {
		offset_y = TOP_ROUNDED_PADDING;
		offset_x = LEFT_ROUNDED_PADDING;
	}

	if(this->loaded_info.is_fullscreen) {
		offset_x = get_fullscreen_offset_x(top_width, top_height, bot_width, bot_height);
		offset_y = get_fullscreen_offset_y(top_width, top_height, bot_width, bot_height);
		max_x = curr_desk_mode.width;
		max_y = curr_desk_mode.height;
	}
	sf::Vector2f new_top_screen_size = sf::Vector2f(top_width, top_height);
	sf::Vector2f new_bot_screen_size = sf::Vector2f(bot_width, bot_height);
	if(do_work) {
		this->m_out_rect_top.out_rect.setSize(new_top_screen_size);
		this->m_out_rect_top.out_rect.setTextureRect(sf::IntRect(0, 0, top_screen_size.x, top_screen_size.y));
		this->m_out_rect_bot.out_rect.setSize(new_bot_screen_size);
		this->m_out_rect_bot.out_rect.setTextureRect(sf::IntRect(0, 0, bot_screen_size.x, bot_screen_size.y));
	}
	this->set_position_screens(new_top_screen_size, new_bot_screen_size, offset_x, offset_y, max_x, max_y, do_work);
	if((!this->loaded_info.is_fullscreen) && this->loaded_info.rounded_corners_fix) {
		this->m_height += BOTTOM_ROUNDED_PADDING;
		this->m_width += RIGHT_ROUNDED_PADDING;
	}
}

void WindowScreen::create_window(bool re_prepare_size) {
	if(!this->m_info.is_fullscreen) {
		this->m_info.show_mouse = true;
	}
	else {
		bool success = false;
		if((this->m_info.fullscreen_mode_width > 0) && (this->m_info.fullscreen_mode_height > 0) && (this->m_info.fullscreen_mode_bpp > 0)) {
			sf::VideoMode mode_created = sf::VideoMode(this->m_info.fullscreen_mode_width, this->m_info.fullscreen_mode_height, this->m_info.fullscreen_mode_bpp);
			if(mode_created.isValid()) {
				this->curr_desk_mode = mode_created;
				success = true;
			}
		}
		if(!success) {
			std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
			if(modes.size() > 0)
				this->curr_desk_mode = modes[0];
			else
				this->curr_desk_mode = sf::VideoMode::getDesktopMode();
		}
		this->m_window_width = curr_desk_mode.width;
		this->m_window_height = curr_desk_mode.height;
		this->m_info.show_mouse = false;
	}
	if(re_prepare_size)
		this->prepare_size_ratios(false, false);
	this->future_operations.call_create = true;
}

void WindowScreen::update_view_size() {
	if(!this->loaded_info.is_fullscreen) {
		this->m_window_width = this->m_width;
		this->m_window_height = this->m_height;
	}
	this->m_view.reset(sf::FloatRect(0, 0, this->m_window_width, this->m_window_height));
	this->m_win.setView(this->m_view);
}

void WindowScreen::open() {
	this->m_info.is_fullscreen = false;
	this->create_window(true);
}

void WindowScreen::update_screen_settings() {
	this->resize_window_and_out_rects();
	this->update_view_size();
}

void WindowScreen::rotate() {
	this->m_out_rect_top.out_rect.setRotation(this->loaded_info.top_rotation);
	this->m_out_rect_bot.out_rect.setRotation(this->loaded_info.bot_rotation);
	this->loaded_operations.call_screen_settings_update = true;
}

sf::Vector2f WindowScreen::getShownScreenSize(bool is_top, int &crop_kind) {
	if(crop_kind >= this->possible_crops.size())
		crop_kind = 0;
	int width = this->possible_crops[crop_kind]->top_width;
	int height = this->possible_crops[crop_kind]->top_height;
	if(!is_top) {
		width = this->possible_crops[crop_kind]->bot_width;
		height = this->possible_crops[crop_kind]->bot_height;
	}
	return sf::Vector2f(width, height);
}

void WindowScreen::crop() {
	if(this->loaded_info.crop_kind >= this->possible_crops.size())
		this->loaded_info.crop_kind = 0;

	sf::Vector2f top_screen_size = getShownScreenSize(true, this->loaded_info.crop_kind);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, this->loaded_info.crop_kind);

	this->resize_in_rect(this->m_in_rect_top, this->possible_crops[this->loaded_info.crop_kind]->top_x, this->possible_crops[this->loaded_info.crop_kind]->top_y, top_screen_size.x, top_screen_size.y);
	this->resize_in_rect(this->m_in_rect_bot, TOP_WIDTH_3DS + this->possible_crops[this->loaded_info.crop_kind]->bot_x, this->possible_crops[this->loaded_info.crop_kind]->bot_y, bot_screen_size.x, bot_screen_size.y);
	this->loaded_operations.call_screen_settings_update = true;
}

void WindowScreen::setWinSize(bool is_main_thread) {
	int width = this->m_width;
	int height = this->m_height;
	if(this->loaded_info.is_fullscreen) {
		width = this->m_window_width;
		height = this->m_window_height;
	}
	int win_width = this->m_win.getSize().x;
	int win_height = this->m_win.getSize().y;
	if((win_width != width) || (win_height != height)) {
		this->events_access->lock();
		this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		this->m_win.setSize(sf::Vector2u(width, height));
		this->events_access->unlock();
	}
}

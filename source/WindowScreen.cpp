#include <SFML/Graphics.hpp>

#include "utils.hpp"
#include "hw_defs.hpp"
#include "3dscapture.hpp"
#include "frontend.hpp"

#include <cstring>
#include <cmath>
#include <mutex>
#include <queue>
#include "font_ttf.h"

static int top_screen_crop_widths[] = {TOP_WIDTH_3DS, TOP_SPECIAL_DS_WIDTH_3DS, TOP_SCALED_DS_WIDTH_3DS, WIDTH_DS, WIDTH_SCALED_GBA, WIDTH_GBA, WIDTH_SCALED_GB, WIDTH_GB, WIDTH_SNES, WIDTH_NES};
static int top_screen_crop_heights[] = {HEIGHT_3DS, HEIGHT_3DS, HEIGHT_3DS, HEIGHT_DS, HEIGHT_SCALED_GBA, HEIGHT_GBA, HEIGHT_SCALED_GB, HEIGHT_GB, HEIGHT_SNES, HEIGHT_NES};
static int bot_screen_crop_widths[] = {BOT_WIDTH_3DS, BOT_WIDTH_3DS, BOT_WIDTH_3DS, WIDTH_DS, 0, 0, 0, 0, 0, 0};
static int bot_screen_crop_heights[] = {HEIGHT_3DS, HEIGHT_3DS, HEIGHT_3DS, HEIGHT_DS, 0, 0, 0, 0, 0, 0};
static int top_screen_crop_x[] = {0, (TOP_WIDTH_3DS - TOP_SPECIAL_DS_WIDTH_3DS) / 2, (TOP_WIDTH_3DS - TOP_SCALED_DS_WIDTH_3DS) / 2, (TOP_WIDTH_3DS - WIDTH_DS) / 2,
								(TOP_WIDTH_3DS - WIDTH_SCALED_GBA) / 2,	(TOP_WIDTH_3DS - WIDTH_GBA) / 2, (TOP_WIDTH_3DS - WIDTH_SCALED_GB) / 2, (TOP_WIDTH_3DS - WIDTH_GB) / 2,
								(TOP_WIDTH_3DS - WIDTH_SNES) / 2, (TOP_WIDTH_3DS - WIDTH_NES) / 2};
static int top_screen_crop_y[] = {0, 0, 0, 0, (HEIGHT_3DS - HEIGHT_SCALED_GBA) / 2, (HEIGHT_3DS - HEIGHT_GBA) / 2, (HEIGHT_3DS - HEIGHT_SCALED_GB) / 2, (HEIGHT_3DS - HEIGHT_GB) / 2, (HEIGHT_3DS - HEIGHT_SNES) / 2, (HEIGHT_3DS - HEIGHT_NES) / 2};
static int bot_screen_crop_x[] = {0, 0, 0, (BOT_WIDTH_3DS - WIDTH_DS) / 2, 0, 0, 0, 0, 0, 0};
static int bot_screen_crop_y[] = {0, 0, 0, HEIGHT_3DS - HEIGHT_DS, 0, 0, 0, 0, 0, 0};
static std::string crop_names[] = {"3DS", "16:10", "Scaled DS", "Native DS", "Scaled GBA", "Native GBA", "Scaled VC GB", "VC GB", "VC SNES", "VC NES"};

WindowScreen::WindowScreen(WindowScreen::ScreenType stype, DisplayData* display_data, AudioData* audio_data, std::mutex* events_access) {
	this->m_stype = stype;
	this->events_access = events_access;
	this->m_prepare_save = 0;
	this->m_prepare_load = 0;
	this->m_prepare_open = false;
	this->m_prepare_quit = false;
	reset_screen_info(this->m_info);
	this->font_load_success = this->text_font.loadFromMemory(font_ttf, font_ttf_len);
	this->notification = new TextRectangle(this->font_load_success, this->text_font);
	this->in_tex.create(IN_VIDEO_WIDTH, IN_VIDEO_HEIGHT);
	this->m_in_rect_top.setTexture(&this->in_tex);
	this->m_in_rect_bot.setTexture(&this->in_tex);
	this->display_data = display_data;
	this->audio_data = audio_data;
	WindowScreen::reset_operations(future_operations);
	this->done_display = true;
	this->saved_buf = new VideoOutputData;
	this->win_title = NAME;
	if(this->m_stype == WindowScreen::ScreenType::TOP)
		this->win_title += "_top";
	if(this->m_stype == WindowScreen::ScreenType::BOTTOM)
		this->win_title += "_bot";
}

WindowScreen::~WindowScreen() {
	delete this->saved_buf;
	this->saved_buf = NULL;
}

void WindowScreen::build() {
	sf::Vector2f top_screen_size = sf::Vector2f(TOP_WIDTH_3DS, HEIGHT_3DS);
	sf::Vector2f bot_screen_size = sf::Vector2f(BOT_WIDTH_3DS, HEIGHT_3DS);

	int width = TOP_WIDTH_3DS;
	if(this->m_stype == WindowScreen::ScreenType::BOTTOM)
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
	this->future_operations.call_crop = true;
	this->future_operations.call_rotate = true;
	this->future_operations.call_blur = true;
	this->future_operations.call_screen_settings_update = true;
	if(this->m_win.isOpen())
		this->create_window(false);
	this->prepare_size_ratios(true, true);
}

void WindowScreen::poll() {
	double old_scaling = 0.0;
	this->poll_window();
	while(!events_queue.empty()) {
		SFEvent event_data = events_queue.front();
		events_queue.pop();
		switch (event_data.type) {
		case sf::Event::Closed:
			this->m_prepare_quit = true;
			break;

		case sf::Event::TextEntered:
			switch (event_data.unicode) {
			case 'c':
				if(this->m_stype == WindowScreen::ScreenType::BOTTOM) {
					if(this->m_info.crop_kind == Crop::DEFAULT_3DS)
						this->m_info.crop_kind = Crop::NATIVE_DS;
					else
						this->m_info.crop_kind = Crop::DEFAULT_3DS;
				}
				else {
					this->m_info.crop_kind = static_cast<Crop>((this->m_info.crop_kind + 1) % Crop::CROP_END);
				}
				this->print_notification("Crop: " + crop_names[this->m_info.crop_kind]);
				this->prepare_size_ratios(false, false);
				this->future_operations.call_crop = true;

				break;

			case 's':
				this->m_info.is_fullscreen = false;
				this->display_data->split = !this->display_data->split;
				break;

			case 'a':
				this->m_info.async = !this->m_info.async;
				this->print_notification_on_off("Async", this->m_info.async);
				break;

			case 'b':
				this->m_info.is_blurred = !this->m_info.is_blurred;
				this->future_operations.call_blur = true;
				this->print_notification_on_off("Blur", this->m_info.is_blurred);
				break;

			case 'v':
				this->m_info.v_sync_enabled = !this->m_info.v_sync_enabled;
				this->print_notification_on_off("VSync", this->m_info.v_sync_enabled);
				break;

			case 'i':
				this->m_info.bfi = !this->m_info.bfi;
				this->print_notification_on_off("BFI", this->m_info.bfi);
				break;

			case '-':
				if(this->m_info.is_fullscreen)
					break;
				old_scaling = this->m_info.scaling;
				this->m_info.scaling -= 0.5;
				if (this->m_info.scaling < 1.25)
					this->m_info.scaling = 1.0;
				if(old_scaling != this->m_info.scaling) {
					this->print_notification_float("Scaling", this->m_info.scaling, 1);
					this->future_operations.call_screen_settings_update = true;
				}
				break;

			case '0':
				if(this->m_info.is_fullscreen)
					break;
				old_scaling = this->m_info.scaling;
				this->m_info.scaling += 0.5;
				if (this->m_info.scaling > 44.75)
					this->m_info.scaling = 45.0;
				if(old_scaling != this->m_info.scaling) {
					this->print_notification_float("Scaling", this->m_info.scaling, 1);
					this->future_operations.call_screen_settings_update = true;
				}
				break;

			case 'o':
				this->m_prepare_open = true;
				break;

			case 't':
				this->m_info.bottom_pos = static_cast<BottomRelativePosition>((this->m_info.bottom_pos + 1) % (BottomRelativePosition::BOT_REL_POS_END));
				this->prepare_size_ratios(false, false);
				this->future_operations.call_screen_settings_update = true;
				break;

			case '6':
				this->m_info.subscreen_offset_algorithm = static_cast<OffsetAlgorithm>((this->m_info.subscreen_offset_algorithm + 1) % (OffsetAlgorithm::OFF_ALGO_END));
				this->future_operations.call_screen_settings_update = true;
				break;

			case '7':
				this->m_info.subscreen_attached_offset_algorithm = static_cast<OffsetAlgorithm>((this->m_info.subscreen_attached_offset_algorithm + 1) % (OffsetAlgorithm::OFF_ALGO_END));
				this->future_operations.call_screen_settings_update = true;
				break;

			case '4':
				this->m_info.total_offset_algorithm_x = static_cast<OffsetAlgorithm>((this->m_info.total_offset_algorithm_x + 1) % (OffsetAlgorithm::OFF_ALGO_END));
				this->future_operations.call_screen_settings_update = true;
				break;

			case '5':
				this->m_info.total_offset_algorithm_y = static_cast<OffsetAlgorithm>((this->m_info.total_offset_algorithm_y + 1) % (OffsetAlgorithm::OFF_ALGO_END));
				this->future_operations.call_screen_settings_update = true;
				break;

			case 'y':
				this->prepare_size_ratios(true, false);
				this->future_operations.call_screen_settings_update = true;
				break;

			case 'u':
				this->prepare_size_ratios(false, true);
				this->future_operations.call_screen_settings_update = true;
				break;

			case 'f':
				this->m_info.is_fullscreen = !this->m_info.is_fullscreen;
				this->create_window(true);
				break;

			case '8':
				this->m_info.top_rotation = (this->m_info.top_rotation + 270) % 360;
				this->m_info.bot_rotation = (this->m_info.bot_rotation + 270) % 360;
				this->prepare_size_ratios(false, false);
				this->future_operations.call_rotate = true;

				break;

			case '9':
				this->m_info.top_rotation = (this->m_info.top_rotation + 90) % 360;
				this->m_info.bot_rotation = (this->m_info.bot_rotation + 90) % 360;
				this->prepare_size_ratios(false, false);
				this->future_operations.call_rotate = true;
				break;

			case 'h':
				this->m_info.top_rotation = (this->m_info.top_rotation + 270) % 360;
				this->prepare_size_ratios(false, false);
				this->future_operations.call_rotate = true;

				break;

			case 'j':
				this->m_info.top_rotation = (this->m_info.top_rotation + 90) % 360;
				this->prepare_size_ratios(false, false);
				this->future_operations.call_rotate = true;

				break;

			case 'k':
				this->m_info.bot_rotation = (this->m_info.bot_rotation + 270) % 360;
				this->prepare_size_ratios(false, false);
				this->future_operations.call_rotate = true;

				break;

			case 'l':
				this->m_info.bot_rotation = (this->m_info.bot_rotation + 90) % 360;
				this->prepare_size_ratios(false, false);
				this->future_operations.call_rotate = true;

				break;

			case 'z':
				old_scaling = this->m_info.menu_scaling_factor;
				this->m_info.menu_scaling_factor -= 0.1;
				if(this->m_info.menu_scaling_factor < 0.35)
					this->m_info.menu_scaling_factor = 0.3;
				if(old_scaling != this->m_info.menu_scaling_factor) {
					this->print_notification_float("Menu Scaling", this->m_info.menu_scaling_factor, 1);
				}
				break;


			case 'x':
				old_scaling = this->m_info.menu_scaling_factor;
				this->m_info.menu_scaling_factor += 0.1;
				if(this->m_info.menu_scaling_factor > 4.95)
					this->m_info.menu_scaling_factor = 5.0;
				if(old_scaling != this->m_info.menu_scaling_factor) {
					this->print_notification_float("Menu Scaling", this->m_info.menu_scaling_factor, 1);
				}
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

			default:
				break;
			}

			break;
			
		case sf::Event::KeyPressed:
			switch (event_data.code) {
			case sf::Keyboard::Escape:
				this->m_prepare_quit = true;
				break;
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
			}

			break;
		case sf::Event::JoystickButtonPressed:
			break;
		case sf::Event::JoystickMoved:
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

void WindowScreen::display_thread(CaptureData* capture_data) {
	while(capture_data->running) {
		this->display_lock.lock();
		if(!capture_data->running)
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
	if(this->m_stype == WindowScreen::ScreenType::JOINT)
		should_be_open = !should_be_open;
	if(this->m_win.isOpen() ^ should_be_open) {
		if(this->m_win.isOpen())
			this->close();
		else
			this->open();
	}
	loaded_operations = future_operations;
	if(this->m_win.isOpen() || this->loaded_operations.call_create) {
		WindowScreen::reset_operations(future_operations);
		memcpy(this->saved_buf, out_buf, sizeof(VideoOutputData));
		loaded_info = m_info;
		this->notification->setTextFactor(this->loaded_info.menu_scaling_factor);
		this->notification->prepareRenderText();
		this->frame_time = frame_time;
		this->scheduled_work_on_window = this->window_needs_work();
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
	float approx_factor = pow(0.1, decimals) * (0.5);
	int int_part = (int)(value + approx_factor);
	int dec_part = (int)((value + approx_factor - int_part) * pow(10, decimals));
	std::string status_text = std::to_string(int_part);

	if(decimals > 0) {
		if(!dec_part) {
			status_text += ".";
			for(int i = 0; i < decimals; i++)
				status_text += "0";
		}
		else
			status_text += "." + std::to_string(dec_part);
	}
	this->print_notification(base_text + ": " + status_text);
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
			events_queue.emplace(event.type, event.key.code, event.text.unicode, joystickId, event.joystickButton.button, event.joystickMove.axis, event.joystickMove.position);
		}
		this->events_access->unlock();
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
			this->m_win.create(sf::VideoMode(this->m_width, this->m_height), this->win_title);
			this->update_view_size();
		}
		else
			this->m_win.create(this->curr_desk_mode, this->win_title, sf::Style::Fullscreen);
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
	this->is_window_factory_done = true;
}

void WindowScreen::pre_texture_conversion_processing() {
	//Place preprocessing window-specific effects here
	this->in_tex.update((uint8_t*)this->saved_buf, IN_VIDEO_WIDTH, IN_VIDEO_HEIGHT, 0, 0);
}

void WindowScreen::post_texture_conversion_processing(out_rect_data &rect_data, const sf::RectangleShape &in_rect, bool actually_draw, bool is_top) {
	if((is_top && this->m_stype == WindowScreen::ScreenType::BOTTOM) || ((!is_top) && this->m_stype == WindowScreen::ScreenType::TOP))
		return;

	rect_data.out_tex.clear();
	if(actually_draw) {
		rect_data.out_tex.draw(in_rect);
		//Place postprocessing effects here
	}
	rect_data.out_tex.display();
}

void WindowScreen::window_bg_processing() {
	//Place BG processing here
}

void WindowScreen::display_data_to_window(bool actually_draw) {
	this->post_texture_conversion_processing(this->m_out_rect_top, this->m_in_rect_top, actually_draw, true);
	this->post_texture_conversion_processing(this->m_out_rect_bot, this->m_in_rect_bot, actually_draw, false);

	this->m_win.clear();
	this->window_bg_processing();
	if(this->m_stype != WindowScreen::ScreenType::BOTTOM)
		this->m_win.draw(this->m_out_rect_top.out_rect);
	if(this->m_stype != WindowScreen::ScreenType::TOP)
		this->m_win.draw(this->m_out_rect_bot.out_rect);
	this->notification->draw(this->m_win);
	this->m_win.display();
}

void WindowScreen::window_render_call() {
	this->m_win.setVerticalSyncEnabled(this->loaded_info.v_sync_enabled);
	this->m_win.setMouseCursorVisible(this->loaded_info.show_mouse);
	this->pre_texture_conversion_processing();

	this->display_data_to_window(true);

	if(this->loaded_info.bfi) {
		if(this->frame_time > 0.0) {
			sf::sleep(sf::seconds(frame_time / (this->loaded_info.bfi_divider * 1.1)));
			this->display_data_to_window(false);
		}
	}
}

int WindowScreen::apply_offset_algo(int offset_contribute, OffsetAlgorithm chosen_algo) {
	switch(chosen_algo) {
		case NO_DISTANCE:
			return 0;
		case HALF_DISTANCE:
			return offset_contribute / 2;
		case MAX_DISTANCE:
			return offset_contribute;
		default:
			return 0;
	}
}

void WindowScreen::set_position_screens(sf::Vector2f &curr_top_screen_size, sf::Vector2f &curr_bot_screen_size, int offset_x, int offset_y, int max_x, int max_y) {
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

	if(this->m_stype == WindowScreen::ScreenType::TOP)
		bot_screen_width = bot_screen_height = 0;
	if(this->m_stype == WindowScreen::ScreenType::BOTTOM)
		top_screen_width = top_screen_height = 0;

	int greatest_width = top_screen_width;
	if(greatest_width < bot_screen_width)
		greatest_width = bot_screen_width;
	int greatest_height = top_screen_height;
	if(greatest_height < bot_screen_height)
		greatest_height = bot_screen_height;

	if(this->m_stype == WindowScreen::ScreenType::JOINT) {
		switch(this->loaded_info.bottom_pos) {
			case UNDER_TOP:
				bot_screen_x = apply_offset_algo(greatest_width - bot_screen_width, this->loaded_info.subscreen_offset_algorithm);
				top_screen_x = apply_offset_algo(greatest_width - top_screen_width, this->loaded_info.subscreen_offset_algorithm);
				bot_screen_y = top_screen_height;
				if(max_y > 0)
					bot_screen_y += apply_offset_algo(max_y - bot_screen_height - top_screen_height - offset_y, this->loaded_info.subscreen_attached_offset_algorithm);
				break;
			case RIGHT_TOP:
				bot_screen_y = apply_offset_algo(greatest_height - bot_screen_height, this->loaded_info.subscreen_offset_algorithm);
				top_screen_y = apply_offset_algo(greatest_height - top_screen_height, this->loaded_info.subscreen_offset_algorithm);
				bot_screen_x = top_screen_width;
				if(max_x > 0)
					bot_screen_x += apply_offset_algo(max_x - bot_screen_width - top_screen_width - offset_x, this->loaded_info.subscreen_attached_offset_algorithm);
				break;
			case ABOVE_TOP:
				bot_screen_x = apply_offset_algo(greatest_width - bot_screen_width, this->loaded_info.subscreen_offset_algorithm);
				top_screen_x = apply_offset_algo(greatest_width - top_screen_width, this->loaded_info.subscreen_offset_algorithm);
				top_screen_y = bot_screen_height;
				if(max_y > 0)
					top_screen_y += apply_offset_algo(max_y - bot_screen_height - top_screen_height - offset_y, this->loaded_info.subscreen_attached_offset_algorithm);
				break;
			case LEFT_TOP:
				bot_screen_y = apply_offset_algo(greatest_height - bot_screen_height, this->loaded_info.subscreen_offset_algorithm);
				top_screen_y = apply_offset_algo(greatest_height - top_screen_height, this->loaded_info.subscreen_offset_algorithm);
				top_screen_x = bot_screen_width;
				if(max_x > 0)
					top_screen_x += apply_offset_algo(max_x - bot_screen_width - top_screen_width - offset_x, this->loaded_info.subscreen_attached_offset_algorithm);
				break;
			default:
				break;
		}
	}
	this->m_out_rect_top.out_rect.setPosition(top_screen_x + offset_x + get_screen_corner_modifier_x(this->loaded_info.top_rotation, top_screen_width), top_screen_y + offset_y + get_screen_corner_modifier_y(this->loaded_info.top_rotation, top_screen_height));
	this->m_out_rect_bot.out_rect.setPosition(bot_screen_x + offset_x + get_screen_corner_modifier_x(this->loaded_info.bot_rotation, bot_screen_width), bot_screen_y + offset_y + get_screen_corner_modifier_y(this->loaded_info.bot_rotation, bot_screen_height));

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

int WindowScreen::prepare_screen_ratio(sf::Vector2f &screen_size, int own_rotation, int width_limit, int height_limit, int other_rotation) {
	int own_width = screen_size.x;
	int own_height = screen_size.y;
	if((own_width <= 0) || (own_height <= 0))
		return 0;

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

void WindowScreen::calc_scaling_resize_screens(sf::Vector2f &own_screen_size, sf::Vector2f &other_screen_size, int &own_scaling, int &other_scaling, int own_rotation, int other_rotation, bool increase, bool mantain, bool set_to_zero) {
	int chosen_ratio = prepare_screen_ratio(own_screen_size, own_rotation, other_screen_size.x, other_screen_size.y, other_rotation);
	if(increase && (chosen_ratio > own_scaling) && (own_scaling > 0))
		own_scaling += 1;
	else if(mantain && (chosen_ratio >= own_scaling) && (own_scaling > 0))
		own_scaling = own_scaling;
	else
		own_scaling = chosen_ratio;
	if(set_to_zero)
		own_scaling = 0;
	int own_height = own_scaling * own_screen_size.y;
	int own_width = own_scaling * own_screen_size.x;
	other_scaling = prepare_screen_ratio(other_screen_size, other_rotation, own_width, own_height, own_rotation);
	if(this->m_stype == WindowScreen::ScreenType::JOINT) {
		// Due to size differences, it may be possible that
		// the chosen screen might be able to increase its
		// scaling even more without compromising the other one...
		int other_height = other_scaling * other_screen_size.y;
		int other_width = other_scaling * other_screen_size.x;
		own_scaling = prepare_screen_ratio(own_screen_size, own_rotation, other_width, other_height, other_rotation);
	}
}

void WindowScreen::prepare_size_ratios(bool top_increase, bool bot_increase) {
	if(!this->m_info.is_fullscreen)
		return;

	sf::Vector2f top_screen_size = getShownScreenSize(true, this->m_info.crop_kind);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, this->m_info.crop_kind);

	if(this->m_stype == WindowScreen::ScreenType::TOP) {
		bot_increase = true;
		top_increase = false;
	}
	if(this->m_stype == WindowScreen::ScreenType::BOTTOM) {
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
		calc_scaling_resize_screens(top_screen_size, bot_screen_size, this->m_info.top_scaling, this->m_info.bot_scaling, this->m_info.top_rotation, this->m_info.bot_rotation, top_increase, try_mantain_ratio, this->m_stype == WindowScreen::ScreenType::BOTTOM);
	else
		calc_scaling_resize_screens(bot_screen_size, top_screen_size, this->m_info.bot_scaling, this->m_info.top_scaling, this->m_info.bot_rotation, this->m_info.top_rotation, bot_increase, try_mantain_ratio, this->m_stype == WindowScreen::ScreenType::TOP);
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
	return apply_offset_algo(curr_desk_mode.width - offset_contribute, this->loaded_info.total_offset_algorithm_x);
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
	return apply_offset_algo(curr_desk_mode.height - offset_contribute, this->loaded_info.total_offset_algorithm_y);
}

void WindowScreen::resize_window_and_out_rects() {
	sf::Vector2f top_screen_size = getShownScreenSize(true, this->loaded_info.crop_kind);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, this->loaded_info.crop_kind);
	int top_height = this->loaded_info.scaling * top_screen_size.y;
	int top_width = this->loaded_info.scaling * top_screen_size.x;
	int bot_height = this->loaded_info.scaling * bot_screen_size.y;
	int bot_width = this->loaded_info.scaling * bot_screen_size.x;
	int offset_x = 0;
	int offset_y = 0;
	int max_x = 0;
	int max_y = 0;

	if(this->loaded_info.is_fullscreen) {
		top_height = this->loaded_info.top_scaling * top_screen_size.y;
		top_width = this->loaded_info.top_scaling * top_screen_size.x;
		bot_height = this->loaded_info.bot_scaling * bot_screen_size.y;
		bot_width = this->loaded_info.bot_scaling * bot_screen_size.x;
		offset_x = get_fullscreen_offset_x(top_width, top_height, bot_width, bot_height);
		offset_y = get_fullscreen_offset_y(top_width, top_height, bot_width, bot_height);
		max_x = curr_desk_mode.width;
		max_y = curr_desk_mode.height;
	}
	sf::Vector2f new_top_screen_size = sf::Vector2f(top_width, top_height);
	sf::Vector2f new_bot_screen_size = sf::Vector2f(bot_width, bot_height);
	this->m_out_rect_top.out_rect.setSize(new_top_screen_size);
	this->m_out_rect_top.out_rect.setTextureRect(sf::IntRect(0, 0, top_screen_size.x, top_screen_size.y));
	this->m_out_rect_bot.out_rect.setSize(new_bot_screen_size);
	this->m_out_rect_bot.out_rect.setTextureRect(sf::IntRect(0, 0, bot_screen_size.x, bot_screen_size.y));
	this->set_position_screens(new_top_screen_size, new_bot_screen_size, offset_x, offset_y, max_x, max_y);
}

void WindowScreen::create_window(bool re_prepare_size) {
	if(!this->m_info.is_fullscreen) {
		this->m_info.show_mouse = true;
	}
	else {
		this->curr_desk_mode = sf::VideoMode::getDesktopMode();
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

sf::Vector2f WindowScreen::getShownScreenSize(bool is_top, Crop &crop_kind) {
	if(crop_kind >= Crop::CROP_END)
		crop_kind = Crop::DEFAULT_3DS;
	const int* widths = top_screen_crop_widths;
	const int* heights = top_screen_crop_heights;
	if(!is_top) {
		widths = bot_screen_crop_widths;
		heights = bot_screen_crop_heights;
	}
	int crop_arr_index = crop_kind - Crop::DEFAULT_3DS;
	return sf::Vector2f(widths[crop_arr_index], heights[crop_arr_index]);
}

void WindowScreen::crop() {
	if(this->loaded_info.crop_kind >= Crop::CROP_END)
		this->loaded_info.crop_kind = Crop::DEFAULT_3DS;
	int crop_arr_index = this->loaded_info.crop_kind - Crop::DEFAULT_3DS;

	sf::Vector2f top_screen_size = getShownScreenSize(true, this->loaded_info.crop_kind);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, this->loaded_info.crop_kind);

	this->resize_in_rect(this->m_in_rect_top, top_screen_crop_x[crop_arr_index], top_screen_crop_y[crop_arr_index], top_screen_size.x, top_screen_size.y);
	this->resize_in_rect(this->m_in_rect_bot, TOP_WIDTH_3DS + bot_screen_crop_x[crop_arr_index], bot_screen_crop_y[crop_arr_index], bot_screen_size.x, bot_screen_size.y);
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
	win_width = this->m_win.getSize().x;
	win_height = this->m_win.getSize().y;
	int old_x_win_pos = this->m_win.getPosition().x;
	int old_y_win_pos = this->m_win.getPosition().y;
	int x_win_pos = old_x_win_pos;
	int y_win_pos = old_y_win_pos;
	if((x_win_pos + (win_width - 200)) < 0)
		x_win_pos = -(win_width - 200);
	if(y_win_pos < 0)
		y_win_pos = 0;
	if((x_win_pos != old_x_win_pos) || (y_win_pos != old_y_win_pos))
		this->m_win.setPosition(sf::Vector2i(x_win_pos, y_win_pos));
}

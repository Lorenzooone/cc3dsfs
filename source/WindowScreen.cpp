#include "frontend.hpp"

#include <cstring>
#include "font_ttf.h"

#define DEFAULT_FS_WIDTH 1920
#define DEFAULT_FS_HEIGHT 1080

#define LEFT_ROUNDED_PADDING 5
#define RIGHT_ROUNDED_PADDING 5
#define TOP_ROUNDED_PADDING 0
#define BOTTOM_ROUNDED_PADDING 5

WindowScreen::WindowScreen(ScreenType stype, CaptureStatus* capture_status, DisplayData* display_data, AudioData* audio_data) {
	this->m_stype = stype;
	insert_basic_crops(this->possible_crops, this->m_stype);
	insert_basic_pars(this->possible_pars);
	this->m_prepare_save = 0;
	this->m_prepare_load = 0;
	this->m_prepare_open = false;
	this->m_prepare_quit = false;
	this->ret_val = 0;
	reset_screen_info(this->m_info);
	this->font_load_success = this->text_font.loadFromMemory(font_ttf, font_ttf_len);
	this->notification = new TextRectangle(this->font_load_success, this->text_font);
	this->init_menus();
	FPSArrayInit(&this->in_fps);
	FPSArrayInit(&this->draw_fps);
	FPSArrayInit(&this->poll_fps);
	this->in_tex.create(IN_VIDEO_WIDTH, IN_VIDEO_HEIGHT);
	this->m_in_rect_top.setTexture(&this->in_tex);
	this->m_in_rect_bot.setTexture(&this->in_tex);
	this->display_data = display_data;
	this->audio_data = audio_data;
	this->last_window_creation_time = std::chrono::high_resolution_clock::now();
	this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
	this->last_draw_time = std::chrono::high_resolution_clock::now();
	this->last_poll_time = std::chrono::high_resolution_clock::now();
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
	if(this->display_data->mono_app_mode && this->m_stype == ScreenType::JOINT)
		this->m_info.is_fullscreen = true;
}

WindowScreen::~WindowScreen() {
	this->possible_files.clear();
	this->possible_resolutions.clear();
	delete this->saved_buf;
	delete this->notification;
	this->destroy_menus();
	FPSArrayDestroy(&this->in_fps);
	FPSArrayDestroy(&this->draw_fps);
	FPSArrayDestroy(&this->poll_fps);
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
	if(this->curr_menu != CONNECT_MENU_TYPE)
		this->curr_menu = DEFAULT_MENU_TYPE;
	this->future_operations.call_crop = true;
	this->future_operations.call_rotate = true;
	this->future_operations.call_blur = true;
	this->future_operations.call_screen_settings_update = true;
	if(this->m_win.isOpen())
		this->create_window(false);
	this->prepare_size_ratios(true, true);
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
	FPSArrayInsertElement(&in_fps, frame_time);
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
	if(!should_be_open)
		this->m_info.is_fullscreen = false;
	this->loaded_menu = this->curr_menu;
	loaded_operations = future_operations;
	if(this->loaded_operations.call_create) {
		this->last_draw_time = std::chrono::high_resolution_clock::now();
		this->last_poll_time = std::chrono::high_resolution_clock::now();
	}
	if(this->m_win.isOpen() || this->loaded_operations.call_create) {
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_draw_time;
		FPSArrayInsertElement(&draw_fps, diff.count());
		this->last_draw_time = curr_time;
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
		this->prepare_menu_draws(view_size_x, view_size_y);
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
		this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		this->m_win.close();
		while(!events_queue.empty())
			events_queue.pop();
		this->loaded_operations.call_close = false;
		this->loaded_operations.call_create = false;
	}
	if(this->loaded_operations.call_create) {
		this->notification->setShowText(false);
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
	this->execute_menu_draws();
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

void WindowScreen::calc_scaling_resize_screens(ResizingScreenData *own_screen, ResizingScreenData* other_screen, bool increase, bool mantain, bool set_to_zero, bool cycle) {
	int other_width = other_screen->size.x;
	int other_height = other_screen->size.y;
	get_par_size(other_width, other_height, 1, other_screen->par);
	int chosen_ratio = prepare_screen_ratio(own_screen->size, own_screen->rotation, other_width, other_height, other_screen->rotation, own_screen->par);
	if(chosen_ratio <= 0) {
		chosen_ratio = prepare_screen_ratio(own_screen->size, own_screen->rotation, 0, 0, other_screen->rotation, own_screen->par);
		if(chosen_ratio <= 0)
			chosen_ratio = 0;
	}
	int old_scaling = (*own_screen->scaling);
	if(increase && (chosen_ratio > (*own_screen->scaling)) && (chosen_ratio > 0))
		(*own_screen->scaling) += 1;
	else if(mantain && (chosen_ratio >= (*own_screen->scaling)) && (chosen_ratio > 0) && ((*own_screen->scaling) >= 0))
		(*own_screen->scaling) = (*own_screen->scaling);
	else if(cycle && (chosen_ratio <= (*own_screen->scaling)) && ((*other_screen->scaling) == 0))
		(*own_screen->scaling) = 0;
	else
		(*own_screen->scaling) = chosen_ratio;
	if(set_to_zero)
		(*own_screen->scaling) = 0;
	int own_height = own_screen->size.y;
	int own_width = own_screen->size.x;
	get_par_size(own_width, own_height, (*own_screen->scaling), own_screen->par);
	if((increase && ((old_scaling == (*own_screen->scaling)) || ((*other_screen->scaling) == 0)) || (mantain && ((*other_screen->scaling) == 0))) && ((*own_screen->scaling) > 0))
		(*other_screen->scaling) = 0;
	else
		(*other_screen->scaling) = prepare_screen_ratio(other_screen->size, other_screen->rotation, own_width, own_height, own_screen->rotation, other_screen->par);
	if((*other_screen->scaling) < 0)
		(*other_screen->scaling) = 0;
	if((this->m_stype == ScreenType::JOINT) && (((*own_screen->scaling) > 0) || (((*other_screen->scaling) == 0) && ((*own_screen->scaling) == 0)))) {
		// Due to size differences, it may be possible that
		// the chosen screen might be able to increase its
		// scaling even more without compromising the other one...
		other_width = other_screen->size.x;
		other_height = other_screen->size.y;
		get_par_size(other_width, other_height, *other_screen->scaling, other_screen->par);
		(*own_screen->scaling) = prepare_screen_ratio(own_screen->size, own_screen->rotation, other_width, other_height, other_screen->rotation, own_screen->par);
		if((*own_screen->scaling) < 0)
			(*own_screen->scaling) = 0;
	}
}

void WindowScreen::prepare_size_ratios(bool top_increase, bool bot_increase, bool cycle) {
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
	ResizingScreenData top_screen_resize_data = {.size = top_screen_size, .scaling = &this->m_info.top_scaling, .rotation = this->m_info.top_rotation, .par = this->possible_pars[this->m_info.top_par]};
	ResizingScreenData bot_screen_resize_data = {.size = bot_screen_size, .scaling = &this->m_info.bot_scaling, .rotation = this->m_info.bot_rotation, .par = this->possible_pars[this->m_info.bot_par]};
	if(prioritize_top)
		calc_scaling_resize_screens(&top_screen_resize_data, &bot_screen_resize_data, top_increase, try_mantain_ratio, this->m_stype == ScreenType::BOTTOM, cycle);
	else
		calc_scaling_resize_screens(&bot_screen_resize_data, &top_screen_resize_data, bot_increase, try_mantain_ratio, this->m_stype == ScreenType::TOP, cycle);
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
	if(!this->m_info.is_fullscreen)
		this->m_info.show_mouse = !this->display_data->mono_app_mode;
	else {
		bool success = false;
		if((this->m_info.fullscreen_mode_width > 0) && (this->m_info.fullscreen_mode_height > 0) && (this->m_info.fullscreen_mode_bpp > 0)) {
			sf::VideoMode mode_created = sf::VideoMode(this->m_info.fullscreen_mode_width, this->m_info.fullscreen_mode_height, this->m_info.fullscreen_mode_bpp);
			if(mode_created.isValid()) {
				this->curr_desk_mode = mode_created;
				success = true;
			}
		}
		#ifdef __APPLE__
		if(!success) {
			sf::VideoMode mode_created = sf::VideoMode(DEFAULT_FS_WIDTH, DEFAULT_FS_HEIGHT, 32);
			if(mode_created.isValid()) {
				this->curr_desk_mode = mode_created;
				success = true;
			}
			else {
				mode_created = sf::VideoMode(DEFAULT_FS_WIDTH, DEFAULT_FS_HEIGHT, 24);
				if(mode_created.isValid()) {
					this->curr_desk_mode = mode_created;
					success = true;
				}
			}
		}
		#endif
		if(!success) {
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
		this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		this->m_win.setSize(sf::Vector2u(width, height));
	}
}

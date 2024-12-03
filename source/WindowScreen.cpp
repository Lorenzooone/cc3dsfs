#include "frontend.hpp"

#define GL_SILENCE_DEPRECATION
#include <SFML/OpenGL.hpp>
#include <cstring>
#include "font_ttf.h"
#include "shaders_list.hpp"
#include "devicecapture.hpp"

#define LEFT_ROUNDED_PADDING 5
#define RIGHT_ROUNDED_PADDING 5
#define TOP_ROUNDED_PADDING 0
#define BOTTOM_ROUNDED_PADDING 5

#define FALLBACK_FS_RESOLUTION_WIDTH 1920
#define FALLBACK_FS_RESOLUTION_HEIGHT 1080
#define FALLBACK_FS_RESOLUTION_BPP 32

#define NUM_FRAMES_BLENDED 2

static bool loaded_shaders = false;
static int n_shader_refs = 0;

struct shader_and_data {
	shader_and_data(shader_list_enum value) : shader(sf::Shader()), is_valid(false), shader_enum(value) {}

	sf::Shader shader;
	bool is_valid;
	shader_list_enum shader_enum;
};

static std::vector<shader_and_data> usable_shaders;

WindowScreen::WindowScreen(ScreenType stype, CaptureStatus* capture_status, DisplayData* display_data, SharedData* shared_data, AudioData* audio_data, ConsumerMutex *draw_lock, bool created_proper_folder) {
	this->draw_lock = draw_lock;
	this->m_stype = stype;
	insert_basic_crops(this->possible_crops, this->m_stype, false, false);
	insert_basic_crops(this->possible_crops_ds, this->m_stype, true, false);
	insert_basic_crops(this->possible_crops_with_games, this->m_stype, false, true);
	insert_basic_crops(this->possible_crops_ds_with_games, this->m_stype, true, true);
	insert_basic_pars(this->possible_pars);
	this->m_prepare_save = 0;
	this->m_prepare_load = 0;
	this->m_prepare_open = false;
	this->m_prepare_quit = false;
	this->ret_val = 0;
	reset_screen_info(this->m_info);
	this->font_load_success = this->text_font.openFromMemory(font_ttf, font_ttf_len);
	this->notification = new TextRectangle(this->font_load_success, this->text_font);
	this->init_menus();
	FPSArrayInit(&this->in_fps);
	FPSArrayInit(&this->draw_fps);
	FPSArrayInit(&this->poll_fps);
	(void)this->in_tex.resize({MAX_IN_VIDEO_WIDTH * NUM_FRAMES_BLENDED, MAX_IN_VIDEO_HEIGHT});
	this->m_in_rect_top.setTexture(&this->in_tex);
	this->m_in_rect_bot.setTexture(&this->in_tex);
	this->display_data = display_data;
	this->shared_data = shared_data;
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
	if(sf::Shader::isAvailable() && (!loaded_shaders)) {
		shader_strings_init();
		for(int i = 0; i < TOTAL_NUM_SHADERS; i++) {
			usable_shaders.emplace_back(static_cast<shader_list_enum>(i));
			shader_and_data* current_shader = &usable_shaders[usable_shaders.size()-1];
			if(current_shader->shader.loadFromMemory(get_shader_string(current_shader->shader_enum), sf::Shader::Type::Fragment)) {
				current_shader->is_valid = true;
				auto* const defaultStreamBuffer = sf::err().rdbuf();
				sf::err().rdbuf(nullptr);
				sf::Glsl::Vec2 old_pos = {0.0, 0.0};
				current_shader->shader.setUniform("old_frame_offset", old_pos);
				sf::err().rdbuf(defaultStreamBuffer);
			}
		}
		loaded_shaders = true;
	}
	n_shader_refs += 1;
	this->was_last_frame_null = true;
	this->created_proper_folder = created_proper_folder;
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
	if(sf::Shader::isAvailable() && (n_shader_refs == 1)) {
		while(!usable_shaders.empty())
			usable_shaders.pop_back();
		loaded_shaders = false;
	}
	n_shader_refs -= 1;
}

void WindowScreen::build() {
	sf::Vector2f top_screen_size = {(float)TOP_WIDTH_3DS, (float)HEIGHT_3DS};
	sf::Vector2f bot_screen_size = {(float)BOT_WIDTH_3DS, (float)HEIGHT_3DS};

	int width = TOP_WIDTH_3DS;
	if(this->m_stype == ScreenType::BOTTOM)
		width = BOT_WIDTH_3DS;

	(void)this->m_out_rect_top.out_tex.resize({(unsigned int)top_screen_size.x, (unsigned int)top_screen_size.y});
	this->m_out_rect_top.out_rect.setSize(top_screen_size);
	this->m_out_rect_top.out_rect.setTexture(&this->m_out_rect_top.out_tex.getTexture());

	(void)this->m_out_rect_bot.out_tex.resize({(unsigned int)bot_screen_size.x, (unsigned int)bot_screen_size.y});
	this->m_out_rect_bot.out_rect.setSize(bot_screen_size);
	this->m_out_rect_bot.out_rect.setTexture(&this->m_out_rect_bot.out_tex.getTexture());

	this->m_view.setRotation(sf::degrees(0));

	this->reload();
}

void WindowScreen::reload() {
	if(this->curr_menu != CONNECT_MENU_TYPE)
		this->setup_no_menu();
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
			this->draw_lock->lock();
			(void)this->m_win.setActive(true);
			this->main_thread_owns_window = is_main_thread;
			this->draw_lock->unlock();
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
	if(!this->main_thread_owns_window) {
		this->draw_lock->lock();
		(void)this->m_win.setActive(false);
		this->draw_lock->unlock();
	}
	this->done_display = true;
}

void WindowScreen::end() {
	if(this->main_thread_owns_window) {
		this->draw_lock->lock();
		(void)this->m_win.setActive(false);
		this->draw_lock->unlock();
	}
	this->display_lock.unlock();
}

void WindowScreen::after_thread_join() {
	if(this->m_win.isOpen()) {
		this->draw_lock->lock();
		(void)this->m_win.setActive(true);
		this->m_win.close();
		this->draw_lock->unlock();
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
		this->curr_frame_texture_pos = (this->curr_frame_texture_pos + 1) % NUM_FRAMES_BLENDED;
		auto curr_time = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> diff = curr_time - this->last_draw_time;
		FPSArrayInsertElement(&draw_fps, diff.count());
		this->last_draw_time = curr_time;
		WindowScreen::reset_operations(future_operations);
		if(out_buf != NULL)
			memcpy(this->saved_buf, out_buf, sizeof(VideoOutputData));
		else {
			memset(this->saved_buf, 0, sizeof(VideoOutputData));
			this->was_last_frame_null = true;
		}
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
		// This must be done in the main thread for it to work on Windows... :/
		this->m_win.setMouseCursorVisible(this->loaded_info.show_mouse);
		this->is_window_factory_done = true;
		if(!this->loaded_info.async)
			this->display_call(true);
	}
	else
		this->was_last_frame_null = true;
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
	operations.call_titlebar = false;
}

void WindowScreen::free_ownership_of_window(bool is_main_thread) {
	if(is_main_thread == this->main_thread_owns_window) {
		if((this->scheduled_work_on_window) || (is_main_thread == this->loaded_info.async)) {
			this->draw_lock->lock();
			(void)this->m_win.setActive(false);
			this->draw_lock->unlock();
		}
	}
}

void WindowScreen::resize_in_rect(sf::RectangleShape &in_rect, int start_x, int start_y, int width, int height) {
	int target_x = start_x;
	int target_y = start_y;
	int target_width = width;
	int target_height = height;
	int target_rotation = -1 * this->capture_status->device.base_rotation;

	if((this->capture_status->device.base_rotation / 10) % 2) {
		std::swap(target_x, target_y);
		std::swap(target_width, target_height);
	}

	in_rect.setTextureRect(sf::IntRect({target_x, target_y}, {target_width, target_height}));

	in_rect.setSize({(float)target_width, (float)target_height});
	in_rect.setOrigin({target_width / 2.0f, target_height / 2.0f});

	in_rect.setRotation(sf::degrees(target_rotation));
	in_rect.setPosition({width / 2.0f, height / 2.0f});
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
		this->draw_lock->lock();
		(void)this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		this->m_win.close();
		while(!events_queue.empty())
			events_queue.pop();
		this->draw_lock->unlock();
		this->loaded_operations.call_close = false;
		this->loaded_operations.call_create = false;
	}
	if(this->loaded_operations.call_create) {
		this->draw_lock->lock();
		(void)this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		bool previously_open = this->m_win.isOpen();
		sf::Vector2i prev_pos = sf::Vector2i(0, 0);
		if(previously_open)
			prev_pos = this->m_win.getPosition();
		if(!this->loaded_info.is_fullscreen) {
			this->update_screen_settings();
			if(this->loaded_info.have_titlebar)
				this->m_win.create(sf::VideoMode({(unsigned int)this->m_width, (unsigned int)this->m_height}), this->title_factory());
			else
				this->m_win.create(sf::VideoMode({(unsigned int)this->m_width, (unsigned int)this->m_height}), this->title_factory(), sf::Style::None);
			this->update_view_size();
		}
		else {
			if(!this->loaded_info.failed_fullscreen)
				this->m_win.create(this->curr_desk_mode, this->title_factory(), sf::Style::Default, sf::State::Fullscreen);
			else if(this->loaded_info.have_titlebar)
				this->m_win.create(this->curr_desk_mode, this->title_factory());
			else
				this->m_win.create(this->curr_desk_mode, this->title_factory(), sf::Style::None);
		}
		if(previously_open && this->loaded_operations.call_titlebar)
			this->m_win.setPosition(prev_pos);
		this->last_window_creation_time = std::chrono::high_resolution_clock::now();
		this->update_screen_settings();
		this->draw_lock->unlock();
		this->loaded_operations.call_create = false;
	}
	if(this->m_win.isOpen()) {
		this->setWinSize(is_main_thread);
	}
	if((is_main_thread == this->main_thread_owns_window) && (this->main_thread_owns_window == this->loaded_info.async)) {
		this->draw_lock->lock();
		(void)this->m_win.setActive(false);
		this->draw_lock->unlock();
	}
	this->update_connection();
}

std::string WindowScreen::title_factory() {
	std::string title = this->win_title;
	if(this->capture_status->connected)
		title += " - " + get_name_of_device(this->capture_status);
	return title;
}

// These values may be undefined under Windows...
#ifndef GL_BGR
#define GL_BGR 0x80e0
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

void WindowScreen::update_texture() {
	unsigned int m_texture = this->in_tex.getNativeHandle();
	if (this->saved_buf && m_texture)
	{
		// Copy pixels from the given array to the texture
		glBindTexture(GL_TEXTURE_2D, m_texture);
		GLenum format = GL_RGB;
		GLenum type = GL_UNSIGNED_BYTE;
		if(this->capture_status->device.video_data_type == VIDEO_DATA_BGR)
			format = GL_BGR;
		if(this->capture_status->device.video_data_type == VIDEO_DATA_RGB16)
			type =  GL_UNSIGNED_SHORT_5_6_5;
		if(this->capture_status->device.video_data_type == VIDEO_DATA_BGR16)
			type =  GL_UNSIGNED_SHORT_5_6_5_REV;
		glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(this->curr_frame_texture_pos * MAX_IN_VIDEO_WIDTH), static_cast<GLint>(0), static_cast<GLsizei>(this->capture_status->device.width), static_cast<GLsizei>(this->capture_status->device.height), format, type, this->saved_buf);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// Force an OpenGL flush, so that the texture data will appear updated
		// in all contexts immediately (solves problems in multi-threaded apps)
		glFlush();
	}
}

void WindowScreen::pre_texture_conversion_processing() {
	if(this->loaded_menu == CONNECT_MENU_TYPE)
		return;
	if(!this->capture_status->connected)
		return;
	this->draw_lock->lock();
	//Place preprocessing window-specific effects here
	this->update_texture();
	this->draw_lock->unlock();
}

int WindowScreen::_choose_shader(bool is_input, bool is_top) {
	if(!is_input)
		return NO_EFFECT_FRAGMENT_SHADER;
	if(this->was_last_frame_null) {
		this->was_last_frame_null = false;
		return NO_EFFECT_FRAGMENT_SHADER;
	}
	InputColorspaceMode *in_colorspace = &this->loaded_info.in_colorspace_top;
	FrameBlendingMode *frame_blending = &this->loaded_info.frame_blending_top;
	if(!is_top) {
		in_colorspace = &this->loaded_info.in_colorspace_bot;
		frame_blending = &this->loaded_info.frame_blending_bot;
	}

	switch(*frame_blending) {
		case FULL_FRAME_BLENDING:
			switch(*in_colorspace) {
				case DS_COLORSPACE:
					return FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_2;
				case GBA_COLORSPACE:
					return FRAME_BLENDING_BIT_CRUSHER_FRAGMENT_SHADER_3;
				default:
					return FRAME_BLENDING_FRAGMENT_SHADER;
			}
			break;
		case DS_3D_BOTH_SCREENS_FRAME_BLENDING:
			switch(*in_colorspace) {
				case DS_COLORSPACE:
					return BIT_MERGER_CRUSHER_FRAGMENT_SHADER_2;
				case GBA_COLORSPACE:
					return BIT_CRUSHER_FRAGMENT_SHADER_3;
				default:
					return BIT_MERGER_FRAGMENT_SHADER_2;
			}
			break;
		default:
			switch(*in_colorspace) {
				case DS_COLORSPACE:
					return BIT_CRUSHER_FRAGMENT_SHADER_2;
				case GBA_COLORSPACE:
					return BIT_CRUSHER_FRAGMENT_SHADER_3;
				default:
					return NO_EFFECT_FRAGMENT_SHADER;
			}
			break;
	}
}

int WindowScreen::choose_shader(bool is_input, bool is_top) {
	int chosen_shader = _choose_shader(is_input, is_top);
	if((chosen_shader >= 0) && (chosen_shader < usable_shaders.size()) && usable_shaders[chosen_shader].is_valid)
		return chosen_shader;
	return -1;
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
		sf::RectangleShape final_in_rect = in_rect;
		sf::IntRect text_coords_rect = final_in_rect.getTextureRect();
		text_coords_rect.position.x += this->curr_frame_texture_pos * MAX_IN_VIDEO_WIDTH;
		final_in_rect.setTextureRect(text_coords_rect);
		if(this->capture_status->connected && actually_draw) {
			bool use_default_shader = true;
			if(sf::Shader::isAvailable()) {
				int chosen_shader = choose_shader(true, is_top);
				if(chosen_shader >= 0) {
					float old_frame_pos_x = ((float)(NUM_FRAMES_BLENDED - 1)) / NUM_FRAMES_BLENDED;
					if(this->curr_frame_texture_pos > 0)
						old_frame_pos_x = -1.0 / NUM_FRAMES_BLENDED;
					sf::Glsl::Vec2 old_pos = {old_frame_pos_x, 0.0};
					usable_shaders[chosen_shader].shader.setUniform("old_frame_offset", old_pos);
					rect_data.out_tex.draw(final_in_rect, &usable_shaders[chosen_shader].shader);
					use_default_shader = false;
				}
			}
			if(use_default_shader)
				rect_data.out_tex.draw(final_in_rect);
			//Place postprocessing effects here
		}
	}
	rect_data.out_tex.display();
}

void WindowScreen::draw_rect_to_window(const sf::RectangleShape &out_rect, bool is_top) {
	if((is_top && this->m_stype == ScreenType::BOTTOM) || ((!is_top) && this->m_stype == ScreenType::TOP))
		return;
	if(this->loaded_menu == CONNECT_MENU_TYPE)
		return;

	bool use_default_shader = true;
	if(sf::Shader::isAvailable()) {
		int chosen_shader = choose_shader(false, is_top);
		if(chosen_shader >= 0) {
			this->m_win.draw(out_rect, &usable_shaders[chosen_shader].shader);
			use_default_shader = false;
		}
	}
	if(use_default_shader)
		this->m_win.draw(out_rect);
}

void WindowScreen::window_bg_processing() {
	//Place BG processing here
}

void WindowScreen::display_data_to_window(bool actually_draw, bool is_debug) {
	this->draw_lock->lock();
	this->post_texture_conversion_processing(this->m_out_rect_top, this->m_in_rect_top, actually_draw, true, is_debug);
	this->post_texture_conversion_processing(this->m_out_rect_bot, this->m_in_rect_bot, actually_draw, false, is_debug);

	if(is_debug)
		this->m_win.clear(sf::Color::Green);
	else
		this->m_win.clear();
	this->window_bg_processing();
	this->draw_rect_to_window(this->m_out_rect_top.out_rect, true);
	this->draw_rect_to_window(this->m_out_rect_bot.out_rect, false);
	this->execute_menu_draws();
	this->notification->draw(this->m_win);
	this->m_win.display();
	this->draw_lock->unlock();
}

void WindowScreen::window_render_call() {
	auto curr_time = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double> diff = curr_time - this->last_window_creation_time;
	if(diff.count() > this->v_sync_timeout)
		this->m_win.setVerticalSyncEnabled(this->loaded_info.v_sync_enabled);
	else
		this->m_win.setVerticalSyncEnabled(false);
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
		float scale_issue_fix_offset = -0.005;
		this->m_out_rect_top.out_rect.setPosition({top_screen_x + scale_issue_fix_offset + offset_x + get_screen_corner_modifier_x(this->loaded_info.top_rotation, top_screen_width), top_screen_y + offset_y + get_screen_corner_modifier_y(this->loaded_info.top_rotation, top_screen_height) + scale_issue_fix_offset});
		this->m_out_rect_bot.out_rect.setPosition({bot_screen_x + scale_issue_fix_offset + offset_x + get_screen_corner_modifier_x(this->loaded_info.bot_rotation, bot_screen_width), bot_screen_y + offset_y + get_screen_corner_modifier_y(this->loaded_info.bot_rotation, bot_screen_height) + scale_issue_fix_offset});
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

float WindowScreen::get_max_float_screen_multiplier(ResizingScreenData *own_screen, int width_limit, int height_limit, int other_rotation) {
	if((own_screen->size.x < 1.0) || (own_screen->size.y < 1.0))
		return 0;

	if((other_rotation / 10) % 2)
		std::swap(width_limit, height_limit);
	int desk_height = curr_desk_mode.size.y;
	int desk_width = curr_desk_mode.size.x;
	int height_max = desk_height;
	int width_max = desk_width;
	switch(this->m_info.bottom_pos) {
		case UNDER_TOP:
		case ABOVE_TOP:
			height_max = desk_height - height_limit;
			break;
		case LEFT_TOP:
		case RIGHT_TOP:
			width_max = desk_width - width_limit;
			break;
		default:
			break;
	}

	return get_par_mult_factor(own_screen->size.x, own_screen->size.y, width_max, height_max, own_screen->par, (own_screen->rotation / 10) % 2);
}

int WindowScreen::prepare_screen_ratio(ResizingScreenData *own_screen, int width_limit, int height_limit, int other_rotation) {
	return this->get_max_float_screen_multiplier(own_screen, width_limit, height_limit, other_rotation);
}

void WindowScreen::calc_scaling_resize_screens(ResizingScreenData *own_screen, ResizingScreenData* other_screen, bool increase, bool mantain, bool set_to_zero, bool cycle) {
	int other_width = other_screen->size.x;
	int other_height = other_screen->size.y;
	get_par_size(other_width, other_height, 1, other_screen->par);
	int chosen_ratio = prepare_screen_ratio(own_screen, other_width, other_height, other_screen->rotation);
	if(chosen_ratio <= 0) {
		chosen_ratio = prepare_screen_ratio(own_screen, 0, 0, other_screen->rotation);
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
	get_par_size(own_width, own_height, *own_screen->scaling, own_screen->par);
	if((increase && ((old_scaling == (*own_screen->scaling)) || ((*other_screen->scaling) == 0)) || (mantain && ((*other_screen->scaling) == 0))) && ((*own_screen->scaling) > 0))
		(*other_screen->scaling) = 0;
	else
		(*other_screen->scaling) = prepare_screen_ratio(other_screen, own_width, own_height, own_screen->rotation);
	if((*other_screen->scaling) < 0)
		(*other_screen->scaling) = 0;
	if((this->m_stype == ScreenType::JOINT) && (((*own_screen->scaling) > 0) || (((*other_screen->scaling) == 0) && ((*own_screen->scaling) == 0)))) {
		// Due to size differences, it may be possible that
		// the chosen screen might be able to increase its
		// scaling even more without compromising the other one...
		other_width = other_screen->size.x;
		other_height = other_screen->size.y;
		get_par_size(other_width, other_height, *other_screen->scaling, other_screen->par);
		(*own_screen->scaling) = prepare_screen_ratio(own_screen, other_width, other_height, other_screen->rotation);
		if((*own_screen->scaling) < 0)
			(*own_screen->scaling) = 0;
	}
}

bool WindowScreen::can_non_integerly_scale() {
	return ((this->m_info.top_scaling != 0) || (this->m_info.bot_scaling != 0)) && ((this->m_info.use_non_integer_scaling_top && (this->m_info.top_scaling != 0)) || (this->m_info.use_non_integer_scaling_bottom && (this->m_info.bot_scaling != 0)));
}

void WindowScreen::rescale_nonint_subscreen(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data) {
	int new_width = main_screen_resize_data->size.x;
	int new_height = main_screen_resize_data->size.y;
	get_par_size(new_width, new_height, *main_screen_resize_data->non_int_scaling, main_screen_resize_data->par);
	*sub_screen_resize_data->non_int_scaling = this->get_max_float_screen_multiplier(sub_screen_resize_data, new_width, new_height, main_screen_resize_data->rotation);
	if(!sub_screen_resize_data->use_non_int_scaling)
		*sub_screen_resize_data->non_int_scaling = static_cast<int>(*sub_screen_resize_data->non_int_scaling);
}

void WindowScreen::direct_scale_nonint_screen(ResizingScreenData *main_screen_resize_data, int sub_min_width, int sub_height_min, int sub_screen_rotation) {
	*main_screen_resize_data->non_int_scaling = this->get_max_float_screen_multiplier(main_screen_resize_data, sub_min_width, sub_height_min, sub_screen_rotation);
	if(!main_screen_resize_data->use_non_int_scaling)
		*main_screen_resize_data->non_int_scaling = static_cast<int>(*main_screen_resize_data->non_int_scaling);
}

void WindowScreen::non_int_scale_screens_with_main(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data, int sub_min_width, int sub_height_min) {
	this->direct_scale_nonint_screen(main_screen_resize_data, sub_min_width, sub_height_min, sub_screen_resize_data->rotation);
	this->rescale_nonint_subscreen(main_screen_resize_data, sub_screen_resize_data);
}

void WindowScreen::non_int_scale_screens_both(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data, int free_width, int free_height, float multiplier_main, int main_min_width, int main_height_min, int sub_min_width, int sub_height_min) {
	int main_free_width = (free_width * multiplier_main) + 0.5;
	int main_free_height = (free_height * multiplier_main) + 0.5;
	int sub_free_width = free_width - main_free_width;
	int sub_free_height = free_height - main_free_height;
	this->direct_scale_nonint_screen(main_screen_resize_data, sub_min_width + sub_free_width, sub_height_min + sub_free_height, sub_screen_resize_data->rotation);
	this->direct_scale_nonint_screen(sub_screen_resize_data, main_min_width + main_free_width, main_height_min + main_free_height, main_screen_resize_data->rotation);
	this->rescale_nonint_subscreen(main_screen_resize_data, sub_screen_resize_data);
	this->rescale_nonint_subscreen(sub_screen_resize_data, main_screen_resize_data);
}

void WindowScreen::non_integer_scale_screens(ResizingScreenData *top_screen_resize_data, ResizingScreenData *bot_screen_resize_data) {
	if(!this->can_non_integerly_scale()) {
		this->m_info.non_integer_top_scaling = this->m_info.top_scaling;
		this->m_info.non_integer_bot_scaling = this->m_info.bot_scaling;
		return;
	}
	if(this->m_info.top_scaling == 0) {
		this->m_info.non_integer_top_scaling = 0;
		this->m_info.non_integer_bot_scaling = this->get_max_float_screen_multiplier(bot_screen_resize_data, 0, 0, 0);
		return;
	}
	if(this->m_info.bot_scaling == 0) {
		this->m_info.non_integer_bot_scaling = 0;
		this->m_info.non_integer_top_scaling = this->get_max_float_screen_multiplier(top_screen_resize_data, 0, 0, 0);
		return;
	}
	int more_top_top_scaling = this->m_info.top_scaling;
	int more_top_bot_scaling = this->m_info.bot_scaling;
	int more_bot_top_scaling = this->m_info.top_scaling;
	int more_bot_bot_scaling = this->m_info.bot_scaling;
	top_screen_resize_data->scaling = &more_top_top_scaling;
	bot_screen_resize_data->scaling = &more_top_bot_scaling;
	calc_scaling_resize_screens(top_screen_resize_data, bot_screen_resize_data, true, false, this->m_stype == ScreenType::BOTTOM, false);
	top_screen_resize_data->scaling = &more_bot_top_scaling;
	bot_screen_resize_data->scaling = &more_bot_bot_scaling;
	calc_scaling_resize_screens(bot_screen_resize_data, top_screen_resize_data, true, false, this->m_stype == ScreenType::TOP, false);
	top_screen_resize_data->scaling = &this->m_info.top_scaling;
	bot_screen_resize_data->scaling = &this->m_info.bot_scaling;
	int min_top_scaling = more_bot_top_scaling + 1;
	int min_bot_scaling = more_top_bot_scaling + 1;
	if(min_top_scaling > this->m_info.top_scaling)
		min_top_scaling = this->m_info.top_scaling;
	if(min_bot_scaling > this->m_info.bot_scaling)
		min_bot_scaling = this->m_info.bot_scaling;
	int min_top_screen_width = top_screen_resize_data->size.x;
	int min_top_screen_height = top_screen_resize_data->size.y;
	int min_bot_screen_width = bot_screen_resize_data->size.x;
	int min_bot_screen_height = bot_screen_resize_data->size.y;
	get_par_size(min_top_screen_width, min_top_screen_height, min_top_scaling, top_screen_resize_data->par);
	get_par_size(min_bot_screen_width, min_bot_screen_height, min_bot_scaling, bot_screen_resize_data->par);

	ResizingScreenData *bigger_screen_resize_data = top_screen_resize_data;
	ResizingScreenData *smaller_screen_resize_data = bot_screen_resize_data;
	int min_bigger_screen_width = min_top_screen_width;
	int min_bigger_screen_height = min_top_screen_height;
	int min_smaller_screen_width = min_bot_screen_width;
	int min_smaller_screen_height = min_bot_screen_height;
	bool is_top_bigger = this->m_info.top_scaling >= this->m_info.bot_scaling;
	if(!is_top_bigger) {
		std::swap(bigger_screen_resize_data, smaller_screen_resize_data);
		std::swap(min_bigger_screen_width, min_smaller_screen_width);
		std::swap(min_bigger_screen_height, min_smaller_screen_height);
	}

	if((top_screen_resize_data->rotation / 10) % 2)
		std::swap(min_top_screen_width, min_top_screen_height);
	if((bot_screen_resize_data->rotation / 10) % 2)
		std::swap(min_bot_screen_width, min_bot_screen_height);

	int free_width = curr_desk_mode.size.x - (min_bot_screen_width + min_top_screen_width);
	int free_height = curr_desk_mode.size.y - (min_bot_screen_height + min_top_screen_height);

	switch(this->m_info.non_integer_mode) {
		case SMALLER_PRIORITY:
			this->non_int_scale_screens_with_main(smaller_screen_resize_data, bigger_screen_resize_data, min_bigger_screen_width, min_bigger_screen_height);
			break;
		case BIGGER_PRIORITY:
			this->non_int_scale_screens_with_main(bigger_screen_resize_data, smaller_screen_resize_data, min_smaller_screen_width, min_smaller_screen_height);
			break;
		case EQUAL_PRIORITY:
			this->non_int_scale_screens_both(bigger_screen_resize_data, smaller_screen_resize_data, free_width, free_height, 0.5, min_bigger_screen_width, min_bigger_screen_height, min_smaller_screen_width, min_smaller_screen_height);
			break;
		case PROPORTIONAL_PRIORITY:
			this->non_int_scale_screens_both(bigger_screen_resize_data, smaller_screen_resize_data, free_width, free_height, ((float)(*bigger_screen_resize_data->scaling)) / ((*bigger_screen_resize_data->scaling) + (*smaller_screen_resize_data->scaling)), min_bigger_screen_width, min_bigger_screen_height, min_smaller_screen_width, min_smaller_screen_height);
			break;
		case INVERSE_PROPORTIONAL_PRIORITY:
			this->non_int_scale_screens_both(bigger_screen_resize_data, smaller_screen_resize_data, free_width, free_height, ((float)(*smaller_screen_resize_data->scaling)) / ((*bigger_screen_resize_data->scaling) + (*smaller_screen_resize_data->scaling)), min_bigger_screen_width, min_bigger_screen_height, min_smaller_screen_width, min_smaller_screen_height);
			break;
		default:
			break;
	}
}

void WindowScreen::prepare_size_ratios(bool top_increase, bool bot_increase, bool cycle) {
	if(!this->m_info.is_fullscreen)
		return;

	sf::Vector2f top_screen_size = getShownScreenSize(true, &this->m_info);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, &this->m_info);

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
	if((this->m_info.top_par >= this->possible_pars.size()) || (this->m_info.top_par < 0))
		this->m_info.top_par = 0;
	if((this->m_info.bot_par >= this->possible_pars.size()) || (this->m_info.bot_par < 0))
		this->m_info.bot_par = 0;
	ResizingScreenData top_screen_resize_data = {.size = top_screen_size, .scaling = &this->m_info.top_scaling, .non_int_scaling = &this->m_info.non_integer_top_scaling, .use_non_int_scaling = this->m_info.use_non_integer_scaling_top, .rotation = this->m_info.top_rotation, .par = this->possible_pars[this->m_info.top_par]};
	ResizingScreenData bot_screen_resize_data = {.size = bot_screen_size, .scaling = &this->m_info.bot_scaling, .non_int_scaling = &this->m_info.non_integer_bot_scaling, .use_non_int_scaling = this->m_info.use_non_integer_scaling_bottom, .rotation = this->m_info.bot_rotation, .par = this->possible_pars[this->m_info.bot_par]};
	if(prioritize_top)
		calc_scaling_resize_screens(&top_screen_resize_data, &bot_screen_resize_data, top_increase, try_mantain_ratio, this->m_stype == ScreenType::BOTTOM, cycle);
	else
		calc_scaling_resize_screens(&bot_screen_resize_data, &top_screen_resize_data, bot_increase, try_mantain_ratio, this->m_stype == ScreenType::TOP, cycle);

	this->non_integer_scale_screens(&top_screen_resize_data, &bot_screen_resize_data);
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
	return (curr_desk_mode.size.x - offset_contribute) * this->loaded_info.total_offset_x;
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
	return (curr_desk_mode.size.y - offset_contribute) * this->loaded_info.total_offset_y;
}

void WindowScreen::resize_window_and_out_rects(bool do_work) {
	sf::Vector2f top_screen_size = getShownScreenSize(true, &this->loaded_info);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, &this->loaded_info);
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
		top_scaling = this->loaded_info.non_integer_top_scaling;
		bot_scaling = this->loaded_info.non_integer_bot_scaling;
	}
	if((this->loaded_info.top_par >= this->possible_pars.size()) || (this->loaded_info.top_par < 0))
		this->loaded_info.top_par = 0;
	if((this->loaded_info.bot_par >= this->possible_pars.size()) || (this->loaded_info.bot_par < 0))
		this->loaded_info.bot_par = 0;
	get_par_size(top_width, top_height, top_scaling, this->possible_pars[this->loaded_info.top_par]);
	get_par_size(bot_width, bot_height, bot_scaling, this->possible_pars[this->loaded_info.bot_par]);

	if((!this->loaded_info.is_fullscreen) && this->loaded_info.rounded_corners_fix) {
		offset_y = TOP_ROUNDED_PADDING;
		offset_x = LEFT_ROUNDED_PADDING;
	}

	if(this->loaded_info.is_fullscreen) {
		offset_x = get_fullscreen_offset_x(top_width, top_height, bot_width, bot_height);
		offset_y = get_fullscreen_offset_y(top_width, top_height, bot_width, bot_height);
		max_x = curr_desk_mode.size.x;
		max_y = curr_desk_mode.size.y;
	}
	sf::Vector2f new_top_screen_size = {(float)top_width, (float)top_height};
	sf::Vector2f new_bot_screen_size = {(float)bot_width, (float)bot_height};
	if(do_work) {
		this->m_out_rect_top.out_rect.setSize(new_top_screen_size);
		this->m_out_rect_top.out_rect.setTextureRect(sf::IntRect({0, 0}, {(int)top_screen_size.x, (int)top_screen_size.y}));
		this->m_out_rect_bot.out_rect.setSize(new_bot_screen_size);
		this->m_out_rect_bot.out_rect.setTextureRect(sf::IntRect({0, 0}, {(int)bot_screen_size.x, (int)bot_screen_size.y}));
	}
	this->set_position_screens(new_top_screen_size, new_bot_screen_size, offset_x, offset_y, max_x, max_y, do_work);
	if((!this->loaded_info.is_fullscreen) && this->loaded_info.rounded_corners_fix) {
		this->m_height += BOTTOM_ROUNDED_PADDING;
		this->m_width += RIGHT_ROUNDED_PADDING;
	}
}

void WindowScreen::create_window(bool re_prepare_size, bool reset_text) {
	if(reset_text)
		this->notification->setShowText(false);
	this->m_info.failed_fullscreen = false;
	if(!this->m_info.is_fullscreen) {
		this->m_info.show_mouse = !this->display_data->mono_app_mode;
	}
	else {
		bool success = false;
		if((this->m_info.fullscreen_mode_width > 0) && (this->m_info.fullscreen_mode_height > 0) && (this->m_info.fullscreen_mode_bpp > 0)) {
			sf::VideoMode mode_created = sf::VideoMode({(unsigned int)this->m_info.fullscreen_mode_width, (unsigned int)this->m_info.fullscreen_mode_height}, this->m_info.fullscreen_mode_bpp);
			if(mode_created.isValid()) {
				this->curr_desk_mode = mode_created;
				success = true;
			}
		}
		if(!success) {
			this->curr_desk_mode = sf::VideoMode::getDesktopMode();
			success = this->curr_desk_mode.isValid();
		}
		#ifdef __APPLE__
		if(!success) {
			this->curr_desk_mode = sf::VideoMode({2 * FALLBACK_FS_RESOLUTION_WIDTH, 2 * FALLBACK_FS_RESOLUTION_HEIGHT}, FALLBACK_FS_RESOLUTION_BPP);
			success = this->curr_desk_mode.isValid();
		}
		if(!success) {
			this->curr_desk_mode = sf::VideoMode({FALLBACK_FS_RESOLUTION_WIDTH, FALLBACK_FS_RESOLUTION_HEIGHT}, FALLBACK_FS_RESOLUTION_BPP);
			success = this->curr_desk_mode.isValid();
		}
		#endif
		if(!success) {
			std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
			if(modes.size() > 0) {
				this->curr_desk_mode = modes[0];
				success = this->curr_desk_mode.isValid();
			}
		}
		if(!success) {
			if((this->m_info.fullscreen_mode_width <= 0) || (this->m_info.fullscreen_mode_height <= 0) || (this->m_info.fullscreen_mode_bpp <= 0)) {
				this->m_info.fullscreen_mode_width = FALLBACK_FS_RESOLUTION_WIDTH;
				this->m_info.fullscreen_mode_height = FALLBACK_FS_RESOLUTION_HEIGHT;
				this->m_info.fullscreen_mode_bpp = FALLBACK_FS_RESOLUTION_BPP;
			}
			this->curr_desk_mode = sf::VideoMode({(unsigned int)this->m_info.fullscreen_mode_width, (unsigned int)this->m_info.fullscreen_mode_height}, this->m_info.fullscreen_mode_bpp);
			this->m_info.failed_fullscreen = true;
		}
		this->m_window_width = curr_desk_mode.size.x;
		this->m_window_height = curr_desk_mode.size.y;
		this->m_info.show_mouse = false;
	}
	if(re_prepare_size)
		this->prepare_size_ratios(false, false);
	this->reset_held_times();
	this->future_operations.call_create = true;
}

void WindowScreen::update_view_size() {
	if(!this->loaded_info.is_fullscreen) {
		this->m_window_width = this->m_width;
		this->m_window_height = this->m_height;
	}
	this->m_view = sf::View(sf::FloatRect({0, 0}, {(float)this->m_window_width, (float)this->m_window_height}));
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
	this->m_out_rect_top.out_rect.setRotation(sf::degrees(this->loaded_info.top_rotation));
	this->m_out_rect_bot.out_rect.setRotation(sf::degrees(this->loaded_info.bot_rotation));
	this->loaded_operations.call_screen_settings_update = true;
}

std::vector<const CropData*>* WindowScreen::get_crop_data_vector(ScreenInfo* info) {
	std::vector<const CropData*> *crops = &this->possible_crops;
	if(info->allow_games_crops)
		crops = &this->possible_crops_with_games;
	if(this->display_data->last_connected_ds) {
		crops = &this->possible_crops_ds;
		if(info->allow_games_crops)
			crops = &this->possible_crops_ds_with_games;
	}
	return crops;
}

int* WindowScreen::get_crop_index_ptr(ScreenInfo* info) {
	int *crop_ptr = &info->crop_kind;
	if(this->display_data->last_connected_ds)
		crop_ptr = &info->crop_kind_ds;
	return crop_ptr;
}

sf::Vector2f WindowScreen::getShownScreenSize(bool is_top, ScreenInfo* info) {
	std::vector<const CropData*> *crops = this->get_crop_data_vector(info);
	int *crop_kind = this->get_crop_index_ptr(info);
	if(((*crop_kind) >= crops->size()) || ((*crop_kind) < 0))
		*crop_kind = 0;
	int width = (*crops)[*crop_kind]->top_width;
	int height = (*crops)[*crop_kind]->top_height;
	if(!is_top) {
		width = (*crops)[*crop_kind]->bot_width;
		height = (*crops)[*crop_kind]->bot_height;
	}
	return {(float)width, (float)height};
}

void WindowScreen::crop() {
	std::vector<const CropData*> *crops = this->get_crop_data_vector(&this->loaded_info);
	int *crop_value = this->get_crop_index_ptr(&this->loaded_info);

	sf::Vector2f top_screen_size = getShownScreenSize(true, &this->loaded_info);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, &this->loaded_info);

	this->resize_in_rect(this->m_in_rect_top, this->capture_status->device.top_screen_x + (*crops)[*crop_value]->top_x, this->capture_status->device.top_screen_y + (*crops)[*crop_value]->top_y, top_screen_size.x, top_screen_size.y);
	this->resize_in_rect(this->m_in_rect_bot, this->capture_status->device.bot_screen_x + (*crops)[*crop_value]->bot_x, this->capture_status->device.bot_screen_y + (*crops)[*crop_value]->bot_y, bot_screen_size.x, bot_screen_size.y);
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
		this->draw_lock->lock();
		(void)this->m_win.setActive(true);
		this->main_thread_owns_window = is_main_thread;
		this->m_win.setSize({(unsigned int)width, (unsigned int)height});
		this->draw_lock->unlock();
	}
}

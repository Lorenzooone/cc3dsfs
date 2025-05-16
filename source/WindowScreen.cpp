#include "frontend.hpp"

#define GL_SILENCE_DEPRECATION
#include <SFML/OpenGL.hpp>
#include <cstring>
#include <cmath>
#include "font_ttf.h"
#include "shaders_list.hpp"
#include "devicecapture.hpp"
#include <conversions.hpp>

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

static bool is_size_valid(sf::Vector2f size) {
	return (size.x > 0.0) && (size.y > 0.0);
}

WindowScreen::WindowScreen(ScreenType stype, CaptureStatus* capture_status, DisplayData* display_data, SharedData* shared_data, AudioData* audio_data, ConsumerMutex *draw_lock, bool created_proper_folder, bool disable_frame_blending) {
	this->draw_lock = draw_lock;
	this->m_stype = stype;
	insert_basic_crops(this->possible_crops, this->m_stype, false, false);
	insert_basic_crops(this->possible_crops_ds, this->m_stype, true, false);
	insert_basic_crops(this->possible_crops_with_games, this->m_stype, false, true);
	insert_basic_crops(this->possible_crops_ds_with_games, this->m_stype, true, true);
	insert_basic_pars(this->possible_pars);
	insert_basic_color_profiles(this->possible_color_profiles);
	this->sent_shader_color_data = NULL;
	this->m_prepare_save = 0;
	this->m_prepare_load = 0;
	this->m_prepare_open = false;
	this->m_prepare_quit = false;
	this->m_scheduled_split = false;
	this->ret_val = 0;
	reset_screen_info(this->m_info);
	bool font_load_success = this->text_font.openFromMemory(font_ttf, font_ttf_len);
	this->notification = new TextRectangle(font_load_success, this->text_font);
	this->text_rectangle_pool = new TextRectanglePool(font_load_success, &this->text_font);
	this->init_menus();
	FPSArrayInit(&this->in_fps);
	FPSArrayInit(&this->draw_fps);
	FPSArrayInit(&this->poll_fps);
	this->last_update_texture_data_type = VIDEO_DATA_RGB;
	this->texture_software_based_conv = NO_SOFTWARE_CONV;
	this->num_frames_to_blend = NUM_FRAMES_BLENDED;
	if(disable_frame_blending)
		this->num_frames_to_blend = 1;
	const size_t top_width = MAX_IN_VIDEO_WIDTH_TOP * this->num_frames_to_blend;
	const size_t top_height = MAX_IN_VIDEO_HEIGHT_TOP;
	const size_t top_single_width = MAX_IN_VIDEO_WIDTH_SINGLE_TOP * this->num_frames_to_blend;
	const size_t top_single_height = MAX_IN_VIDEO_HEIGHT_SINGLE_TOP;
	const size_t bottom_width = MAX_IN_VIDEO_WIDTH_BOTTOM * this->num_frames_to_blend;
	const size_t bottom_height = MAX_IN_VIDEO_HEIGHT_BOTTOM;
	size_t full_width = MAX_IN_VIDEO_WIDTH * this->num_frames_to_blend;
	size_t full_height = MAX_IN_VIDEO_HEIGHT;
	if(this->m_stype == ScreenType::TOP) {
		full_width = top_width;
		full_height = top_height;
	}
	if(this->m_stype == ScreenType::BOTTOM) {
		full_width = bottom_width;
		full_height = bottom_height;
	}
	this->shared_texture_available = this->full_in_tex.resize({(unsigned int)full_width, (unsigned int)full_height});
	if(this->shared_texture_available) {
		this->m_in_rect_top.setTexture(&this->full_in_tex);
		this->m_in_rect_top_right.setTexture(&this->full_in_tex);
		this->m_in_rect_bot.setTexture(&this->full_in_tex);
	}
	else {
		if((this->m_stype == ScreenType::TOP) || (this->m_stype == ScreenType::JOINT)) {
			full_width = top_width;
			full_height = top_height;
			(void)this->top_l_in_tex.resize({ (unsigned int)top_single_width, (unsigned int)top_single_height });
			(void)this->top_r_in_tex.resize({ (unsigned int)top_single_width, (unsigned int)top_single_height });
			this->m_in_rect_top.setTexture(&this->top_l_in_tex);
			this->m_in_rect_top_right.setTexture(&this->top_r_in_tex);
		}
		else {
			this->m_in_rect_top.setTexture(&this->bot_in_tex);
			this->m_in_rect_top_right.setTexture(&this->bot_in_tex);
		}
		if((this->m_stype == ScreenType::BOTTOM) || (this->m_stype == ScreenType::JOINT)) {
			(void)this->bot_in_tex.resize({ (unsigned int)bottom_width, (unsigned int)bottom_height });
			this->m_in_rect_bot.setTexture(&this->bot_in_tex);
		}
		else
			this->m_in_rect_bot.setTexture(&this->top_l_in_tex);
	}
	this->display_data = display_data;
	this->shared_data = shared_data;
	this->audio_data = audio_data;
	this->last_window_creation_time = std::chrono::high_resolution_clock::now();
	this->last_mouse_action_time = std::chrono::high_resolution_clock::now();
	this->last_touch_left_time = std::chrono::high_resolution_clock::now();
	this->last_draw_time = std::chrono::high_resolution_clock::now();
	this->last_poll_time = std::chrono::high_resolution_clock::now();
	this->touch_right_click_action.started = false;
	this->reset_held_times();
	WindowScreen::reset_operations(future_operations);
	this->done_display = true;
	this->saved_buf = new VideoOutputData;
	this->win_title = NAME;
	if(this->m_stype == ScreenType::TOP) {
		this->win_title += "_top";
		this->own_out_text_data.preamble_name += " top window";
	}
	if(this->m_stype == ScreenType::BOTTOM) {
		this->win_title += "_bot";
		this->own_out_text_data.preamble_name += " bottom window";
	}
	if(this->m_stype == ScreenType::JOINT)
		this->own_out_text_data.preamble_name += " joint window";
	this->last_connected_status = false;
	this->last_enabled_3d = false;
	this->last_interleaved_3d = false;
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
	this->main_thread_owns_window = true;
	this->created_proper_folder = created_proper_folder;
	this->is_window_windowed = false;
	this->saved_windowed_pos = sf::Vector2i(0, 0);
}

WindowScreen::~WindowScreen() {
	this->possible_files.clear();
	this->possible_resolutions.clear();
	delete this->saved_buf;
	delete this->notification;
	this->destroy_menus();
	delete this->text_rectangle_pool;
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
	sf::Vector2f top_screen_size = {(float)TOP_WIDTH_3DS * 2, (float)HEIGHT_3DS};
	sf::Vector2f bot_screen_size = {(float)BOT_WIDTH_3DS, (float)HEIGHT_3DS};

	(void)this->m_out_rect_top.out_tex.resize({(unsigned int)top_screen_size.x, (unsigned int)top_screen_size.y});
	(void)this->m_out_rect_top.backup_tex.resize({(unsigned int)top_screen_size.x, (unsigned int)top_screen_size.y});
	this->m_out_rect_top.out_rect.setSize(top_screen_size);
	this->m_out_rect_top.out_rect.setTexture(&this->m_out_rect_top.out_tex.getTexture());

	(void)this->m_out_rect_top_right.out_tex.resize({(unsigned int)top_screen_size.x / 2, (unsigned int)top_screen_size.y});
	(void)this->m_out_rect_top_right.backup_tex.resize({(unsigned int)top_screen_size.x / 2, (unsigned int)top_screen_size.y});
	this->m_out_rect_top_right.out_rect.setSize(top_screen_size);
	this->m_out_rect_top_right.out_rect.setTexture(&this->m_out_rect_top_right.out_tex.getTexture());

	(void)this->m_out_rect_bot.out_tex.resize({(unsigned int)bot_screen_size.x, (unsigned int)bot_screen_size.y});
	(void)this->m_out_rect_bot.backup_tex.resize({(unsigned int)bot_screen_size.x, (unsigned int)bot_screen_size.y});
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

void WindowScreen::draw(double frame_time, VideoOutputData* out_buf, InputVideoDataType video_data_type) {
	FPSArrayInsertElement(&in_fps, frame_time);
	if(!this->done_display)
		return;

	bool should_be_open = this->m_info.window_enabled;
	if(this->m_win.isOpen() ^ should_be_open) {
		if(this->m_win.isOpen())
			this->close();
		else
			this->open();
	}
	if(!should_be_open)
		this->m_info.is_fullscreen = false;
	bool curr_enabled_3d = get_3d_enabled(this->capture_status);
	if(this->last_enabled_3d != curr_enabled_3d) {
		this->prepare_size_ratios(false, false);
		this->future_operations.call_crop = true;
	}
	this->last_enabled_3d = curr_enabled_3d;
	if(curr_enabled_3d && (this->last_interleaved_3d != this->display_data->interleaved_3d)) {
		this->prepare_size_ratios(false, false);
		this->future_operations.call_crop = true;
	}
	this->last_interleaved_3d = this->display_data->interleaved_3d;
	this->loaded_menu = this->curr_menu;
	this->loaded_menu_ptr = this->curr_menu_ptr;
	loaded_operations = future_operations;
	if(this->loaded_operations.call_create) {
		this->last_draw_time = std::chrono::high_resolution_clock::now();
		this->last_poll_time = std::chrono::high_resolution_clock::now();
	}
	if(this->m_win.isOpen() || this->loaded_operations.call_create) {
		this->curr_frame_texture_pos = (this->curr_frame_texture_pos + 1) % this->num_frames_to_blend;
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
		this->curr_video_data_type = video_data_type;
		loaded_info = m_info;
		m_info.initial_pos_x = DEFAULT_NO_POS_WINDOW_VALUE;
		m_info.initial_pos_y = DEFAULT_NO_POS_WINDOW_VALUE;
		this->notification->setTextFactor((float)this->loaded_info.menu_scaling_factor);
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

static bool is_vertically_rotated(int rotation) {
	return (rotation / 10) % 2;
}

static bool is_negatively_rotated(int rotation) {
	return ((rotation / 10) % 4) >= 2;
}

void WindowScreen::resize_in_rect(sf::RectangleShape &in_rect, int start_x, int start_y, int width, int height) {
	int target_x = start_x;
	int target_y = start_y;
	int target_width = width;
	int target_height = height;
	int target_rotation = -1 * this->capture_status->device.base_rotation;

	if(is_vertically_rotated(this->capture_status->device.base_rotation)) {
		std::swap(target_x, target_y);
		std::swap(target_width, target_height);
	}

	in_rect.setTextureRect(sf::IntRect({target_x, target_y}, {target_width, target_height}));

	in_rect.setSize({(float)target_width, (float)target_height});
	in_rect.setOrigin({target_width / 2.0f, target_height / 2.0f});

	in_rect.setRotation(sf::degrees((float)target_rotation));
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
		this->m_out_rect_top.backup_tex.setSmooth(this->loaded_info.is_blurred);
		this->m_out_rect_top_right.out_tex.setSmooth(this->loaded_info.is_blurred);
		this->m_out_rect_top_right.backup_tex.setSmooth(this->loaded_info.is_blurred);
		this->m_out_rect_bot.out_tex.setSmooth(this->loaded_info.is_blurred);
		this->m_out_rect_bot.backup_tex.setSmooth(this->loaded_info.is_blurred);
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
		// Was this called while the window is in Windowed mode?
		// Regardless of how it will change...
		if(previously_open && this->is_window_windowed) {
			prev_pos = this->m_win.getPosition();
			saved_windowed_pos = prev_pos;
		}
		if((this->loaded_info.initial_pos_x != DEFAULT_NO_POS_WINDOW_VALUE) && (this->loaded_info.initial_pos_y != DEFAULT_NO_POS_WINDOW_VALUE))
			prev_pos = sf::Vector2i(this->loaded_info.initial_pos_x, this->loaded_info.initial_pos_y);
		this->is_window_windowed = true;
		if(!this->loaded_info.is_fullscreen) {
			this->update_screen_settings();
			if(this->loaded_info.have_titlebar)
				this->m_win.create(sf::VideoMode({(unsigned int)this->m_width, (unsigned int)this->m_height}), this->title_factory());
			else
				this->m_win.create(sf::VideoMode({(unsigned int)this->m_width, (unsigned int)this->m_height}), this->title_factory(), sf::Style::None);
			this->update_view_size();
		}
		else {
			if(!this->loaded_info.failed_fullscreen) {
				this->m_win.create(this->curr_desk_mode, this->title_factory(), sf::Style::Default, sf::State::Fullscreen);
				this->is_window_windowed = false;
			}
			else if(this->loaded_info.have_titlebar)
				this->m_win.create(this->curr_desk_mode, this->title_factory());
			else
				this->m_win.create(this->curr_desk_mode, this->title_factory(), sf::Style::None);
		}
		if(previously_open && (!this->loaded_info.is_fullscreen))
			this->m_win.setPosition(saved_windowed_pos);
		if((this->loaded_info.initial_pos_x != DEFAULT_NO_POS_WINDOW_VALUE) && (this->loaded_info.initial_pos_y != DEFAULT_NO_POS_WINDOW_VALUE) && (!this->loaded_info.is_fullscreen))
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

std::string WindowScreen::_title_factory() {
	std::string title = this->win_title;
	if(this->capture_status->connected)
		title += " - " + get_name_of_device(this->capture_status);
	return title;
 }
 
sf::String WindowScreen::title_factory() {
	std::string title = this->_title_factory();
	return sf::String::fromUtf8(title.begin(), title.end());
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

void WindowScreen::opengl_error_out(std::string error_base, std::string error_str) {
	UpdateOutText(this->own_out_text_data, error_base + ": " + error_str, error_base + ": " + error_str, TEXT_KIND_ERROR);
}

void WindowScreen::opengl_error_check(std::string error_base) {
	GLenum glCheckInternalError = glGetError();
	while (glCheckInternalError != GL_NO_ERROR) {
		// Do error processing here... Simply logging it for now
		this->opengl_error_out(error_base, std::to_string(glCheckInternalError));
		glCheckInternalError = glGetError();
	}
}

bool WindowScreen::single_update_texture(unsigned int m_texture, InputVideoDataType video_data_type, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height, bool manually_converted) {
	if(!(this->saved_buf && m_texture))
		return false;

	this->opengl_error_check("Previous OpenGL Error");
	// Copy pixels from the given array to the texture
	glBindTexture(GL_TEXTURE_2D, m_texture);
	GLenum format = GL_RGB;
	GLenum type = GL_UNSIGNED_BYTE;
	size_t format_size = sizeof(VideoPixelRGB);

	if(manually_converted && (this->texture_software_based_conv == TO_RGBA_SOFTWARE_CONV))
		format = GL_RGBA;

	if(!manually_converted) {
		if(video_data_type == VIDEO_DATA_BGR) {
			format = GL_BGR;
			format_size = sizeof(VideoPixelBGR);
		}
		if(video_data_type == VIDEO_DATA_RGB16) {
			type = GL_UNSIGNED_SHORT_5_6_5;
			format_size = sizeof(VideoPixelRGB16);
		}
		if(video_data_type == VIDEO_DATA_BGR16) {
			type = GL_UNSIGNED_SHORT_5_6_5_REV;
			format_size = sizeof(VideoPixelBGR16);
		}
	}

	this->opengl_error_check("BindTexture OpenGL Error");
	glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(this->curr_frame_texture_pos * MAX_IN_VIDEO_WIDTH), static_cast<GLint>(0), static_cast<GLsizei>(width), static_cast<GLsizei>(height), format, type, ((uint8_t*)this->saved_buf) + (pos_y_data * width * format_size));
	GLenum glCheckInternalError = glGetError();
	while (glCheckInternalError != GL_NO_ERROR) {
		bool processed = false;
		if(glCheckInternalError == GL_INVALID_ENUM) {
			if((format != GL_RGB) || (type != GL_UNSIGNED_BYTE)) {
				UpdateOutText(this->own_out_text_data, "Switching to software-based texture updating", "", TEXT_KIND_NORMAL);
				this->last_update_texture_data_type = video_data_type;
				this->texture_software_based_conv = TO_RGB_SOFTWARE_CONV;
				return true;
			}
		}
		if(glCheckInternalError == GL_INVALID_OPERATION) {
			if((format != GL_RGBA) || (type != GL_UNSIGNED_BYTE)) {
				UpdateOutText(this->own_out_text_data, "Switching to software-based texture updating", "", TEXT_KIND_NORMAL);
				this->last_update_texture_data_type = video_data_type;
				this->texture_software_based_conv = TO_RGBA_SOFTWARE_CONV;
				return true;
			}
		}
		if(!processed) {
			// Do error processing here... Simply logging it for now
			this->opengl_error_out("SubImage2D OpenGL Error", std::to_string(glCheckInternalError));
			processed = true;
		}
		glCheckInternalError = glGetError();
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	this->opengl_error_check("TexParameteri OpenGL Error");

	// Force an OpenGL flush, so that the texture data will appear updated
	// in all contexts immediately (solves problems in multi-threaded apps)
	glFlush();
	return false;
}

void WindowScreen::execute_single_update_texture(bool &manually_converted, bool do_full, bool is_top, bool is_second) {
	InputVideoDataType video_data_type = this->curr_video_data_type;
	size_t top_width = TOP_WIDTH_3DS;
	size_t top_height = HEIGHT_3DS;
	size_t single_top_width = TOP_WIDTH_3DS;
	size_t single_top_height = HEIGHT_3DS;
	size_t bot_width = BOT_WIDTH_3DS;
	size_t bot_height = HEIGHT_3DS;
	if(!this->capture_status->device.is_3ds) {
		top_width = WIDTH_DS;
		top_height = HEIGHT_DS;
		single_top_width = WIDTH_DS;
		single_top_height = HEIGHT_DS;
		bot_width = WIDTH_DS;
		bot_height = HEIGHT_DS;
	}
	if(get_3d_enabled(this->capture_status))
		top_width *= 2;

	if(is_vertically_rotated(this->capture_status->device.base_rotation)) {
		std::swap(top_width, top_height);
		std::swap(single_top_width, single_top_height);
		std::swap(bot_width, bot_height);
	}
	size_t full_width = std::max(top_width, bot_width);
	size_t full_height = top_height + bot_height;
	size_t width = full_width;
	size_t height = full_height;
	size_t pos_x_data = 0;
	size_t pos_y_data = 0;
	if(this->m_stype == ScreenType::TOP) {
		full_height = top_height;
		height = full_height;
		pos_x_data = this->get_pos_x_screen_inside_data(true);
		pos_y_data = this->get_pos_y_screen_inside_data(true);
	}
	if(this->m_stype == ScreenType::BOTTOM) {
		full_height = bot_height;
		height = full_height;
		pos_x_data = this->get_pos_x_screen_inside_data(false);
		pos_y_data = this->get_pos_y_screen_inside_data(false);
	}
	size_t pos_x_conv = pos_x_data;
	size_t pos_y_conv = pos_y_data;

	sf::Texture* target_texture = &this->full_in_tex;
	if(!do_full) {
		if(is_top) {
			height = single_top_height;
			pos_x_data = this->get_pos_x_screen_inside_data(true, is_second);
			pos_y_data = this->get_pos_y_screen_inside_data(true, is_second);
			sf::Texture* top_l_texture = &this->top_l_in_tex;
			sf::Texture* top_r_texture = &this->top_r_in_tex;
			if(get_3d_enabled(this->capture_status) && (!this->display_data->interleaved_3d)) {
				if(!this->capture_status->device.is_second_top_screen_right)
					std::swap(top_l_texture, top_r_texture);
			}
			target_texture = top_l_texture;
			if(is_second)
				target_texture = top_r_texture;
		}
		else {
			height = bot_height;
			pos_x_data = this->get_pos_x_screen_inside_data(false);
			pos_y_data = this->get_pos_y_screen_inside_data(false);
			target_texture = &this->bot_in_tex;
		}
	}

	if(is_vertically_rotated(this->capture_status->device.base_rotation))
		std::swap(pos_x_data, pos_y_data);

	unsigned int m_texture = target_texture->getNativeHandle();
	bool retry = true;
	while(retry) {
		bool software_based_conv = manually_converted || ((this->texture_software_based_conv != NO_SOFTWARE_CONV) && (video_data_type == this->last_update_texture_data_type));

		if(software_based_conv) {
			if(!manually_converted) {
				if(this->texture_software_based_conv == TO_RGB_SOFTWARE_CONV)
					manualConvertOutputToRGB(this->saved_buf, this->saved_buf, pos_x_conv, pos_y_conv, full_width, full_height, video_data_type);
				if(this->texture_software_based_conv == TO_RGBA_SOFTWARE_CONV)
					manualConvertOutputToRGBA(this->saved_buf, this->saved_buf, pos_x_conv, pos_y_conv, full_width, full_height, video_data_type);
			}
			manually_converted = true;
		}
		else {
			this->texture_software_based_conv = NO_SOFTWARE_CONV;
			this->last_update_texture_data_type = video_data_type;
		}

		retry = this->single_update_texture(m_texture, video_data_type, pos_x_data, pos_y_data, width, height, manually_converted);
	}
}

void WindowScreen::update_texture() {
	bool manually_converted = false;
	if(this->shared_texture_available)
		this->execute_single_update_texture(manually_converted, true);
	else {
		if((this->m_stype == ScreenType::TOP) || (this->m_stype == ScreenType::JOINT)) {
			this->execute_single_update_texture(manually_converted, false, true);
			if(get_3d_enabled(this->capture_status))
				this->execute_single_update_texture(manually_converted, false, true, true);
		}
		if((this->m_stype == ScreenType::BOTTOM) || (this->m_stype == ScreenType::JOINT))
			this->execute_single_update_texture(manually_converted, false, false);
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

static void transpose_matrix_to_new_one(float target[3][3], const float source[3][3]) {
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			target[i][j] = source[j][i];
}

int WindowScreen::_choose_color_emulation_shader(bool is_top) {
	int color_profile_index = this->loaded_info.top_color_correction;
	if(!is_top)
		color_profile_index = this->loaded_info.bot_color_correction;
	if((color_profile_index >= ((int)possible_color_profiles.size())) || (color_profile_index < 0))
		color_profile_index = 0;

	const ShaderColorEmulationData* shader_color_data = possible_color_profiles[color_profile_index];

	if(!shader_color_data->is_valid)
		return -1;

	int shader_index = COLOR_EMULATION_FRAGMENT_SHADER;
	if(sent_shader_color_data == shader_color_data)
		return shader_index;

	const float contrast = 1.0;
	const float brightness = 0.0;
	const float saturation = 1.0;
	// Calculate saturation weights.
	// Note: We are using the Rec. 709 luminance vector here.
	// From Open AGB Firm, thanks to profi200
	const float rwgt  = (1.f - saturation) * 0.2126f;
	const float gwgt  = (1.f - saturation) * 0.7152f;
	const float bwgt  = (1.f - saturation) * 0.0722f;
	float saturation_matrix[3][3] = {
		{rwgt + saturation, gwgt, bwgt},
		{rwgt, gwgt + saturation, bwgt},
		{rwgt, gwgt, bwgt + saturation}
	};
	// OpenGL is column major... Do the transpose of the matrixes
	float saturation_matrix_transposed[3][3];
	float color_corr_matrix_transposed[3][3];
	transpose_matrix_to_new_one(color_corr_matrix_transposed, shader_color_data->rgb_mod);
	transpose_matrix_to_new_one(saturation_matrix_transposed, saturation_matrix);

	usable_shaders[shader_index].shader.setUniform("targetContrast", (float)(pow(contrast, shader_color_data->targetGamma)));
	usable_shaders[shader_index].shader.setUniform("targetBrightness", brightness / contrast);
	usable_shaders[shader_index].shader.setUniform("targetLuminance", shader_color_data->lum);
	usable_shaders[shader_index].shader.setUniform("targetGamma", shader_color_data->targetGamma);
	usable_shaders[shader_index].shader.setUniform("displayGamma", shader_color_data->displayGamma);
	usable_shaders[shader_index].shader.setUniform("color_corr_mat", sf::Glsl::Mat3((float*)color_corr_matrix_transposed));
	usable_shaders[shader_index].shader.setUniform("saturation_mat", sf::Glsl::Mat3((float*)saturation_matrix_transposed));

	sent_shader_color_data = shader_color_data;
	return shader_index;
}

int WindowScreen::_choose_base_input_shader(bool is_top) {
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

int WindowScreen::_choose_shader(PossibleShaderTypes shader_type, bool is_top) {
	if(this->was_last_frame_null) {
		this->was_last_frame_null = false;
		return NO_EFFECT_FRAGMENT_SHADER;
	}
	switch(shader_type) {
		case BASE_FINAL_OUTPUT_SHADER_TYPE:
			return NO_EFFECT_FRAGMENT_SHADER;
		case BASE_INPUT_SHADER_TYPE:
			return _choose_base_input_shader(is_top);
		case COLOR_PROCESSING_SHADER_TYPE:
			return _choose_color_emulation_shader(is_top);
		default:
			return NO_EFFECT_FRAGMENT_SHADER;
	}
}

int WindowScreen::choose_shader(PossibleShaderTypes shader_type, bool is_top) {
	int chosen_shader = _choose_shader(shader_type, is_top);
	if((chosen_shader >= 0) && (chosen_shader < ((int)usable_shaders.size())) && usable_shaders[chosen_shader].is_valid)
		return (int)chosen_shader;
	return -1;
}

void WindowScreen::apply_shader_to_texture(sf::RectangleShape &rect_data, sf::RenderTexture* &to_process_tex_data, sf::RenderTexture* &backup_tex_data, PossibleShaderTypes shader_type, bool is_top) {
	int chosen_shader = choose_shader(shader_type, is_top);
	if(chosen_shader < 0)
		return;
	to_process_tex_data->display();
	sf::RectangleShape in_rect;
	in_rect.setTexture(&to_process_tex_data->getTexture());
	const sf::IntRect texture_rect = rect_data.getTextureRect();
	in_rect.setTextureRect(texture_rect);
	in_rect.setSize({(float)texture_rect.size.x, (float)texture_rect.size.y});
	in_rect.setPosition({(float)texture_rect.position.x, (float)texture_rect.position.y});
	in_rect.setRotation(sf::degrees(0));
	in_rect.setOrigin({0, 0});
	backup_tex_data->draw(in_rect, &usable_shaders[chosen_shader].shader);
	std::swap(to_process_tex_data, backup_tex_data);
	rect_data.setTexture(&to_process_tex_data->getTexture());
}

bool WindowScreen::apply_shaders_to_input(sf::RectangleShape &rect_data, sf::RenderTexture* &to_process_tex_data, sf::RenderTexture* &backup_tex_data, const sf::RectangleShape &final_in_rect, bool is_top) {
	if(!sf::Shader::isAvailable())
		return false;

	int chosen_shader = choose_shader(BASE_INPUT_SHADER_TYPE, is_top);
	if(chosen_shader < 0)
		return false;

	float old_frame_pos_x = ((float)(this->num_frames_to_blend - 1)) / this->num_frames_to_blend;
	if(this->curr_frame_texture_pos > 0)
		old_frame_pos_x = -1.0f / this->num_frames_to_blend;
	sf::Glsl::Vec2 old_pos = {old_frame_pos_x, 0.0};
	usable_shaders[chosen_shader].shader.setUniform("old_frame_offset", old_pos);
	to_process_tex_data->draw(final_in_rect, &usable_shaders[chosen_shader].shader);

	this->apply_shader_to_texture(rect_data, to_process_tex_data, backup_tex_data, COLOR_PROCESSING_SHADER_TYPE, is_top);
	return true;
}

void WindowScreen::post_texture_conversion_processing(sf::RectangleShape &rect_data, sf::RenderTexture* &to_process_tex_data, sf::RenderTexture* &backup_tex_data, const sf::RectangleShape &in_rect, bool actually_draw, bool is_top, bool is_debug) {
	if((is_top && this->m_stype == ScreenType::BOTTOM) || ((!is_top) && this->m_stype == ScreenType::TOP))
		return;
	if(this->loaded_menu == CONNECT_MENU_TYPE)
		return;

	rect_data.setTexture(&to_process_tex_data->getTexture());
	if(is_debug) {
		if(is_top)
			to_process_tex_data->clear(sf::Color::Red);
		else
			to_process_tex_data->clear(sf::Color::Blue);
	}
	else {
		to_process_tex_data->clear();
		sf::RectangleShape final_in_rect = in_rect;
		sf::IntRect text_coords_rect = final_in_rect.getTextureRect();
		text_coords_rect.position.x += this->curr_frame_texture_pos * MAX_IN_VIDEO_WIDTH;
		final_in_rect.setTextureRect(text_coords_rect);
		if(this->capture_status->connected && actually_draw) {
			bool use_default_shader = !(this->apply_shaders_to_input(rect_data, to_process_tex_data, backup_tex_data, final_in_rect, is_top));
			if(use_default_shader)
				to_process_tex_data->draw(final_in_rect);
			//Place postprocessing effects here
		}
	}
	to_process_tex_data->display();
}

void WindowScreen::draw_rect_to_window(const sf::RectangleShape &out_rect, bool is_top) {
	if((is_top && this->m_stype == ScreenType::BOTTOM) || ((!is_top) && this->m_stype == ScreenType::TOP))
		return;
	if(this->loaded_menu == CONNECT_MENU_TYPE)
		return;

	bool use_default_shader = true;
	if(sf::Shader::isAvailable()) {
		int chosen_shader = choose_shader(BASE_FINAL_OUTPUT_SHADER_TYPE, is_top);
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

sf::Vector2u WindowScreen::get_3d_size_multiplier(ScreenInfo* info) {
	if((!get_3d_enabled(this->capture_status)) || (this->display_data->interleaved_3d))
		return {1, 1};
	SecondScreen3DRelativePosition second_screen_pos = get_second_screen_pos(info, this->m_stype);
	if((second_screen_pos == UNDER_FIRST) || (second_screen_pos == ABOVE_FIRST))
		return {1, 2};
	return {2, 1};
}

sf::Vector2u WindowScreen::get_desk_mode_3d_multiplied(ScreenInfo* info) {
	if((!get_3d_enabled(this->capture_status)) || (this->display_data->interleaved_3d) || (this->m_stype == ScreenType::BOTTOM) || (info->top_scaling == 0))
		return curr_desk_mode.size;
	return curr_desk_mode.size.componentWiseDiv(get_3d_size_multiplier(info));
}

sf::Vector2f WindowScreen::get_3d_offset_out_rect(ScreenInfo* info, bool is_second_screen) {
	if((!get_3d_enabled(this->capture_status)) || (this->display_data->interleaved_3d))
		return {0, 0};
	float x_contribution = (float)this->m_width_no_manip;
	float y_contribution = (float)this->m_height_no_manip;
	if(info->is_fullscreen) {
		sf::Vector2u curr_desk_mode_3d_size = get_desk_mode_3d_multiplied(info);
		x_contribution = (float)curr_desk_mode_3d_size.x;
		y_contribution = (float)curr_desk_mode_3d_size.y;
	}
	float offset_x_first_screen = 0;
	float offset_y_first_screen = 0;
	if((!info->is_fullscreen) && info->rounded_corners_fix) {
		x_contribution -= LEFT_ROUNDED_PADDING;
		y_contribution -= TOP_ROUNDED_PADDING;
	}
	SecondScreen3DRelativePosition second_screen_pos = get_second_screen_pos(info, this->m_stype);
	if(second_screen_pos == ABOVE_FIRST)
		offset_y_first_screen = y_contribution;
	if(second_screen_pos == LEFT_FIRST)
		offset_x_first_screen = x_contribution;
	if(!is_second_screen)
		return {offset_x_first_screen, offset_y_first_screen};
	if((second_screen_pos == UNDER_FIRST) || (second_screen_pos == ABOVE_FIRST))
		return {0, (-2 * offset_y_first_screen) + y_contribution};
	return {(-2 * offset_x_first_screen) + x_contribution, 0};
}

void WindowScreen::display_data_to_window(bool actually_draw, bool is_debug) {
	this->draw_lock->lock();
	sf::RectangleShape out_rect_top = this->m_out_rect_top.out_rect;
	sf::RectangleShape out_rect_top_right = this->m_out_rect_top_right.out_rect;
	sf::RectangleShape out_rect_bot = this->m_out_rect_bot.out_rect;
	sf::RectangleShape in_rect_top = this->m_in_rect_top;
	sf::RectangleShape in_rect_top_right = this->m_in_rect_top_right;
	sf::RectangleShape in_rect_bot = this->m_in_rect_bot;
	this->m_out_rect_top.to_process_tex = &this->m_out_rect_top.out_tex;
	this->m_out_rect_top.to_backup_tex = &this->m_out_rect_top.backup_tex;
	this->post_texture_conversion_processing(out_rect_top, this->m_out_rect_top.to_process_tex, this->m_out_rect_top.to_backup_tex, in_rect_top, actually_draw, true, is_debug);
	this->m_out_rect_bot.to_process_tex = &this->m_out_rect_bot.out_tex;
	this->m_out_rect_bot.to_backup_tex = &this->m_out_rect_bot.backup_tex;
	this->post_texture_conversion_processing(out_rect_bot, this->m_out_rect_bot.to_process_tex, this->m_out_rect_bot.to_backup_tex, in_rect_bot, actually_draw, false, is_debug);
	bool has_to_do_top_right_screen = get_3d_enabled(this->capture_status) && ((!this->display_data->interleaved_3d) || (!this->shared_texture_available)) && (this->m_stype != ScreenType::BOTTOM) && is_size_valid(out_rect_top.getSize());
	if(has_to_do_top_right_screen) {
		out_rect_top_right.setTextureRect(out_rect_top.getTextureRect());
		this->m_out_rect_top_right.to_process_tex = &this->m_out_rect_top_right.out_tex;
		this->m_out_rect_top_right.to_backup_tex = &this->m_out_rect_top_right.backup_tex;
		this->post_texture_conversion_processing(out_rect_top_right, this->m_out_rect_top_right.to_process_tex, this->m_out_rect_top_right.to_backup_tex, in_rect_top_right, actually_draw, true, is_debug);
	}

	if(is_debug)
		this->m_win.clear(sf::Color::Green);
	else
		this->m_win.clear();
	this->window_bg_processing();
	bool needs_to_glue_textures_3d = has_to_do_top_right_screen && this->display_data->interleaved_3d && (!this->shared_texture_available);
	if(needs_to_glue_textures_3d) {
		float x_divisor = 2.0f;
		float y_divisor = 1.0f;
		out_rect_top.setSize({out_rect_top.getSize().x / x_divisor, out_rect_top.getSize().y / y_divisor});
		sf::IntRect texture_rect_top = out_rect_top.getTextureRect();
		texture_rect_top.size.x /= (int)x_divisor;
		texture_rect_top.size.y /= (int)y_divisor;
		out_rect_top.setTextureRect(texture_rect_top);
	}
	this->draw_rect_to_window(out_rect_top, true);
	this->draw_rect_to_window(out_rect_bot, false);
	if(has_to_do_top_right_screen) {
		sf::Vector2f offset_second_screen = get_3d_offset_out_rect(&this->loaded_info, true);
		if(needs_to_glue_textures_3d) {
			float base_multiplier = 1.0f;
			if(is_negatively_rotated(this->loaded_info.top_rotation))
				base_multiplier = -1.0f;
			if(is_vertically_rotated(this->loaded_info.top_rotation))
				offset_second_screen = {0, base_multiplier * out_rect_top.getSize().x / 1.0f};
			else
				offset_second_screen = {base_multiplier * out_rect_top.getSize().x / 1.0f, 0};
		}
		out_rect_top_right.setTextureRect(out_rect_top.getTextureRect());
		out_rect_top_right.setSize(out_rect_top.getSize());
		out_rect_top_right.setPosition(out_rect_top.getPosition() + offset_second_screen);
		out_rect_top_right.setRotation(out_rect_top.getRotation());
		this->draw_rect_to_window(out_rect_top_right, true);
		if(!needs_to_glue_textures_3d) {
			out_rect_bot.setPosition(out_rect_bot.getPosition() + offset_second_screen);
			this->draw_rect_to_window(out_rect_bot, false);
		}
	}
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
				sf::sleep(sf::seconds((float)(frame_time / (this->loaded_info.bfi_divider * 1.1))));
			}
			for(int i = 0; i < this->loaded_info.bfi_amount; i++) {
				sf::sleep(sf::seconds((float)(frame_time / (this->loaded_info.bfi_divider * 1.1))));
				this->display_data_to_window(false);
			}
		}
	}
}

static bool should_have_separator(sf::Vector2f top_screen_size, sf::Vector2f bot_screen_size, float top_scaling, float bot_scaling, ScreenType stype) {
	if(stype != ScreenType::JOINT)
		return false;
	if((!is_size_valid(top_screen_size)) || (!is_size_valid(bot_screen_size)))
		return false;
	if((top_scaling <= 0) || (bot_scaling <= 0))
		return false;
	return true;
}

static int get_separator_size_xy(float top_scaling, float bot_scaling, int separator_size, float separator_multiplier, bool is_fullscreen) {
	if((!is_fullscreen) && (separator_multiplier == SEP_FOLLOW_SCALING_MULTIPLIER))
		return (int)(separator_size * top_scaling);
	return (int)(separator_size * separator_multiplier);
}

static int get_separator_size_x(sf::Vector2f top_screen_size, sf::Vector2f bot_screen_size, float top_scaling, float bot_scaling, ScreenType stype, BottomRelativePosition bottom_pos, int separator_size, float separator_multiplier, bool is_fullscreen) {
	if(!should_have_separator(top_screen_size, bot_screen_size, top_scaling, bot_scaling, stype))
		return 0;
	if((bottom_pos != RIGHT_TOP) && (bottom_pos != LEFT_TOP))
		return 0;
	return get_separator_size_xy(top_scaling, bot_scaling, separator_size, separator_multiplier, is_fullscreen);
}

static int get_separator_size_y(sf::Vector2f top_screen_size, sf::Vector2f bot_screen_size, float top_scaling, float bot_scaling, ScreenType stype, BottomRelativePosition bottom_pos, int separator_size, float separator_multiplier, bool is_fullscreen) {
	if(!should_have_separator(top_screen_size, bot_screen_size, top_scaling, bot_scaling, stype))
		return 0;
	if((bottom_pos != UNDER_TOP) && (bottom_pos != ABOVE_TOP))
		return 0;
	return get_separator_size_xy(top_scaling, bot_scaling, separator_size, separator_multiplier, is_fullscreen);
}

void WindowScreen::set_position_screens(sf::Vector2f &curr_top_screen_size, sf::Vector2f &curr_bot_screen_size, int offset_x, int offset_y, int max_x, int max_y, int separator_size_x, int separator_size_y, bool do_work) {
	int top_screen_x = 0, top_screen_y = 0;
	int bot_screen_x = 0, bot_screen_y = 0;
	int top_screen_width = (int)curr_top_screen_size.x;
	int top_screen_height = (int)curr_top_screen_size.y;
	int bot_screen_width = (int)curr_bot_screen_size.x;
	int bot_screen_height = (int)curr_bot_screen_size.y;

	if(is_vertically_rotated(this->loaded_info.top_rotation))
		std::swap(top_screen_width, top_screen_height);
	if(is_vertically_rotated(this->loaded_info.bot_rotation))
		std::swap(bot_screen_width, bot_screen_height);

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
				bot_screen_x = (int)((greatest_width - bot_screen_width) * this->loaded_info.subscreen_offset);
				top_screen_x = (int)((greatest_width - top_screen_width) * this->loaded_info.subscreen_offset);
				bot_screen_y = top_screen_height + separator_size_y;
				if(max_y > 0)
					bot_screen_y += (int)((max_y - bot_screen_height - separator_size_y - top_screen_height - offset_y) * this->loaded_info.subscreen_attached_offset);
				break;
			case RIGHT_TOP:
				bot_screen_y = (int)((greatest_height - bot_screen_height) * this->loaded_info.subscreen_offset);
				top_screen_y = (int)((greatest_height - top_screen_height) * this->loaded_info.subscreen_offset);
				bot_screen_x = top_screen_width + separator_size_x;
				if(max_x > 0)
					bot_screen_x += (int)((max_x - bot_screen_width - separator_size_x - top_screen_width - offset_x) * this->loaded_info.subscreen_attached_offset);
				break;
			case ABOVE_TOP:
				bot_screen_x = (int)((greatest_width - bot_screen_width) * this->loaded_info.subscreen_offset);
				top_screen_x = (int)((greatest_width - top_screen_width) * this->loaded_info.subscreen_offset);
				top_screen_y = bot_screen_height + separator_size_y;
				if(max_y > 0)
					top_screen_y += (int)((max_y - bot_screen_height - separator_size_y - top_screen_height - offset_y) * this->loaded_info.subscreen_attached_offset);
				break;
			case LEFT_TOP:
				bot_screen_y = (int)((greatest_height - bot_screen_height) * this->loaded_info.subscreen_offset);
				top_screen_y = (int)((greatest_height - top_screen_height) * this->loaded_info.subscreen_offset);
				top_screen_x = bot_screen_width + separator_size_x;
				if(max_x > 0)
					top_screen_x += (int)((max_x - bot_screen_width - separator_size_x - top_screen_width - offset_x) * this->loaded_info.subscreen_attached_offset);
				break;
			default:
				break;
		}
	}
	if(do_work) {
		float scale_issue_fix_offset = -0.005f;
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

float WindowScreen::get_max_float_screen_multiplier(ResizingScreenData *own_screen, int width_limit, int height_limit, int other_rotation, bool is_two_screens_merged) {
	if(!is_size_valid(own_screen->size))
		return 0;

	if(is_vertically_rotated(other_rotation))
		std::swap(width_limit, height_limit);
	sf::Vector2f other_screen_size = sf::Vector2f((float)width_limit, (float)height_limit);
	if((!is_size_valid(other_screen_size)) && is_two_screens_merged)
		other_screen_size = sf::Vector2f(1, 1);
	sf::Vector2u curr_desk_mode_3d_size = get_desk_mode_3d_multiplied(&this->m_info);
	int desk_height = curr_desk_mode_3d_size.y - get_separator_size_y(own_screen->size, other_screen_size, 1.0, 1.0, this->m_stype, this->m_info.bottom_pos, this->m_info.separator_pixel_size, this->m_info.separator_fullscreen_multiplier, true);
	int desk_width = curr_desk_mode_3d_size.x - get_separator_size_x(own_screen->size, other_screen_size, 1.0, 1.0, this->m_stype, this->m_info.bottom_pos, this->m_info.separator_pixel_size, this->m_info.separator_fullscreen_multiplier, true);
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

	float own_width = own_screen->size.x;
	float own_height = own_screen->size.y;
	get_par_size(own_width, own_height, 1.0, own_screen->par, own_screen->divide_3d_par);
	if(is_vertically_rotated(own_screen->rotation))
		std::swap(own_width, own_height);
	if((own_height == 0) || (own_width == 0))
		return 0;
	float factor_width = width_max / own_width;
	float factor_height = height_max / own_height;
	if(factor_height < factor_width)
		return factor_height;
	return factor_width;
}

int WindowScreen::prepare_screen_ratio(ResizingScreenData *own_screen, int width_limit, int height_limit, int other_rotation) {
	return (int)this->get_max_float_screen_multiplier(own_screen, width_limit, height_limit, other_rotation);
}

int WindowScreen::get_ratio_compared_other_screen(ResizingScreenData *own_screen, ResizingScreenData* other_screen, int other_scaling) {
	int other_width = (int)other_screen->size.x;
	int other_height = (int)other_screen->size.y;
	get_par_size(other_width, other_height, (float)other_scaling, other_screen->par, other_screen->divide_3d_par);
	return prepare_screen_ratio(own_screen, other_width, other_height, other_screen->rotation);
}

void WindowScreen::calc_scaling_resize_screens(ResizingScreenData *own_screen, ResizingScreenData* other_screen, bool increase, bool mantain, bool set_to_zero, bool cycle) {
	int chosen_ratio = get_ratio_compared_other_screen(own_screen, other_screen, 1);
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
	if(((increase && ((old_scaling == (*own_screen->scaling)) || ((*other_screen->scaling) == 0))) || (mantain && ((*other_screen->scaling) == 0))) && ((*own_screen->scaling) > 0))
		(*other_screen->scaling) = 0;
	else
		(*other_screen->scaling) = get_ratio_compared_other_screen(other_screen, own_screen, *own_screen->scaling);
	if((*other_screen->scaling) < 0)
		(*other_screen->scaling) = 0;
	if((this->m_stype == ScreenType::JOINT) && (((*own_screen->scaling) > 0) || (((*other_screen->scaling) == 0) && ((*own_screen->scaling) == 0)))) {
		// Due to size differences, it may be possible that
		// the chosen screen might be able to increase its
		// scaling even more without compromising the other one...
		(*own_screen->scaling) = get_ratio_compared_other_screen(own_screen, other_screen, *other_screen->scaling);
		if((*own_screen->scaling) < 0)
			(*own_screen->scaling) = 0;
	}
}

bool WindowScreen::can_non_integerly_scale() {
	return ((this->m_info.top_scaling != 0) || (this->m_info.bot_scaling != 0)) && ((this->m_info.use_non_integer_scaling_top && (this->m_info.top_scaling != 0)) || (this->m_info.use_non_integer_scaling_bottom && (this->m_info.bot_scaling != 0)));
}

void WindowScreen::rescale_nonint_subscreen(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data) {
	int new_width = (int)main_screen_resize_data->size.x;
	int new_height = (int)main_screen_resize_data->size.y;
	get_par_size(new_width, new_height, *main_screen_resize_data->non_int_scaling, main_screen_resize_data->par, main_screen_resize_data->divide_3d_par);
	*sub_screen_resize_data->non_int_scaling = this->get_max_float_screen_multiplier(sub_screen_resize_data, new_width, new_height, main_screen_resize_data->rotation);
	if(!sub_screen_resize_data->use_non_int_scaling)
		*sub_screen_resize_data->non_int_scaling = std::floor(*sub_screen_resize_data->non_int_scaling);
}

void WindowScreen::direct_scale_nonint_screen(ResizingScreenData *main_screen_resize_data, int sub_min_width, int sub_height_min, int sub_screen_rotation) {
	*main_screen_resize_data->non_int_scaling = this->get_max_float_screen_multiplier(main_screen_resize_data, sub_min_width, sub_height_min, sub_screen_rotation);
	if(!main_screen_resize_data->use_non_int_scaling)
		*main_screen_resize_data->non_int_scaling = std::floor(*main_screen_resize_data->non_int_scaling);
}

void WindowScreen::non_int_scale_screens_with_main(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data, int sub_min_width, int sub_height_min) {
	this->direct_scale_nonint_screen(main_screen_resize_data, sub_min_width, sub_height_min, sub_screen_resize_data->rotation);
	this->rescale_nonint_subscreen(main_screen_resize_data, sub_screen_resize_data);
}

void WindowScreen::non_int_scale_screens_both(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data, int free_width, int free_height, float multiplier_main, int main_min_width, int main_height_min, int sub_min_width, int sub_height_min) {
	int main_free_width = (int)((free_width * multiplier_main) + 0.5f);
	int main_free_height = (int)((free_height * multiplier_main) + 0.5f);
	int sub_free_width = free_width - main_free_width;
	int sub_free_height = free_height - main_free_height;
	this->direct_scale_nonint_screen(main_screen_resize_data, sub_min_width + sub_free_width, sub_height_min + sub_free_height, sub_screen_resize_data->rotation);
	this->direct_scale_nonint_screen(sub_screen_resize_data, main_min_width + main_free_width, main_height_min + main_free_height, main_screen_resize_data->rotation);
	this->rescale_nonint_subscreen(main_screen_resize_data, sub_screen_resize_data);
	this->rescale_nonint_subscreen(sub_screen_resize_data, main_screen_resize_data);
}

void WindowScreen::non_integer_scale_screens(ResizingScreenData *top_screen_resize_data, ResizingScreenData *bot_screen_resize_data) {
	if(!this->can_non_integerly_scale()) {
		this->m_info.non_integer_top_scaling = (float)this->m_info.top_scaling;
		this->m_info.non_integer_bot_scaling = (float)this->m_info.bot_scaling;
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
	int min_top_screen_width = (int)top_screen_resize_data->size.x;
	int min_top_screen_height = (int)top_screen_resize_data->size.y;
	int min_bot_screen_width = (int)bot_screen_resize_data->size.x;
	int min_bot_screen_height = (int)bot_screen_resize_data->size.y;
	get_par_size(min_top_screen_width, min_top_screen_height, (float)min_top_scaling, top_screen_resize_data->par, top_screen_resize_data->divide_3d_par);
	get_par_size(min_bot_screen_width, min_bot_screen_height, (float)min_bot_scaling, bot_screen_resize_data->par, bot_screen_resize_data->divide_3d_par);

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

	if(is_vertically_rotated(top_screen_resize_data->rotation))
		std::swap(min_top_screen_width, min_top_screen_height);
	if(is_vertically_rotated(bot_screen_resize_data->rotation))
		std::swap(min_bot_screen_width, min_bot_screen_height);

	sf::Vector2u curr_desk_mode_3d_size = get_desk_mode_3d_multiplied(&this->m_info);
	int free_width = curr_desk_mode_3d_size.x - (min_bot_screen_width + min_top_screen_width);
	int free_height = curr_desk_mode_3d_size.y - (min_bot_screen_height + min_top_screen_height);

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

void WindowScreen::merge_screens_data(ResizingScreenData* top_screen_resize_data, ResizingScreenData* bot_screen_resize_data, ResizingScreenData* merged_screen_resize_data, BottomRelativePosition bottom_pos) {
	// Due to PARs and rotations, this is a bit annoying, but whatever...
	float top_width = top_screen_resize_data->size.x;
	float top_height = top_screen_resize_data->size.y;
	get_par_size(top_width, top_height, 1.0, top_screen_resize_data->par, top_screen_resize_data->divide_3d_par);
	float bot_width = bot_screen_resize_data->size.x;
	float bot_height = bot_screen_resize_data->size.y;
	get_par_size(bot_width, bot_height, 1.0, bot_screen_resize_data->par, bot_screen_resize_data->divide_3d_par);
	if(is_vertically_rotated(top_screen_resize_data->rotation))
		std::swap(top_width, top_height);
	if(is_vertically_rotated(bot_screen_resize_data->rotation))
		std::swap(bot_width, bot_height);
	float final_width = std::max(top_width, bot_width);
	float final_height = std::max(top_height, bot_height);
	switch(bottom_pos) {
		case UNDER_TOP:
		case ABOVE_TOP:
			final_height = top_height + bot_height;
			break;
		case LEFT_TOP:
		case RIGHT_TOP:
			final_width = top_width + bot_width;
			break;
		default:
			break;
	}
	merged_screen_resize_data->size.x = final_width;
	merged_screen_resize_data->size.y = final_height;
	if(top_screen_resize_data->use_non_int_scaling || bot_screen_resize_data->use_non_int_scaling)
		merged_screen_resize_data->use_non_int_scaling = true;
	else
		merged_screen_resize_data->use_non_int_scaling = false;
	merged_screen_resize_data->rotation = 0;
	*merged_screen_resize_data->scaling = 1;
	*merged_screen_resize_data->non_int_scaling = 1.0f;
	merged_screen_resize_data->par = get_base_par();
}

bool WindowScreen::prepare_screens_same_scaling_factor(ResizingScreenData* top_screen_resize_data, ResizingScreenData* bot_screen_resize_data) {
	if((!is_size_valid(top_screen_resize_data->size)) || (!is_size_valid(bot_screen_resize_data->size)))
		return false;
	int scaling_merged = 1;
	float non_int_scaling_merged = 1.0;
	ResizingScreenData merged_screen_resize_data = {.size = {}, .scaling = &scaling_merged, .non_int_scaling = &non_int_scaling_merged, .use_non_int_scaling = false, .rotation = 0, .par = NULL, .divide_3d_par = false};
	merge_screens_data(top_screen_resize_data, bot_screen_resize_data, &merged_screen_resize_data, this->m_info.bottom_pos);
	non_int_scaling_merged = this->get_max_float_screen_multiplier(&merged_screen_resize_data, 0, 0, 0, true);
	scaling_merged = (int)non_int_scaling_merged;
	if(non_int_scaling_merged < 1.0)
		return false;
	*top_screen_resize_data->non_int_scaling = (float)scaling_merged;
	*bot_screen_resize_data->non_int_scaling = (float)scaling_merged;
	if(merged_screen_resize_data.use_non_int_scaling) {
		*top_screen_resize_data->non_int_scaling = non_int_scaling_merged;
		*bot_screen_resize_data->non_int_scaling = non_int_scaling_merged;
	}
	return true;
}

bool WindowScreen::get_divide_3d_par(bool is_top, ScreenInfo* info) {
	if(!get_3d_enabled(this->capture_status))
		return false;
	if(!is_top)
		return info->squish_3d_bot;
	return info->squish_3d_top;
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
	if((this->m_info.top_par >= ((int)this->possible_pars.size())) || (this->m_info.top_par < 0))
		this->m_info.top_par = 0;
	if((this->m_info.bot_par >= ((int)this->possible_pars.size())) || (this->m_info.bot_par < 0))
		this->m_info.bot_par = 0;
	ResizingScreenData top_screen_resize_data = {.size = top_screen_size, .scaling = &this->m_info.top_scaling, .non_int_scaling = &this->m_info.non_integer_top_scaling, .use_non_int_scaling = this->m_info.use_non_integer_scaling_top, .rotation = this->m_info.top_rotation, .par = this->possible_pars[this->m_info.top_par], .divide_3d_par = get_divide_3d_par(true, &this->m_info)};
	ResizingScreenData bot_screen_resize_data = {.size = bot_screen_size, .scaling = &this->m_info.bot_scaling, .non_int_scaling = &this->m_info.non_integer_bot_scaling, .use_non_int_scaling = this->m_info.use_non_integer_scaling_bottom, .rotation = this->m_info.bot_rotation, .par = this->possible_pars[this->m_info.bot_par], .divide_3d_par = get_divide_3d_par(false, &this->m_info)};

	bool processed = false;
	if(this->m_stype == ScreenType::JOINT && this->m_info.force_same_scaling)
		processed = this->prepare_screens_same_scaling_factor(&top_screen_resize_data, &bot_screen_resize_data);

	if(!processed) {
		if(prioritize_top)
			calc_scaling_resize_screens(&top_screen_resize_data, &bot_screen_resize_data, top_increase, try_mantain_ratio, this->m_stype == ScreenType::BOTTOM, cycle);
		else
			calc_scaling_resize_screens(&bot_screen_resize_data, &top_screen_resize_data, bot_increase, try_mantain_ratio, this->m_stype == ScreenType::TOP, cycle);

		this->non_integer_scale_screens(&top_screen_resize_data, &bot_screen_resize_data);
	}
}

int WindowScreen::get_fullscreen_offset_x(int top_width, int top_height, int bot_width, int bot_height, int separator_contribute) {
	if(is_vertically_rotated(this->loaded_info.top_rotation))
		std::swap(top_width, top_height);
	if(is_vertically_rotated(this->loaded_info.bot_rotation))
		std::swap(bot_width, bot_height);
	int greatest_width = std::max(top_width, bot_width);
	int offset_contribute = 0;
	switch(this->loaded_info.bottom_pos) {
		case UNDER_TOP:
		case ABOVE_TOP:
			offset_contribute = greatest_width;
			break;
		case LEFT_TOP:
		case RIGHT_TOP:
			offset_contribute = top_width + bot_width + separator_contribute;
			break;
		default:
			return 0;
	}
	sf::Vector2u curr_desk_mode_3d_size = get_desk_mode_3d_multiplied(&this->loaded_info);
	return (int)((curr_desk_mode_3d_size.x - offset_contribute) * this->loaded_info.total_offset_x);
}

int WindowScreen::get_fullscreen_offset_y(int top_width, int top_height, int bot_width, int bot_height, int separator_contribute) {
	if(is_vertically_rotated(this->loaded_info.top_rotation))
		std::swap(top_width, top_height);
	if(is_vertically_rotated(this->loaded_info.bot_rotation))
		std::swap(bot_width, bot_height);
	int greatest_height = std::max(top_height, bot_height);
	int offset_contribute = 0;
	switch(this->loaded_info.bottom_pos) {
		case UNDER_TOP:
		case ABOVE_TOP:
			offset_contribute = top_height + bot_height + separator_contribute;
			break;
		case LEFT_TOP:
		case RIGHT_TOP:
			offset_contribute = greatest_height;
			break;
		default:
			return 0;
	}
	sf::Vector2u curr_desk_mode_3d_size = get_desk_mode_3d_multiplied(&this->loaded_info);
	return (int)((curr_desk_mode_3d_size.y - offset_contribute) * this->loaded_info.total_offset_y);
}

void WindowScreen::resize_window_and_out_rects(bool do_work) {
	sf::Vector2f top_screen_size = getShownScreenSize(true, &this->loaded_info);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, &this->loaded_info);
	int top_width = (int)top_screen_size.x;
	int top_height = (int)top_screen_size.y;
	float top_scaling = (float)this->loaded_info.scaling;
	int bot_width = (int)bot_screen_size.x;
	int bot_height = (int)bot_screen_size.y;
	float bot_scaling = (float)this->loaded_info.scaling;
	int offset_x = 0;
	int offset_y = 0;
	int max_x = 0;
	int max_y = 0;
	int separator_size_x = 0;
	int separator_size_y = 0;
	float sep_multiplier = this->loaded_info.separator_windowed_multiplier;

	if(this->loaded_info.is_fullscreen) {
		top_scaling = this->loaded_info.non_integer_top_scaling;
		bot_scaling = this->loaded_info.non_integer_bot_scaling;
		sep_multiplier = this->loaded_info.separator_fullscreen_multiplier;
	}
	if((this->loaded_info.top_par >= ((int)this->possible_pars.size())) || (this->loaded_info.top_par < 0))
		this->loaded_info.top_par = 0;
	if((this->loaded_info.bot_par >= ((int)this->possible_pars.size())) || (this->loaded_info.bot_par < 0))
		this->loaded_info.bot_par = 0;
	get_par_size(top_width, top_height, top_scaling, this->possible_pars[this->loaded_info.top_par], get_divide_3d_par(true, &this->loaded_info));
	get_par_size(bot_width, bot_height, bot_scaling, this->possible_pars[this->loaded_info.bot_par], get_divide_3d_par(false, &this->loaded_info));

	if((!this->loaded_info.is_fullscreen) && this->loaded_info.rounded_corners_fix) {
		offset_y = TOP_ROUNDED_PADDING;
		offset_x = LEFT_ROUNDED_PADDING;
	}

	separator_size_x = get_separator_size_x(top_screen_size, bot_screen_size, top_scaling, bot_scaling, this->m_stype, this->loaded_info.bottom_pos, this->loaded_info.separator_pixel_size, sep_multiplier, this->loaded_info.is_fullscreen);
	separator_size_y = get_separator_size_y(top_screen_size, bot_screen_size, top_scaling, bot_scaling, this->m_stype, this->loaded_info.bottom_pos, this->loaded_info.separator_pixel_size, sep_multiplier, this->loaded_info.is_fullscreen);
	if(this->loaded_info.is_fullscreen) {
		offset_x = get_fullscreen_offset_x(top_width, top_height, bot_width, bot_height, separator_size_x);
		offset_y = get_fullscreen_offset_y(top_width, top_height, bot_width, bot_height, separator_size_y);
		sf::Vector2u curr_desk_mode_3d_size = get_desk_mode_3d_multiplied(&this->loaded_info);
		max_x = curr_desk_mode_3d_size.x;
		max_y = curr_desk_mode_3d_size.y;
	}
	sf::Vector2f new_top_screen_size = {(float)top_width, (float)top_height};
	sf::Vector2f new_bot_screen_size = {(float)bot_width, (float)bot_height};
	if(do_work) {
		this->m_out_rect_top.out_rect.setSize(new_top_screen_size);
		this->m_out_rect_top.out_rect.setTextureRect(sf::IntRect({0, 0}, {(int)top_screen_size.x, (int)top_screen_size.y}));
		this->m_out_rect_bot.out_rect.setSize(new_bot_screen_size);
		this->m_out_rect_bot.out_rect.setTextureRect(sf::IntRect({0, 0}, {(int)bot_screen_size.x, (int)bot_screen_size.y}));
	}
	this->set_position_screens(new_top_screen_size, new_bot_screen_size, offset_x, offset_y, max_x, max_y, separator_size_x, separator_size_y, do_work);
	this->m_width_no_manip = this->m_width;
	this->m_height_no_manip = this->m_height;
	if(get_3d_enabled(this->capture_status) && (!this->display_data->interleaved_3d) && (this->m_stype != ScreenType::BOTTOM) && is_size_valid(this->m_out_rect_top.out_rect.getSize())) {
		sf::Vector2f offset_first_screen = get_3d_offset_out_rect(&this->loaded_info, false);
		this->m_out_rect_top.out_rect.setPosition(this->m_out_rect_top.out_rect.getPosition() + offset_first_screen);
		this->m_out_rect_bot.out_rect.setPosition(this->m_out_rect_bot.out_rect.getPosition() + offset_first_screen);
		if(!this->loaded_info.is_fullscreen) {
			sf::Vector2f offset_to_apply = offset_first_screen;
			if((offset_first_screen.x <= 0) && (offset_first_screen.y <= 0))
				offset_to_apply = get_3d_offset_out_rect(&this->loaded_info, true);
			this->m_width += (int)offset_to_apply.x;
			this->m_height += (int)offset_to_apply.y;
		}
	}
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
	this->m_out_rect_top.out_rect.setRotation(sf::degrees((float)this->loaded_info.top_rotation));
	this->m_out_rect_bot.out_rect.setRotation(sf::degrees((float)this->loaded_info.bot_rotation));
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
	if(((*crop_kind) >= ((int)crops->size())) || ((*crop_kind) < 0))
		*crop_kind = 0;
	int width = (*crops)[*crop_kind]->top_width;
	int height = (*crops)[*crop_kind]->top_height;
	if(get_3d_enabled(this->capture_status) && this->display_data->interleaved_3d)
		width *= 2;
	if(!is_top) {
		width = (*crops)[*crop_kind]->bot_width;
		height = (*crops)[*crop_kind]->bot_height;
	}
	return {(float)width, (float)height};
}

int WindowScreen::get_pos_x_screen_inside_data(bool is_top, bool is_second) {
	if(!is_top)
		return this->capture_status->device.bot_screen_x;
	if(!is_second)
		return this->capture_status->device.top_screen_x;
	return this->capture_status->device.second_top_screen_x;
}

int WindowScreen::get_pos_y_screen_inside_data(bool is_top, bool is_second) {
	if(!is_top)
		return this->capture_status->device.bot_screen_y;
	if(!is_second)
		return this->capture_status->device.top_screen_y;
	return this->capture_status->device.second_top_screen_y;
}

int WindowScreen::get_pos_x_screen_inside_in_tex(bool is_top, bool is_second) {
	if(!this->shared_texture_available)
		return 0;
	if(is_vertically_rotated(this->capture_status->device.base_rotation)) {
		if(this->m_stype == ScreenType::TOP)
			return this->get_pos_x_screen_inside_data(is_top, is_second) - this->get_pos_x_screen_inside_data(is_top, false);
		if(this->m_stype == ScreenType::BOTTOM)
			return 0;
	}
	return this->get_pos_x_screen_inside_data(is_top, is_second);
}

int WindowScreen::get_pos_y_screen_inside_in_tex(bool is_top, bool is_second) {
	if(!this->shared_texture_available)
		return 0;
	if(!is_vertically_rotated(this->capture_status->device.base_rotation)) {
		if(this->m_stype == ScreenType::TOP)
			return this->get_pos_y_screen_inside_data(is_top, is_second) - this->get_pos_y_screen_inside_data(is_top, false);
		if(this->m_stype == ScreenType::BOTTOM)
			return 0;
	}
	return this->get_pos_y_screen_inside_data(is_top, is_second);
}

void WindowScreen::crop() {
	std::vector<const CropData*> *crops = this->get_crop_data_vector(&this->loaded_info);
	int *crop_value = this->get_crop_index_ptr(&this->loaded_info);

	sf::Vector2f top_screen_size = getShownScreenSize(true, &this->loaded_info);
	sf::Vector2f bot_screen_size = getShownScreenSize(false, &this->loaded_info);

	this->resize_in_rect(this->m_in_rect_bot, this->get_pos_x_screen_inside_in_tex(false) + (*crops)[*crop_value]->bot_x, this->get_pos_y_screen_inside_in_tex(false) + (*crops)[*crop_value]->bot_y, (int)bot_screen_size.x, (int)bot_screen_size.y);
	if(get_3d_enabled(this->capture_status) && (!this->display_data->interleaved_3d)) {
		int left_top_screen_x = this->get_pos_x_screen_inside_in_tex(true, false);
		int left_top_screen_y = this->get_pos_y_screen_inside_in_tex(true, false);
		int right_top_screen_x = this->get_pos_x_screen_inside_in_tex(true, true);
		int right_top_screen_y = this->get_pos_y_screen_inside_in_tex(true, true);
		if(!this->capture_status->device.is_second_top_screen_right) {
			std::swap(left_top_screen_x, right_top_screen_x);
			std::swap(left_top_screen_y, right_top_screen_y);
		}
		this->resize_in_rect(this->m_in_rect_top, left_top_screen_x + (*crops)[*crop_value]->top_x, left_top_screen_y + (*crops)[*crop_value]->top_y, (int)top_screen_size.x, (int)top_screen_size.y);
		this->resize_in_rect(this->m_in_rect_top_right, right_top_screen_x + (*crops)[*crop_value]->top_x, right_top_screen_y + (*crops)[*crop_value]->top_y, (int)top_screen_size.x, (int)top_screen_size.y);
	}
	else {
		bool is_3d_interleaved = get_3d_enabled(this->capture_status) && this->display_data->interleaved_3d;
		float x_multiplier = 1.0f;
		if(is_3d_interleaved)
			x_multiplier = 2.0f;
		int starting_in_rect_x = (int)(this->get_pos_x_screen_inside_in_tex(true) + ((*crops)[*crop_value]->top_x * x_multiplier));
		int starting_in_rect_y = (int)(this->get_pos_y_screen_inside_in_tex(true) + (*crops)[*crop_value]->top_y);
		if(is_3d_interleaved && (!this->shared_texture_available))
			top_screen_size.x /= 2.0f;
		this->resize_in_rect(this->m_in_rect_top, starting_in_rect_x, starting_in_rect_y, (int)top_screen_size.x, (int)top_screen_size.y);
		if(is_3d_interleaved && (!this->shared_texture_available)) {
			int second_screen_x_starting_pos = std::max(0, ((int)((*crops)[*crop_value]->top_x * x_multiplier)) - TOP_WIDTH_3DS);
			this->resize_in_rect(this->m_in_rect_top_right, second_screen_x_starting_pos, starting_in_rect_y, (int)top_screen_size.x, (int)top_screen_size.y);
		}
	}
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

void WindowScreen::process_own_out_text_data(bool print_to_notification) {
	if(!this->own_out_text_data.consumed) {
		ConsumeOutText(this->own_out_text_data, false);
		if(print_to_notification && this->notification->isTimerTextDone())
			this->print_notification(this->own_out_text_data.small_text, this->own_out_text_data.kind);
		this->own_out_text_data.consumed = true;
	}
}

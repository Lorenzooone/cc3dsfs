#ifndef __FRONTEND_HPP
#define __FRONTEND_HPP

#include <SFML/Graphics.hpp>

#include <mutex>
#include "utils.hpp"
#include "audio.hpp"

enum Crop { DEFAULT_3DS, SPECIAL_DS, SCALED_DS, NATIVE_DS, SCALED_GBA, NATIVE_GBA, SCALED_GB, NATIVE_GB, NATIVE_SNES, NATIVE_NES, CROP_END };
enum BottomRelativePosition { UNDER_TOP, LEFT_TOP, ABOVE_TOP, RIGHT_TOP, BOT_REL_POS_END };
enum OffsetAlgorithm { NO_DISTANCE, HALF_DISTANCE, MAX_DISTANCE, OFF_ALGO_END };

struct ScreenInfo {
	bool is_blurred;
	Crop crop_kind;
	double scaling;
	bool is_fullscreen;
	BottomRelativePosition bottom_pos;
	OffsetAlgorithm subscreen_offset_algorithm, subscreen_attached_offset_algorithm, total_offset_algorithm_x, total_offset_algorithm_y;
	int top_rotation, bot_rotation;
	bool show_mouse;
	bool v_sync_enabled;
	bool async;
	int top_scaling, bot_scaling;
	bool bfi;
	double bfi_divider;
};

struct DisplayData {
	bool split;
};

struct  __attribute__ ((packed)) VideoOutputData {
	uint8_t screen_data[IN_VIDEO_SIZE][4];
};

struct SFEvent {
	SFEvent(sf::Event::EventType type, sf::Keyboard::Key code, uint32_t unicode) : type(type), code(code), unicode(unicode) {}

	sf::Event::EventType type;
	sf::Keyboard::Key code;
	uint32_t unicode;
};

struct out_rect_data {
	sf::RectangleShape out_rect;
	sf::RenderTexture out_tex;
};

void reset_screen_info(ScreenInfo &info);
void load_screen_info(std::string key, std::string value, std::string base, ScreenInfo &info);
std::string save_screen_info(std::string base, const ScreenInfo &info);

class TextRectangle {
public:
	TextRectangle(bool font_load_success, sf::Font &text_font);
	void setSize(int width, int height);
	void setSelected(bool selected);
	void setDuration(float on_seconds);
	void startTimer(bool do_start);
	void setProportionalBox(bool proportional_box);
	void prepareRenderText();
	void setText(std::string text);
	void setShowText(bool show_text);
	void draw(sf::RenderTarget &window);

private:
	out_rect_data text_rect;
	sf::Text actual_text;
	int width, height;
	bool font_load_success;
	bool is_done_showing_text;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_time_start;
	int time_phase;
	sf::Color *base_bg_color;
	sf::Color *selected_bg_color;
	const float base_time_slide_factor = 0.5;
	const float base_pixel_slide_factor = 48.0;

	struct TextData {
		bool is_timed;
		bool start_timer;
		bool selected;
		bool show_text;
		bool render_text;
		bool proportional_box;
		std::string printed_text;
		float duration;
		float font_pixel_height;
	};

	TextData future_data;
	TextData loaded_data;

	void reset_data(TextData &data);
	void updateText();
	void updateSlides(float* time_seconds);
};

class WindowScreen {
public:
	enum ScreenType { TOP, BOTTOM, JOINT };
	ScreenInfo m_info;

	WindowScreen(WindowScreen::ScreenType stype, DisplayData* display_data, AudioData* audio_data, std::mutex* events_access);
	~WindowScreen();

	void build();
	void reload();
	void poll();
	void close();
	void display_call(bool is_main_thread);
	void display_thread(CaptureData* capture_data);
	void end();
	void draw(double frame_time, VideoOutputData* out_buf);

	int load_data();
	int save_data();
	bool open_capture();
	bool close_capture();

private:
	struct ScreenOperations {
		bool call_create;
		bool call_close;
		bool call_crop;
		bool call_rotate;
		bool call_blur;
		bool call_screen_settings_update;
	};
	std::string win_title;
	sf::RenderWindow m_win;
	int m_width, m_height;
	int m_window_width, m_window_height;
	int m_prepare_load;
	int m_prepare_save;
	bool m_prepare_open;
	bool m_prepare_quit;
	double frame_time;
	DisplayData* display_data;
	AudioData* audio_data;
	std::mutex* events_access;

	sf::Texture in_tex;

	sf::Font text_font;
	bool font_load_success;

	volatile bool main_thread_owns_window;
	volatile bool is_window_factory_done;
	volatile bool scheduled_work_on_window;
	volatile bool is_thread_done;

	sf::RectangleShape m_in_rect_top, m_in_rect_bot;
	out_rect_data m_out_rect_top, m_out_rect_bot;
	WindowScreen::ScreenType m_stype;

	std::queue<SFEvent> events_queue;

	sf::View m_view;
	sf::VideoMode curr_desk_mode;

	TextRectangle* notification;

	ConsumerMutex display_lock;
	bool done_display;
	VideoOutputData *saved_buf;
	ScreenInfo loaded_info;
	ScreenOperations future_operations;
	ScreenOperations loaded_operations;

	static void reset_operations(ScreenOperations &operations);
	void free_ownership_of_window(bool is_main_thread);

	void resize_in_rect(sf::RectangleShape &in_rect, int start_x, int start_y, int width, int height);
	int get_screen_corner_modifier_x(int rotation, int width);
	int get_screen_corner_modifier_y(int rotation, int height);
	void print_notification(std::string text);
	void print_notification_on_off(std::string base_text, bool value);
	void poll_window();
	void prepare_screen_rendering();
	bool window_needs_work();
	void window_factory(bool is_main_thread);
	void window_render_call();
	int apply_offset_algo(int offset_contribute, OffsetAlgorithm chosen_algo);
	void set_position_screens(sf::Vector2f &curr_top_screen_size, sf::Vector2f &curr_bot_screen_size, int offset_x, int offset_y, int max_x, int max_y);
	int prepare_screen_ratio(sf::Vector2f &screen_size, int own_rotation, int width_limit, int height_limit, int other_rotation);
	void calc_scaling_resize_screens(sf::Vector2f &own_screen_size, sf::Vector2f &other_screen_size, int &own_scaling, int &other_scaling, int own_rotation, int other_rotation, bool increase, bool mantain, bool set_to_zero);
	void prepare_size_ratios(bool top_increase, bool bot_increase);
	int get_fullscreen_offset_x(int top_width, int top_height, int bot_width, int bot_height);
	int get_fullscreen_offset_y(int top_width, int top_height, int bot_width, int bot_height);
	void resize_window_and_out_rects();
	void create_window(bool re_prepare_size);
	void update_view_size();
	void open();
	void update_screen_settings();
	void rotate();
	sf::Vector2f getShownScreenSize(bool is_top, Crop &crop_kind);
	void crop();
	void setWinSize(bool is_main_thread);
};

void update_output(WindowScreen &top_screen, WindowScreen &bot_screen, WindowScreen &joint_screen, bool &reload, bool split, double frame_time, VideoOutputData *out_buf);
void screen_display_thread(WindowScreen *screen, CaptureData* capture_data);
#endif

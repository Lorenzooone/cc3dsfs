#ifndef __FRONTEND_HPP
#define __FRONTEND_HPP

#include <SFML/Graphics.hpp>

#include <mutex>
#include <queue>
#include <chrono>
#include "utils.hpp"
#include "audio_data.hpp"
#include "capture_structs.hpp"
#include "hw_defs.hpp"
#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "ConnectionMenu.hpp"

enum Crop { DEFAULT_3DS, SPECIAL_DS, SCALED_DS, NATIVE_DS, SCALED_GBA, NATIVE_GBA, SCALED_GB, NATIVE_GB, SCALED_SNES, NATIVE_SNES, NATIVE_NES, CROP_END };
enum ParCorrection { PAR_NORMAL, PAR_SNES_HORIZONTAL, PAR_SNES_VERTICAL, PAR_END };
enum BottomRelativePosition { UNDER_TOP, LEFT_TOP, ABOVE_TOP, RIGHT_TOP, BOT_REL_POS_END };
enum OffsetAlgorithm { NO_DISTANCE, HALF_DISTANCE, MAX_DISTANCE, OFF_ALGO_END };
enum CurrMenuType { DEFAULT_MENU_TYPE, CONNECT_MENU_TYPE };

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
	double menu_scaling_factor;
	bool rounded_corners_fix;
	ParCorrection top_par;
	ParCorrection bot_par;
};

struct DisplayData {
	bool split = false;
	CurrMenuType curr_menu = DEFAULT_MENU_TYPE;
};

#pragma pack(push, 1)

struct PACKED VideoOutputData {
	uint8_t screen_data[IN_VIDEO_SIZE][4];
};

#pragma pack(pop)

void reset_screen_info(ScreenInfo &info);
bool load_screen_info(std::string key, std::string value, std::string base, ScreenInfo &info);
std::string save_screen_info(std::string base, const ScreenInfo &info);

class WindowScreen {
public:
	enum ScreenType { TOP, BOTTOM, JOINT };
	ScreenInfo m_info;

	WindowScreen(WindowScreen::ScreenType stype, CaptureStatus* capture_status, DisplayData* display_data, AudioData* audio_data, std::mutex* events_access);
	~WindowScreen();

	void build();
	void reload();
	void poll();
	void close();
	void display_call(bool is_main_thread);
	void display_thread();
	void end();
	void after_thread_join();
	void draw(double frame_time, VideoOutputData* out_buf);
	void setup_connection_menu(DevicesList *devices_list);
	int check_connection_menu_result();
	void end_connection_menu();

	void print_notification(std::string text, TextKind kind = TEXT_KIND_NORMAL);
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
	CaptureStatus* capture_status;
	std::string win_title;
	sf::RenderWindow m_win;
	int m_width, m_height;
	int m_window_width, m_window_height;
	int m_prepare_load;
	int m_prepare_save;
	bool last_connected_status;
	bool m_prepare_open;
	bool m_prepare_quit;
	bool font_load_success;
	double frame_time;
	DisplayData* display_data;
	AudioData* audio_data;
	std::mutex* events_access;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_mouse_action_time;
	const float mouse_timeout = 5.0f;
	CurrMenuType loaded_menu;
	ConnectionMenu *connection_menu;

	sf::Texture in_tex;

	sf::Font text_font;

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
	void print_notification_on_off(std::string base_text, bool value);
	void print_notification_float(std::string base_text, float value, int decimals);
	void poll_window();
	bool common_poll(SFEvent &event_data);
	bool main_poll(SFEvent &event_data);
	void prepare_screen_rendering();
	bool window_needs_work();
	void window_factory(bool is_main_thread);
	void pre_texture_conversion_processing();
	void post_texture_conversion_processing(out_rect_data &rect_data, const sf::RectangleShape &in_rect, bool actually_draw, bool is_top);
	void window_bg_processing();
	void display_data_to_window(bool actually_draw);
	void window_render_call();
	int apply_offset_algo(int offset_contribute, OffsetAlgorithm chosen_algo);
	void set_position_screens(sf::Vector2f &curr_top_screen_size, sf::Vector2f &curr_bot_screen_size, int offset_x, int offset_y, int max_x, int max_y, bool do_work = true);
	int prepare_screen_ratio(sf::Vector2f &screen_size, int own_rotation, int width_limit, int height_limit, int other_rotation, ParCorrection own_par);
	void calc_scaling_resize_screens(sf::Vector2f &own_screen_size, sf::Vector2f &other_screen_size, int &own_scaling, int &other_scaling, int own_rotation, int other_rotation, bool increase, bool mantain, bool set_to_zero, ParCorrection own_par, ParCorrection other_par);
	void prepare_size_ratios(bool top_increase, bool bot_increase);
	int get_fullscreen_offset_x(int top_width, int top_height, int bot_width, int bot_height);
	int get_fullscreen_offset_y(int top_width, int top_height, int bot_width, int bot_height);
	void resize_window_and_out_rects(bool do_work = true);
	void create_window(bool re_prepare_size);
	std::string title_factory();
	void update_view_size();
	void open();
	void update_screen_settings();
	void rotate();
	sf::Vector2f getShownScreenSize(bool is_top, Crop &crop_kind);
	void crop();
	void setWinSize(bool is_main_thread);
	void update_connection();
};

struct FrontendData {
	WindowScreen *top_screen;
	WindowScreen *bot_screen;
	WindowScreen *joint_screen;
	DisplayData display_data;
	bool reload;
};

void get_par_size(float &width, float &height, float multiplier_factor, ParCorrection &correction_factor);
void get_par_size(int &width, int &height, float multiplier_factor, ParCorrection &correction_factor);
void default_sleep();
void update_output(FrontendData* frontend_data, double frame_time = 0, VideoOutputData *out_buf = NULL);
void screen_display_thread(WindowScreen *screen);
#endif

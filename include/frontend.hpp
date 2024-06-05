#ifndef __FRONTEND_HPP
#define __FRONTEND_HPP

#include <SFML/Graphics.hpp>

#include <mutex>
#include <queue>
#include <vector>
#include <chrono>
#include "utils.hpp"
#include "audio_data.hpp"
#include "capture_structs.hpp"
#include "TextRectangle.hpp"
#include "sfml_gfx_structs.hpp"
#include "ConnectionMenu.hpp"
#include "MainMenu.hpp"
#include "VideoMenu.hpp"
#include "CropMenu.hpp"
#include "PARMenu.hpp"
#include "RotationMenu.hpp"
#include "OffsetMenu.hpp"
#include "AudioMenu.hpp"
#include "BFIMenu.hpp"
#include "RelativePositionMenu.hpp"
#include "ResolutionMenu.hpp"
#include "FileConfigMenu.hpp"
#include "ExtraSettingsMenu.hpp"
#include "StatusMenu.hpp"
#include "LicenseMenu.hpp"
#include "display_structs.hpp"

struct HeldTime {
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	bool started;
};

struct FPSArray {
	double *data;
	int index;
};

class WindowScreen {
public:
	ScreenInfo m_info;

	WindowScreen(ScreenType stype, CaptureStatus* capture_status, DisplayData* display_data, AudioData* audio_data);
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
	void setup_connection_menu(DevicesList *devices_list, bool reset_data = true);
	int check_connection_menu_result();
	void end_connection_menu();
	void update_save_menu();

	void print_notification(std::string text, TextKind kind = TEXT_KIND_NORMAL);
	int load_data();
	int save_data();
	bool open_capture();
	bool close_capture();
	int get_ret_val();

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
	int ret_val;
	bool font_load_success;
	double frame_time;
	DisplayData* display_data;
	AudioData* audio_data;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_mouse_action_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_window_creation_time;
	HeldTime touch_right_click_action;
	HeldTime touch_action;
	HeldTime pgdown_action;
	HeldTime extra_pgdown_action;
	HeldTime enter_action;
	HeldTime extra_enter_action;
	HeldTime right_click_action;
	HeldTime controller_button_action;
	bool consumed_touch_long_press;
	const float touch_long_press_timer = 1.5f;
	const float mouse_timeout = 5.0f;
	const float v_sync_timeout = 5.0f;
	const float bad_resolution_timeout = 30.0f;
	CurrMenuType curr_menu = DEFAULT_MENU_TYPE;
	CurrMenuType loaded_menu;
	ConnectionMenu *connection_menu;
	MainMenu *main_menu;
	VideoMenu *video_menu;
	CropMenu *crop_menu;
	PARMenu *par_menu;
	RotationMenu *rotation_menu;
	OffsetMenu *offset_menu;
	AudioMenu *audio_menu;
	BFIMenu *bfi_menu;
	RelativePositionMenu *relpos_menu;
	ResolutionMenu *resolution_menu;
	FileConfigMenu *fileconfig_menu;
	ExtraSettingsMenu *extra_menu;
	StatusMenu *status_menu;
	LicenseMenu *license_menu;
	std::vector<const CropData*> possible_crops;
	std::vector<const PARData*> possible_pars;
	std::vector<sf::VideoMode> possible_resolutions;
	std::vector<FileData> possible_files;
	FPSArray in_fps;
	FPSArray draw_fps;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_draw_time;
	FPSArray poll_fps;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_poll_time;

	sf::Texture in_tex;

	sf::Font text_font;

	volatile bool main_thread_owns_window;
	volatile bool is_window_factory_done;
	volatile bool scheduled_work_on_window;
	volatile bool is_thread_done;

	sf::RectangleShape m_in_rect_top, m_in_rect_bot;
	out_rect_data m_out_rect_top, m_out_rect_bot;
	ScreenType m_stype;

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

	void init_menus();
	void destroy_menus();
	void prepare_menu_draws(int view_size_x, int view_size_y);
	void execute_menu_draws();

	static void reset_operations(ScreenOperations &operations);
	void free_ownership_of_window(bool is_main_thread);

	void resize_in_rect(sf::RectangleShape &in_rect, int start_x, int start_y, int width, int height);
	int get_screen_corner_modifier_x(int rotation, int width);
	int get_screen_corner_modifier_y(int rotation, int height);
	void print_notification_on_off(std::string base_text, bool value);
	void print_notification_float(std::string base_text, float value, int decimals);
	void set_close(int ret_val);
	void split_change();
	void fullscreen_change();
	void async_change();
	void vsync_change();
	void blur_change();
	void padding_change();
	void crop_value_change(int new_crop_value);
	void par_value_change(int new_par_value, bool is_top);
	void offset_change(float &value, float change);
	void menu_scaling_change(bool positive);
	void window_scaling_change(bool positive);
	void rotation_change(int &value, bool right);
	void ratio_change(bool top_priority);
	void bfi_change();
	void bottom_pos_change(int new_bottom_pos);
	bool query_reset_request();
	void reset_held_times();
	void poll_window();
	bool common_poll(SFEvent &event_data);
	bool main_poll(SFEvent &event_data);
	bool no_menu_poll(SFEvent &event_data);
	void prepare_screen_rendering();
	bool window_needs_work();
	void window_factory(bool is_main_thread);
	void pre_texture_conversion_processing();
	void post_texture_conversion_processing(out_rect_data &rect_data, const sf::RectangleShape &in_rect, bool actually_draw, bool is_top, bool is_debug);
	void window_bg_processing();
	void display_data_to_window(bool actually_draw, bool is_debug = false);
	void window_render_call();
	void set_position_screens(sf::Vector2f &curr_top_screen_size, sf::Vector2f &curr_bot_screen_size, int offset_x, int offset_y, int max_x, int max_y, bool do_work = true);
	int prepare_screen_ratio(sf::Vector2f &screen_size, int own_rotation, int width_limit, int height_limit, int other_rotation, const PARData *own_par);
	void calc_scaling_resize_screens(sf::Vector2f &own_screen_size, sf::Vector2f &other_screen_size, int &own_scaling, int &other_scaling, int own_rotation, int other_rotation, bool increase, bool mantain, bool set_to_zero, const PARData *own_par, const PARData *other_par);
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
	sf::Vector2f getShownScreenSize(bool is_top, int &crop_kind);
	void crop();
	void setWinSize(bool is_main_thread);
	void setup_main_menu(bool reset_data = true);
	void setup_video_menu(bool reset_data = true);
	void setup_crop_menu(bool reset_data = true);
	void setup_par_menu(bool is_top, bool reset_data = true);
	void setup_offset_menu(bool reset_data = true);
	void setup_rotation_menu(bool reset_data = true);
	void setup_audio_menu(bool reset_data = true);
	void setup_bfi_menu(bool reset_data = true);
	void setup_fileconfig_menu(bool is_save, bool reset_data = true);
	void setup_resolution_menu(bool reset_data = true);
	void setup_extra_menu(bool reset_data = true);
	void setup_status_menu(bool reset_data = true);
	void setup_licenses_menu(bool reset_data = true);
	void setup_relative_pos_menu(bool reset_data = true);
	void update_connection();
};

struct FrontendData {
	WindowScreen *top_screen;
	WindowScreen *bot_screen;
	WindowScreen *joint_screen;
	DisplayData display_data;
	bool reload;
};

void FPSArrayInit(FPSArray *array);
void FPSArrayDestroy(FPSArray *array);
void FPSArrayInsertElement(FPSArray *array, double frame_time);
bool is_allowed_crop(const CropData* crop_data, ScreenType s_type);
void insert_basic_crops(std::vector<const CropData*> &crop_vector, ScreenType s_type);
void insert_basic_pars(std::vector<const PARData*> &par_vector);
void reset_display_data(DisplayData *display_data);
void reset_fullscreen_info(ScreenInfo &info);
void reset_screen_info(ScreenInfo &info);
bool load_screen_info(std::string key, std::string value, std::string base, ScreenInfo &info);
std::string save_screen_info(std::string base, const ScreenInfo &info);
void get_par_size(float &width, float &height, float multiplier_factor, const PARData *correction_factor);
void get_par_size(int &width, int &height, float multiplier_factor, const PARData *correction_factor);
void default_sleep();
void update_output(FrontendData* frontend_data, double frame_time = 0.0, VideoOutputData *out_buf = NULL);
void screen_display_thread(WindowScreen *screen);
#endif

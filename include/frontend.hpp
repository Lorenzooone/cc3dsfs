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
#include "TextRectanglePool.hpp"
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
#include "ShortcutMenu.hpp"
#include "ActionSelectionMenu.hpp"
#include "ScalingRatioMenu.hpp"
#include "ISNitroMenu.hpp"
#include "VideoEffectsMenu.hpp"
#include "InputMenu.hpp"
#include "AudioDeviceMenu.hpp"
#include "SeparatorMenu.hpp"
#include "ColorCorrectionMenu.hpp"
#include "Main3DMenu.hpp"
#include "SecondScreen3DRelativePositionMenu.hpp"
#include "USBConflictResolutionMenu.hpp"
#include "Optimize3DSMenu.hpp"
#include "display_structs.hpp"
#include "event_structs.hpp"
#include "shaders_list.hpp"

struct HeldTime {
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	bool started;
};

struct FPSArray {
	double *data;
	int index;
};

struct OutTextData {
	std::string full_text;
	std::string small_text;
	std::string preamble_name = NAME;
	bool consumed = true;
	TextKind kind;
};

class WindowScreen {
public:
	ScreenInfo m_info;

	WindowScreen(ScreenType stype, CaptureStatus* capture_status, DisplayData* display_data, SharedData* shared_data, AudioData* audio_data, ConsumerMutex *draw_lock, bool created_proper_folder, bool disable_frame_blending);
	~WindowScreen();

	void build();
	void reload();
	void poll(bool do_everything = true);
	void close();
	void display_call(bool is_main_thread);
	void display_thread();
	void end();
	void after_thread_join();
	void draw(double frame_time, VideoOutputData* out_buf, InputVideoDataType video_data_type);
	void setup_connection_menu(std::vector<CaptureDevice> *devices_list, bool reset_data = true);
	int check_connection_menu_result();
	void end_connection_menu();
	void update_ds_3ds_connection(bool changed_type);
	void update_capture_specific_settings();
	void update_save_menu();

	void print_notification(std::string text, TextKind kind = TEXT_KIND_NORMAL);
	void process_own_out_text_data(bool print_to_notification = true);
	int load_data();
	int save_data();
	bool open_capture();
	bool close_capture();
	bool scheduled_split();
	int get_ret_val();

private:
	enum PossibleShaderTypes { BASE_INPUT_SHADER_TYPE, BASE_FINAL_OUTPUT_SHADER_TYPE, COLOR_PROCESSING_SHADER_TYPE };
	enum PossibleSoftwareConvTypes { NO_SOFTWARE_CONV, TO_RGB_SOFTWARE_CONV, TO_RGBA_SOFTWARE_CONV };
	struct ScreenOperations {
		bool call_create;
		bool call_close;
		bool call_crop;
		bool call_rotate;
		bool call_blur;
		bool call_screen_settings_update;
		bool call_titlebar;
	};
	struct ResizingScreenData {
		sf::Vector2f size;
		int *scaling;
		float* non_int_scaling;
		bool use_non_int_scaling;
		int rotation;
		const PARData *par;
		bool divide_3d_par;
	};
	OutTextData own_out_text_data;
	InputVideoDataType curr_video_data_type;
	InputVideoDataType last_update_texture_data_type;
	PossibleSoftwareConvTypes texture_software_based_conv;
	bool created_proper_folder;
	CaptureStatus* capture_status;
	std::string win_title;
	sf::RenderWindow m_win;
	bool is_window_windowed;
	sf::Vector2i saved_windowed_pos;
	int m_width, m_height;
	int m_width_no_manip, m_height_no_manip;
	int m_window_width, m_window_height;
	int m_prepare_load;
	int m_prepare_save;
	bool last_connected_status;
	bool last_enabled_3d;
	bool last_interleaved_3d;
	bool m_prepare_open;
	bool m_prepare_quit;
	bool m_scheduled_split;
	int ret_val;
	double frame_time;
	DisplayData* display_data;
	SharedData* shared_data;
	AudioData* audio_data;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_mouse_action_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_window_creation_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_touch_left_time;
	HeldTime touch_right_click_action;
	HeldTime touch_action;
	HeldTime pgdown_action;
	HeldTime extra_pgdown_action;
	HeldTime enter_action;
	HeldTime extra_enter_action;
	HeldTime right_click_action;
	HeldTime controller_button_action;
	bool consumed_touch_long_press;
	const float touch_short_press_timer = 0.2f;
	const float touch_long_press_timer = 1.5f;
	const float mouse_timeout = 5.0f;
	const float v_sync_timeout = 5.0f;
	const float bad_resolution_timeout = 30.0f;
	const float menu_change_timeout = 0.3f;
	const float input_data_format_change_timeout = 1.0f;

	CurrMenuType curr_menu = DEFAULT_MENU_TYPE;
	GenericMenu* curr_menu_ptr = NULL;
	CurrMenuType loaded_menu;
	GenericMenu* loaded_menu_ptr;

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
	ShortcutMenu *shortcut_menu;
	ActionSelectionMenu *action_selection_menu;
	ISNitroMenu *is_nitro_menu;
	ScalingRatioMenu *scaling_ratio_menu;
	VideoEffectsMenu *video_effects_menu;
	InputMenu *input_menu;
	AudioDeviceMenu *audio_device_menu;
	SeparatorMenu *separator_menu;
	ColorCorrectionMenu *color_correction_menu;
	Main3DMenu *main_3d_menu;
	SecondScreen3DRelativePositionMenu *second_screen_3d_relpos_menu;
	USBConflictResolutionMenu *usb_conflict_resolution_menu;
	Optimize3DSMenu* optimize_3ds_menu;

	std::vector<const CropData*> possible_crops;
	std::vector<const CropData*> possible_crops_ds;
	std::vector<const CropData*> possible_crops_with_games;
	std::vector<const CropData*> possible_crops_ds_with_games;
	std::vector<const PARData*> possible_pars;
	std::vector<const ShaderColorEmulationData*> possible_color_profiles;
	std::vector<sf::VideoMode> possible_resolutions;
	std::vector<std::string> possible_audio_devices;
	std::vector<FileData> possible_files;
	std::vector<std::string> possible_buttons_names;
	std::vector<const WindowCommand **> possible_buttons_ptrs;
	std::vector<bool> possible_buttons_extras;
	int chosen_button;
	std::vector<const WindowCommand*> possible_actions;
	FPSArray in_fps;
	FPSArray draw_fps;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_draw_time;
	FPSArray poll_fps;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_poll_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_menu_change_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_data_format_change_time;
	int curr_frame_texture_pos = 0;

	int num_frames_to_blend;
	sf::Texture full_in_tex;
	bool shared_texture_available;
	sf::Texture top_l_in_tex;
	sf::Texture top_r_in_tex;
	sf::Texture bot_in_tex;

	sf::Font text_font;

	volatile bool main_thread_owns_window;
	volatile bool is_window_factory_done;
	volatile bool scheduled_work_on_window;
	volatile bool is_thread_done;

	bool was_last_frame_null;
	sf::RectangleShape m_in_rect_top, m_in_rect_bot, m_in_rect_top_right;
	out_rect_data m_out_rect_top, m_out_rect_bot, m_out_rect_top_right;
	ScreenType m_stype;

	const ShaderColorEmulationData* sent_shader_color_data;

	std::queue<SFEvent> events_queue;

	sf::View m_view;
	sf::VideoMode curr_desk_mode;

	TextRectangle* notification;
	TextRectanglePool* text_rectangle_pool;

	ConsumerMutex display_lock;
	ConsumerMutex *draw_lock;
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
	int get_pos_x_screen_inside_data(bool is_top, bool is_second = false);
	int get_pos_y_screen_inside_data(bool is_top, bool is_second = false);
	int get_pos_x_screen_inside_in_tex(bool is_top, bool is_second = false);
	int get_pos_y_screen_inside_in_tex(bool is_top, bool is_second = false);
	int get_screen_corner_modifier_x(int rotation, int width);
	int get_screen_corner_modifier_y(int rotation, int height);
	void print_notification_on_off(std::string base_text, bool value);
	void print_notification_float(std::string base_text, float value, int decimals);
	void set_close(int ret_val);
	bool can_execute_cmd(const WindowCommand* window_cmd, bool is_extra, bool is_always);
	bool execute_cmd(PossibleWindowCommands id);
	void split_change();
	void fullscreen_change();
	void async_change();
	void vsync_change();
	void blur_change();
	void fast_poll_change();
	void padding_change();
	void game_crop_enable_change();
	void request_3d_change();
	void interleaved_3d_change();
	void squish_3d_change(bool is_top);
	void audio_device_change(audio_output_device_data new_device_data);
	void is_nitro_capture_type_change(bool positive);
	void is_nitro_capture_speed_change(bool positive);
	void is_nitro_capture_reset_hw();
	void is_nitro_battery_change(bool positive);
	void is_nitro_ac_adapter_change();
	void crop_value_change(int new_crop_value, bool do_print_notification = true, bool do_cycle = true);
	void par_value_change(int new_par_value, bool is_top);
	void offset_change(float &value, float change);
	void menu_scaling_change(bool positive);
	void window_scaling_change(bool positive);
	void rotation_change(int &value, bool right);
	void force_same_scaling_change();
	void ratio_change(bool top_priority, bool cycle = false);
	void bfi_change();
	void input_toggle_change(bool &target);
	void bottom_pos_change(int new_bottom_pos);
	void non_int_scaling_change(bool target_top);
	void non_int_mode_change(bool positive);
	void titlebar_change();
	void input_colorspace_mode_change(bool positive);
	void frame_blending_mode_change(bool positive);
	void separator_size_change(int change);
	void separator_multiplier_change(bool positive, float& multiplier_to_check, float lower_limit, float upper_limit);
	void separator_windowed_multiplier_change(bool positive);
	void separator_fullscreen_multiplier_change(bool positive);
	void color_correction_value_change(int new_color_correction_value);
	void second_screen_3d_pos_change(int new_second_screen_3d_pos);
	void second_screen_3d_match_bottom_pos_change();
	void devices_allowed_change(PossibleCaptureDevices device);
	void input_video_data_format_request_change(bool positive);
	bool query_reset_request();
	void reset_held_times(bool force = true);
	void poll_window(bool do_everything);
	bool common_poll(SFEvent &event_data);
	bool main_poll(SFEvent &event_data);
	bool no_menu_poll(SFEvent &event_data);
	void prepare_screen_rendering();
	bool window_needs_work();
	void window_factory(bool is_main_thread);
	void opengl_error_out(std::string error_base, std::string error_str);
	void opengl_error_check(std::string error_base);
	bool single_update_texture(unsigned int m_texture, InputVideoDataType video_data_type, size_t pos_x_data, size_t pos_y_data, size_t width, size_t height, bool manually_converted);
	void execute_single_update_texture(bool &manually_converted, bool do_full, bool is_top = false, bool is_second = false);
	void update_texture();
	int _choose_base_input_shader(bool is_top);
	int _choose_color_emulation_shader(bool is_top);
	int _choose_shader(PossibleShaderTypes shader_type, bool is_top);
	int choose_shader(PossibleShaderTypes shader_type, bool is_top);
	void apply_shader_to_texture(sf::RectangleShape &rect_data, sf::RenderTexture* &to_process_tex_data, sf::RenderTexture* &backup_tex_data, PossibleShaderTypes shader_type, bool is_top);
	bool apply_shaders_to_input(sf::RectangleShape &rect_data, sf::RenderTexture* &to_process_tex_data, sf::RenderTexture* &backup_tex_data, const sf::RectangleShape &final_in_rect, bool is_top);
	void pre_texture_conversion_processing();
	void post_texture_conversion_processing(sf::RectangleShape &rect_data, sf::RenderTexture* &to_process_tex_data, sf::RenderTexture* &backup_tex_data, const sf::RectangleShape &in_rect, bool actually_draw, bool is_top, bool is_debug);
	void draw_rect_to_window(const sf::RectangleShape &out_rect, bool is_top);
	void window_bg_processing();
	void display_data_to_window(bool actually_draw, bool is_debug = false);
	void window_render_call();
	void set_position_screens(sf::Vector2f &curr_top_screen_size, sf::Vector2f &curr_bot_screen_size, int offset_x, int offset_y, int max_x, int max_y, int separator_size_x, int separator_size_y, bool do_work = true);
	float get_max_float_screen_multiplier(ResizingScreenData *own_screen, int width_limit, int height_limit, int other_rotation, bool is_two_screens_merged = false);
	bool get_divide_3d_par(bool is_top, ScreenInfo* info);
	sf::Vector2u get_3d_size_multiplier(ScreenInfo* info);
	sf::Vector2u get_desk_mode_3d_multiplied(ScreenInfo* info);
	sf::Vector2f get_3d_offset_out_rect(ScreenInfo* info, bool is_second_screen);
	int prepare_screen_ratio(ResizingScreenData *own_screen, int width_limit, int height_limit, int other_rotation);
	int get_ratio_compared_other_screen(ResizingScreenData *own_screen, ResizingScreenData* other_screen, int other_scaling);
	bool can_non_integerly_scale();
	void calc_scaling_resize_screens(ResizingScreenData *own_screen, ResizingScreenData* other_screen, bool increase, bool mantain, bool set_to_zero, bool cycle);
	void rescale_nonint_subscreen(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data);
	void direct_scale_nonint_screen(ResizingScreenData *main_screen_resize_data, int sub_min_width, int sub_min_height, int sub_screen_rotation);
	void non_int_scale_screens_with_main(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data, int sub_min_width, int sub_height_min);
	void non_int_scale_screens_both(ResizingScreenData *main_screen_resize_data, ResizingScreenData *sub_screen_resize_data, int free_width, int free_height, float multiplier_main, int main_min_width, int main_height_min, int sub_min_width, int sub_height_min);
	void non_integer_scale_screens(ResizingScreenData *top_screen_resize_data, ResizingScreenData *bot_screen_resize_data);
	void merge_screens_data(ResizingScreenData* top_screen_resize_data, ResizingScreenData* bot_screen_resize_data, ResizingScreenData* merged_screen_resize_data, BottomRelativePosition bottom_pos);
	bool prepare_screens_same_scaling_factor(ResizingScreenData* top_screen_resize_data, ResizingScreenData* bot_screen_resize_data);
	void prepare_size_ratios(bool top_increase, bool bot_increase, bool cycle = false);
	int get_fullscreen_offset_x(int top_width, int top_height, int bot_width, int bot_height, int separator_contribute);
	int get_fullscreen_offset_y(int top_width, int top_height, int bot_width, int bot_height, int separator_contribute);
	void resize_window_and_out_rects(bool do_work = true);
	void create_window(bool re_prepare_size, bool reset_text  = true);
	std::string _title_factory();
	sf::String title_factory();
	void update_view_size();
	void open();
	void update_screen_settings();
	void rotate();
	std::vector<const CropData*>* get_crop_data_vector(ScreenInfo* info);
	int* get_crop_index_ptr(ScreenInfo* info);
	sf::Vector2f getShownScreenSize(bool is_top, ScreenInfo* info);
	void crop();
	void setWinSize(bool is_main_thread);
	bool can_setup_menu();
	void switch_to_menu(CurrMenuType new_menu_type, GenericMenu* new_menu, bool reset_data = true, bool update_last_menu_change_time = true);
	void setup_no_menu();
	void setup_main_menu(bool reset_data = true, bool skip_setup_check = false);
	void setup_video_menu(bool reset_data = true);
	void setup_crop_menu(bool reset_data = true);
	void setup_par_menu(bool is_top, bool reset_data = true);
	void setup_offset_menu(bool reset_data = true);
	void setup_rotation_menu(bool reset_data = true);
	void setup_audio_menu(bool reset_data = true);
	void setup_bfi_menu(bool reset_data = true);
	void setup_fileconfig_menu(bool is_save, bool reset_data = true, bool skip_setup_check = false);
	void setup_resolution_menu(bool reset_data = true);
	void setup_audio_devices_menu(bool reset_data = true);
	void setup_extra_menu(bool reset_data = true);
	void setup_shortcuts_menu(bool reset_data = true);
	void setup_action_selection_menu(bool reset_data = true);
	void setup_status_menu(bool reset_data = true);
	void setup_licenses_menu(bool reset_data = true);
	void setup_relative_pos_menu(bool reset_data = true);
	void setup_scaling_ratio_menu(bool reset_data = true);
	void setup_is_nitro_menu(bool reset_data = true);
	void setup_video_effects_menu(bool reset_data = true);
	void setup_input_menu(bool reset_data = true);
	void setup_separator_menu(bool reset_data = true);
	void setup_color_correction_menu(bool reset_data = true);
	void setup_main_3d_menu(bool reset_data = true);
	void setup_second_screen_3d_relpos_menu(bool reset_data = true);
	void setup_usb_conflict_resolution_menu(bool reset_data = true);
	void setup_optimize_3ds_menu(bool reset_data = true);
	void update_connection();
};

struct FrontendData {
	WindowScreen *top_screen;
	WindowScreen *bot_screen;
	WindowScreen *joint_screen;
	DisplayData display_data;
	SharedData shared_data;
	bool reload;
};

void ConsumeOutText(OutTextData &out_text_data, bool update_consumed = true);
void UpdateOutText(OutTextData &out_text_data, std::string full_text, std::string small_text, TextKind kind);

void FPSArrayInit(FPSArray *array);
void FPSArrayDestroy(FPSArray *array);
void FPSArrayInsertElement(FPSArray *array, double frame_time);

void insert_basic_crops(std::vector<const CropData*> &crop_vector, ScreenType s_type, bool is_ds, bool allow_game_specific);
void insert_basic_pars(std::vector<const PARData*> &par_vector);
void insert_basic_color_profiles(std::vector<const ShaderColorEmulationData*> &color_profiles_vector);

void reset_display_data(DisplayData *display_data);
void reset_input_data(InputData* input_data);
void reset_shared_data(SharedData* shared_data);
void reset_fullscreen_info(ScreenInfo &info);

void sanitize_enabled_info(ScreenInfo &top_bot_info, ScreenInfo &top_info, ScreenInfo &bot_info);
void override_set_data_to_screen_info(override_win_data &override_win, ScreenInfo &info);
void reset_screen_info(ScreenInfo &info);
bool load_screen_info(std::string key, std::string value, std::string base, ScreenInfo &info);
std::string save_screen_info(std::string base, const ScreenInfo &info);

const PARData* get_base_par();
void get_par_size(int &width, int &height, float multiplier_factor, const PARData *correction_factor, bool divide_3d_par);
void get_par_size(float &width, float &height, float multiplier_factor, const PARData *correction_factor, bool divide_3d_par);

SecondScreen3DRelativePosition get_second_screen_pos(ScreenInfo* info, ScreenType stype);

void update_output(FrontendData* frontend_data, double frame_time = 0.0, VideoOutputData *out_buf = NULL, InputVideoDataType video_data_type = VIDEO_DATA_RGB);
void update_connected_3ds_ds(FrontendData* frontend_data, const CaptureDevice &old_cc_device, const CaptureDevice &new_cc_device);
void update_connected_specific_settings(FrontendData* frontend_data, const CaptureDevice &cc_device);

void screen_display_thread(WindowScreen *screen);

std::string get_name_non_int_mode(NonIntegerScalingModes input);
std::string get_name_frame_blending_mode(FrameBlendingMode input);
std::string get_name_input_colorspace_mode(InputColorspaceMode input);
bool is_input_data_valid(InputData* input_data, bool consider_buttons);
void default_sleep(float wanted_ms = -1);

#endif

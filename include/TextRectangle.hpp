#ifndef __TEXTRECTANGLE_HPP
#define __TEXTRECTANGLE_HPP

#include <SFML/Graphics.hpp>

#include <chrono>
#include "sfml_gfx_structs.hpp"

#define BASE_PIXEL_FONT_HEIGHT 24

#define PERCENTAGE_POS_TEXT_ERROR (-1)

enum TextKind {TEXT_KIND_NORMAL, TEXT_KIND_SELECTED, TEXT_KIND_SUCCESS, TEXT_KIND_WARNING, TEXT_KIND_ERROR, TEXT_KIND_OPAQUE_ERROR, TEXT_KIND_TITLE, TEXT_KIND_TEXTBOX, TEXT_KIND_TEXTBOX_TRANSPARENT};
enum FontKind {FONT_KIND_NORMAL, FONT_KIND_MONO};
enum TextPosKind {POS_KIND_NORMAL, POS_KIND_PRIORITIZE_TOP_LEFT, POS_KIND_PRIORITIZE_BOTTOM_LEFT};

typedef sf::Vector2i percentage_pos_text_t;
typedef sf::Vector2i size_text_t;
typedef sf::Vector2i position_text_t;

class TextRectangle {
public:
	TextRectangle(bool font_load_success, sf::Font *text_font, bool font_mono_load_success, sf::Font *text_font_mono);
	~TextRectangle();
	void setRectangleKind(TextKind kind);
	void setTextFactor(float size_multiplier);
	void setSize(int width, int height);
	size_text_t getFinalSize();
	size_text_t getFinalSizeNoMultipliers();
	percentage_pos_text_t getCoordInRectanglePercentage(int coord_x, int coord_y);
	bool isCoordInRectangle(int coord_x, int coord_y);
	void setDuration(float on_seconds);
	void setPosition(int pos_x, int pos_y, TextPosKind kind=POS_KIND_NORMAL);
	position_text_t getFinalPosition();
	position_text_t getFinalPositionNoMultipliers();
	void startTimer(bool do_start);
	void setProportionalBox(bool proportional_box);
	void setTightAndCentered(bool tight_and_centered);
	void prepareRenderText();
	void setText(std::string text);
	void setShowText(bool show_text);
	bool getShowText();
	void draw(sf::RenderTarget &window);
	void changeFont(FontKind new_font_kind);
	void setLineSpacing(float new_line_spacing);
	void setCharacterSpacing(float new_character_spacing);
	void setLocked(bool new_locked);
	// Only applies to text displayed with timer set to true
	bool isTimerTextDone();

private:
	out_rect_data text_rect;
	sf::Text actual_text;
	sf::Font *text_font;
	sf::Font *text_font_mono;
	bool font_load_success;
	bool font_mono_load_success;
	bool is_done_showing_text;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_time_start;
	int time_phase;
	sf::Color curr_color;
	const float base_time_slide_factor = 0.5;
	const float base_pixel_slide_factor = 2.0;
	int pos_x_center_contrib = 0;
	int pos_y_center_contrib = 0;

	struct TextData {
		bool is_timed;
		bool start_timer;
		TextKind kind;
		bool show_text;
		bool render_text;
		bool proportional_box;
		bool tight_and_centered;
		std::string printed_text;
		float duration;
		float font_pixel_height;
		int width;
		int height;
		int stored_width;
		int stored_height;
		int pos_x;
		int pos_y;
		int stored_pos_x;
		int stored_pos_y;
		bool locked;
		bool schedule_font_swap;
		FontKind font_kind;
		float line_spacing;
		float character_spacing;
		TextPosKind pos_kind;
	};

	TextData future_data;
	TextData loaded_data;

	void setRealSize(int width, int height, bool check_previous = true);
	void reset_data(TextData &data);
	void setTextWithLineWrapping(int x_limit = 0);
	void updateText(int x_limit = 0);
	void updateSlides(float* time_seconds);
	float getNoBlurCharacterSpacing();
	bool isFontLoaded(TextData &reference_data);
	sf::Font* getFont(TextData &reference_data);
};
#endif

#ifndef __TEXTRECTANGLE_HPP
#define __TEXTRECTANGLE_HPP

#include <SFML/Graphics.hpp>

#include <chrono>
#include "sfml_gfx_structs.hpp"

#define BASE_PIXEL_FONT_HEIGHT 24

enum TextKind {TEXT_KIND_NORMAL, TEXT_KIND_SELECTED, TEXT_KIND_SUCCESS, TEXT_KIND_WARNING, TEXT_KIND_ERROR, TEXT_KIND_OPAQUE_ERROR, TEXT_KIND_TITLE};

class TextRectangle {
public:
	TextRectangle(bool font_load_success, sf::Font &text_font);
	~TextRectangle();
	void setRectangleKind(TextKind kind);
	void setTextFactor(float size_multiplier);
	void setSize(int width, int height);
	bool isCoordInRectangle(int coord_x, int coord_y);
	void setDuration(float on_seconds);
	void setPosition(int pos_x, int pos_y);
	void startTimer(bool do_start);
	void setProportionalBox(bool proportional_box);
	void prepareRenderText();
	void setText(std::string text);
	void setShowText(bool show_text);
	void draw(sf::RenderTarget &window);

private:
	out_rect_data text_rect;
	sf::Text actual_text;
	bool font_load_success;
	bool is_done_showing_text;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_time_start;
	int time_phase;
	sf::Color curr_color;
	const float base_time_slide_factor = 0.5;
	const float base_pixel_slide_factor = 2.0;

	struct TextData {
		bool is_timed;
		bool start_timer;
		TextKind kind;
		bool show_text;
		bool render_text;
		bool proportional_box;
		std::string printed_text;
		float duration;
		float font_pixel_height;
		int width;
		int height;
		int pos_x;
		int pos_y;
	};

	TextData future_data;
	TextData loaded_data;

	void setRealSize(int width, int height, bool check_previous = true);
	void reset_data(TextData &data);
	void setTextWithLineWrapping(int x_limit = 0);
	void updateText(int x_limit = 0);
	void updateSlides(float* time_seconds);
};
#endif

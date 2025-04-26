#ifndef __TEXTRECTANGLEPOOL_HPP
#define __TEXTRECTANGLEPOOL_HPP

#include "TextRectangle.hpp"

class TextRectanglePool {
public:
	TextRectanglePool(bool font_load_success, sf::Font *text_font);
	~TextRectanglePool();
	void request_num_text_rectangles(int num_wanted_text_rectangles);
	TextRectangle* get_text_rectangle(int index);

private:
	bool font_load_success;
	sf::Font *text_font;
	int num_loaded_text_rectangles;
	TextRectangle** text_rectangles_list;
};

#endif

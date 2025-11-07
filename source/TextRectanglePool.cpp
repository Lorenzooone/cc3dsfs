#include "TextRectangle.hpp"
#include "TextRectanglePool.hpp"

TextRectanglePool::TextRectanglePool(bool font_load_success, sf::Font *text_font, bool font_mono_load_success, sf::Font *text_font_mono) {
	this->font_load_success = font_load_success;
	this->text_font = text_font;
	this->font_mono_load_success = font_mono_load_success;
	this->text_font_mono = text_font_mono;
	this->num_loaded_text_rectangles = 0;
	this->text_rectangles_list = NULL;
}

TextRectanglePool::~TextRectanglePool() {
	if(this->num_loaded_text_rectangles > 0) {
		for(int i = 0; i < this->num_loaded_text_rectangles; i++)
			delete this->text_rectangles_list[i];
		delete []this->text_rectangles_list;
	}
	this->num_loaded_text_rectangles = 0;
}

void TextRectanglePool::request_num_text_rectangles(int num_wanted_text_rectangles) {
	if(this->num_loaded_text_rectangles > num_wanted_text_rectangles)
		return;

	TextRectangle** new_text_rectangle_ptrs = new TextRectangle*[num_wanted_text_rectangles];

	// Copy other TextRectangles to new list
	for(int i = 0; i < this->num_loaded_text_rectangles; i++) {
		new_text_rectangle_ptrs[i] = this->text_rectangles_list[i];
	}

	// Create new TextRectangles
	for(int i = this->num_loaded_text_rectangles; i < num_wanted_text_rectangles; i++) {
		new_text_rectangle_ptrs[i] = new TextRectangle(this->font_load_success, this->text_font, this->font_mono_load_success, this->text_font_mono);
	}

	if(this->num_loaded_text_rectangles > 0)
		delete []this->text_rectangles_list;

	this->text_rectangles_list = new_text_rectangle_ptrs;
	this->num_loaded_text_rectangles = num_wanted_text_rectangles;
}

TextRectangle* TextRectanglePool::get_text_rectangle(int index) {
	if((this->num_loaded_text_rectangles <= index) || (index < 0))
		return NULL;

	return this->text_rectangles_list[index];
}

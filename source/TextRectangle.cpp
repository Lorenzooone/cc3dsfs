#include <SFML/Graphics.hpp>

#include "frontend.hpp"

#include <chrono>

TextRectangle::TextRectangle(bool font_load_success, sf::Font &text_font) {
	this->font_load_success = font_load_success;
	this->setSize(1, 1);
	this->text_rect.out_rect.setTexture(&this->text_rect.out_tex.getTexture());
	if(this->font_load_success)
		this->actual_text.setFont(text_font);
	this->time_phase = 0;
	this->base_bg_color = new sf::Color(40, 40, 80, 192);
	this->selected_bg_color = new sf::Color(90, 150, 210, 192);
	this->success_bg_color = new sf::Color(90, 210, 90, 192);
	this->warning_bg_color = new sf::Color(200, 200, 90, 192);
	this->error_bg_color = new sf::Color(210, 90, 90, 192);
	this->reset_data(this->future_data);
}

TextRectangle::~TextRectangle() {
	delete this->base_bg_color;
	delete this->selected_bg_color;
	delete this->success_bg_color;
	delete this->warning_bg_color;
	delete this->error_bg_color;
}

void TextRectangle::setSize(int width, int height) {
	this->width = width;
	this->height = height;
	this->text_rect.out_rect.setSize(sf::Vector2f(this->width, this->height));
	this->text_rect.out_tex.create(this->width, this->height);
	this->text_rect.out_rect.setTextureRect(sf::IntRect(0, 0, this->width, this->height));
}

void TextRectangle::setRectangleKind(TextKind kind) {
	if(this->future_data.kind != kind) {
		this->future_data.render_text = true;
	}
	this->future_data.kind = kind;
}

void TextRectangle::setDuration(float on_seconds) {
	this->future_data.duration = on_seconds;
}

void TextRectangle::startTimer(bool do_start) {
	this->future_data.start_timer = do_start;
	this->future_data.is_timed = do_start;
}

void TextRectangle::setProportionalBox(bool proportional_box) {
	if(this->future_data.proportional_box != proportional_box) {
		this->future_data.render_text = true;
	}
	this->future_data.proportional_box = proportional_box;
}

void TextRectangle::prepareRenderText() {
	this->loaded_data = this->future_data;
	this->future_data.render_text = false;
	this->future_data.start_timer = false;
}

void TextRectangle::setText(std::string text) {
	this->future_data.printed_text = text;
	this->future_data.render_text = true;
}

void TextRectangle::setShowText(bool show_text) {
	this->future_data.show_text = show_text;
	if(!this->future_data.show_text)
		this->startTimer(false);
}

void TextRectangle::draw(sf::RenderTarget &window) {
	if(this->loaded_data.render_text) {
		this->updateText(window.getView().getSize().x);
	}
	if(font_load_success && this->loaded_data.show_text && (!this->is_done_showing_text)) {
		if(this->loaded_data.is_timed) {
			float time_seconds[3];
			this->updateSlides(time_seconds);
			if(this->loaded_data.start_timer) {
				this->time_phase = 0;
				this->clock_time_start = std::chrono::high_resolution_clock::now();
				this->loaded_data.start_timer = false;
			}
			const auto curr_time = std::chrono::high_resolution_clock::now();
			const std::chrono::duration<double> diff = curr_time - this->clock_time_start;
			double factor = diff.count() / time_seconds[this->time_phase];
			if(factor < 0)
				factor = 0;
			if(factor > 1)
				factor = 1;

			int rect_curr_height = 0;

			if(this->time_phase == 0)
				rect_curr_height = -this->height + ((int)(factor * this->height));
			else if(this->time_phase == 2)
				rect_curr_height = ((int)(factor * this->height)) * -1;
			this->text_rect.out_rect.setPosition(0, rect_curr_height);

			if(factor >= 1) {
				this->time_phase++;
				this->clock_time_start = std::chrono::high_resolution_clock::now();
			}
			if(this->time_phase >= 3)
				this->is_done_showing_text = true;
		}
		window.draw(this->text_rect.out_rect);
	}
}

void TextRectangle::reset_data(TextData &data) {
	data.is_timed = false;
	data.start_timer = false;
	data.kind = TEXT_KIND_NORMAL;
	data.show_text = false;
	data.render_text = false;
	data.proportional_box = true;
	data.printed_text = "Sample Text";
	data.duration = 2.5;
	data.font_pixel_height = 24;
}

void TextRectangle::setTextWithLineWrapping(int x_limit) {
	if(x_limit == 0) {
		this->actual_text.setString(this->loaded_data.printed_text);
		return;
	}
	bool is_done = false;
	bool line_started = false;
	std::string new_text = "";
	int pos = 0;
	while(!is_done) {
		std::string curr_text = new_text;
		if(line_started)
			curr_text += " ";
		auto space_pos = this->loaded_data.printed_text.find(' ', pos);
		if(space_pos == std::string::npos) {
			is_done = true;
			space_pos = this->loaded_data.printed_text.length();
		}
		for(int i = pos; i < space_pos; i++)
			curr_text += this->loaded_data.printed_text[i];
		this->actual_text.setString(curr_text);
		sf::FloatRect globalBounds = this->actual_text.getGlobalBounds();
		int bounds_width = globalBounds.width + (globalBounds.left * 2);
		if(line_started && (bounds_width > x_limit)) {
			curr_text = new_text + "\n";
			for(int i = pos; i < space_pos; i++)
				curr_text += this->loaded_data.printed_text[i];
		}
		pos = space_pos + 1;
		line_started = true;
		new_text = curr_text;
	}
	this->actual_text.setString(new_text);
}

void TextRectangle::updateText(int x_limit) {
	// set the character size
	this->actual_text.setCharacterSize(this->loaded_data.font_pixel_height); // in pixels, not points!
	// set the color
	this->actual_text.setFillColor(sf::Color::White);
	// set the text style
	//this->actual_text.setStyle(sf::Text::Bold);
	this->setTextWithLineWrapping(x_limit);
	sf::FloatRect globalBounds = this->actual_text.getGlobalBounds();
	int new_width = this->width;
	int new_height = this->height;
	int bounds_width = globalBounds.width + (globalBounds.left * 2);
	int bounds_height = globalBounds.height + (globalBounds.top * 2);
	if(new_width < bounds_width)
		new_width = bounds_width;
	if(new_height < bounds_height)
		new_height = bounds_height;
	if((new_width != this->width) || (new_height != this->height))
		this->setSize(new_width, new_height);
	if(this->loaded_data.proportional_box) {
		this->actual_text.setPosition(5, 0);
		globalBounds = this->actual_text.getGlobalBounds();
		this->setSize(globalBounds.width + (globalBounds.left * 2), globalBounds.height + (globalBounds.top * 2));
	}
	else {
		this->actual_text.setPosition(0, 0);
		globalBounds = this->actual_text.getGlobalBounds();
		this->actual_text.setPosition((new_width - (globalBounds.width + (globalBounds.left * 4))) / 2, (new_height - (globalBounds.height + (globalBounds.top * 4))) / 2);
	}
	sf::Color *used_color = this->base_bg_color;
	switch(this->loaded_data.kind) {
		case TEXT_KIND_SELECTED:
			used_color = this->selected_bg_color;
			break;
		case TEXT_KIND_SUCCESS:
			used_color = this->success_bg_color;
			break;
		case TEXT_KIND_WARNING:
			used_color = this->warning_bg_color;
			break;
		case TEXT_KIND_ERROR:
			used_color = this->error_bg_color;
			break;
		default:
			break;
	}
	this->text_rect.out_tex.clear(*used_color);
	this->text_rect.out_tex.draw(this->actual_text);
	this->text_rect.out_tex.display();
	this->is_done_showing_text = false;
	this->loaded_data.start_timer = this->loaded_data.is_timed;
}

void TextRectangle::updateSlides(float* time_seconds) {
	time_seconds[0] = base_time_slide_factor * (this->height / (loaded_data.font_pixel_height * base_pixel_slide_factor));
	time_seconds[1] = loaded_data.duration;
	time_seconds[2] = base_time_slide_factor * (this->height / (loaded_data.font_pixel_height * base_pixel_slide_factor));
}

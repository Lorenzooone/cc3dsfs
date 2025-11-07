#include "TextRectangle.hpp"
#include <cmath>

#define TEXT_RECTANGLE_POS_APPROX 1.02

TextRectangle::TextRectangle(bool font_load_success, sf::Font *text_font, bool font_mono_load_success, sf::Font *text_font_mono) : actual_text(*text_font) {
	this->reset_data(this->future_data);
	this->text_font = text_font;
	this->text_font_mono = text_font_mono;
	this->font_load_success = font_load_success;
	this->font_mono_load_success = font_mono_load_success;
	this->is_done_showing_text = true;
	this->setRealSize(1, 1, false);
	this->text_rect.out_rect.setTexture(&this->text_rect.out_tex.getTexture());
	if(this->font_load_success)
		this->actual_text.setFont(*this->text_font);
	this->time_phase = 0;
	this->reset_data(this->future_data);
	this->loaded_data.font_kind = this->future_data.font_kind;
}

TextRectangle::~TextRectangle() {
}


void TextRectangle::changeFont(FontKind new_font_kind) {
	if(this->future_data.font_kind != new_font_kind) {
		this->future_data.render_text = true;
		this->future_data.schedule_font_swap = true;
	}
	this->future_data.font_kind = new_font_kind;
}

void TextRectangle::setSize(int width, int height) {
	if((height == this->future_data.height) && (width == this->future_data.width))
		return;
	this->future_data.width = width;
	this->future_data.height = height;
	this->future_data.locked = false;
	this->future_data.render_text = true;
}

size_text_t TextRectangle::getFinalSize() {
	return size_text_t({this->future_data.stored_width, this->future_data.stored_height});
}

size_text_t TextRectangle::getFinalSizeNoMultipliers() {
	size_text_t size = this->getFinalSize();
	if(this->future_data.tight_and_centered)
		size.x = (int)(size.x / TEXT_RECTANGLE_POS_APPROX);
	return size;
}

void TextRectangle::setRectangleKind(TextKind kind) {
	if(this->future_data.kind != kind) {
		this->future_data.render_text = true;
	}
	this->future_data.kind = kind;
}

void TextRectangle::setTextFactor(float size_multiplier) {
	float new_pixel_height = BASE_PIXEL_FONT_HEIGHT * size_multiplier;
	if(this->future_data.font_pixel_height == new_pixel_height)
		return;
	this->future_data.font_pixel_height = new_pixel_height;
	this->future_data.locked = false;
	this->future_data.render_text = true;
}

void TextRectangle::setDuration(float on_seconds) {
	this->future_data.duration = on_seconds;
}

void TextRectangle::setLineSpacing(float new_line_spacing) {
	if(this->future_data.line_spacing != new_line_spacing)
		this->future_data.render_text = true;
	this->future_data.line_spacing = new_line_spacing;
}

void TextRectangle::setCharacterSpacing(float new_character_spacing) {
	if(this->future_data.character_spacing != new_character_spacing)
		this->future_data.render_text = true;
	this->future_data.character_spacing = new_character_spacing;
}

void TextRectangle::setPosition(int pos_x, int pos_y, TextPosKind kind) {
	if((pos_x == this->future_data.pos_x) && (pos_y == this->future_data.pos_y) && (kind == this->future_data.pos_kind))
		return;
	if(kind != POS_KIND_NORMAL)
		this->future_data.render_text = true;
	this->future_data.pos_x = pos_x;
	this->future_data.pos_y = pos_y;
	this->future_data.pos_kind = kind;
	this->future_data.locked = false;
}

position_text_t TextRectangle::getFinalPosition() {
	return position_text_t({this->future_data.stored_pos_x, this->future_data.stored_pos_y});
}

position_text_t TextRectangle::getFinalPositionNoMultipliers() {
	position_text_t pos = this->getFinalPosition();
	size_text_t full_size = this->getFinalSize();
	size_text_t base_size = this->getFinalSizeNoMultipliers();
	pos.x += (full_size.x - base_size.x) / 2;
	pos.y += (full_size.y - base_size.y) / 2;
	return pos;
}

void TextRectangle::setLocked(bool new_locked) {
	this->future_data.locked = new_locked;
}

percentage_pos_text_t TextRectangle::getCoordInRectanglePercentage(int coord_x, int coord_y) {
	percentage_pos_text_t percentage;
	percentage.x = PERCENTAGE_POS_TEXT_ERROR;
	percentage.y = PERCENTAGE_POS_TEXT_ERROR;
	if(!(this->isFontLoaded(this->future_data) && this->future_data.show_text))
		return percentage;
	int left = this->future_data.stored_pos_x;
	int right = left + this->future_data.stored_width;
	int up = this->future_data.stored_pos_y;
	int down = up + this->future_data.stored_height;
	if(coord_x < left)
		return percentage;
	if(coord_y < up)
		return percentage;
	if(coord_x >= right)
		return percentage;
	if(coord_y >= down)
		return percentage;
	percentage.x = ((coord_x - left) * 100) / (right - left);
	percentage.y = ((coord_y - up) * 100) / (down - up);
	return percentage;
}

bool TextRectangle::isCoordInRectangle(int coord_x, int coord_y) {
	percentage_pos_text_t result = this->getCoordInRectanglePercentage(coord_x, coord_y);
	return (result.x != PERCENTAGE_POS_TEXT_ERROR) && (result.y != PERCENTAGE_POS_TEXT_ERROR);
}

bool TextRectangle::isFontLoaded(TextData &reference_data) {
	if(reference_data.font_kind == FONT_KIND_NORMAL)
		return this->font_load_success;
	if(reference_data.font_kind == FONT_KIND_MONO)
		return this->font_mono_load_success;
	return false;
}

sf::Font* TextRectangle::getFont(TextData &reference_data) {
	if(reference_data.font_kind == FONT_KIND_NORMAL)
		return this->text_font;
	if(reference_data.font_kind == FONT_KIND_MONO)
		return this->text_font_mono;
	return NULL;
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

void TextRectangle::setTightAndCentered(bool tight_and_centered) {
	if(this->future_data.tight_and_centered != tight_and_centered) {
		this->future_data.render_text = true;
	}
	this->future_data.tight_and_centered = tight_and_centered;
}

void TextRectangle::prepareRenderText() {
	this->future_data.stored_width = this->loaded_data.stored_width;
	this->future_data.stored_height = this->loaded_data.stored_height;
	this->future_data.stored_pos_x = this->loaded_data.stored_pos_x;
	this->future_data.stored_pos_y = this->loaded_data.stored_pos_y;
	this->loaded_data = this->future_data;
	this->future_data.render_text = false;
	this->future_data.start_timer = false;
	this->future_data.schedule_font_swap = false;
}

void TextRectangle::setText(std::string text) {
	if(this->future_data.printed_text != text)
		this->future_data.render_text = true;
	this->future_data.printed_text = text;
}

void TextRectangle::setShowText(bool show_text) {
	this->future_data.show_text = show_text;
	if(!this->future_data.show_text)
		this->startTimer(false);
	if(this->future_data.show_text)
		this->future_data.render_text = true;
}

bool TextRectangle::getShowText() {
	return this->future_data.show_text;
}

void TextRectangle::draw(sf::RenderTarget &window) {
	if(this->loaded_data.schedule_font_swap && this->isFontLoaded(this->loaded_data)) {
		this->actual_text.setFont(*this->getFont(this->loaded_data));
	}
	if(this->loaded_data.render_text) {
		this->updateText((int)window.getView().getSize().x);
	}
	if(this->isFontLoaded(this->loaded_data) && this->loaded_data.show_text && (!this->is_done_showing_text)) {
		if(this->loaded_data.locked) {
			this->loaded_data.width = this->loaded_data.stored_width;
			this->loaded_data.height = this->loaded_data.stored_height;
			this->loaded_data.pos_x = this->loaded_data.stored_pos_x;
			this->loaded_data.pos_y = this->loaded_data.stored_pos_y;
			this->pos_x_center_contrib = 0;
			this->pos_y_center_contrib = 0;
		}
		this->loaded_data.pos_x += this->pos_x_center_contrib;
		this->loaded_data.pos_y += this->pos_y_center_contrib;
		this->text_rect.out_rect.setPosition({(float)this->loaded_data.pos_x, (float)this->loaded_data.pos_y});
		if(this->loaded_data.is_timed) {
			if(this->loaded_data.stored_height != 0)
				this->loaded_data.height = this->loaded_data.stored_height;

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
				rect_curr_height = -this->loaded_data.height + ((int)(factor * this->loaded_data.height));
			else if(this->time_phase == 2)
				rect_curr_height = ((int)(factor * this->loaded_data.height)) * -1;
			this->loaded_data.pos_y += rect_curr_height;
			this->text_rect.out_rect.setPosition({(float)this->loaded_data.pos_x, (float)this->loaded_data.pos_y});

			if(factor >= 1) {
				this->time_phase++;
				this->clock_time_start = std::chrono::high_resolution_clock::now();
			}
			if(this->time_phase >= 3)
				this->is_done_showing_text = true;
		}
		this->loaded_data.stored_pos_x = this->loaded_data.pos_x;
		this->loaded_data.stored_pos_y = this->loaded_data.pos_y;
		this->loaded_data.stored_width = this->loaded_data.width;
		this->loaded_data.stored_height = this->loaded_data.height;
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
	data.tight_and_centered = false;
	data.printed_text = "Sample Text";
	data.duration = 2.5;
	data.font_pixel_height = BASE_PIXEL_FONT_HEIGHT;
	data.width = 1;
	data.height = 1;
	data.pos_x = 0;
	data.pos_y = 0;
	data.font_kind = FONT_KIND_NORMAL;
	data.schedule_font_swap = false;
	data.line_spacing = 1.0;
	data.character_spacing = 1.0;
	data.stored_width = data.width;
	data.stored_height = data.height;
	data.stored_pos_x = data.pos_x;
	data.stored_pos_y = data.pos_y;
	data.locked = false;
	data.pos_kind = POS_KIND_NORMAL;
}

static sf::String convert_to_utf8(const std::string &str) {
	return sf::String::fromUtf8(str.begin(), str.end());
}

void TextRectangle::setTextWithLineWrapping(int x_limit) {
	if(x_limit == 0) {
		this->actual_text.setString(convert_to_utf8(this->loaded_data.printed_text));
		return;
	}
	bool is_done = false;
	bool line_started = false;
	std::string new_text = "";
	size_t pos = 0;
	while(!is_done) {
		std::string curr_text = new_text;
		if(line_started)
			curr_text += " ";
		size_t space_pos = this->loaded_data.printed_text.find(' ', pos);
		if(space_pos == std::string::npos) {
			is_done = true;
			space_pos = this->loaded_data.printed_text.length();
		}
		for(size_t i = pos; i < space_pos; i++)
			curr_text += this->loaded_data.printed_text[i];
		this->actual_text.setString(convert_to_utf8(curr_text));
		sf::FloatRect globalBounds = this->actual_text.getGlobalBounds();
		int bounds_width = (int)(globalBounds.size.x + (globalBounds.position.x * 2));
		if(line_started && (bounds_width > x_limit)) {
			curr_text = new_text + "\n";
			for(size_t i = pos; i < space_pos; i++)
				curr_text += this->loaded_data.printed_text[i];
		}
		pos = space_pos + 1;
		line_started = true;
		new_text = curr_text;
	}
	this->actual_text.setString(convert_to_utf8(new_text));
}

void TextRectangle::setRealSize(int width, int height, bool check_previous) {
	this->loaded_data.width = width;
	this->loaded_data.height = height;
	this->pos_x_center_contrib = 0;
	this->pos_y_center_contrib = 0;
	if(check_previous) {
		if((height == this->text_rect.out_rect.getSize().y) && (width == this->text_rect.out_rect.getSize().x))
			return;
	}
	this->text_rect.out_rect.setSize(sf::Vector2f((float)this->loaded_data.width, (float)this->loaded_data.height));
	(void)this->text_rect.out_tex.resize({(unsigned int)this->loaded_data.width, (unsigned int)this->loaded_data.height});
	this->text_rect.out_rect.setTextureRect(sf::IntRect({0, 0}, {this->loaded_data.width, this->loaded_data.height}));
}

float TextRectangle::getNoBlurCharacterSpacing() {
	const float whitespaceWidth = this->getFont(this->loaded_data)->getGlyph(U' ', this->actual_text.getCharacterSize(), false).advance;
    float pixels_letter_spacing = (float)round((whitespaceWidth / 3.0f) * (this->loaded_data.character_spacing - 1.0));
    return (float)((pixels_letter_spacing / (whitespaceWidth / 3.0f)) + 1.0);
}

void TextRectangle::updateText(int x_limit) {
	if(!this->loaded_data.locked) {
		// set the character size
		this->actual_text.setCharacterSize((unsigned int)this->loaded_data.font_pixel_height); // in pixels, not points!
		// set the text style
		//this->actual_text.setStyle(sf::Text::Bold);
		this->actual_text.setPosition({0, 0});
		this->setTextWithLineWrapping(x_limit);
		sf::FloatRect globalBounds;
		this->actual_text.setLineSpacing(this->loaded_data.line_spacing);
		this->actual_text.setLetterSpacing(this->getNoBlurCharacterSpacing());
		if(this->loaded_data.tight_and_centered) {
			globalBounds = this->actual_text.getGlobalBounds();
			int new_width = this->loaded_data.width;
			int new_height = this->loaded_data.height;
			int base_width = (int)(globalBounds.size.x + (globalBounds.position.x * 2));
			int bounds_width = (int)(base_width * TEXT_RECTANGLE_POS_APPROX);
			int base_height = (int)(globalBounds.size.y + (globalBounds.position.y * 2));
			int bounds_height = base_height;
			if(new_width < bounds_width) {
				bounds_width = new_width;
				base_width = new_width;
			}
			if(new_height < bounds_height) {
				bounds_height = new_height;
				base_height = new_width;
			}
			this->setRealSize(bounds_width, bounds_height);
			switch(this->loaded_data.pos_kind) {
				case POS_KIND_NORMAL:
					this->actual_text.setPosition({(float)((int)((this->loaded_data.width - (globalBounds.size.x + (globalBounds.position.x * 2))) / 2)), (float)((int)((this->loaded_data.height - (globalBounds.size.y + (globalBounds.position.y * 2))) / 2))});
					this->pos_x_center_contrib = (new_width - this->loaded_data.width) / 2;
					this->pos_y_center_contrib = (new_height - this->loaded_data.height) / 2;
					break;
				case POS_KIND_PRIORITIZE_TOP_LEFT:
					this->actual_text.setPosition({0, 0});
					this->pos_x_center_contrib = (base_width - this->loaded_data.width) / 2;
					this->pos_y_center_contrib = 0;
					break;
				case POS_KIND_PRIORITIZE_BOTTOM_LEFT:
					this->actual_text.setPosition({0, 0});
					this->pos_x_center_contrib = (base_width - this->loaded_data.width) / 2;
					this->pos_y_center_contrib = -this->loaded_data.height;
					break;
				default:
					break;
			}
		}
		else if(this->loaded_data.proportional_box) {
			globalBounds = this->actual_text.getGlobalBounds();
			int new_width = this->loaded_data.width;
			int new_height = this->loaded_data.height;
			int bounds_width = (int)(globalBounds.size.x + (globalBounds.position.x * 2));
			int bounds_height = (int)(globalBounds.size.y + (globalBounds.position.y * 2));
			if(new_width < bounds_width)
				new_width = bounds_width;
			if(new_height < bounds_height)
				new_height = bounds_height;
			this->setRealSize(new_width, new_height);
			this->actual_text.setPosition({(float)((int)(5 * (((float)this->loaded_data.font_pixel_height) / BASE_PIXEL_FONT_HEIGHT))), 0});
			globalBounds = this->actual_text.getGlobalBounds();
			this->setRealSize((int)(globalBounds.size.x + (globalBounds.position.x * 2)), (int)(globalBounds.size.y + (globalBounds.position.y * 2)));
		}
		else {
			this->setRealSize(this->loaded_data.width, this->loaded_data.height);
			this->actual_text.setPosition({0, 0});
			globalBounds = this->actual_text.getGlobalBounds();
			this->actual_text.setPosition({(float)((int)((this->loaded_data.width - (globalBounds.size.x + (globalBounds.position.x * 2))) / 2)), (float)((int)((this->loaded_data.height - (globalBounds.size.y + (globalBounds.position.y * 2))) / 2))});
		}
	}
	else {
		sf::Vector2f old_actual_text_pos = this->actual_text.getPosition();
		this->actual_text.setPosition({0, 0});
		this->setTextWithLineWrapping(x_limit);
		this->actual_text.setPosition(old_actual_text_pos);
	}
	sf::Color color_text = sf::Color(255, 255, 255, 255);
	switch(this->loaded_data.kind) {
		case TEXT_KIND_NORMAL:
			this->curr_color = sf::Color(40, 40, 80, 192);
			break;
		case TEXT_KIND_SELECTED:
			this->curr_color = sf::Color(90, 150, 210, 192);
			break;
		case TEXT_KIND_SUCCESS:
			this->curr_color = sf::Color(90, 210, 90, 192);
			break;
		case TEXT_KIND_WARNING:
			this->curr_color = sf::Color(200, 200, 90, 192);
			break;
		case TEXT_KIND_ERROR:
			this->curr_color = sf::Color(210, 90, 90, 192);
			break;
		case TEXT_KIND_OPAQUE_ERROR:
			this->curr_color = sf::Color(160, 60, 60, 192);
			break;
		case TEXT_KIND_TEXTBOX:
			this->curr_color = sf::Color(255, 255, 255, 255);
			color_text = sf::Color(0, 0, 0, 255);
			break;
		case TEXT_KIND_TEXTBOX_TRANSPARENT:
			this->curr_color = sf::Color(255, 255, 255, 0);
			color_text = sf::Color(0, 0, 0, 255);
			break;
		case TEXT_KIND_TITLE:
			this->curr_color = sf::Color(30, 30, 60, 0);
			break;
		default:
			break;
	}
	this->actual_text.setFillColor(color_text);
	this->text_rect.out_tex.clear(this->curr_color);
	this->text_rect.out_tex.draw(this->actual_text);
	this->text_rect.out_tex.display();
	this->is_done_showing_text = false;
	this->loaded_data.start_timer = this->loaded_data.is_timed;
}

// Only applies to text displayed with timer set to true
bool TextRectangle::isTimerTextDone() {
	return this->is_done_showing_text;
}

void TextRectangle::updateSlides(float* time_seconds) {
	time_seconds[0] = base_time_slide_factor * (this->loaded_data.height / (loaded_data.font_pixel_height * base_pixel_slide_factor));
	time_seconds[1] = loaded_data.duration;
	time_seconds[2] = base_time_slide_factor * (this->loaded_data.height / (loaded_data.font_pixel_height * base_pixel_slide_factor));
}

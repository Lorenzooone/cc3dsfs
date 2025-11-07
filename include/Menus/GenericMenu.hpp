#ifndef __GENERICMENU_HPP
#define __GENERICMENU_HPP

#include <SFML/Graphics.hpp>
#include "event_structs.hpp"

class GenericMenu {
public:
	virtual ~GenericMenu() {}
	virtual bool poll(SFEvent &event_data) = 0;
	virtual void draw(float scaling_factor, sf::RenderTarget &window) = 0;
	virtual void reset_data(bool full_reset) = 0;
	virtual void reset_output_option() = 0;
	virtual void on_menu_unloaded() {};
	bool is_inside_textbox = false;
};
#endif

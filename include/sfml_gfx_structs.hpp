#ifndef __SFML_GFX_STRUCTS_HPP
#define __SFML_GFX_STRUCTS_HPP

struct out_rect_data {
	sf::RectangleShape out_rect;
	sf::RenderTexture out_tex;
	sf::RenderTexture backup_tex;
	sf::RenderTexture* to_process_tex;
	sf::RenderTexture* to_backup_tex;
};

#endif

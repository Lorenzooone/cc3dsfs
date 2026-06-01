#include <SFML/Graphics.hpp>
#include "font_ttf.h"
#include "font_mono_ttf.h"
#include "TextRectangle.hpp"
#include "TextRectanglePool.hpp"
#include "utils.hpp"
#include "frontend.hpp"
#include <thread>
#include <mutex>

#define FPS_WINDOW_SIZE 64

#define NUM_NUMBERS 10
#define NUM_NOTIFICATIONS (3 + NUM_NUMBERS)

static double _FPSArrayGetAverage(FPSArray *array) {
	int available_fps = FPS_WINDOW_SIZE;
	if(array->index < available_fps)
		available_fps = array->index;
	if(available_fps == 0)
		return 0.0;
	double fps_sum = 0;
	for(int i = 0; i < available_fps; i++)
		fps_sum += array->data[i];
	return fps_sum / available_fps;
}

void print_notification(TextRectangle* notification, std::string text, TextKind kind, int x, int y, bool timed = false) {
	if (text == "")
		return;

	notification->setText(text);
	notification->setRectangleKind(kind);
	notification->startTimer(timed);
	notification->setShowText(true);
	notification->setPosition(x, y, POS_KIND_NORMAL);
}

static void print_notif_enabled(bool condition, TextRectangle* notification, std::string text, TextKind kind, int x, int y, bool timed = false) {
	std::string enabled_str = "Enabled";
	if(!condition)
		enabled_str = "Disabled";
	print_notification(notification, text + ": " + enabled_str, kind, x, y, timed);
}

static void counter_step(bool* to_close, int* counter, double* goal_frametime, ConsumerMutex* counter_lock) {
	std::chrono::time_point<std::chrono::high_resolution_clock> last_draw_time = std::chrono::high_resolution_clock::now();
	while(!(*to_close)) {
		*counter = ((*counter) + 1) % NUM_NUMBERS;
		counter_lock->unlock();

		auto curr_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = curr_time - last_draw_time;
		while(diff.count() < (*goal_frametime)) {
			default_sleep(((*goal_frametime) - diff.count()) * 1000.0);
			curr_time = std::chrono::high_resolution_clock::now();
			diff = curr_time - last_draw_time;
		}
		last_draw_time = curr_time;
	}
	counter_lock->unlock();
}

int main()
{
	double goal_frametime = 1.0 / 60.0;
	bool vsync_requested = false;
	bool vsync_enabled = false;
	bool target_framerate_requested = false;
	bool target_framerate = false;
	sf::Font text_font;
	sf::Font text_font_mono;
	bool font_load_success = text_font.openFromMemory(font_ttf, font_ttf_len);
	bool font_mono_load_success = text_font_mono.openFromMemory(font_mono_ttf, font_mono_ttf_len);
	TextRectangle* notifications[NUM_NOTIFICATIONS];
	for(int i = 0; i < NUM_NOTIFICATIONS; i++)
		notifications[i] = new TextRectangle(font_load_success, &text_font, font_mono_load_success, &text_font_mono);

	FPSArray draw_fps;
	FPSArrayInit(&draw_fps);
	std::chrono::time_point<std::chrono::high_resolution_clock> last_draw_time = std::chrono::high_resolution_clock::now();
	
	sf::RenderWindow window( sf::VideoMode( { 600, 400 } ), "SFML works!" );
	sf::CircleShape shape( 100.f );
	shape.setFillColor( sf::Color::Green );

	window.setVerticalSyncEnabled(vsync_requested);

	int counter = 0;
	bool to_close = false;
	ConsumerMutex counter_lock;
	std::thread counter_thread(counter_step, &to_close, &counter, &goal_frametime, &counter_lock);

	while ( window.isOpen() )
	{
		window.setMouseCursorVisible(true);
		if(vsync_enabled != vsync_requested) {
			window.setVerticalSyncEnabled(vsync_requested);
			vsync_enabled = vsync_requested;
			print_notif_enabled(vsync_enabled, notifications[1], "VSync", TEXT_KIND_NORMAL, 200, 0, true);
		}

		if(target_framerate != target_framerate_requested) {
			target_framerate = target_framerate_requested;
			print_notif_enabled(target_framerate, notifications[1], "60 FPS limit", TEXT_KIND_NORMAL, 200, 0, true);
		}

		while ( const std::optional event = window.pollEvent() )
		{
			if ( event->is<sf::Event::Closed>() )
				to_close = true;
			else if(const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
				if(keyPressed->code == sf::Keyboard::Key::Escape)
					to_close = true;
			}
			else if(const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
				if(textEntered->unicode == 'v')
					vsync_requested = !vsync_requested;
				if(textEntered->unicode == 'f')
					target_framerate_requested = !target_framerate_requested;
			}
		}

		int curr_counter = counter;
		print_notification(notifications[0], get_float_str_decimals(_FPSArrayGetAverage(&draw_fps), 2), TEXT_KIND_NORMAL, 0, 0);

		TextKind kind_num_fixed = TEXT_KIND_SUCCESS;
		if(curr_counter % 2)
			kind_num_fixed = TEXT_KIND_ERROR;
		print_notification(notifications[2], "TEST", kind_num_fixed, 0, 300);

		for(int i = 0; i < NUM_NUMBERS; i++) {
			TextKind kind_num = TEXT_KIND_NORMAL;
			if(curr_counter == i)
				kind_num = TEXT_KIND_SUCCESS;
			print_notification(notifications[i + NUM_NOTIFICATIONS - NUM_NUMBERS], std::to_string(i), kind_num, i * 50, 200);
		}
		auto pre_curr_time = std::chrono::high_resolution_clock::now();
		for(int i = 0; i < NUM_NOTIFICATIONS; i++)
			notifications[i]->prepareRenderText();
		window.clear();
		window.draw( shape );
		for(int i = 0; i < NUM_NOTIFICATIONS; i++)
			notifications[i]->draw(window);
		window.display();

		auto curr_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = curr_time - pre_curr_time;
		printf("RENDER TIME: %f\n", diff.count());
		diff = curr_time - last_draw_time;
		FPSArrayInsertElement(&draw_fps, diff.count());
		last_draw_time = curr_time;

		if(target_framerate) {
			bool timed_out = !counter_lock.timed_lock();
		}

		if(to_close)
			window.close();
	}
	
	counter_thread.join();
}

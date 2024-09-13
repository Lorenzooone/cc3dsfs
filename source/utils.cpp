#include "utils.hpp"

#if defined (__linux__) && defined(XLIB_BASED)
#include <X11/Xlib.h>
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <cmath>

#define xstr(a) str(a)
#define str(a) #a

#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 1
#define APP_VERSION_REVISION 1
#ifdef RASPI
#define APP_VERSION_LETTER R
#else
#define APP_VERSION_LETTER M
#endif

static bool checked_be_once = false;
static bool _is_be = false;

bool is_big_endian(void) {
	if(checked_be_once)
		return _is_be;
    union {
        uint32_t i;
        char c[4];
    } value = {0x01020304};

	checked_be_once = true;
	_is_be = value.c[0] == 1;
    return _is_be;
}

uint32_t to_le(uint32_t value) {
	if(is_big_endian())
		value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
	return value;
}

uint32_t to_be(uint32_t value) {
	if(!is_big_endian())
		value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
	return value;
}

uint16_t to_le(uint16_t value) {
	if(is_big_endian())
		value = ((value & 0xFF) << 8) | ((value & 0xFF00) >> 8);
	return value;
}

uint16_t to_be(uint16_t value) {
	if(!is_big_endian())
		value = ((value & 0xFF) << 8) | ((value & 0xFF00) >> 8);
	return value;
}

void init_threads(void) {
	#if defined(__linux__) && defined(XLIB_BASED)
	XInitThreads();
	#endif
}

std::string get_version_string(bool get_letter) {
	std::string version_str = std::to_string(APP_VERSION_MAJOR) + "." + std::to_string(APP_VERSION_MINOR) + "." + std::to_string(APP_VERSION_REVISION);
	if(get_letter)
		return version_str + xstr(APP_VERSION_LETTER);
	return version_str;
}

std::string get_float_str_decimals(float value, int decimals) {
	float approx_factor = pow(0.1, decimals) * (0.5);
	int int_part = (int)(value + approx_factor);
	int dec_part = (int)((value + approx_factor - int_part) * pow(10, decimals));
	std::string return_text = std::to_string(int_part);

	if(decimals > 0) {
		if(!dec_part) {
			return_text += ".";
			for(int i = 0; i < decimals; i++)
				return_text += "0";
		}
		else {
			return_text += ".";
			for(int i = 0; i < decimals; i++)
				return_text += std::to_string((dec_part % ((int)pow(10, decimals - i))) / ((int)pow(10, decimals - i - 1)));
		}
	}
	
	return return_text;
}

std::string LayoutNameGenerator(int index) {
	if(index == STARTUP_FILE_INDEX)
		return std::string(NAME) + ".cfg";
	return "layout" + std::to_string(index) + ".cfg";
}

std::string LayoutPathGenerator(int index) {
	bool success = false;
	std::string cfg_dir;
	#if !(defined(_WIN32) || defined(_WIN64))
	if(const char* env_p = std::getenv("HOME")) {
		cfg_dir = std::string(env_p) + "/.config/" + std::string(NAME);
		success = true;
	}
	#endif
	if(!success)
		cfg_dir = ".config/" + std::string(NAME);
	if(index == STARTUP_FILE_INDEX)
		return cfg_dir + "/";
	return cfg_dir + "/presets/";
}

std::string load_layout_name(int index, bool &success) {
	if(index == STARTUP_FILE_INDEX) {
		success = true;
		return "Initial";
	}
	std::string name = LayoutNameGenerator(index);
	std::string path = LayoutPathGenerator(index);

	std::ifstream file(path + name);
	std::string line;
	success = false;

	if(!file.good()) {
		return std::to_string(index);
	}

	success = true;
	try {
		while(std::getline(file, line)) {
			std::istringstream kvp(line);
			std::string key;

			if(std::getline(kvp, key, '=')) {
				std::string value;
				if(std::getline(kvp, value)) {

					if(key == "name") {
						file.close();
						return value;
					}
				}
			}
		}
	}
	catch(...) {
		success = false;
	}

	file.close();
	return std::to_string(index);
}
//============================================================================

ConsumerMutex::ConsumerMutex() {
	count = 0;
}
	
void ConsumerMutex::lock() {
	access_mutex.lock();
	bool success = false;
	while(!success) {
		if(count) {
			count--;
			success = true;
		}
		else {
			condition.wait(access_mutex);
		}
	}
	access_mutex.unlock();
}
	
bool ConsumerMutex::timed_lock() {
	access_mutex.lock();
	bool success = false;
	while(!success) {
		if(count) {
			count--;
			success = true;
		}
		else {
			auto result = condition.wait_for(access_mutex, max_timed_wait);
			if((result == std::cv_status::timeout) && (!count))
				break;
		}
	}
	access_mutex.unlock();
	return success;
}
	
bool ConsumerMutex::try_lock() {
	access_mutex.lock();
	bool success = false;
	if(count) {
		count--;
		success = true;
	}
	access_mutex.unlock();
	return success;
}
	
void ConsumerMutex::unlock() {
	access_mutex.lock();
	// Enforce 1 max
	count = 1;
	condition.notify_one();
	access_mutex.unlock();
}

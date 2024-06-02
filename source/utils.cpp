#include "utils.hpp"

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
#define APP_VERSION_MINOR 0
#define APP_VERSION_REVISION 0
#ifdef RASPI
#define APP_VERSION_LETTER R
#else
#define APP_VERSION_LETTER M
#endif

bool is_big_endian(void) {
    union {
        uint32_t i;
        char c[4];
    } value = {0x01020304};

    return value.c[0] == 1;
}

std::string get_version_string() {
	return std::to_string(APP_VERSION_MAJOR) + "." + std::to_string(APP_VERSION_MINOR) + "." + std::to_string(APP_VERSION_REVISION) + xstr(APP_VERSION_LETTER);
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

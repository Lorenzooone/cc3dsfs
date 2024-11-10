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

static uint32_t reverse_endianness(uint32_t value) {
	return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

static uint16_t reverse_endianness(uint16_t value) {
	return ((value & 0xFF) << 8) | ((value & 0xFF00) >> 8);
}

uint32_t to_le(uint32_t value) {
	if(is_big_endian())
		value = reverse_endianness(value);
	return value;
}

uint32_t to_be(uint32_t value) {
	if(!is_big_endian())
		value = reverse_endianness(value);
	return value;
}

uint16_t to_le(uint16_t value) {
	if(is_big_endian())
		value = reverse_endianness(value);
	return value;
}

uint16_t to_be(uint16_t value) {
	if(!is_big_endian())
		value = reverse_endianness(value);
	return value;
}

uint32_t from_le(uint32_t value) {
	if(is_big_endian())
		value = reverse_endianness(value);
	return value;
}

uint32_t from_be(uint32_t value) {
	if(!is_big_endian())
		value = reverse_endianness(value);
	return value;
}

uint16_t from_le(uint16_t value) {
	if(is_big_endian())
		value = reverse_endianness(value);
	return value;
}

uint16_t from_be(uint16_t value) {
	if(!is_big_endian())
		value = reverse_endianness(value);
	return value;
}

std::string to_hex(uint16_t value) {
	const int num_digits = sizeof(value) * 2;
	char digits[num_digits];
	for(int i = 0; i < num_digits; i++) {
		uint8_t subvalue = (value >> (4 * (num_digits - 1 - i))) & 0xF;
		char digit = '0' + subvalue;
		if(subvalue >= 0xA)
			digit = 'A' + (subvalue - 0xA);
		digits[i] = digit;
	}
	return static_cast<std::string>(digits);
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

std::string LayoutPathGenerator(int index, bool created_proper_folder) {
	bool success = false;
	std::string cfg_dir;
	#if !(defined(_WIN32) || defined(_WIN64))
	const char* env_p = std::getenv("HOME");
	if(created_proper_folder && env_p) {
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

std::string load_layout_name(int index, bool created_proper_folder, bool &success) {
	if(index == STARTUP_FILE_INDEX) {
		success = true;
		return "Initial";
	}
	std::string name = LayoutNameGenerator(index);
	std::string path = LayoutPathGenerator(index, created_proper_folder);

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

void ConsumerMutex::update_time_multiplier(float time_multiplier) {
	if (time_multiplier <= 0)
		return;
	this->time_multiplier = time_multiplier;
}

double ConsumerMutex::get_time_s() {
	return 1.0 / ((base_time_fps) * (1.0 / this->time_multiplier));
}

void ConsumerMutex::lock() {
	access_mutex.lock();
	bool success = false;
	while (!success) {
		if (count) {
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
	std::chrono::duration<double>max_timed_wait = std::chrono::duration<double>(this->get_time_s());
	access_mutex.lock();
	bool success = false;
	while (!success) {
		if (count) {
			count--;
			success = true;
		}
		else {
			auto result = condition.wait_for(access_mutex, max_timed_wait);
			if ((result == std::cv_status::timeout) && (!count))
				break;
		}
	}
	access_mutex.unlock();
	return success;
}

bool ConsumerMutex::try_lock() {
	access_mutex.lock();
	bool success = false;
	if (count) {
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
	condition.notify_all();
	access_mutex.unlock();
}

//============================================================================

SharedConsumerMutex::SharedConsumerMutex(int num_elements) {
	this->num_elements = num_elements;
	if(this->num_elements <= 0)
		this->num_elements = 1;
	this->counts = new int[this->num_elements];
	for(int i = 0; i < this->num_elements; i++)
		this->counts[i] = 0;
}

SharedConsumerMutex::~SharedConsumerMutex() {
	delete []this->counts;
}

void SharedConsumerMutex::update_time_multiplier(float time_multiplier) {
	if (time_multiplier <= 0)
		return;
	this->time_multiplier = time_multiplier;
}

double SharedConsumerMutex::get_time_s() {
	return 1.0 / ((base_time_fps) * (1.0 / this->time_multiplier));
}

void SharedConsumerMutex::general_lock(int* index) {
	access_mutex.lock();
	bool success = false;
	while (!success) {
		for (int i = 0; i < num_elements; i++)
			if (counts[i]) {
				counts[i]--;
				success = true;
				*index = i;
				break;
			}
		if (!success)
			condition.wait(access_mutex);
	}
	access_mutex.unlock();
}

bool SharedConsumerMutex::general_timed_lock(int* index) {
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_end = clock_start + std::chrono::nanoseconds((int)(this->get_time_s() * 1000 * 1000));
	access_mutex.lock();
	bool success = false;
	auto result = std::cv_status::no_timeout;
	while (!success) {
		for (int i = 0; i < num_elements; i++)
			if (counts[i]) {
				counts[i]--;
				success = true;
				*index = i;
				break;
			}
		if(!success) {
			const auto curr_time = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double>timed_wait = clock_end - curr_time;
			if (curr_time >= clock_end)
				result = std::cv_status::timeout;
			if (result == std::cv_status::timeout)
				break;
			result = condition.wait_for(access_mutex, timed_wait);
		}
	}
	access_mutex.unlock();
	return success;
}

bool SharedConsumerMutex::general_try_lock(int* index) {
	access_mutex.lock();
	bool success = false;
	for (int i = 0; i < num_elements; i++)
		if (counts[i]) {
			counts[i]--;
			success = true;
			*index = i;
			break;
		}
	access_mutex.unlock();
	return success;
}

void SharedConsumerMutex::specific_unlock(int index) {
	if ((index < 0) || (index >= num_elements))
		return;
	access_mutex.lock();
	// Enforce 1 max
	counts[index] = 1;
	condition.notify_all();
	access_mutex.unlock();
}

void SharedConsumerMutex::specific_lock(int index) {
	if ((index < 0) || (index >= num_elements))
		return;
	access_mutex.lock();
	bool success = false;
	while (!success) {
		if(counts[index]) {
			counts[index]--;
			success = true;
		}
		else
			condition.wait(access_mutex);
	}
	access_mutex.unlock();
}

bool SharedConsumerMutex::specific_timed_lock(int index) {
	if ((index < 0) || (index >= num_elements))
		return false;
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_start = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_end = clock_start + std::chrono::nanoseconds((int)(this->get_time_s() * 1000 * 1000));
	access_mutex.lock();
	bool success = false;
	auto result = std::cv_status::no_timeout;
	while (!success) {
		if (counts[index]) {
			counts[index]--;
			success = true;
			break;
		}
		if (!success) {
			const auto curr_time = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double>timed_wait = clock_end - curr_time;
			if (curr_time >= clock_end)
				result = std::cv_status::timeout;
			if (result == std::cv_status::timeout)
				break;
			result = condition.wait_for(access_mutex, timed_wait);
		}
	}
	access_mutex.unlock();
	return success;
}

bool SharedConsumerMutex::specific_try_lock(int index) {
	if((index < 0) || (index >= num_elements))
		return false;
	access_mutex.lock();
	bool success = false;
	if (counts[index]) {
		counts[index]--;
		success = true;
	}
	access_mutex.unlock();
	return success;
}

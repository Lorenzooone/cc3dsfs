#include "utils.hpp"

#if defined (__linux__) && defined(XLIB_BASED)
#include <X11/Xlib.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <queue>
#include <cmath>

#ifdef SFML_SYSTEM_ANDROID
// Since we want to get the native activity from SFML, we'll have to use an
// extra header here:
#include <SFML/System/NativeActivity.hpp>
#endif
#ifdef ANDROID_COMPILATION
#include <android/log.h>
// These headers are only needed for direct NDK/JDK interaction
#include <android/native_activity.h>
#endif

#define xstr(a) str(a)
#define str(a) #a

#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 2
#define APP_VERSION_REVISION 0
#ifdef RASPI
#define APP_VERSION_LETTER R
#else
#define APP_VERSION_LETTER M
#endif

static bool checked_be_once = false;
static bool _is_be = false;
std::chrono::time_point<std::chrono::high_resolution_clock> clock_start_program;

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

uint32_t reverse_endianness(uint32_t value) {
	return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

uint16_t reverse_endianness(uint16_t value) {
	return ((value & 0xFF) << 8) | ((value & 0xFF00) >> 8);
}

static uint64_t reverse_ux(uint64_t data, int bits_num) {
	uint64_t out_data = 0;
	for(int i = 0; i < bits_num; i++)
		out_data |= ((data >> i) & 1) << (bits_num - 1 - i);
	return out_data;
}

uint8_t reverse_u8(uint8_t data) {
	return (uint8_t)reverse_ux(data, sizeof(uint8_t) * 8);
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

uint16_t read_le16(const uint8_t* data, size_t count, size_t multiplier) {
	data += count * multiplier;
	return data[0] | (data[1] << 8);
}

uint16_t read_be16(const uint8_t* data, size_t count, size_t multiplier) {
	data += count * multiplier;
	return data[1] | (data[0] << 8);
}

uint32_t read_le32(const uint8_t* data, size_t count, size_t multiplier) {
	data += count * multiplier;
	return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

uint32_t read_be32(const uint8_t* data, size_t count, size_t multiplier) {
	data += count * multiplier;
	return data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
}

uint64_t read_le64(const uint8_t* data, size_t count, size_t multiplier) {
	data += count * multiplier;
	return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24) | (((uint64_t)data[4]) << 32) | (((uint64_t)data[5]) << 40) | (((uint64_t)data[6]) << 48) | (((uint64_t)data[7]) << 56);
}

uint64_t read_be64(const uint8_t* data, size_t count, size_t multiplier) {
	data += count * multiplier;
	return data[7] | (data[6] << 8) | (data[5] << 16) | (data[4] << 24) | (((uint64_t)data[3]) << 32) | (((uint64_t)data[2]) << 40) | (((uint64_t)data[1]) << 48) | (((uint64_t)data[0]) << 56);
}

void write_le16(uint8_t* data, uint16_t value, size_t count, size_t multiplier) {
	data += count * multiplier;
	data[0] = value & 0xFF;
	data[1] = (value >> 8) & 0xFF;
}

void write_be16(uint8_t* data, uint16_t value, size_t count, size_t multiplier) {
	data += count * multiplier;
	data[0] = (value >> 8) & 0xFF;
	data[1] = value & 0xFF;
}

void write_le32(uint8_t* data, uint32_t value, size_t count, size_t multiplier) {
	data += count * multiplier;
	data[0] = value & 0xFF;
	data[1] = (value >> 8) & 0xFF;
	data[2] = (value >> 16) & 0xFF;
	data[3] = (value >> 24) & 0xFF;
}

void write_be32(uint8_t* data, uint32_t value, size_t count, size_t multiplier) {
	data += count * multiplier;
	data[0] = (value >> 24) & 0xFF;
	data[1] = (value >> 16) & 0xFF;
	data[2] = (value >> 8) & 0xFF;
	data[3] = value & 0xFF;
}

void write_le64(uint8_t* data, uint64_t value, size_t count, size_t multiplier) {
	data += count * multiplier;
	data[0] = value & 0xFF;
	data[1] = (value >> 8) & 0xFF;
	data[2] = (value >> 16) & 0xFF;
	data[3] = (value >> 24) & 0xFF;
	data[4] = (value >> 32) & 0xFF;
	data[5] = (value >> 40) & 0xFF;
	data[6] = (value >> 48) & 0xFF;
	data[7] = (value >> 56) & 0xFF;
}

void write_be64(uint8_t* data, uint64_t value, size_t count, size_t multiplier) {
	data += count * multiplier;
	data[0] = (value >> 56) & 0xFF;
	data[1] = (value >> 48) & 0xFF;
	data[2] = (value >> 40) & 0xFF;
	data[3] = (value >> 32) & 0xFF;
	data[4] = (value >> 24) & 0xFF;
	data[5] = (value >> 16) & 0xFF;
	data[6] = (value >> 8) & 0xFF;
	data[7] = value & 0xFF;
}

void write_string(uint8_t* data, std::string text) {
	for(size_t i = 0; i < text.size(); i++)
		data[i] = (uint8_t)text[i];
}

std::string read_string(uint8_t* data, size_t size) {
	uint8_t* new_data = new uint8_t[size + 1];
	memcpy(new_data, data, size);
	new_data[size] = '\0';
	std::string out = std::string((const char*)new_data);
	delete []new_data;
	return out;
}

uint32_t rotate_bits_left(uint32_t value) {
	return (value << 1) | ((value & 0x80000000) >> 31);
}

uint32_t rotate_bits_right(uint32_t value) {
	return (value >> 1) | ((value & 1) << 31);
}

void init_start_time() {
	clock_start_program = std::chrono::high_resolution_clock::now();
}

uint32_t ms_since_start() {
	const auto curr_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double>diff = curr_time - clock_start_program;
	return (uint32_t)(diff.count() * 1000);
}

static std::string to_hex_base(const uint64_t value, char* digits, const int num_digits) {
	for(int i = 0; i < num_digits; i++) {
		uint8_t subvalue = (value >> (4 * (num_digits - 1 - i))) & 0xF;
		char digit = '0' + subvalue;
		if(subvalue >= 0xA)
			digit = 'A' + (subvalue - 0xA);
		digits[i] = digit;
	}
	digits[num_digits] = '\0';
	return static_cast<std::string>(digits);
}

std::string to_hex_u8(const uint8_t value) {
	const int num_digits = sizeof(value) * 2;
	char digits[num_digits + 1];
	return to_hex_base(value, digits, num_digits);
}

std::string to_hex_u16(const uint16_t value) {
	const int num_digits = sizeof(value) * 2;
	char digits[num_digits + 1];
	return to_hex_base(value, digits, num_digits);
}

std::string to_hex_u32(const uint32_t value) {
	const int num_digits = sizeof(value) * 2;
	char digits[num_digits + 1];
	return to_hex_base(value, digits, num_digits);
}

std::string to_hex_u60(const uint64_t value) {
	const int num_digits = (sizeof(value) * 2) - 1;
	char digits[num_digits + 1];
	return to_hex_base(value, digits, num_digits);
}

std::string to_hex_u64(const uint64_t value) {
	const int num_digits = sizeof(value) * 2;
	char digits[num_digits + 1];
	return to_hex_base(value, digits, num_digits);
}

void init_threads(void) {
	#if defined(__linux__) && defined(XLIB_BASED)
	XInitThreads();
	#endif
	/*
	#ifdef _WIN32
	timeBeginPeriod(4);
	#endif
	*/
}

void complete_threads(void) {
	/*
	#ifdef _WIN32
	timeEndPeriod(4);
	#endif
	*/
}

std::string get_version_string(bool get_letter) {
	std::string version_str = std::to_string(APP_VERSION_MAJOR) + "." + std::to_string(APP_VERSION_MINOR) + "." + std::to_string(APP_VERSION_REVISION);
	if(get_letter)
		return version_str + xstr(APP_VERSION_LETTER);
	return version_str;
}

std::string get_float_str_decimals(float value, int decimals) {
	float approx_factor = (float)(pow(0.1f, decimals) * (0.5f));
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

void ActualConsoleOutTextError(std::string out_string) {
	#ifdef ANDROID_COMPILATION
	__android_log_print(ANDROID_LOG_ERROR, NAME, "%s", out_string.c_str());
	#else
	std::cerr << out_string << std::endl;
	#endif
}

void ActualConsoleOutText(std::string out_string) {
	#ifdef ANDROID_COMPILATION
	__android_log_print(ANDROID_LOG_INFO, NAME, "%s", out_string.c_str());
	#else
	std::cout << out_string << std::endl;
	#endif
}

std::string LayoutNameGenerator(int index) {
	if(index == STARTUP_FILE_INDEX)
		return std::string(NAME) + ".cfg";
	return "layout" + std::to_string(index) + ".cfg";
}

#ifdef ANDROID_COMPILATION
ANativeActivity* getAndroidNativeActivity() {
	#ifdef SFML_SYSTEM_ANDROID
	return sf::getNativeActivity();
	#else
	return NULL;
	#endif
}
#endif

std::string get_base_path(bool for_presets, bool do_existence_check) {
	std::string cfg_dir = "./";
	const char* env_p = NULL;
	bool add_config = true;

	#ifdef ANDROID_COMPILATION
	ANativeActivity* native_activity_ptr = getAndroidNativeActivity();
	if(native_activity_ptr)
		env_p = native_activity_ptr->internalDataPath;
	#elif !(defined(_WIN32) || defined(_WIN64))
	std::string folder_env = std::string(NAME) + "_CFG_DIR";
	std::transform(folder_env.begin(), folder_env.end(), folder_env.begin(), ::toupper);
	env_p = std::getenv(folder_env.c_str());
	if(env_p == NULL)
		env_p = std::getenv("HOME");
	else
		add_config = false;
	#endif

	if(env_p != NULL) {
		std::string target_folder = std::string(env_p);
		if((target_folder != "")  && ((!do_existence_check) || std::filesystem::is_directory(std::filesystem::path(target_folder))))
			cfg_dir = std::string(env_p) + "/";
	}

	if(add_config)
		cfg_dir += ".config/" + std::string(NAME);

	if(!for_presets)
		return cfg_dir + "/";
	return cfg_dir + "/presets/";
}

std::string get_base_path_keys() {
	return get_base_path(false) + "keys/";
}

std::string LayoutPathGenerator(int index) {
	return get_base_path(index != STARTUP_FILE_INDEX);
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
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_end = clock_start + std::chrono::microseconds((int)(this->get_time_s() * 1000 * 1000));
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
	std::chrono::time_point<std::chrono::high_resolution_clock> clock_end = clock_start + std::chrono::microseconds((int)(this->get_time_s() * 1000 * 1000));
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

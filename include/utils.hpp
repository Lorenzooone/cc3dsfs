#ifndef __UTILS_HPP
#define __UTILS_HPP

#include <mutex>
#include <condition_variable>
#include <chrono>

#ifdef SFML_SYSTEM_ANDROID
#define ANDROID_COMPILATION
#else
// Possible to add more stuff here...
#ifdef ANDROID_COMPILATION
#undef ANDROID_COMPILATION
#endif
#endif

#ifdef ANDROID_COMPILATION
// These headers are needed for direct NDK/JDK interaction
#include <android/native_activity.h>
#include <android/log.h>
#endif

#if _MSC_VER && !__INTEL_COMPILER
#define PACKED
#define ALIGNED(x) alignas(x)
#define STDCALL __stdcall
#else
#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#define STDCALL
#endif

#define NAME "cc3dsfs"

#ifdef ANDROID_COMPILATION
#define PRINT_FUNCTION(...) __android_log_print(ANDROID_LOG_INFO, NAME, __VA_ARGS__)
#else
#define PRINT_FUNCTION(...) printf(__VA_ARGS__)
#endif

// This isn't precise, however we can use it...
// Use these to properly sleep, with small wakes to check if data is ready
#define USB_FPS 60
#define BAD_USB_FPS 25
#define USB_NUM_CHECKS 1
#define USB_CHECKS_PER_SECOND ((USB_FPS + (USB_FPS / 12)) * USB_NUM_CHECKS)

#define STARTUP_FILE_INDEX -1
#define SIMPLE_RESET_DATA_INDEX -2
#define CREATE_NEW_FILE_INDEX -3

#ifdef ANDROID_COMPILATION
ANativeActivity* getAndroidNativeActivity();
#endif
std::string get_version_string(bool get_letter = true);
void ActualConsoleOutTextError(std::string out_string);
void ActualConsoleOutText(std::string out_string);
std::string LayoutNameGenerator(int index);
std::string LayoutPathGenerator(int index, bool created_proper_folder);
std::string load_layout_name(int index, bool created_proper_folder, bool &success);
uint32_t reverse_endianness(uint32_t value);
uint16_t reverse_endianness(uint16_t value);
bool is_big_endian(void);
uint32_t to_le(uint32_t value);
uint16_t to_le(uint16_t value);
uint32_t to_be(uint32_t value);
uint16_t to_be(uint16_t value);
uint32_t from_le(uint32_t value);
uint16_t from_le(uint16_t value);
uint32_t from_be(uint32_t value);
uint16_t from_be(uint16_t value);
uint16_t read_le16(const uint8_t* data, size_t count = 0, size_t multiplier = sizeof(uint16_t));
uint16_t read_be16(const uint8_t* data, size_t count = 0, size_t multiplier = sizeof(uint16_t));
uint32_t read_le32(const uint8_t* data, size_t count = 0, size_t multiplier = sizeof(uint32_t));
uint32_t read_be32(const uint8_t* data, size_t count = 0, size_t multiplier = sizeof(uint32_t));
uint64_t read_le64(const uint8_t* data, size_t count = 0, size_t multiplier = sizeof(uint64_t));
uint64_t read_be64(const uint8_t* data, size_t count = 0, size_t multiplier = sizeof(uint64_t));
void write_le16(uint8_t* data, uint16_t value, size_t count = 0, size_t multiplier = sizeof(uint16_t));
void write_be16(uint8_t* data, uint16_t value, size_t count = 0, size_t multiplier = sizeof(uint16_t));
void write_le32(uint8_t* data, uint32_t value, size_t count = 0, size_t multiplier = sizeof(uint32_t));
void write_be32(uint8_t* data, uint32_t value, size_t count = 0, size_t multiplier = sizeof(uint32_t));
void write_le64(uint8_t* data, uint64_t value, size_t count = 0, size_t multiplier = sizeof(uint64_t));
void write_be64(uint8_t* data, uint64_t value, size_t count = 0, size_t multiplier = sizeof(uint64_t));
void write_string(uint8_t* data, std::string text);
std::string read_string(uint8_t* data, size_t size);
uint32_t rotate_bits_left(uint32_t value);
uint32_t rotate_bits_right(uint32_t value);
void init_start_time();
uint32_t ms_since_start();
std::string to_hex(uint16_t value);
std::string get_float_str_decimals(float value, int decimals);
void init_threads(void);
void complete_threads(void);

class ConsumerMutex {
public:
	ConsumerMutex();
	void lock();
	bool timed_lock();
	bool try_lock();
	void unlock();
	void update_time_multiplier(float time_multiplier);
	double get_time_s();

private:
	std::mutex access_mutex;
	std::condition_variable_any condition;
	const float base_time_fps = 30;
	int count = 0;
	float time_multiplier = 1.0f;
};

class SharedConsumerMutex {
public:
	SharedConsumerMutex(int num_elements);
	~SharedConsumerMutex();
	void general_lock(int* index);
	bool general_timed_lock(int* index);
	bool general_try_lock(int* index);
	void specific_lock(int index);
	bool specific_timed_lock(int index);
	bool specific_try_lock(int index);
	void specific_unlock(int index);
	void update_time_multiplier(float time_multiplier);
	double get_time_s();

private:
	std::mutex access_mutex;
	std::condition_variable_any condition;
	const float base_time_fps = 30;
	int* counts;
	int num_elements;
	float time_multiplier = 1.0f;
};

#endif

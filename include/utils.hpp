#ifndef __UTILS_HPP
#define __UTILS_HPP

#include <mutex>
#include <condition_variable>
#include <chrono>

#if _MSC_VER && !__INTEL_COMPILER
#define PACKED
#else
#define PACKED __attribute__((packed))
#endif

#define NAME "cc3dsfs"

// This isn't precise, however we can use it...
// Use these to properly sleep, with small wakes to check if data is ready
#define USB_FPS 60
#define BAD_USB_FPS 25
#define USB_NUM_CHECKS 1
#define USB_CHECKS_PER_SECOND ((USB_FPS + (USB_FPS / 12)) * USB_NUM_CHECKS)

#define STARTUP_FILE_INDEX -1
#define SIMPLE_RESET_DATA_INDEX -2
#define CREATE_NEW_FILE_INDEX -3

std::string get_version_string(bool get_letter = true);
std::string LayoutNameGenerator(int index);
std::string LayoutPathGenerator(int index);
std::string load_layout_name(int index, bool &success);
bool is_big_endian(void);
uint32_t to_le(uint32_t value);
uint32_t to_be(uint32_t value);
uint16_t to_le(uint16_t value);
uint16_t to_be(uint16_t value);
uint32_t from_le(uint32_t value);
uint32_t from_be(uint32_t value);
uint16_t from_le(uint16_t value);
uint16_t from_be(uint16_t value);
std::string get_float_str_decimals(float value, int decimals);
void init_threads(void);

class ConsumerMutex {
public:
	ConsumerMutex();
	void lock();
	bool timed_lock();
	bool try_lock();
	void unlock();

private:
	std::mutex access_mutex;
	std::condition_variable_any condition;
	const std::chrono::duration<double>max_timed_wait = std::chrono::duration<double>(1.0 / 30);
	int count = 0;
};

#endif

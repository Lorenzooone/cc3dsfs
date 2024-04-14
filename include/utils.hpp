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
#define USB_NUM_CHECKS 3
#define USB_CHECKS_PER_SECOND ((USB_FPS + (USB_FPS / 12)) * USB_NUM_CHECKS)

bool is_big_endian(void);

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
	const std::chrono::duration<double>max_timed_wait = std::chrono::duration<double>(1.0 / 45);
	int count = 0;
};

#endif

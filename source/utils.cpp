#include "utils.hpp"

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <cmath>

bool is_big_endian(void) {
    union {
        uint32_t i;
        char c[4];
    } value = {0x01020304};

    return value.c[0] == 1;
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
		else
			return_text += "." + std::to_string(dec_part);
	}
	
	return return_text;
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

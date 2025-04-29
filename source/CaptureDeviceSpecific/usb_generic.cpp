#include "usb_generic.hpp"
#include <thread>
#include <mutex>

#ifdef SFML_SYSTEM_ANDROID
// These headers are only needed for direct NDK/JDK interaction
#include <android/native_activity.h>
#include <jni.h>
// Since we want to get the native activity from SFML, we'll have to use an
// extra header here:
#include <SFML/System/NativeActivity.hpp>
#endif

static bool usb_initialized = false;
static libusb_context* usb_ctx = NULL; // libusb session context
static int usb_thread_registered = 0;
std::thread usb_thread;
std::mutex usb_thread_mutex;

void usb_init() {
	if(usb_initialized)
		return;
	#ifdef SFML_SYSTEM_ANDROID
	// First we'll need the native activity handle
	ANativeActivity& activity = *sf::getNativeActivity();
	// Retrieve the JVM and JNI environment
	JavaVM& vm  = *activity.vm;
	JNIEnv& env = *activity.env;
	libusb_set_option(usb_ctx, LIBUSB_OPTION_ANDROID_JAVAVM, &vm);
	#endif
	int result = libusb_init(&usb_ctx); // open session
	if (result < 0) {
		usb_ctx = NULL;
		usb_initialized = false;
		return;
	}
	usb_initialized = true;
}

void usb_close() {
	if(usb_initialized)
		libusb_exit(usb_ctx);
	usb_ctx = NULL;
	usb_initialized = false;
}

bool usb_is_initialized() {
	return usb_initialized;
}

libusb_context* get_usb_ctx() {
	return usb_ctx;
}

void libusb_check_and_detach_kernel_driver(void* handle, int interface) {
	if(handle == NULL)
		return;
	libusb_device_handle* in_handle = (libusb_device_handle*)handle;
	int retval = libusb_kernel_driver_active(in_handle, interface);
	if(retval == 1)
		libusb_detach_kernel_driver(in_handle, interface);
}

int libusb_check_and_set_configuration(void* handle, int wanted_configuration) {
	if(handle == NULL)
		return LIBUSB_ERROR_OTHER;
	libusb_device_handle* in_handle = (libusb_device_handle*)handle;
	int curr_configuration = 0;
	int result = libusb_get_configuration(in_handle, &curr_configuration);
	if(result != LIBUSB_SUCCESS)
		return result;
	if(curr_configuration != wanted_configuration)
		result = libusb_set_configuration(in_handle, wanted_configuration);
	return result;
}

static void libusb_usb_thread_function() {
	if(!usb_is_initialized())
		return;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 300000;
	while(usb_thread_registered > 0)
		libusb_handle_events_timeout_completed(get_usb_ctx(), &tv, NULL);
}

void libusb_register_to_event_thread() {
	if(!usb_is_initialized())
		return;
	usb_thread_mutex.lock();
	int old_usb_thread_registered = usb_thread_registered;
	usb_thread_registered += 1;
	if(old_usb_thread_registered == 0)
		usb_thread = std::thread(libusb_usb_thread_function);
	usb_thread_mutex.unlock();
}

void libusb_unregister_from_event_thread() {
	if(!usb_is_initialized())
		return;
	usb_thread_mutex.lock();
	usb_thread_registered -= 1;
	if(usb_thread_registered == 0)
		usb_thread.join();
	usb_thread_mutex.unlock();
}

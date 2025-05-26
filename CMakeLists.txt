cmake_minimum_required(VERSION 3.16)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build archs for Mac OS X" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_PARALLEL_MSVC FALSE CACHE BOOL "Enables parallel compilation with MSVC")

#Workaround due to cmake parallel being broken, and needing to set these
#flags before the project is defined...
if(${CMAKE_PARALLEL_MSVC})
	message("Parallel build MSVC configuration")
	set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} /MP")
	set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /MP")
endif()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(TARGET_LANGUAGES CXX C)
project(cc3dsfs VERSION 1.0.0 LANGUAGES ${TARGET_LANGUAGES})
include(ExternalProject)

if((${CMAKE_PARALLEL_MSVC}) AND (NOT (MSVC)))
	message(FATAL_ERROR "Parallel build option specific to MSVC used with wrong compiler!")
endif()

set(N3DSXL_LOOPY_SUPPORT 1)
set(NEW_DS_LOOPY_SUPPORT 1)
set(IS_DEVICES_SUPPORT 1)
set(OLD_DS_3DS_LOOPY_SUPPORT 1)
set(CYPRESS_NISETRO_SUPPORT 1)
set(OPTIMIZE_3DS_SUPPORT 1)

set(USE_FTD2XX_FOR_NEW_DS_LOOPY ${NEW_DS_LOOPY_SUPPORT})
set(USE_LIBUSB_FOR_NEW_DS_LOOPY ${NEW_DS_LOOPY_SUPPORT})
set(USE_FTD3XX_FOR_N3DSXL_LOOPY ${N3DSXL_LOOPY_SUPPORT})
set(USE_LIBUSB_FOR_N3DSXL_LOOPY ${N3DSXL_LOOPY_SUPPORT})
set(USE_CYPRESS_USB_SUPPORT ${CYPRESS_NISETRO_SUPPORT} OR ${OPTIMIZE_3DS_SUPPORT})
set(USE_LIBUSB_SUPPORT ${IS_DEVICES_SUPPORT} OR ${OLD_DS_3DS_LOOPY_SUPPORT} OR ${USE_LIBUSB_FOR_NEW_DS_LOOPY} OR ${USE_LIBUSB_FOR_N3DSXL_LOOPY} OR ${USE_CYPRESS_USB_SUPPORT})
if(NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
	#Performance outside of Windows is very bad for the official drivers...
	set(USE_FTD2XX_FOR_NEW_DS_LOOPY 0)
	#Availability issues and various bugs, forcing the use of an older release...
	#Just avoid using it, when possible...
	set(USE_FTD3XX_FOR_N3DSXL_LOOPY 0)
endif()
set(WINDOWS_ARM64 0)
set(WINDOWS_x86_32 0)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(SFML_USE_STATIC_STD_LIBS TRUE)
set(SFML_CLONE_USE_GIT_SHALLOW FALSE)
set(CMAKE_HOST_CXX_COMPILER ${CMAKE_CXX_COMPILER} CACHE STRING "Host Compiler")
if(FORCE_WINDOWS_FTD3XX_SHARED_LIB)
	set(WINDOWS_FTD3XX_USE_SHARED_LIB 1)
else()
	set(WINDOWS_FTD3XX_USE_SHARED_LIB 0)
endif()
if(FORCE_WINDOWS_FTD2XX_SHARED_LIB)
	set(WINDOWS_FTD2XX_USE_SHARED_LIB 1)
else()
	set(WINDOWS_FTD2XX_USE_SHARED_LIB 0)
endif()
set(FETCHCONTENT_QUIET FALSE)
set(EXTRA_CXX_FLAGS "")
if(MSVC)
	list(APPEND EXTRA_CXX_FLAGS "/W3")
else()
	list(APPEND EXTRA_CXX_FLAGS "-Wall")
	list(APPEND EXTRA_CXX_FLAGS "-Wextra")
	list(APPEND EXTRA_CXX_FLAGS "-Wno-unused")
	list(APPEND EXTRA_CXX_FLAGS "-Wno-unused-parameter")
	list(APPEND EXTRA_CXX_FLAGS "-Wno-ignored-qualifiers")
	list(APPEND EXTRA_CXX_FLAGS "-Wno-missing-field-initializers")
endif()

set(RASPBERRY_PI_COMPILATION FALSE CACHE BOOL "Option for compiling the Raspberry Pi GPIO library in")
set(SFML_BUILD_NETWORK FALSE)
set(EXTRA_LIBRARIES "")
if(MINGW_CROSS_COMPILATION)
	list(APPEND EXTRA_LIBRARIES libpthread.a)
endif()
set(FINAL_EXTENSION "")
set(HOST_FINAL_EXTENSION "")
set(SETUP_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/setup_scripts)
set(USB_RULES_DIR ${CMAKE_SOURCE_DIR}/usb_rules)
set(SCRIPT_EXTENSION "")

if (RASPBERRY_PI_COMPILATION)
	list(APPEND EXTRA_LIBRARIES gpiod)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DRASPI 1")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DRASPI 1")
	endif()
endif()

if((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND ((${CMAKE_GENERATOR_PLATFORM} MATCHES "ARM64") OR (("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "") AND (${CMAKE_SYSTEM_PROCESSOR} MATCHES "ARM64"))))
	set(WINDOWS_ARM64 1)
endif()

if((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND ((${CMAKE_GENERATOR_PLATFORM} MATCHES "Win32") OR (("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "") AND ((${CMAKE_SYSTEM_PROCESSOR} MATCHES "Win32") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "X86")))))
	set(WINDOWS_x86_32 1)
endif()

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
# Git shallow clone isn't shallow in cmake... This is bad. :/
# Replace this if they ever fix it.
if(SFML_CLONE_USE_GIT_SHALLOW)
	FetchContent_Declare(SFML
		GIT_REPOSITORY https://github.com/SFML/SFML.git
		GIT_TAG master
		GIT_SHALLOW ON)
else()
	FetchContent_Declare(SFML
		URL https://github.com/SFML/SFML/archive/refs/heads/master.zip
	)
endif()

if(USE_LIBUSB_SUPPORT)
	FetchContent_Declare(LibUSB
		GIT_REPOSITORY https://github.com/libusb/libusb-cmake.git
		GIT_TAG main
		OVERRIDE_FIND_PACKAGE)
endif()

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	set(HOST_FINAL_EXTENSION ".exe")
	set(SCRIPT_EXTENSION ".bat")
else()
	set(SCRIPT_EXTENSION ".sh")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(FINAL_EXTENSION ".exe")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
else()
	if(NOT ${SFML_USE_DRM})
		if(MSVC)
			list(APPEND EXTRA_CXX_FLAGS "/DXLIB_BASED 1")
		else()
			list(APPEND EXTRA_CXX_FLAGS "-DXLIB_BASED 1")
		endif()
	endif()
endif()

set(FTDI_BASE_URL file://${CMAKE_SOURCE_DIR}/ftdi_lib_files/)
set(FTD3XX_BASE_URL ${FTDI_BASE_URL})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(FTD3XX_BUILD_COMMAND ${SETUP_SCRIPTS_DIR}/windows_setup_ftd3xx)
	set(FTD3XX_URL_TIME 2024/01)
	set(FTD3XX_VER 1.3.0.9)
	set(FTD3XX_VOL FTD3XXLibrary_v${FTD3XX_VER})
	set(FTD3XX_ARCHIVE ${FTD3XX_VOL}.zip)
	set(FTD3XX_SUBFOLDER win)
	set(FTD3XX_LIB FTD3XX.lib)
	set(FTD3XX_DLL FTD3XX.dll)
	if(WINDOWS_ARM64)
		set(FTD3XX_LIB FTD3XXWU.lib)
		set(FTD3XX_DLL FTD3XXWU.dll)
		set(FTD3XX_URL_TIME 2024/06)
		set(FTD3XX_BUILD_COMMAND ${SETUP_SCRIPTS_DIR}/windows_setup_ftd3xx_winusb)
		set(FTD3XX_ARCHIVE FTD3XX_WinUSB_Package.zip)
		set(FTD3XX_PATH_WINDOWS_ARCH ARM64)
	elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set(FTD3XX_PATH_WINDOWS_ARCH Win32)
	else ()
		set(FTD3XX_PATH_WINDOWS_ARCH x64)
	endif()
	if(${WINDOWS_FTD3XX_USE_SHARED_LIB})
		set(FTD3XX_BUILD_COMMAND ${FTD3XX_BUILD_COMMAND}_dll)
		set(FTD3XX_WINDOWS_PATH_SPECIFIER DLL)
		if(WINDOWS_ARM64)
			set(FTD3XX_WINDOWS_PATH_SPECIFIER Dynamic)
		endif()
	else()
		set(FTD3XX_WINDOWS_PATH_SPECIFIER Static_Lib)
		if(WINDOWS_ARM64)
			set(FTD3XX_WINDOWS_PATH_SPECIFIER Static)
		endif()
	endif()
	set(FTD3XX_PRE_WINDOWS_PATH ${FTD3XX_PATH_WINDOWS_ARCH}/${FTD3XX_WINDOWS_PATH_SPECIFIER})
	if(WINDOWS_ARM64)
		set(FTD3XX_PRE_WINDOWS_PATH ${FTD3XX_WINDOWS_PATH_SPECIFIER}/${FTD3XX_PATH_WINDOWS_ARCH})
	endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(FTD3XX_BUILD_COMMAND ${SETUP_SCRIPTS_DIR}/macos_setup_ftd3xx)
	set(FTD3XX_URL_TIME 2023/03)
	set(FTD3XX_VER 1.0.5)
	set(FTD3XX_VOL d3xx-osx.${FTD3XX_VER})
	set(FTD3XX_ARCHIVE ${FTD3XX_VOL}.dmg)
	set(FTD3XX_LIB libftd3xx-static.a)
	set(FTD3XX_MOUNTED_FOLDER /Volumes/${FTD3XX_VOL})
	set(FTD3XX_SUBFOLDER macos)
else()
	set(FTD3XX_BUILD_COMMAND ${SETUP_SCRIPTS_DIR}/linux_setup_ftd3xx)
	set(FTD3XX_URL_TIME 2023/03)
	set(FTD3XX_VER 1.0.5)
	if((${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm"))
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(FTD3XX_VOL libftd3xx-linux-arm-v8-${FTD3XX_VER})
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(FTD3XX_VOL libftd3xx-linux-arm-v7_32-${FTD3XX_VER})
		endif()
	else()
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(FTD3XX_VOL libftd3xx-linux-x86_64-${FTD3XX_VER})
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(FTD3XX_VOL libftd3xx-linux-x86_32-${FTD3XX_VER})
		endif()
	endif()
	set(FTD3XX_ARCHIVE ${FTD3XX_VOL}.tgz)
	set(FTD3XX_LIB libftd3xx-static.a)
	set(FTD3XX_SUBFOLDER linux)
endif()

set(FTD2XX_BASE_URL ${FTDI_BASE_URL})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(FTD2XX_BUILD_COMMAND ${SETUP_SCRIPTS_DIR}/windows_setup_ftd2xx)
	set(FTD2XX_URL_TIME 2023/09)
	set(FTD2XX_VER 2.12.36.4-WHQL-Certified)
	set(FTD2XX_VOL CDM-v${FTD2XX_VER})
	set(FTD2XX_ARCHIVE ${FTD2XX_VOL}.zip)
	set(FTD2XX_SUBFOLDER win)
	set(FTD2XX_HEADER_PATH ftd2xx.h)
	if(${WINDOWS_FTD2XX_USE_SHARED_LIB})
		set(FTD2XX_BUILD_COMMAND ${FTD2XX_BUILD_COMMAND}_dll)
		set(FTD2XX_WINDOWS_PATH_SPECIFIER "")
		if(WINDOWS_ARM64)
			set(FTD2XX_WINDOWS_PATH_SPECIFIER Release)
			set(FTD2XX_LIB FTD2XX.lib)
			set(FTD2XX_DLL FTD2XX.dll)
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(FTD2XX_LIB ftd2xx.lib)
			set(FTD2XX_DLL ftd2xx.dll)
		else()
			set(FTD2XX_LIB ftd2xx.lib)
			set(FTD2XX_DLL ftd2xx64.lib)
		endif()
	else()
		set(FTD2XX_WINDOWS_PATH_SPECIFIER Static)
		set(FTD2XX_LIB ftd2xx.lib)
		if(WINDOWS_ARM64)
			set(FTD2XX_WINDOWS_PATH_SPECIFIER Release)
			set(FTD2XX_LIB FTD2XXstatic.lib)
		endif()
	endif()
	if(WINDOWS_ARM64)
		set(FTD2XX_URL_TIME 2022/02)
		set(FTD2XX_VER 2.12.36.4-for-ARM64-Signed-Distributable)
		set(FTD2XX_VOL CDM-v${FTD2XX_VER})
		set(FTD2XX_ARCHIVE ${FTD2XX_VOL}.zip)
		set(FTD2XX_PATH_WINDOWS_ARCH ARM64)
		set(FTD2XX_HEADER_PATH x86/FTD2XX.H)
	elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set(FTD2XX_PATH_WINDOWS_ARCH i386)
	else ()
		set(FTD2XX_PATH_WINDOWS_ARCH amd64)
	endif()
	set(FTD2XX_PRE_WINDOWS_PATH ${FTD2XX_WINDOWS_PATH_SPECIFIER}/${FTD2XX_PATH_WINDOWS_ARCH})
	if(WINDOWS_ARM64)
		set(FTD2XX_PRE_WINDOWS_PATH ${FTD2XX_PATH_WINDOWS_ARCH}/${FTD2XX_WINDOWS_PATH_SPECIFIER})
	endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(FTD2XX_BUILD_COMMAND ${SETUP_SCRIPTS_DIR}/macos_setup_ftd2xx)
	set(FTD2XX_URL_TIME 2024/04)
	set(FTD2XX_VER 1.4.30)
	set(FTD2XX_VOL D2XX${FTD2XX_VER})
	set(FTD2XX_ARCHIVE ${FTD2XX_VOL}.dmg)
	set(FTD2XX_LIB libftd2xx.a)
	set(FTD2XX_MOUNTED_FOLDER /Volumes/dmg)
	set(FTD2XX_SUBFOLDER macos)
else()
	set(FTD2XX_BUILD_COMMAND ${SETUP_SCRIPTS_DIR}/linux_setup_ftd3xx)
	set(FTD2XX_URL_TIME 2022/07)
	set(FTD2XX_VER 1.4.27)
	if((${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm"))
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(FTD2XX_VOL libftd2xx-arm-v8-${FTD2XX_VER})
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(FTD2XX_VOL libftd2xx-arm-v7-hf-${FTD2XX_VER})
		endif()
	else()
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(FTD2XX_VOL libftd2xx-x86_64-${FTD2XX_VER})
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(FTD2XX_VOL libftd2xx-x86_32-${FTD2XX_VER})
		endif()
	endif()
	set(FTD2XX_ARCHIVE ${FTD2XX_VOL}.tgz)
	set(FTD2XX_LIB libftd2xx.a)
	set(FTD2XX_SUBFOLDER linux)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
		FetchContent_Declare(FTD3XX
			URL ${FTD3XX_BASE_URL}${FTD3XX_URL_TIME}/${FTD3XX_ARCHIVE}
			DOWNLOAD_NO_EXTRACT TRUE
		)
	endif()
	if(USE_FTD2XX_FOR_NEW_DS_LOOPY)
		FetchContent_Declare(FTD2XX
			URL ${FTD2XX_BASE_URL}${FTD2XX_URL_TIME}/${FTD2XX_ARCHIVE}
			DOWNLOAD_NO_EXTRACT TRUE
		)
	endif()
else()
	if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
		FetchContent_Declare(FTD3XX
			URL ${FTD3XX_BASE_URL}${FTD3XX_URL_TIME}/${FTD3XX_ARCHIVE}
		)
	endif()
	if(USE_FTD2XX_FOR_NEW_DS_LOOPY)
		FetchContent_Declare(FTD2XX
			URL ${FTD2XX_BASE_URL}${FTD2XX_URL_TIME}/${FTD2XX_ARCHIVE}
		)
	endif()
endif()

set(TOOLS_DATA_DIR ${CMAKE_BINARY_DIR}/tools_and_data)
file(MAKE_DIRECTORY ${TOOLS_DATA_DIR})

set(FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES "")
set(FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES_SEPARATE "")
set(SOURCE_CPP_EXTRA_FILES "")
set(EXTRA_DEPENDENCIES "")
set(SOURCE_CPP_DEVICE_FILES_BASE_PATH "source/CaptureDeviceSpecific")
set(SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH "${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/ISDevices")
set(SOURCE_CPP_FTD2_FILES_BASE_PATH "${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/DSCapture_FTD2")
set(SOURCE_CPP_FTD3_FILES_BASE_PATH "${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/3DSCapture_FTD3")
set(SOURCE_CPP_CYPRESS_USB_FILES_BASE_PATH "${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/CypressShared")
set(SOURCE_CPP_CYPRESS_NISETRO_DEVICES_FILES_BASE_PATH "${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/Nisetro")
set(SOURCE_CPP_CYPRESS_OPTIMIZE_3DS_FILES_BASE_PATH "${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/Optimize_3DS")
if(N3DSXL_LOOPY_SUPPORT)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_FTD3_FILES_BASE_PATH}/3dscapture_ftd3_shared.cpp ${SOURCE_CPP_FTD3_FILES_BASE_PATH}/3dscapture_ftd3_compatibility.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_FTD3")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_FTD3")
	endif()
endif()
if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_FTD3_FILES_BASE_PATH}/3dscapture_ftd3_driver_comms.cpp ${SOURCE_CPP_FTD3_FILES_BASE_PATH}/3dscapture_ftd3_driver_acquisition.cpp)
	list(APPEND FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES FTD3XX)
	list(APPEND EXTRA_DEPENDENCIES FTD3XX_BUILD_PROJECT)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_FTD3XX")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_FTD3XX")
	endif()
endif()
if(USE_LIBUSB_FOR_N3DSXL_LOOPY)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_FTD3_FILES_BASE_PATH}/3dscapture_ftd3_libusb_comms.cpp ${SOURCE_CPP_FTD3_FILES_BASE_PATH}/3dscapture_ftd3_libusb_acquisition.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_FTD3_LIBUSB")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_FTD3_LIBUSB")
	endif()
endif()
if(USE_LIBUSB_SUPPORT)
	list(APPEND FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES LibUSB)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/usb_generic.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_LIBUSB")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_LIBUSB")
	endif()
endif()
if(IS_DEVICES_SUPPORT)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH}/usb_is_device_communications.cpp ${SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH}/usb_is_device_acquisition.cpp ${SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH}/usb_is_nitro_acquisition_capture.cpp ${SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH}/usb_is_twl_acquisition_capture.cpp ${SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH}/usb_is_nitro_acquisition_emulator.cpp ${SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH}/usb_is_device_is_driver.cpp ${SOURCE_CPP_IS_DEVICES_FILES_BASE_PATH}/usb_is_device_libusb.cpp ${TOOLS_DATA_DIR}/is_twl_cap_crc32_table.cpp ${TOOLS_DATA_DIR}/is_twl_cap_init_seed_table.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_IS_DEVICES_USB")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_IS_DEVICES_USB")
	endif()
endif()
if(OLD_DS_3DS_LOOPY_SUPPORT)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_DEVICE_FILES_BASE_PATH}/usb_ds_3ds_capture.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_DS_3DS_USB")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_DS_3DS_USB")
	endif()
endif()
if(USE_CYPRESS_USB_SUPPORT)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_CYPRESS_USB_FILES_BASE_PATH}/cypress_shared_driver_comms.cpp ${SOURCE_CPP_CYPRESS_USB_FILES_BASE_PATH}/cypress_shared_libusb_comms.cpp ${SOURCE_CPP_CYPRESS_USB_FILES_BASE_PATH}/cypress_shared_communications.cpp)
endif()
if(CYPRESS_NISETRO_SUPPORT)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_CYPRESS_NISETRO_DEVICES_FILES_BASE_PATH}/cypress_nisetro_communications.cpp ${SOURCE_CPP_CYPRESS_NISETRO_DEVICES_FILES_BASE_PATH}/cypress_nisetro_acquisition.cpp ${TOOLS_DATA_DIR}/nisetro_ds_fw.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_CYNI_USB")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_CYNI_USB")
	endif()
endif()
if(OPTIMIZE_3DS_SUPPORT)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_CYPRESS_OPTIMIZE_3DS_FILES_BASE_PATH}/cypress_optimize_3ds_communications.cpp ${SOURCE_CPP_CYPRESS_OPTIMIZE_3DS_FILES_BASE_PATH}/cypress_optimize_3ds_acquisition.cpp ${TOOLS_DATA_DIR}/optimize_new_3ds_fw.cpp ${TOOLS_DATA_DIR}/optimize_new_3ds_565_fpga_pl.cpp ${TOOLS_DATA_DIR}/optimize_new_3ds_888_fpga_pl.cpp ${TOOLS_DATA_DIR}/optimize_old_3ds_fw.cpp ${TOOLS_DATA_DIR}/optimize_old_3ds_565_fpga_pl.cpp ${TOOLS_DATA_DIR}/optimize_old_3ds_888_fpga_pl.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_CYPRESS_OPTIMIZE")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_CYPRESS_OPTIMIZE")
	endif()
endif()
if(NEW_DS_LOOPY_SUPPORT)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_FTD2_FILES_BASE_PATH}/dscapture_ftd2_shared.cpp ${SOURCE_CPP_FTD2_FILES_BASE_PATH}/dscapture_ftd2_compatibility.cpp ${TOOLS_DATA_DIR}/ftd2_ds2_fw_1.cpp ${TOOLS_DATA_DIR}/ftd2_ds2_fw_2.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_FTD2")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_FTD2")
	endif()
endif()
if(USE_LIBUSB_FOR_NEW_DS_LOOPY)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_FTD2_FILES_BASE_PATH}/dscapture_ftd2_libusb_comms.cpp ${SOURCE_CPP_FTD2_FILES_BASE_PATH}/dscapture_ftd2_libusb_acquisition.cpp)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_FTD2_LIBUSB")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_FTD2_LIBUSB")
	endif()
endif()
if(USE_FTD2XX_FOR_NEW_DS_LOOPY)
	list(APPEND FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES FTD2XX)
	list(APPEND SOURCE_CPP_EXTRA_FILES ${SOURCE_CPP_FTD2_FILES_BASE_PATH}/dscapture_ftd2_driver_comms.cpp ${SOURCE_CPP_FTD2_FILES_BASE_PATH}/dscapture_ftd2_driver_acquisition.cpp)
	list(APPEND EXTRA_DEPENDENCIES FTD2XX_BUILD_PROJECT)
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DUSE_FTD2_DRIVER")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DUSE_FTD2_DRIVER")
	endif()
endif()

if(NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Android"))
	FetchContent_MakeAvailable(SFML ${FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES})
	# Find libusb
	set(LIBUSB_INCLUDE_DIR ${libusb_SOURCE_DIR}/libusb/libusb)
	FetchContent_MakeAvailable(${FETCH_CONTENT_MAKE_AVAILABLE_LIBRARIES_SEPARATE})
else()
	set(SFML_DIR "${CMAKE_ANDROID_NDK}/sources/third_party/sfml/lib/${CMAKE_ANDROID_ARCH_ABI}/cmake/SFML")
	# Find SFML
	find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Window System Main)
	set(OLD_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
	set(CMAKE_MODULE_PATH ${SFML_DIR})
    find_package(EGL REQUIRED)
    find_package(GLES REQUIRED)
	set(CMAKE_MODULE_PATH ${OLD_CMAKE_MODULE_PATH})
	set(LIBUSB_DIR "${CMAKE_ANDROID_NDK}/sources/third_party/libusb")
	set(LIBUSB_INCLUDE_DIR ${LIBUSB_DIR}/include/libusb)
endif()

set(EXTRA_INCLUDE_DIRECTORIES "")
if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
	list(APPEND EXTRA_INCLUDE_DIRECTORIES ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER})
endif()
if(USE_FTD2XX_FOR_NEW_DS_LOOPY)
	list(APPEND EXTRA_INCLUDE_DIRECTORIES ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER})
endif()
if(USE_LIBUSB_SUPPORT)
	list(APPEND EXTRA_INCLUDE_DIRECTORIES ${LIBUSB_INCLUDE_DIR})
endif()

if(IS_DIRECTORY "${sfml_SOURCE_DIR}")
	set_property(DIRECTORY ${sfml_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
endif()

if(USE_LIBUSB_SUPPORT)
	if(IS_DIRECTORY "${libusb_SOURCE_DIR}")
		set_property(DIRECTORY ${libusb_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
	endif()
endif()

if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
	file(MAKE_DIRECTORY ${ftd3xx_BINARY_DIR}/win)
	file(MAKE_DIRECTORY ${ftd3xx_BINARY_DIR}/macos)
	file(MAKE_DIRECTORY ${ftd3xx_BINARY_DIR}/linux)
endif()

if(USE_FTD2XX_FOR_NEW_DS_LOOPY)
	file(MAKE_DIRECTORY ${ftd2xx_BINARY_DIR}/win)
	file(MAKE_DIRECTORY ${ftd2xx_BINARY_DIR}/macos)
	file(MAKE_DIRECTORY ${ftd2xx_BINARY_DIR}/linux)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(FTD3XX_BUILD_COMMAND_PARAMETERS ${ftd3xx_SOURCE_DIR}/${FTD3XX_ARCHIVE} ${FTD3XX_MOUNTED_FOLDER} ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER} ${FTD3XX_LIB})
	set(FTD2XX_BUILD_COMMAND_PARAMETERS ${ftd2xx_SOURCE_DIR}/${FTD2XX_ARCHIVE} ${FTD2XX_MOUNTED_FOLDER} ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER} ${FTD2XX_LIB})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(FTD3XX_BUILD_COMMAND_PARAMETERS ${ftd3xx_SOURCE_DIR} ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER} ${FTD3XX_PRE_WINDOWS_PATH}/${FTD3XX_LIB})
if(WINDOWS_FTD3XX_USE_SHARED_LIB)
	set(FTD3XX_BUILD_COMMAND_PARAMETERS ${FTD3XX_BUILD_COMMAND_PARAMETERS} ${FTD3XX_PRE_WINDOWS_PATH}/${FTD3XX_DLL})
endif()
	set(FTD2XX_BUILD_COMMAND_PARAMETERS ${ftd2xx_SOURCE_DIR} ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER} ${FTD2XX_PRE_WINDOWS_PATH}/${FTD2XX_LIB} ${FTD2XX_HEADER_PATH})
if(WINDOWS_FTD2XX_USE_SHARED_LIB)
	set(FTD2XX_BUILD_COMMAND_PARAMETERS ${FTD2XX_BUILD_COMMAND_PARAMETERS} ${FTD2XX_PRE_WINDOWS_PATH}/${FTD2XX_DLL})
endif()
else()
	set(FTD3XX_BUILD_COMMAND_PARAMETERS ${ftd3xx_SOURCE_DIR} ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER} ${FTD3XX_LIB})
	set(FTD2XX_BUILD_COMMAND_PARAMETERS ${ftd2xx_SOURCE_DIR} ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER} build/${FTD2XX_LIB})
endif()

if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
	ExternalProject_Add(FTD3XX_BUILD_PROJECT
		SOURCE_DIR ${ftd3xx_SOURCE_DIR}
		BINARY_DIR ${ftd3xx_BINARY_DIR}
		BUILD_COMMAND ${FTD3XX_BUILD_COMMAND}${SCRIPT_EXTENSION} ${FTD3XX_BUILD_COMMAND_PARAMETERS}
		UPDATE_COMMAND ""
		CONFIGURE_COMMAND ""
		INSTALL_COMMAND ""
	)
endif()
set(BASE_REPLACEMENT_FTD2XX_SYMBOL F2)
set(BASE_REPLACEMENT_FTD2XX_SYMBOLS_LIST ${SETUP_SCRIPTS_DIR}/ftd_shared_symbols.txt)
set(REPLACEMENT_FTD2XX_SYMBOL ${BASE_REPLACEMENT_FTD2XX_SYMBOL})
set(REPLACEMENT_FTD2XX_SYMBOLS_LIST ${BASE_REPLACEMENT_FTD2XX_SYMBOLS_LIST})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(REPLACEMENT_FTD2XX_SYMBOL _F2)
	set(REPLACEMENT_FTD2XX_SYMBOLS_LIST ${SETUP_SCRIPTS_DIR}/ftd_shared_symbols_macos.txt)
endif()
if((${CMAKE_SYSTEM_NAME} STREQUAL "Windows") AND ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4"))
	set(REPLACEMENT_FTD2XX_SYMBOL _F2)
	set(REPLACEMENT_FTD2XX_SYMBOLS_LIST ${SETUP_SCRIPTS_DIR}/ftd_shared_symbols_macos.txt)
endif()
set(REPLACEMENT_FTD2XX_SYMBOL_EXTRA ${REPLACEMENT_FTD2XX_SYMBOL})
set(REPLACEMENT_FTD2XX_SYMBOLS_LIST_EXTRA ${REPLACEMENT_FTD2XX_SYMBOLS_LIST})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(REPLACEMENT_FTD2XX_SYMBOL_EXTRA ?F2)
	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set(REPLACEMENT_FTD2XX_SYMBOLS_LIST_EXTRA ${SETUP_SCRIPTS_DIR}/ftd_shared_symbols_special_windows32.txt)
	else()
		set(REPLACEMENT_FTD2XX_SYMBOLS_LIST_EXTRA ${SETUP_SCRIPTS_DIR}/ftd_shared_symbols_special_windows.txt)
	endif()
endif()
if(USE_FTD2XX_FOR_NEW_DS_LOOPY)
	ExternalProject_Add(FTD2XX_BUILD_PROJECT
		SOURCE_DIR ${ftd2xx_SOURCE_DIR}
		BINARY_DIR ${ftd2xx_BINARY_DIR}
		BUILD_COMMAND ${FTD2XX_BUILD_COMMAND}${SCRIPT_EXTENSION} ${FTD2XX_BUILD_COMMAND_PARAMETERS}
		COMMAND ${TOOLS_DATA_DIR}/CMakeRenameLibFunctions${HOST_FINAL_EXTENSION} ${REPLACEMENT_FTD2XX_SYMBOLS_LIST} ${REPLACEMENT_FTD2XX_SYMBOL} ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER}/${FTD2XX_LIB}
		COMMAND ${TOOLS_DATA_DIR}/CMakeRenameLibFunctions${HOST_FINAL_EXTENSION} ${REPLACEMENT_FTD2XX_SYMBOLS_LIST_EXTRA} ${REPLACEMENT_FTD2XX_SYMBOL_EXTRA} ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER}/${FTD2XX_LIB}
		COMMAND ${TOOLS_DATA_DIR}/CMakeRenameSymbols${HOST_FINAL_EXTENSION} ${BASE_REPLACEMENT_FTD2XX_SYMBOLS_LIST} ${BASE_REPLACEMENT_FTD2XX_SYMBOL} ${TOOLS_DATA_DIR}/ftd2xx_symbols_renames.h
		UPDATE_COMMAND ""
		CONFIGURE_COMMAND ""
		INSTALL_COMMAND ""
	)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

execute_process(COMMAND ${CMAKE_COMMAND} -B ${TOOLS_DATA_DIR} ${CMAKE_SOURCE_DIR}/tools)
execute_process(COMMAND ${CMAKE_COMMAND} --build ${TOOLS_DATA_DIR})

set(OUTPUT_NAME cc3dsfs)

if(MSVC)
	set_source_files_properties(source/conversions.cpp PROPERTIES COMPILE_OPTIONS "$<$<CONFIG:Release>:/O2>")
else()
	set_source_files_properties(source/conversions.cpp PROPERTIES COMPILE_OPTIONS "$<$<CONFIG:Release>:-O3;-funroll-loops>")
endif()

set(EXECUTABLE_SOURCE_FILES source/cc3dsfs.cpp source/utils.cpp source/audio_data.cpp source/audio.cpp source/frontend.cpp source/TextRectangle.cpp source/TextRectanglePool.cpp source/WindowScreen.cpp source/WindowScreen_Menu.cpp source/devicecapture.cpp source/conversions.cpp source/ExtraButtons.cpp source/Menus/ConnectionMenu.cpp source/Menus/OptionSelectionMenu.cpp source/Menus/MainMenu.cpp source/Menus/VideoMenu.cpp source/Menus/CropMenu.cpp source/Menus/PARMenu.cpp source/Menus/RotationMenu.cpp source/Menus/OffsetMenu.cpp source/Menus/AudioMenu.cpp source/Menus/BFIMenu.cpp source/Menus/RelativePositionMenu.cpp source/Menus/ResolutionMenu.cpp source/Menus/FileConfigMenu.cpp source/Menus/ExtraSettingsMenu.cpp source/Menus/StatusMenu.cpp source/Menus/LicenseMenu.cpp source/WindowCommands.cpp source/Menus/ShortcutMenu.cpp source/Menus/ActionSelectionMenu.cpp source/Menus/ScalingRatioMenu.cpp source/Menus/ISNitroMenu.cpp source/Menus/VideoEffectsMenu.cpp source/CaptureDataBuffers.cpp source/Menus/InputMenu.cpp source/Menus/AudioDeviceMenu.cpp source/Menus/SeparatorMenu.cpp source/Menus/ColorCorrectionMenu.cpp source/Menus/Main3DMenu.cpp source/Menus/SecondScreen3DRelativePositionMenu.cpp source/Menus/USBConflictResolutionMenu.cpp source/Menus/Optimize3DSMenu.cpp ${TOOLS_DATA_DIR}/font_ttf.cpp ${TOOLS_DATA_DIR}/shaders_list.cpp ${SOURCE_CPP_EXTRA_FILES})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	if(MSVC)
		list(APPEND EXTRA_CXX_FLAGS "/DSFML_SYSTEM_ANDROID")
	else()
		list(APPEND EXTRA_CXX_FLAGS "-DSFML_SYSTEM_ANDROID")
	endif()
	# Create the cc3dsfs.so library that contains the application's c++ code
	add_library(${OUTPUT_NAME} SHARED ${EXECUTABLE_SOURCE_FILES})
	list(APPEND EXTRA_LIBRARIES android)
	list(APPEND EXTRA_LIBRARIES log)
	list(APPEND EXTRA_LIBRARIES EGL::EGL)
	list(APPEND EXTRA_LIBRARIES GLES::GLES)
else()
	add_executable(${OUTPUT_NAME} ${EXECUTABLE_SOURCE_FILES})
endif()

if(NOT ("${EXTRA_DEPENDENCIES}" STREQUAL ""))
	add_dependencies(${OUTPUT_NAME} ${EXTRA_DEPENDENCIES})
endif()
target_link_libraries(${OUTPUT_NAME} PRIVATE SFML::Graphics SFML::Audio SFML::Window SFML::System ${EXTRA_LIBRARIES})
if(USE_LIBUSB_SUPPORT)
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
		target_link_libraries(${OUTPUT_NAME} PRIVATE ${LIBUSB_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}/libusb1.0.so)
	else()
		target_link_libraries(${OUTPUT_NAME} PRIVATE usb-1.0)
	endif()
endif()
if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
	target_link_libraries(${OUTPUT_NAME} PRIVATE ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER}/${FTD3XX_LIB})
endif()
if(USE_FTD2XX_FOR_NEW_DS_LOOPY)
	target_link_libraries(${OUTPUT_NAME} PRIVATE ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER}/${FTD2XX_LIB})
endif()
target_include_directories(${OUTPUT_NAME} PRIVATE ${EXTRA_INCLUDE_DIRECTORIES} ${TOOLS_DATA_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/Menus ${CMAKE_SOURCE_DIR}/include/CaptureDeviceSpecific ${CMAKE_SOURCE_DIR}/include/CaptureDeviceSpecific/ISDevices ${CMAKE_SOURCE_DIR}/include/CaptureDeviceSpecific/Nisetro ${CMAKE_SOURCE_DIR}/include/CaptureDeviceSpecific/Optimize_3DS ${CMAKE_SOURCE_DIR}/include/CaptureDeviceSpecific/CypressShared ${CMAKE_SOURCE_DIR}/include/CaptureDeviceSpecific/3DSCapture_FTD3 ${CMAKE_SOURCE_DIR}/include/CaptureDeviceSpecific/DSCapture_FTD2)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_20)
target_compile_options(${OUTPUT_NAME} PRIVATE ${EXTRA_CXX_FLAGS})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	target_link_libraries(${OUTPUT_NAME} PUBLIC
	  -Wl,--whole-archive
	  SFML::Main
	  -Wl,--no-whole-archive
	)
endif()

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/font_ttf.cpp
	COMMENT "Convert font to binary"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/data/font.ttf ${TOOLS_DATA_DIR} font_ttf font_ttf
	DEPENDS ${CMAKE_SOURCE_DIR}/data/font.ttf ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/ftd2_ds2_fw_1.cpp
	COMMENT "Convert fw binary to C - FTD2 NDS 1"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/ftd2_ds2_fw_1.bin ${TOOLS_DATA_DIR} ftd2_ds2_fw_1 ftd2_ds2_fw_1
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/ftd2_ds2_fw_1.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/is_twl_cap_crc32_table.cpp
	COMMENT "Convert binary to C - IS TWL CRC32 table"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/is_twl_cap_crc32_table.bin ${TOOLS_DATA_DIR} is_twl_cap_crc32_table is_twl_cap_crc32_table
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/is_twl_cap_crc32_table.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/is_twl_cap_init_seed_table.cpp
	COMMENT "Convert binary to C - IS TWL Init Seed table"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/is_twl_cap_init_seed_table.bin ${TOOLS_DATA_DIR} is_twl_cap_init_seed_table is_twl_cap_init_seed_table
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/is_twl_cap_init_seed_table.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/ftd2_ds2_fw_2.cpp
	COMMENT "Convert fw binary to C - FTD2 NDS 2"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/ftd2_ds2_fw_2.bin ${TOOLS_DATA_DIR} ftd2_ds2_fw_2 ftd2_ds2_fw_2
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/ftd2_ds2_fw_2.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/nisetro_ds_fw.cpp
	COMMENT "Convert fw binary to C - Nisetro DS"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/nisetro_ds_fw.bin ${TOOLS_DATA_DIR} nisetro_ds_fw nisetro_ds_fw
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/nisetro_ds_fw.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/optimize_new_3ds_fw.cpp
	COMMENT "Convert fw binary to C - Optimize New 3DS"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/optimize_new_3ds_fw.bin ${TOOLS_DATA_DIR} optimize_new_3ds_fw optimize_new_3ds_fw
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/optimize_new_3ds_fw.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/optimize_new_3ds_565_fpga_pl.cpp
	COMMENT "Convert FPGA 565 PL binary to C - Optimize New 3DS"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/optimize_new_3ds_565_fpga_pl.bin ${TOOLS_DATA_DIR} optimize_new_3ds_565_fpga_pl optimize_new_3ds_565_fpga_pl
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/optimize_new_3ds_565_fpga_pl.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/optimize_new_3ds_888_fpga_pl.cpp
	COMMENT "Convert FPGA 888 PL binary to C - Optimize New 3DS"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/optimize_new_3ds_888_fpga_pl.bin ${TOOLS_DATA_DIR} optimize_new_3ds_888_fpga_pl optimize_new_3ds_888_fpga_pl
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/optimize_new_3ds_888_fpga_pl.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/optimize_old_3ds_fw.cpp
	COMMENT "Convert fw binary to C - Optimize Old 3DS"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/optimize_old_3ds_fw.bin ${TOOLS_DATA_DIR} optimize_old_3ds_fw optimize_old_3ds_fw
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/optimize_old_3ds_fw.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/optimize_old_3ds_565_fpga_pl.cpp
	COMMENT "Convert FPGA 565 PL binary to C - Optimize Old 3DS"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/optimize_old_3ds_565_fpga_pl.bin ${TOOLS_DATA_DIR} optimize_old_3ds_565_fpga_pl optimize_old_3ds_565_fpga_pl
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/optimize_old_3ds_565_fpga_pl.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/optimize_old_3ds_888_fpga_pl.cpp
	COMMENT "Convert FPGA 888 PL binary to C - Optimize Old 3DS"
	COMMAND ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/bin/optimize_old_3ds_888_fpga_pl.bin ${TOOLS_DATA_DIR} optimize_old_3ds_888_fpga_pl optimize_old_3ds_888_fpga_pl
	DEPENDS ${CMAKE_SOURCE_DIR}/bin/optimize_old_3ds_888_fpga_pl.bin ${TOOLS_DATA_DIR}/CMakeBin2C${HOST_FINAL_EXTENSION}
)

set(SHADERS_LIST "")
list(APPEND SHADERS_LIST ${CMAKE_SOURCE_DIR}/shaders/bit_crusher_fragment_shader.2_to_x_1_7.frag ${CMAKE_SOURCE_DIR}/shaders/bit_merger_crusher_fragment_shader.2_to_x_1_6.frag ${CMAKE_SOURCE_DIR}/shaders/bit_merger_crusher_fragment_shader_7.frag ${CMAKE_SOURCE_DIR}/shaders/bit_merger_fragment_shader.2_to_x_0_6.frag ${CMAKE_SOURCE_DIR}/shaders/bit_merger_fragment_shader_7.frag ${CMAKE_SOURCE_DIR}/shaders/frame_blending_bit_crusher_fragment_shader.2_to_x_1_7.frag ${CMAKE_SOURCE_DIR}/shaders/frame_blending_fragment_shader.frag ${CMAKE_SOURCE_DIR}/shaders/no_effect_fragment_shader.frag ${CMAKE_SOURCE_DIR}/shaders/color_emulation_fragment_shader.frag)

add_custom_command(
	OUTPUT ${TOOLS_DATA_DIR}/shaders_list.cpp
	COMMENT "Prepare shaders list"
	COMMAND ${TOOLS_DATA_DIR}/CMakeShader2C${HOST_FINAL_EXTENSION} ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_SOURCE_DIR}/source/shaders_list_template.cpp ${TOOLS_DATA_DIR}/shaders_list.cpp
	DEPENDS ${CMAKE_SOURCE_DIR}/source/shaders_list_template.cpp ${SHADERS_LIST} ${TOOLS_DATA_DIR}/CMakeShader2C${HOST_FINAL_EXTENSION}
)

if(WIN32)
	if(USE_FTD3XX_FOR_N3DSXL_LOOPY AND WINDOWS_FTD3XX_USE_SHARED_LIB)
		add_custom_command(
			TARGET ${OUTPUT_NAME}
			COMMENT "Copy FTD3XX DLL"
			POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER}/${FTD3XX_DLL} ${CMAKE_SOURCE_DIR}
			VERBATIM
		)
	endif()
	if(USE_FTD2XX_FOR_NEW_DS_LOOPY AND WINDOWS_FTD2XX_USE_SHARED_LIB)
		add_custom_command(
			TARGET ${OUTPUT_NAME}
			COMMENT "Copy FTD2XX DLL"
			POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${ftd2xx_BINARY_DIR}/${FTD2XX_SUBFOLDER}/${FTD2XX_DLL} ${CMAKE_SOURCE_DIR}
			VERBATIM
		)
	endif()
endif()

add_custom_command(
	TARGET ${OUTPUT_NAME}
	COMMENT "Copy Output"
	POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_SOURCE_DIR}
	VERBATIM
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
install(TARGETS ${OUTPUT_NAME} DESTINATION .)
else()
install(FILES ${CMAKE_SOURCE_DIR}/${OUTPUT_NAME}${FINAL_EXTENSION} DESTINATION .)
endif()
install(FILES LICENSE README.md DESTINATION .)
install(DIRECTORY "other licenses" DESTINATION .
		PATTERN "other licenses/ftd3xx.txt" EXCLUDE)
if(USE_FTD3XX_FOR_N3DSXL_LOOPY)
	install(FILES "other licenses/ftd3xx.txt" DESTINATION "./other licenses")
endif()
if(WIN32)
	if(USE_FTD3XX_FOR_N3DSXL_LOOPY AND WINDOWS_FTD3XX_USE_SHARED_LIB)
		install(FILES ${FTD3XX_DLL} DESTINATION .)
	endif()
	if(USE_FTD2XX_FOR_NEW_DS_LOOPY AND WINDOWS_FTD2XX_USE_SHARED_LIB)
		install(FILES ${FTD2XX_DLL} DESTINATION .)
	endif()
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set_target_properties(${OUTPUT_NAME} PROPERTIES
						  MACOSX_BUNDLE TRUE
						  MACOSX_BUNDLE_BUNDLE_NAME ${OUTPUT_NAME}
						  MACOSX_BUNDLE_GUI_IDENTIFIER ${OUTPUT_NAME}
						  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/data/MacOSXBundleInfo.plist.in)
	set(CMAKE_MACOSX_RPATH TRUE)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
else()
	install(FILES ${USB_RULES_DIR}/51-ftd3xx.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/95-usb3dscapture.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/95-usbdscapture.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/51-isnitro.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/51-istwl.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/50-ftd2xx.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/51-ftd2xx.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/51-nisetro.rules DESTINATION .)
	install(FILES ${USB_RULES_DIR}/51-optimize.rules DESTINATION .)
	install(FILES ${SETUP_SCRIPTS_DIR}/install_usb_rules${SCRIPT_EXTENSION} DESTINATION .)
endif()


set(CPACK_PACKAGE_NAME ${OUTPUT_NAME})
set(CPACK_PACKAGE_VENDOR Lorenzooone)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "3DS Capture Card Fullscreen software")

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES TRUE)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_macos)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	if(WINDOWS_ARM64)
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_windows_arm64)
	elseif(WINDOWS_x86_32)
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_windows_x86_32)
	else()
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_windows_x86_64)
	endif()
else()
	if(RASPBERRY_PI_COMPILATION)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_linux_pi64)
		else()
			set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_linux_pi32)
		endif()
	else()
		set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_linux_${CMAKE_SYSTEM_PROCESSOR})
	endif()
endif()

include(CPack)

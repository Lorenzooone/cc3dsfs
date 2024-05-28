cmake_minimum_required(VERSION 3.16)
project(cc3dsfs VERSION 0.0.0 LANGUAGES CXX)
include(ExternalProject)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(CMAKE_HOST_CXX_COMPILER ${CMAKE_CXX_COMPILER} CACHE STRING "Host Compiler")
set(WINDOWS_FTD3XX_USE_SHARED_LIB 0)
set(FETCHCONTENT_QUIET FALSE)
set(EXTRA_LINUX_CXX_FLAGS "-DXLIB_BASED 1" CACHE STRING "Extra Linux CXX flags")
set(SFML_BUILD_NETWORK FALSE)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1)

set(FTD3XX_BASE_URL https://ftdichip.com/wp-content/uploads/)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(FTD3XX_BUILD_COMMAND ${CMAKE_SOURCE_DIR}/windows_setup_ftd3xx.bat)
    set(FTD3XX_URL_TIME 2024/01)
    set(FTD3XX_VER 1.3.0.9)
    set(FTD3XX_VOL FTD3XXLibrary_v${FTD3XX_VER})
    set(FTD3XX_ARCHIVE ${FTD3XX_VOL}.zip)
    set(FTD3XX_SUBFOLDER win)
    if(${WINDOWS_FTD3XX_USE_SHARED_LIB})
        set(WINDOWS_PATH_SPECIFIER DLL)
        set(FTD3XX_LIB FTD3XX.dll)
    else()
        set(WINDOWS_PATH_SPECIFIER Static_Lib)
        set(FTD3XX_LIB FTD3XX.lib)
    endif()
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(PATH_WINDOWS_ARCH Win32)
    else ()
        set(PATH_WINDOWS_ARCH x64)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(FTD3XX_BUILD_COMMAND ${CMAKE_SOURCE_DIR}/macos_setup_ftd3xx.sh)
    set(FTD3XX_URL_TIME 2023/03)
    set(FTD3XX_VER 1.0.5)
    set(FTD3XX_VOL d3xx-osx.${FTD3XX_VER})
    set(FTD3XX_ARCHIVE ${FTD3XX_VOL}.dmg)
    set(FTD3XX_LIB libftd3xx-static.a)
    set(FTD3XX_MOUNTED_FOLDER /Volumes/${FTD3XX_VOL})
    set(FTD3XX_SUBFOLDER macos)
else()
    set(FTD3XX_BUILD_COMMAND ${CMAKE_SOURCE_DIR}/linux_setup_ftd3xx.sh)
    set(FTD3XX_URL_TIME 2023/03)
    set(FTD3XX_VER 1.0.5)
    if((${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm"))
        if((${CMAKE_SYSTEM_PROCESSOR} MATCHES "64") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "v8"))
            set(FTD3XX_VOL libftd3xx-linux-arm-v8-${FTD3XX_VER})
        elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "v7")
            set(FTD3XX_VOL libftd3xx-linux-arm-v7_32-${FTD3XX_VER})
        endif()
    else()
        if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "64")
            set(FTD3XX_VOL libftd3xx-linux-x86_64-${FTD3XX_VER})
        elseif((${CMAKE_SYSTEM_PROCESSOR} MATCHES "32") OR (${CMAKE_SYSTEM_PROCESSOR} MATCHES "86"))
            set(FTD3XX_VOL libftd3xx-linux-x86_32-${FTD3XX_VER})
        endif()
    endif()
    set(FTD3XX_ARCHIVE ${FTD3XX_VOL}.tgz)
    set(FTD3XX_LIB libftd3xx-static.a)
    set(FTD3XX_SUBFOLDER linux)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
FetchContent_Declare(FTD3XX
    URL ${FTD3XX_BASE_URL}${FTD3XX_URL_TIME}/${FTD3XX_ARCHIVE}
    DOWNLOAD_NO_EXTRACT TRUE
)
else()
FetchContent_Declare(FTD3XX
    URL ${FTD3XX_BASE_URL}${FTD3XX_URL_TIME}/${FTD3XX_ARCHIVE}
)
endif()

FetchContent_MakeAvailable(SFML FTD3XX)

if(IS_DIRECTORY "${sfml_SOURCE_DIR}")
    set_property(DIRECTORY ${sfml_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
endif()

file(MAKE_DIRECTORY ${ftd3xx_BINARY_DIR}/win)
file(MAKE_DIRECTORY ${ftd3xx_BINARY_DIR}/macos)
file(MAKE_DIRECTORY ${ftd3xx_BINARY_DIR}/linux)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
ExternalProject_Add(FTD3XX_BUILD_PROJECT
    SOURCE_DIR ${ftd3xx_SOURCE_DIR}
    BINARY_DIR ${ftd3xx_BINARY_DIR}
    BUILD_COMMAND ${FTD3XX_BUILD_COMMAND} ${ftd3xx_SOURCE_DIR}/${FTD3XX_ARCHIVE} ${FTD3XX_MOUNTED_FOLDER} ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER} ${FTD3XX_LIB}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
ExternalProject_Add(FTD3XX_BUILD_PROJECT
    SOURCE_DIR ${ftd3xx_SOURCE_DIR}
    BINARY_DIR ${ftd3xx_BINARY_DIR}
    BUILD_COMMAND ${FTD3XX_BUILD_COMMAND} ${ftd3xx_SOURCE_DIR} ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER} ${PATH_WINDOWS_ARCH}/${WINDOWS_PATH_SPECIFIER}/${FTD3XX_LIB}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
)
else()
ExternalProject_Add(FTD3XX_BUILD_PROJECT
    SOURCE_DIR ${ftd3xx_SOURCE_DIR}
    BINARY_DIR ${ftd3xx_BINARY_DIR}
    BUILD_COMMAND ${FTD3XX_BUILD_COMMAND} ${ftd3xx_SOURCE_DIR} ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER} ${FTD3XX_LIB}
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(TOOLS_DATA_DIR ${CMAKE_BINARY_DIR}/tools_and_data)
file(MAKE_DIRECTORY ${TOOLS_DATA_DIR})

set(OUTPUT_NAME cc3dsfs)

add_executable(CMakeBin2C tools/bin2c.cpp)
add_executable(${OUTPUT_NAME} source/cc3dsfs.cpp source/utils.cpp source/audio_data.cpp source/audio.cpp source/frontend.cpp source/TextRectangle.cpp source/WindowScreen.cpp source/3dscapture.cpp source/conversions.cpp source/Menus/ConnectionMenu.cpp source/Menus/OptionSelectionMenu.cpp source/Menus/MainMenu.cpp source/Menus/VideoMenu.cpp source/Menus/CropMenu.cpp source/Menus/PARMenu.cpp source/Menus/RotationMenu.cpp source/Menus/OffsetMenu.cpp source/Menus/AudioMenu.cpp source/Menus/BFIMenu.cpp source/Menus/RelativePositionMenu.cpp source/Menus/ResolutionMenu.cpp source/Menus/FileConfigMenu.cpp source/Menus/ExtraSettingsMenu.cpp source/Menus/StatusMenu.cpp source/Menus/LicenseMenu.cpp ${TOOLS_DATA_DIR}/font_ttf.cpp)
add_dependencies(${OUTPUT_NAME} FTD3XX_BUILD_PROJECT CMakeBin2C)
target_link_libraries(${OUTPUT_NAME} PRIVATE sfml-graphics sfml-audio sfml-window sfml-system ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER}/${FTD3XX_LIB})
target_link_directories(${OUTPUT_NAME} PRIVATE ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER})
target_include_directories(${OUTPUT_NAME} PRIVATE ${ftd3xx_BINARY_DIR}/${FTD3XX_SUBFOLDER} ${TOOLS_DATA_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/Menus)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_20)
target_compile_options(${OUTPUT_NAME} PRIVATE ${EXTRA_LINUX_CXX_FLAGS})
target_compile_features(CMakeBin2C PRIVATE cxx_std_17)

add_custom_command(
    OUTPUT ${TOOLS_DATA_DIR}/font_ttf.cpp
    COMMENT "Convert font to binary"
    COMMAND $<TARGET_FILE:CMakeBin2C> ${CMAKE_SOURCE_DIR}/data/font.ttf ${TOOLS_DATA_DIR} font_ttf font_ttf
    DEPENDS ${CMAKE_SOURCE_DIR}/data/font.ttf CMakeBin2C
)

if(WIN32)
    add_custom_command(
        TARGET ${OUTPUT_NAME}
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${sfml_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll ${CMAKE_SOURCE_DIR}
        VERBATIM
    )
endif()

add_custom_command(
    TARGET ${OUTPUT_NAME}
    COMMENT "Copy Output"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_SOURCE_DIR}
    VERBATIM
)

install(TARGETS ${OUTPUT_NAME} DESTINATION .)
install(FILES LICENSE README.md DESTINATION .)
install(DIRECTORY "other licenses" DESTINATION .)
if(WIN32)
install(FILES openal32.dll DESTINATION .)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
set(FW_APP ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUT_NAME}.app)
set(FW_DIRS ${sfml_SOURCE_DIR}/extlibs/libs-osx/Frameworks)
set_target_properties(${OUTPUT_NAME} PROPERTIES
                      MACOSX_BUNDLE TRUE)
set(CMAKE_MACOSX_RPATH 1)
list(APPEND CMAKE_INSTALL_RPATH "@executable_path/../Frameworks" )

add_custom_command(
    TARGET ${OUTPUT_NAME}
    COMMENT "Copy Frameworks"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${FW_DIRS} ${FW_APP}/Contents/Frameworks
    VERBATIM
)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
else()
install(FILES 51-ftd3xx.rules DESTINATION .)
endif()


set(CPACK_PACKAGE_NAME ${OUTPUT_NAME}_${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_PACKAGE_VENDOR Lorenzooone)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "3DS Capture Card Fullscreen software")

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES TRUE)

include(CPack)

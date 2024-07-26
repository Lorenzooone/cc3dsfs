cmake_minimum_required(VERSION 3.16)
project(CMakeBin2C VERSION 1.0.0 LANGUAGES CXX)

add_executable(CMakeBin2C bin2c.cpp)
target_compile_features(CMakeBin2C PRIVATE cxx_std_17)

add_custom_command(
    TARGET CMakeBin2C
    COMMENT "Copy Output"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CMakeBin2C> ${CMAKE_BINARY_DIR}
    VERBATIM
)

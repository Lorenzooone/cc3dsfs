cmake_minimum_required(VERSION 3.16)
unset(ENV{CC})
unset(ENV{CXX})
set(OUTPUT_NAME CMakeBin2C)
project(${OUTPUT_NAME} VERSION 1.0.0 LANGUAGES CXX)

add_executable(${OUTPUT_NAME} bin2c.cpp)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_17)

add_custom_command(
    TARGET ${OUTPUT_NAME}
    COMMENT "Copy Output"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}
    VERBATIM
)

set(OUTPUT_NAME CMakeShader2C)
project(${OUTPUT_NAME} VERSION 1.0.0 LANGUAGES CXX)

add_executable(${OUTPUT_NAME} shader2c.cpp)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_17)

add_custom_command(
    TARGET ${OUTPUT_NAME}
    COMMENT "Copy Output"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}
    VERBATIM
)

set(OUTPUT_NAME CMakePrepareRenameSymbols)
project(${OUTPUT_NAME} VERSION 1.0.0 LANGUAGES CXX)

add_executable(${OUTPUT_NAME} prepare_rename_symbols_list.cpp)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_17)

add_custom_command(
    TARGET ${OUTPUT_NAME}
    COMMENT "Copy Output"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}
    VERBATIM
)

set(OUTPUT_NAME CMakeRenameLibFunctions)
project(${OUTPUT_NAME} VERSION 1.0.0 LANGUAGES CXX)

add_executable(${OUTPUT_NAME} rename_lib_functions.cpp)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_17)

add_custom_command(
    TARGET ${OUTPUT_NAME}
    COMMENT "Copy Output"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}
    VERBATIM
)

set(OUTPUT_NAME CMakeRenameSymbols)
project(${OUTPUT_NAME} VERSION 1.0.0 LANGUAGES CXX)

add_executable(${OUTPUT_NAME} rename_symbols_list.cpp)
target_compile_features(${OUTPUT_NAME} PRIVATE cxx_std_17)

add_custom_command(
    TARGET ${OUTPUT_NAME}
    COMMENT "Copy Output"
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}
    VERBATIM
)

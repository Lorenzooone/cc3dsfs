# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# which C and C++ compiler to use
set(CMAKE_C_COMPILER "/usr/bin/x86_64-w64-mingw32-gcc-posix" "-static-libstdc++" "-static-libgcc")
set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-w64-mingw32-g++-posix" "-static-libstdc++" "-static-libgcc")

# here is the target environment located
#SET(CMAKE_FIND_ROOT_PATH  /usr/i686-w64-mingw32)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Link with libpthread
set(MINGW_CROSS_COMPILATION ON)

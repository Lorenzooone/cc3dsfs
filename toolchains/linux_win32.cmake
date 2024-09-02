# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

# which C and C++ compiler to use
set(CMAKE_C_COMPILER "/usr/bin/i686-w64-mingw32-gcc-posix" "-static-libstdc++" "-static-libgcc")
set(CMAKE_CXX_COMPILER "/usr/bin/i686-w64-mingw32-g++-posix" "-static-libstdc++" "-static-libgcc")

# here is the target environment located
#SET(CMAKE_FIND_ROOT_PATH  /usr/i686-w64-mingw32)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# This doesn't work on Ubuntu 22.04 due to libkernel32.dll being too old. It does work on 24.01.
# For some reason, in windows 32 bit builds, ftd3xx requires some missing
# unspecified symbols. For that reason, we link dynamically with it
set(FORCE_WINDOWS_FTD3XX_SHARED_LIB ON)
# Link with libpthread
set(MINGW_CROSS_COMPILATION ON)

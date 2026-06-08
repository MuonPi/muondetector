set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

find_program(CMAKE_MAKE_PROGRAM ninja PATHS /usr/bin /usr/local/bin NO_DEFAULT_PATH)


set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_STRIP        arm-linux-gnueabihf-strip)

message(STATUS "CMAKE_PREFIX_PATH" ${CMAKE_PREFIX_PATH})
message(STATUS "Qt6_DIR" ${Qt6_DIR})
message(STATUS "Qt6Core_DIR" ${Qt6Core_DIR})

message(STATUS "CMAKE_C_COMPILER" ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_CXX_COMPILER" ${CMAKE_CXX_COMPILER})
message(STATUS "CMAKE_STRIP" ${CMAKE_STRIP})

# Sysroot — can be overridden on the cmake command line via -DCMAKE_SYSROOT=
if(NOT CMAKE_SYSROOT)
    set(CMAKE_SYSROOT /var/armhf/sysroot)
endif()

set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")

# For these categories, look ONLY inside the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # host tools (moc etc.) found via QT_HOST_PATH, not here
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Prevents CMake from re-rooting absolute paths that are already correct
set(CMAKE_SYSROOT_COMPILE "${CMAKE_SYSROOT}")
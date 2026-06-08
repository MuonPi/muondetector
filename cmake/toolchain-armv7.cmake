set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

set(QT_HOST_PATH /usr/lib/qt6 CACHE PATH "" FORCE)
set(Qt6CoreTools_DIR ${QT_HOST_PATH}/lib/cmake/Qt6CoreTools)
set(Qt6QmlTools_DIR  ${QT_HOST_PATH}/lib/cmake/Qt6QmlTools)

# Pin host ninja before generator probe
find_program(CMAKE_MAKE_PROGRAM ninja    PATHS /usr/bin /usr/local/bin NO_DEFAULT_PATH)
find_program(GIT_EXECUTABLE     git      PATHS /usr/bin /usr/local/bin NO_DEFAULT_PATH)
find_program(Python3_EXECUTABLE python3  PATHS /usr/bin /usr/local/bin NO_DEFAULT_PATH)
set(PKG_CONFIG_EXECUTABLE /usr/bin/pkg-config CACHE FILEPATH "" FORCE)

# Hardcode the host cross-compiler directly — do NOT use find_program here,
# it can still be influenced by the cache or prefix path
set(CMAKE_C_COMPILER   /usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)
set(CMAKE_STRIP        /usr/bin/arm-linux-gnueabihf-strip)


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
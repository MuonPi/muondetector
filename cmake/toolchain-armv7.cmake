set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

set(QT_HOST_PATH /usr CACHE PATH "" FORCE)
set(QT_HOST_PATH_CMAKE_DIR /usr/lib/x86_64-linux-gnu/cmake CACHE PATH "" FORCE)
set(Qt6CoreTools_DIR ${QT_HOST_PATH_CMAKE_DIR}/Qt6CoreTools CACHE PATH "" FORCE)
set(Qt6GuiTools_DIR ${QT_HOST_PATH_CMAKE_DIR}/Qt6GuiTools CACHE PATH "" FORCE)
set(Qt6QmlTools_DIR ${QT_HOST_PATH_CMAKE_DIR}/Qt6QmlTools CACHE PATH "" FORCE)
set(Qt6QuickTools_DIR ${QT_HOST_PATH_CMAKE_DIR}/Qt6QuickTools CACHE PATH "" FORCE)

# Pin host tools before CMake's generator/compiler probes. Do not use
# find_program() here: when CMAKE_SYSROOT is supplied on the command line,
# CMake can re-root the lookup and find ARM executables inside the sysroot.
set(CMAKE_MAKE_PROGRAM /usr/bin/ninja CACHE FILEPATH "" FORCE)
set(GIT_EXECUTABLE     /usr/bin/git CACHE FILEPATH "" FORCE)
set(Python3_EXECUTABLE /usr/bin/python3 CACHE FILEPATH "" FORCE)
set(PKG_CONFIG_EXECUTABLE /usr/bin/pkg-config CACHE FILEPATH "" FORCE)
if(EXISTS /usr/bin/ccache)
    set(CCACHE_PROGRAM /usr/bin/ccache CACHE FILEPATH "" FORCE)
else()
    set(CCACHE_PROGRAM "" CACHE FILEPATH "" FORCE)
endif()

# Hardcode the host cross-compiler directly — do NOT use find_program here,
# it can still be influenced by the cache or prefix path
set(CMAKE_C_COMPILER   /usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)
set(CMAKE_STRIP        /usr/bin/arm-linux-gnueabihf-strip)
set(QMAKE_EXECUTABLE   /usr/bin/qmake6)


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

# Keep pkg-config pointed at the target sysroot. Otherwise modules such as
# glib-2.0 and libsecret-1 can resolve to host x86_64 paths during configure.
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
    "${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf/pkgconfig:${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig"
)

# For these categories, look ONLY inside the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # host tools (moc etc.) found via QT_HOST_PATH, not here
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

# Prevents CMake from re-rooting absolute paths that are already correct
set(CMAKE_SYSROOT_COMPILE "${CMAKE_SYSROOT}")

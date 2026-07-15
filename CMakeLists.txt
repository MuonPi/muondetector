cmake_minimum_required (VERSION 3.10)
project (muondetector LANGUAGES CXX C)

string(TIMESTAMP PROJECT_DATE_STRING "%b %d, %Y")

option(MUONDETECTOR_BUILD_GUI "Build the gui for the muondetector" ON)
option(MUONDETECTOR_BUILD_DAEMON "Build the daemon for the muondetector. Defaults to ON on armv7l architecture" ON)
option(MUONDETECTOR_BUILD_HARDWARE_LIB "Build the static library for i2c and uart devices" ON)
option(MUONDETECTOR_BUILD_TESTS "Build integration tests" OFF)
option(MUONDETECTOR_BUILD_TCP_DEBUG_CLIENT "Build tcp connection client for testing" OFF)
option(MUONDETECTOR_BUILD_TCP_DEBUG_SERVER "Build tcp testserver" OFF)
option(PACKAGING_MODE "Disable deploy steps for packaging" OFF)
set(MUONDETECTOR_ON_RASPBERRYPI OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")

execute_process(COMMAND bash "-c" "git rev-parse --short HEAD | tr -d '\n'"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_VERSION_HASH)



if(EXISTS "/sys/firmware/devicetree/base/model")
    file(READ "/sys/firmware/devicetree/base/model" RPI_MODEL)
    string(FIND "${RPI_MODEL}" "Raspberry Pi" RPI_FOUND)

    if(NOT RPI_FOUND EQUAL -1)
        set(MUONDETECTOR_ON_RASPBERRYPI ON)
    endif()
endif()

if (CMAKE_CROSSCOMPILING)
    set(MUONDETECTOR_ON_RASPBERRYPI ON)
endif()

set(MUONDETECTOR_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/bin")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CROSSCOMPILING)
    find_program(CCACHE_PROGRAM ccache)
endif()

if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug
    Release
    RelWithDebInfo
    MinSizeRel
)

option(BUILD_TIDY "Enable clang-tidy during build" OFF)

if(BUILD_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)

    if(CLANG_TIDY_EXE)
        message(STATUS "Enabling clang-tidy: ${CLANG_TIDY_EXE}")

        set(CMAKE_CXX_CLANG_TIDY
            ${CLANG_TIDY_EXE}
            -p=${CMAKE_BINARY_DIR}
        )
    else()
        message(WARNING "BUILD_TIDY=ON but clang-tidy not found")
    endif()
endif()

# Always-on warnings
add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
    -Wno-psabi # Do not warn about this. Only important if linking against external prebuilt C++ libs
    # -ggdb # TODO: Remove
    # -fno-omit-frame-pointer # TODO: Remove
)

# Build mode optimization
if(BUILD_TIDY)
    add_compile_options(-O0 -g)
else()
    add_compile_options(-O3)
    # add_compile_options(-O1)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
add_compile_options(
    -mthumb
    -march=armv7-a
    -mfpu=vfp
    -mfloat-abi=hard
    )
endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv6l")
add_compile_options(
    -march=armv6z
    -mfpu=vfp
    -mfloat-abi=hard
    -mtune=arm1176jzf-s
    )
endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv6l")


if (NOT (WIN32 OR APPLE))
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/MuonPi/muondetector")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MuonPi <developer@muonpi.org>")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
endif ()


if (CMAKE_CROSSCOMPILING)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE armhf)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS
        "/var/armhf/sysroot/lib"
        "/var/armhf/sysroot/usr/lib"
        "/var/armhf/sysroot/usr/lib/arm-linux-gnueabihf"
    )
endif()

set(CPACK_RESOURCE_FILE_LICENSE "${MUONDETECTOR_CONFIG_DIR}/license")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/packages")
set(CPACK_PACKAGE_VENDOR "MuonPi.org")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}${PROJECT_VERSION_ADDITIONAL}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")


include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/capnp.cmake")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/library.cmake")

set(MUONDETECTOR_ALL_FILES
    "${MUONDETECTOR_LIBRARY_SOURCE_FILES}"
    "${MUONDETECTOR_LIBRARY_HEADER_FILES}"
    )
if (MUONDETECTOR_BUILD_GUI)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/qwt.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gui.cmake")

set(MUONDETECTOR_ALL_FILES
    "${MUONDETECTOR_ALL_FILES}"
    "${MUONDETECTOR_GUI_SOURCE_FILES}"
    "${MUONDETECTOR_GUI_HEADER_FILES}"
    )
endif (MUONDETECTOR_BUILD_GUI)

if (MUONDETECTOR_BUILD_HARDWARE_LIB OR MUONDETECTOR_BUILD_DAEMON)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/hardware.cmake")
endif()

if (MUONDETECTOR_BUILD_DAEMON)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/daemon.cmake")

set(MUONDETECTOR_ALL_FILES
    "${MUONDETECTOR_ALL_FILES}"
    "${MUONDETECTOR_DAEMON_SOURCE_FILES}"
    "${MUONDETECTOR_DAEMON_HEADER_FILES}"
    "${MUONDETECTOR_LOGIN_SOURCE_FILES}"
    "${MUONDETECTOR_LOGIN_HEADER_FILES}"
    )
endif (MUONDETECTOR_BUILD_DAEMON)

if (MUONDETECTOR_BUILD_TESTS AND MUONDETECTOR_BUILD_DAEMON)
enable_testing()
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif ()

if (MUONDETECTOR_BUILD_TCP_DEBUG_CLIENT)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/tcpclient.cmake")
endif ()
if (MUONDETECTOR_BUILD_TCP_DEBUG_SERVER)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/testserver.cmake")
endif ()

add_custom_target(clangformat COMMAND clang-format -style=WebKit -i ${MUONDETECTOR_ALL_FILES})

set(CPACK_DEBIAN_PACKAGE_NAME "muondetector")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL gui daemon)


set(CPACK_DEBIAN_GUI_PACKAGE_NAME "muondetector-gui")
set(CPACK_DEBIAN_DAEMON_PACKAGE_NAME "muondetector-daemon")

include(CPack)

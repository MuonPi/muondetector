cmake_minimum_required (VERSION 3.10)
project (muondetector LANGUAGES CXX C)

string(TIMESTAMP PROJECT_DATE_STRING "%b %d, %Y")

option(MUONDETECTOR_BUILD_GUI "Build the gui for the muondetector" ON)
option(MUONDETECTOR_BUILD_DAEMON "Build the daemon for the muondetector. Defaults to ON on armv7l architecture" ON)
option(MUONDETECTOR_BUILD_TESTS "Build integration tests" ON)
option(MUONDETECTOR_BUILD_TCP_DEBUG_CLIENT "Build tcp connection client for testing" ON)
set(MUONDETECTOR_ON_RASPBERRYPI OFF)


include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")

execute_process(COMMAND bash "-c" "git rev-parse --short HEAD | tr -d '\n'"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_VERSION_HASH)


if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(armv7|armv6|aarch64|arm64)")
    set(MUONDETECTOR_BUILD_DAEMON ON)
    set(MUONDETECTOR_ON_RASPBERRYPI ON)
endif ()

set(MUONDETECTOR_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/bin")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

# Build mode optimization
if(BUILD_TIDY)
    add_compile_options(-O0 -g)
else()
    add_compile_options(-O3)
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
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gui.cmake")

set(MUONDETECTOR_ALL_FILES
    "${MUONDETECTOR_ALL_FILES}"
    "${MUONDETECTOR_GUI_SOURCE_FILES}"
    "${MUONDETECTOR_GUI_HEADER_FILES}"
    )
endif (MUONDETECTOR_BUILD_GUI)
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

add_custom_target(clangformat COMMAND clang-format -style=WebKit -i ${MUONDETECTOR_ALL_FILES})


set(CPACK_DEB_COMPONENT_INSTALL ON)

include(CPack)

cmake_minimum_required (VERSION 3.10)
project (muondetector LANGUAGES CXX C)

string(TIMESTAMP PROJECT_DATE_STRING "%b %d, %Y")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")


set(MUONDETECTOR_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/bin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MUONDETECTOR_BUILD_GUI "Build the gui for the muondetector" ON)

set(CMAKE_BUILD_TYPE Release)

add_compile_options(
    -Wall
    -O3
    )
if (MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
else()

add_compile_options(
    -Wextra
    -Wshadow
    -Wpedantic
    )

endif()

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")
add_compile_options(
    -mthumb
    -mthumb-interwork
    -march=armv7-a
    )
endif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")


include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/library.cmake")

if (MUONDETECTOR_BUILD_GUI)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gui.cmake")
endif (MUONDETECTOR_BUILD_GUI)
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/daemon.cmake")
endif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")


if (NOT WIN32)
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/MuonPi/muondetector")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MuonPi <developer@muonpi.org>")
endif ()

set(CPACK_RESOURCE_FILE_LICENSE "${MUONDETECTOR_CONFIG_DIR}/license")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output/packages/")
set(CPACK_PACKAGE_VENDOR "MuonPi.org")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_DEB_COMPONENT_INSTALL ON)

include(CPack)

cmake_minimum_required (VERSION 3.10)
project (muondetector)

set(MUONDETECTOR_BUILD_TIDY OFF)

set(CMAKE_BUILD_TYPE Debug)
message("${CMAKE_HOST_SYSTEM_PROCESSOR}")
add_subdirectory(library)
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64")
add_subdirectory(daemon)
endif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "arm64")
add_subdirectory(gui)
add_subdirectory(analysis)

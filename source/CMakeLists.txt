cmake_minimum_required (VERSION 3.10)
project (muondetector)

set(MUONDETECTOR_BUILD_TIDY OFF)

set(MUONDETECTOR_BUILD_GUI ON)

set(CMAKE_BUILD_TYPE Debug)
message("${CMAKE_HOST_SYSTEM_PROCESSOR}")
add_subdirectory(library)
if (MUONDETECTOR_BUILD_GUI)
add_subdirectory(gui)
endif (MUONDETECTOR_BUILD_GUI)
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")
add_subdirectory(daemon)
else (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")
add_subdirectory(analysis)
add_subdirectory(tools)
endif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")

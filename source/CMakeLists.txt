cmake_minimum_required (VERSION 3.10)
project (muondetector)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")

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


include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/library.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gui.cmake")

if (MUONDETECTOR_BUILD_GUI)
add_subdirectory(gui)
endif (MUONDETECTOR_BUILD_GUI)
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")
add_subdirectory(daemon)
endif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")

cmake_minimum_required(VERSION 3.10)
project(muondetector-tools LANGUAGES CXX)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin/tools")

if(${MUONDETECTOR_BUILD_TIDY})
  set(CMAKE_CXX_CLANG_TIDY
      clang-tidy;
      -header-filter=^global;
      -checks=-*,readability-*,bugprone-*,performace-*,clang-analyzer-*,modernize-*,hicpp-*;
      )
endif(${MUONDETECTOR_BUILD_TIDY})

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019_64/lib/cmake/Qt5")
else()
set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")
endif()

find_package(Qt5 COMPONENTS Network REQUIRED)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_compile_options(
    -Wall
#    -Werror
    -O3
    )
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
add_compile_options(
    -Wextra
    -Wshadow
    -Wpedantic
    )
endif()
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
endif()

set(MAC_ADDRESSES_SOURCE_FILES
    "${PROJECT_SRC_DIR}/getmacaddresses.cpp"
    )

add_executable(getmacaddresses ${MAC_ADDRESSES_SOURCE_FILES})

target_include_directories(getmacaddresses PUBLIC
    $<BUILD_INTERFACE:${PROJECT_HEADER_DIR}>
    )

target_link_libraries(getmacaddresses
    Qt5::Network
    )



install(TARGETS getmacaddresses DESTINATION bin)

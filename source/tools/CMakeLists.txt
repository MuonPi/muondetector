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

set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")

find_package(Qt5 COMPONENTS Network REQUIRED)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
#    -Werror
    -O3
    )

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

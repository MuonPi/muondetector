cmake_minimum_required(VERSION 3.10)
project(muondetector-analysis LANGUAGES CXX)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin/analysis")

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

find_package(Qt5 COMPONENTS Network Widgets REQUIRED)


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

set(COMPARE_V4_SOURCE_FILES
    "${PROJECT_SRC_DIR}/compare_v4.cpp"
    )

add_executable(compare_v4 ${COMPARE_V4_SOURCE_FILES})

set(COMPARE_V5_SOURCE_FILES
    "${PROJECT_SRC_DIR}/compare_v5.cpp"
    )

add_executable(compare_v5 ${COMPARE_V5_SOURCE_FILES})



set(COMPARE_V6_SOURCE_FILES
    "${PROJECT_SRC_DIR}/compare_v6/compare_v6.cpp"
    "${PROJECT_SRC_DIR}/compare_v6/main.cpp"
    "${PROJECT_SRC_DIR}/compare_v6/filemodel.cpp"
    "${PROJECT_SRC_DIR}/compare_v6/mainwindow.cpp"
    "${PROJECT_SRC_DIR}/compare_v6/selectionmodel.cpp"
    "${PROJECT_SRC_DIR}/compare_v6/treeitem.cpp"
    "${PROJECT_SRC_DIR}/compare_v6/mainwindow.ui"
    )

set(COMPARE_V6_HEADER_FILES
    "${PROJECT_HEADER_DIR}/compare_v6/filemodel.h"
    "${PROJECT_HEADER_DIR}/compare_v6/mainwindow.h"
    "${PROJECT_HEADER_DIR}/compare_v6/selectionmodel.h"
    "${PROJECT_HEADER_DIR}/compare_v6/treeitem.h"
    )

add_executable(compare_v6 ${COMPARE_V6_SOURCE_FILES} ${COMPARE_V6_HEADER_FILES})

target_include_directories(compare_v6 PRIVATE
    "${PROJECT_HEADER_DIR}/compare_v6/"
    )

target_link_libraries(compare_v6
    Qt5::Widgets
    )



install(TARGETS compare_v4 compare_v5 compare_v6 DESTINATION bin)

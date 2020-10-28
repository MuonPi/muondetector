cmake_minimum_required(VERSION 3.10)
project(muondetector-gui LANGUAGES CXX)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
set(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../library/include/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")

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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")

find_package(Qt5 COMPONENTS Network Svg Widgets Gui Quick QuickWidgets Qml REQUIRED)

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
    -Werror
    -O3
    )

set(PROJECT_SOURCE_FILES
    "${PROJECT_SRC_DIR}/calibform.cpp"
    "${PROJECT_SRC_DIR}/calibscandialog.cpp"
    "${PROJECT_SRC_DIR}/custom_histogram_widget.cpp"
    "${PROJECT_SRC_DIR}/custom_plot_widget.cpp"
    "${PROJECT_SRC_DIR}/gnssposwidget.cpp"
    "${PROJECT_SRC_DIR}/gpssatsform.cpp"
    "${PROJECT_SRC_DIR}/histogramdataform.cpp"
    "${PROJECT_SRC_DIR}/i2cform.cpp"
    "${PROJECT_SRC_DIR}/logplotswidget.cpp"
    "${PROJECT_SRC_DIR}/main.cpp"
    "${PROJECT_SRC_DIR}/mainwindow.cpp"
    "${PROJECT_SRC_DIR}/map.cpp"
    "${PROJECT_SRC_DIR}/parametermonitorform.cpp"
    "${PROJECT_SRC_DIR}/plotcustom.cpp"
    "${PROJECT_SRC_DIR}/scanform.cpp"
    "${PROJECT_SRC_DIR}/settings.cpp"
    "${PROJECT_SRC_DIR}/spiform.cpp"
    "${PROJECT_SRC_DIR}/status.cpp"
    )

set(PROJECT_HEADER_FILES
    "${PROJECT_HEADER_DIR}/calibform.h"
    "${PROJECT_HEADER_DIR}/calibscandialog.h"
    "${PROJECT_HEADER_DIR}/custom_histogram_widget.h"
    "${PROJECT_HEADER_DIR}/custom_plot_widget.h"
    "${PROJECT_HEADER_DIR}/gnssposwidget.h"
    "${PROJECT_HEADER_DIR}/gpssatsform.h"
    "${PROJECT_HEADER_DIR}/histogramdataform.h"
    "${PROJECT_HEADER_DIR}/i2cform.h"
    "${PROJECT_HEADER_DIR}/logplotswidget.h"
    "${PROJECT_HEADER_DIR}/mainwindow.h"
    "${PROJECT_HEADER_DIR}/map.h"
    "${PROJECT_HEADER_DIR}/parametermonitorform.h"
    "${PROJECT_HEADER_DIR}/plotcustom.h"
    "${PROJECT_HEADER_DIR}/scanform.h"
    "${PROJECT_HEADER_DIR}/settings.h"
    "${PROJECT_HEADER_DIR}/spiform.h"
    "${PROJECT_HEADER_DIR}/status.h"
    )
set(PROJECT_UI_FILES
    "${PROJECT_UI_DIR}/calibform.ui"
    "${PROJECT_UI_DIR}/calibscandialog.ui"
    "${PROJECT_UI_DIR}/gnssposwidget.ui"
    "${PROJECT_UI_DIR}/gpssatsform.ui"
    "${PROJECT_UI_DIR}/histogramdataform.ui"
    "${PROJECT_UI_DIR}/i2cform.ui"
    "${PROJECT_UI_DIR}/logplotswidget.ui"
    "${PROJECT_UI_DIR}/mainwindow.ui"
    "${PROJECT_UI_DIR}/map.ui"
    "${PROJECT_UI_DIR}/parametermonitorform.ui"
    "${PROJECT_UI_DIR}/scanform.ui"
    "${PROJECT_UI_DIR}/settings.ui"
    "${PROJECT_UI_DIR}/spiform.ui"
    "${PROJECT_UI_DIR}/status.ui"
    )
set(PROJECT_RESOURCE_FILES
    "${PROJECT_RES_DIR}/resources.qrc"
    )

add_executable(muondetector-gui ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES} ${PROJECT_UI_FILES} ${PROJECT_RESOURCE_FILES})
target_include_directories(muondetector-gui PUBLIC
    $<BUILD_INTERFACE:${PROJECT_HEADER_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>
    )
target_link_libraries(muondetector-gui
    Qt5::Network Qt5::Svg Qt5::Widgets Qt5::Gui Qt5::Quick Qt5::QuickWidgets Qt5::Qml
    "${CMAKE_CURRENT_BINARY_DIR}/../lib/libmuondetector.a"
    )
target_include_directories(muondetector-gui PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<BUILD_INTERFACE:/usr/include/qwt>
    $<INSTALL_INTERFACE:include/mqtt>)

install(TARGETS muondetector-gui DESTINATION bin)

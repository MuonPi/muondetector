cmake_minimum_required(VERSION 3.10)
project(muondetector-shared LANGUAGES CXX)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

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

find_package(Qt5 COMPONENTS Network REQUIRED)

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
    -Werror
    -O3
    )

set(PROJECT_SOURCE_FILES
    "${PROJECT_SRC_DIR}/tcpconnection.cpp"
    "${PROJECT_SRC_DIR}/tcpmessage.cpp"
    "${PROJECT_SRC_DIR}/mqtthandler.cpp"
    )

set(PROJECT_HEADER_FILES
    "${PROJECT_HEADER_DIR}/muondetector_shared_global.h"
    "${PROJECT_HEADER_DIR}/gpio_pin_definitions.h"
    "${PROJECT_HEADER_DIR}/histogram.h"
    "${PROJECT_HEADER_DIR}/tcpconnection.h"
    "${PROJECT_HEADER_DIR}/tcpmessage.h"
    "${PROJECT_HEADER_DIR}/mqtthandler.h"
    "${PROJECT_HEADER_DIR}/tcpmessage_keys.h"
    "${PROJECT_HEADER_DIR}/ublox_messages.h"
    "${PROJECT_HEADER_DIR}/ubx_msg_key_name_map.h"
    "${PROJECT_HEADER_DIR}/ublox_structs.h"
    "${PROJECT_HEADER_DIR}/muondetector_structs.h"
    )

add_definitions(-DMUONDETECTOR_SHARED_LIBRARY)

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_HEADER_DIR})
target_link_libraries(${PROJECT_NAME} Qt5::Network paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3)
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>)

add_library(muondetector_static STATIC ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})
set_target_properties(muondetector_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_include_directories(muondetector_static PUBLIC ${PROJECT_HEADER_DIR})
target_link_libraries(muondetector_static Qt5::Network paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3)
target_include_directories(muondetector_static PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>)

install(TARGETS ${PROJECT_NAME} muondetector_static DESTINATION lib)
install(FILES ${PROJECT_HEADER_FILES} DESTINATION include)

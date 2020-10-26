cmake_minimum_required(VERSION 3.10)
project(muondetector-login LANGUAGES CXX)


set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
set(DAEMON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../muondetector-daemon/src/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

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
    "${PROJECT_SRC_DIR}/muondetector-login.cpp"
    "${DAEMON_INCLUDE_DIR}/mqtthandler.cpp")

set(PROJECT_HEADER_FILES
    "${DAEMON_INCLUDE_DIR}/mqtthandler.h")

set(PROJECT_INSTALL_FILES
    "${PROJECT_CONFIG_DIR}/muondetector-login.sh")



add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${DAEMON_INCLUDE_DIR}>)
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>)

target_link_libraries(${PROJECT_NAME} Qt5::Network crypto++ paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3)

install(FILES ${PROJECT_INSTALL_FILES} DESTINATION bin)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

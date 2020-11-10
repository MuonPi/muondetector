cmake_minimum_required(VERSION 3.10)
project(muondetector-cluster LANGUAGES CXX C)


set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
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

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")

find_package(Qt5 COMPONENTS Network SerialPort REQUIRED)

find_library(PAHO_MQTT3C paho-mqtt3c REQUIRED)
find_library(PAHO_MQTT3A paho-mqtt3a REQUIRED)
find_library(PAHO_MQTT3CS paho-mqtt3cs REQUIRED)
find_library(PAHO_MQTT3AS paho-mqtt3as REQUIRED)
find_library(PAHO_MQTTPP3 paho-mqttpp3 REQUIRED)

find_library(CRYPTOPP crypto++ REQUIRED)
find_library(CONFIGPP config++ REQUIRED)

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
#    -Werror
    -O3
    )





set(DAEMON_SOURCE_FILES
    "${PROJECT_SRC_DIR}/main.cpp"
    "${PROJECT_SRC_DIR}/core.cpp"
    "${PROJECT_SRC_DIR}/abstractevent.cpp"
    "${PROJECT_SRC_DIR}/combinedevent.cpp"
    "${PROJECT_SRC_DIR}/databasesink.cpp"
    "${PROJECT_SRC_DIR}/mqttlink.cpp"
    "${PROJECT_SRC_DIR}/singleevent.cpp"
    "${PROJECT_SRC_DIR}/abstracteventsink.cpp"
    "${PROJECT_SRC_DIR}/databasesource.cpp"
    "${PROJECT_SRC_DIR}/mqttsink.cpp"
    "${PROJECT_SRC_DIR}/abstracteventsource.cpp"
    "${PROJECT_SRC_DIR}/criterion.cpp"
    "${PROJECT_SRC_DIR}/mqttsource.cpp"
    "${PROJECT_SRC_DIR}/coincidence.cpp"
    "${PROJECT_SRC_DIR}/databaselink.cpp"
    "${PROJECT_SRC_DIR}/messagebypass.cpp"
    "${PROJECT_SRC_DIR}/plausability.cpp"
    )

set(DAEMON_HEADER_FILES
    "${PROJECT_HEADER_DIR}/core.h"
    "${PROJECT_HEADER_DIR}/abstractevent.h"
    "${PROJECT_HEADER_DIR}/abstracteventsink.h"
    "${PROJECT_HEADER_DIR}/abstracteventsource.h"
    "${PROJECT_HEADER_DIR}/coincidence.h"
    "${PROJECT_HEADER_DIR}/combinedevent.h"
    "${PROJECT_HEADER_DIR}/criterion.h"
    "${PROJECT_HEADER_DIR}/databaselink.h"
    "${PROJECT_HEADER_DIR}/databasesink.h"
    "${PROJECT_HEADER_DIR}/databasesource.h"
    "${PROJECT_HEADER_DIR}/messagebypass.h"
    "${PROJECT_HEADER_DIR}/mqttlink.h"
    "${PROJECT_HEADER_DIR}/mqttsink.h"
    "${PROJECT_HEADER_DIR}/mqttsource.h"
    "${PROJECT_HEADER_DIR}/plausability.h"
    "${PROJECT_HEADER_DIR}/singleevent.h"
    )


add_executable(muondetector-cluster ${DAEMON_SOURCE_FILES} ${DAEMON_HEADER_FILES})

target_include_directories(muondetector-cluster PUBLIC
    $<BUILD_INTERFACE:${PROJECT_HEADER_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)

target_include_directories(muondetector-cluster PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>
    )

target_link_libraries(muondetector-cluster
    Qt5::Network Qt5::SerialPort
    crypto++
    pigpiod_if2
    rt
    config++
    paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3
    "${CMAKE_CURRENT_BINARY_DIR}/../lib/libmuondetector.a"
    )


set(CLUSTER_INSTALL_FILES
#    "${PROJECT_CONFIG_DIR}/muondetector.conf"
    )


install(TARGETS muondetector-cluster DESTINATION bin COMPONENT cluster)
#install(FILES ${CLUSTER_INSTALL_FILES} DESTINATION "/etc/muondetector/" COMPONENT cluster)
install(FILES "${PROJECT_CONFIG_DIR}/muondetector-cluster.service" DESTINATION "lib/systemd/system" COMPONENT cluster)

enable_testing()

add_executable(test-coincicence "${PROJECT_TEST_DIR}/coincidence.cpp")
add_test(NAME "Coincidence Class Test" COMMAND test-coincidence)
add_executable(test-combinedevent "${PROJECT_TEST_DIR}/combinedevent.cpp")
add_test(NAME "CombinedEvent Class Test" COMMAND test-combinedevent)
add_executable(test-core "${PROJECT_TEST_DIR}/core.cpp")
add_test(NAME "Core Class Test" COMMAND test-core)
add_executable(test-databaselink "${PROJECT_TEST_DIR}/databaselink.cpp")
add_test(NAME "DatabaseLink Class Test" COMMAND test-databaselink)
add_executable(test-databasesink "${PROJECT_TEST_DIR}/databasesink.cpp")
add_test(NAME "DatabaseSink Class Test" COMMAND test-databasesink)
add_executable(test-databasesource "${PROJECT_TEST_DIR}/databasesource.cpp")
add_test(NAME "DatabaseSource Class Test" COMMAND test-databasesource)
add_executable(test-messagebypass "${PROJECT_TEST_DIR}/messagebypass.cpp")
add_test(NAME "MessageBypass Class Test" COMMAND test-messagebypass)
add_executable(test-mqttlink "${PROJECT_TEST_DIR}/mqttlink.cpp")
add_test(NAME "MqttLink Class Test" COMMAND test-mqttlink)
add_executable(test-mqttsink "${PROJECT_TEST_DIR}/mqttsink.cpp")
add_test(NAME "MqttSink Class Test" COMMAND test-mqttsink)
add_executable(test-mqttsource "${PROJECT_TEST_DIR}/mqttsource.cpp")
add_test(NAME "MqqtSource Class Test" COMMAND test-mqttsource)
add_executable(test-plausability "${PROJECT_TEST_DIR}/plausability.cpp")
add_test(NAME "Plausability Class Test" COMMAND test-plausability)
add_executable(test-singleevent "${PROJECT_TEST_DIR}/singleevent.cpp")
add_test(NAME "SingleEvent Class Test" COMMAND test-singleevent)


#
#set(CPACK_GENERATOR "DEB")
#set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_CONFIG_DIR}/license")
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_CONFIG_DIR}/preinst;${PROJECT_CONFIG_DIR}/postinst;${PROJECT_CONFIG_DIR}/prerm;${PROJECT_CONFIG_DIR}/conffiles")
#set(CPACK_PACKAGE_VENDOR "MuonPi.org")
#set(CPACK_DEBIAN_PACKAGE_SECTION "net")
#set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/MuonPi/muondetector")
#set(CPACK_PACKAGE_VERSION_MAJOR "1")
#set(CPACK_PACKAGE_VERSION_MINOR "3")
#set(CPACK_PACKAGE_VERSION_PATCH "2")
#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Muondetector cluster")
#set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Cluster daemon
#    It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
#set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MuonPi.org <support@muonpi.org>")
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "lftp (>=4.7), libcrypto++6, libmuondetector-shared (>=1.2.2), libqt5serialport5 (>= 5), libpaho-mqttpp (>=1.1.0), libconfig++-dev, pigpiod")
#set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../packages/")

#include(CPack)

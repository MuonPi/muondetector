cmake_minimum_required(VERSION 3.10)
project(muondetector-cluster LANGUAGES CXX C)

set(MUONDETECTOR_ENABLE_TESTING OFF)

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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MUONDETECTOR_ENABLE_TESTING)
find_library(CPP_UNIT cppunit REQUIRED)
endif ()

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
    -Werror
    -O3
    -Wno-unused-private-field
    )


find_library(PAHO_MQTT3C paho-mqtt3c REQUIRED)
find_library(PAHO_MQTT3A paho-mqtt3a REQUIRED)
find_library(PAHO_MQTT3CS paho-mqtt3cs REQUIRED)
find_library(PAHO_MQTT3AS paho-mqtt3as REQUIRED)
find_library(PAHO_MQTTPP3 paho-mqttpp3 REQUIRED)

find_library(CRYPTOPP crypto++ REQUIRED)




set(CLUSTER_SOURCE_FILES
    "${PROJECT_SRC_DIR}/core.cpp"
    "${PROJECT_SRC_DIR}/event.cpp"
    "${PROJECT_SRC_DIR}/databaseeventsink.cpp"
    "${PROJECT_SRC_DIR}/mqttlink.cpp"
    "${PROJECT_SRC_DIR}/asciieventsink.cpp"
    "${PROJECT_SRC_DIR}/asciilogsink.cpp"
    "${PROJECT_SRC_DIR}/mqtteventsink.cpp"
    "${PROJECT_SRC_DIR}/mqtteventsource.cpp"
    "${PROJECT_SRC_DIR}/mqttlogsource.cpp"
    "${PROJECT_SRC_DIR}/coincidence.cpp"
    "${PROJECT_SRC_DIR}/databaselink.cpp"
    "${PROJECT_SRC_DIR}/ratesupervisor.cpp"
    "${PROJECT_SRC_DIR}/timebasesupervisor.cpp"
    "${PROJECT_SRC_DIR}/eventconstructor.cpp"
    "${PROJECT_SRC_DIR}/detector.cpp"
    "${PROJECT_SRC_DIR}/threadrunner.cpp"
    "${PROJECT_SRC_DIR}/detectorlog.cpp"
    "${PROJECT_SRC_DIR}/clusterlog.cpp"
    "${PROJECT_SRC_DIR}/detectortracker.cpp"
    "${PROJECT_SRC_DIR}/log.cpp"
    "${PROJECT_SRC_DIR}/utility.cpp"
    "${PROJECT_SRC_DIR}/statesupervisor.cpp"
    "${PROJECT_SRC_DIR}/mqttlogsink.cpp"
    )

set(CLUSTER_HEADER_FILES
    "${PROJECT_HEADER_DIR}/core.h"
    "${PROJECT_HEADER_DIR}/event.h"
    "${PROJECT_HEADER_DIR}/abstractsink.h"
    "${PROJECT_HEADER_DIR}/abstractsource.h"
    "${PROJECT_HEADER_DIR}/coincidence.h"
    "${PROJECT_HEADER_DIR}/criterion.h"
    "${PROJECT_HEADER_DIR}/databaselink.h"
    "${PROJECT_HEADER_DIR}/databaseeventsink.h"
    "${PROJECT_HEADER_DIR}/mqttlink.h"
    "${PROJECT_HEADER_DIR}/asciieventsink.h"
    "${PROJECT_HEADER_DIR}/asciilogsink.h"
    "${PROJECT_HEADER_DIR}/mqtteventsink.h"
    "${PROJECT_HEADER_DIR}/mqtteventsource.h"
    "${PROJECT_HEADER_DIR}/mqttlogsource.h"
    "${PROJECT_HEADER_DIR}/ratesupervisor.h"
    "${PROJECT_HEADER_DIR}/timebasesupervisor.h"
    "${PROJECT_HEADER_DIR}/eventconstructor.h"
    "${PROJECT_HEADER_DIR}/detector.h"
    "${PROJECT_HEADER_DIR}/threadrunner.h"
    "${PROJECT_HEADER_DIR}/detectorlog.h"
    "${PROJECT_HEADER_DIR}/clusterlog.h"
    "${PROJECT_HEADER_DIR}/detectortracker.h"
    "${PROJECT_HEADER_DIR}/log.h"
    "${PROJECT_HEADER_DIR}/utility.h"
    "${PROJECT_HEADER_DIR}/statesupervisor.h"
    "${PROJECT_HEADER_DIR}/mqttlogsink.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/influxdb.hpp"
    )

add_library(source-files OBJECT ${CLUSTER_SOURCE_FILES} ${CLUSTER_HEADER_FILES})
target_include_directories(source-files PUBLIC
    ${PROJECT_HEADER_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/external/"
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)

target_include_directories(source-files PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>
    )

add_executable(muondetector-cluster "${PROJECT_SRC_DIR}/main.cpp")

target_include_directories(muondetector-cluster PUBLIC
    ${PROJECT_HEADER_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/external/"
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)

target_include_directories(muondetector-cluster PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>
    )

target_link_libraries(muondetector-cluster
    crypto++
#    config++
    paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3
#    "${CMAKE_CURRENT_BINARY_DIR}/../lib/libmuondetector.a"
    source-files
    pthread
    )

target_include_directories(muondetector-cluster PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>)



install(TARGETS muondetector-cluster DESTINATION bin COMPONENT cluster)


if (MUONDETECTOR_ENABLE_TESTING)
enable_testing()

add_executable(test-coincidence "${PROJECT_TEST_DIR}/coincidence.cpp")
target_include_directories(test-coincidence PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-coincidence
    cppunit
    source-files
    pthread
    )
add_test(NAME "Coincidence-Test" COMMAND test-coincidence)

add_executable(test-combinedevent "${PROJECT_TEST_DIR}/combinedevent.cpp")
target_include_directories(test-combinedevent PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-combinedevent
    cppunit
    source-files
    pthread
    )
add_test(NAME "CombinedEvent-Test" COMMAND test-combinedevent)

add_executable(test-abstracteventsink "${PROJECT_TEST_DIR}/abstracteventsink.cpp")
target_include_directories(test-abstracteventsink PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-abstracteventsink
    cppunit
    source-files
    pthread
    )
add_test(NAME "AbstractEventSink-Test" COMMAND test-abstracteventsink)

add_executable(test-abstracteventsource "${PROJECT_TEST_DIR}/abstracteventsource.cpp")
target_include_directories(test-abstracteventsource PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-abstracteventsource
    cppunit
    source-files
    pthread
    )
add_test(NAME "AbstractEventSource-Test" COMMAND test-abstracteventsource)

add_executable(test-core "${PROJECT_TEST_DIR}/core.cpp")
target_include_directories(test-core PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-core
    cppunit
    source-files
    pthread
    )
add_test(NAME "Core-Test" COMMAND test-core)

add_executable(test-databaselink "${PROJECT_TEST_DIR}/databaselink.cpp")
target_include_directories(test-databaselink PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-databaselink
    cppunit
    source-files
    pthread
    )
add_test(NAME "DatabaseLink-Test" COMMAND test-databaselink)

add_executable(test-databasesink "${PROJECT_TEST_DIR}/databasesink.cpp")
target_include_directories(test-databasesink PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-databasesink
    cppunit
    source-files
    pthread
    )
add_test(NAME "DatabaseSink-Test" COMMAND test-databasesink)

add_executable(test-mqttlink "${PROJECT_TEST_DIR}/mqttlink.cpp")
target_include_directories(test-mqttlink PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-mqttlink
    cppunit
    source-files
    pthread
    )
add_test(NAME "MqttLink-Test" COMMAND test-mqttlink)

add_executable(test-mqttsink "${PROJECT_TEST_DIR}/mqttsink.cpp")
target_include_directories(test-mqttsink PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-mqttsink
    cppunit
    source-files
    pthread
    )
add_test(NAME "MqttSink-Test" COMMAND test-mqttsink)

add_executable(test-mqttsource "${PROJECT_TEST_DIR}/mqttsource.cpp")
target_include_directories(test-mqttsource PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-mqttsource
    cppunit
    source-files
    pthread
    )
add_test(NAME "MqqtSource-Test" COMMAND test-mqttsource)

add_executable(test-ratesupervisor "${PROJECT_TEST_DIR}/ratesupervisor.cpp")
target_include_directories(test-ratesupervisor PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-ratesupervisor
    cppunit
    source-files
    pthread
    )
add_test(NAME "RateSupervisor-Test" COMMAND test-ratesupervisor)

add_executable(test-timebasesupervisor "${PROJECT_TEST_DIR}/timebasesupervisor.cpp")
target_include_directories(test-timebasesupervisor PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-timebasesupervisor
    cppunit
    source-files
    pthread
    )
add_test(NAME "TimeBaseSupervisor-Test" COMMAND test-timebasesupervisor)

add_executable(test-detector "${PROJECT_TEST_DIR}/detector.cpp")
target_include_directories(test-detector PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-detector
    cppunit
    source-files
    pthread
    )
add_test(NAME "Detector-Test" COMMAND test-detector)

add_executable(test-eventconstructor "${PROJECT_TEST_DIR}/eventconstructor.cpp")
target_include_directories(test-eventconstructor PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_link_libraries(test-eventconstructor
    cppunit
    source-files
    pthread
    )
add_test(NAME "EventConstructor-Test" COMMAND test-eventconstructor)

endif ()


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

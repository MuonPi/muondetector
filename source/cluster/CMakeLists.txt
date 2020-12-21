cmake_minimum_required(VERSION 3.10)
project(muondetector-cluster LANGUAGES CXX C)

string(TIMESTAMP PROJECT_DATE_STRING "%b %d, %Y")

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
set(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../library/include/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/version.cmake")

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

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")


find_library(MOSQUITTO mosquitto REQUIRED)
find_library(CRYPTOPP crypto++ REQUIRED)
find_library(CURL curl REQUIRED)




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
    "${PROJECT_SRC_DIR}/timebasesupervisor.cpp"
    "${PROJECT_SRC_DIR}/eventconstructor.cpp"
    "${PROJECT_SRC_DIR}/detector.cpp"
    "${PROJECT_SRC_DIR}/threadrunner.cpp"
    "${PROJECT_SRC_DIR}/detectorinfo.cpp"
    "${PROJECT_SRC_DIR}/detectorlog.cpp"
    "${PROJECT_SRC_DIR}/clusterlog.cpp"
    "${PROJECT_SRC_DIR}/detectortracker.cpp"
    "${PROJECT_SRC_DIR}/log.cpp"
    "${PROJECT_SRC_DIR}/utility.cpp"
    "${PROJECT_SRC_DIR}/statesupervisor.cpp"
    "${PROJECT_SRC_DIR}/mqttlogsink.cpp"
    #"${PROJECT_SRC_DIR}/configuration.cpp"
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
    "${PROJECT_HEADER_DIR}/databaselogsink.h"
    "${PROJECT_HEADER_DIR}/mqttlink.h"
    "${PROJECT_HEADER_DIR}/asciieventsink.h"
    "${PROJECT_HEADER_DIR}/asciilogsink.h"
    "${PROJECT_HEADER_DIR}/mqtteventsink.h"
    "${PROJECT_HEADER_DIR}/mqtteventsource.h"
    "${PROJECT_HEADER_DIR}/mqttlogsource.h"
    "${PROJECT_HEADER_DIR}/timebasesupervisor.h"
    "${PROJECT_HEADER_DIR}/eventconstructor.h"
    "${PROJECT_HEADER_DIR}/detector.h"
    "${PROJECT_HEADER_DIR}/threadrunner.h"
    "${PROJECT_HEADER_DIR}/detectorinfo.h"
    "${PROJECT_HEADER_DIR}/detectorlog.h"
    "${PROJECT_HEADER_DIR}/clusterlog.h"
    "${PROJECT_HEADER_DIR}/detectortracker.h"
    "${PROJECT_HEADER_DIR}/log.h"
    "${PROJECT_HEADER_DIR}/utility.h"
    "${PROJECT_HEADER_DIR}/statesupervisor.h"
    "${PROJECT_HEADER_DIR}/mqttlogsink.h"
    #"${PROJECT_HEADER_DIR}/configuration.h"
    "${PROJECT_HEADER_DIR}/userinfo.h"
    )

configure_file(
    "${PROJECT_CONFIG_DIR}/muondetector-cluster.1"
    "${CMAKE_CURRENT_BINARY_DIR}/muondetector-cluster.1"
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
    mosquitto
    source-files
    pthread
    curl
    )

target_include_directories(muondetector-cluster PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>)



include(GNUInstallDirs)

add_custom_target(changelog-cluster ALL COMMAND gzip -cn9 "${PROJECT_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")
add_custom_target(manpage-cluster ALL COMMAND gzip -cn9 "${CMAKE_CURRENT_BINARY_DIR}/muondetector-cluster.1" > "${CMAKE_CURRENT_BINARY_DIR}/muondetector-cluster.1.gz")

install(TARGETS muondetector-cluster DESTINATION bin COMPONENT cluster)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT cluster)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-cluster.1.gz" DESTINATION "share/man/man1/" COMPONENT cluster)
install(FILES "${PROJECT_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT cluster)
install(FILES "${PROJECT_CONFIG_DIR}/muondetector-cluster.service" DESTINATION "/lib/systemd/system" COMPONENT cluster)



set(CPACK_GENERATOR "DEB")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_CONFIG_DIR}/license")
set(CPACK_PACKAGE_VENDOR "MuonPi.org")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/MuonPi/muondetector")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Daemon which calculates coincidences for incoming events")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "It subscribes to a mqtt topic to collect the incoming event messages and keep
track of individual event sources.
With these messages it calculates coincidences and depending on the context
publishes them to another mqtt topic or writes them to a database.
It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MuonPi <developer@muonpi.org>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>=2.28), libstdc++6 (>=7), lftp (>=4.7), libcrypto++6, libconfig++9v5, libcurl4")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../packages/")

include(CPack)

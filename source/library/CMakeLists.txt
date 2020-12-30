cmake_minimum_required(VERSION 3.10)
project(muondetector-shared LANGUAGES CXX)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/version.cmake")

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

if(WIN32)

set(QWT_DIR "C:/Qwt-6.1.4/")
set(CRYPTOPP_DIR "C:/cryptopp/")
set(MQTT_CPP_DIR "C:/paho-mqtt-cpp-1.1.0-win64")
set(MQTT_C_DIR "C:/eclipse-paho-mqtt-c-win64-1.3.6")
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019_64/lib/cmake/Qt5")

else()

set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")

endif()

if (MSVC)
    if("${MSVC_RUNTIME}" STREQUAL "")
        set(MSVC_RUNTIME "static")
    endif()
        # Set compiler options.
    set(variables
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    if(${MSVC_RUNTIME} STREQUAL "static")
      message(STATUS
        "MSVC -> forcing use of statically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
      endforeach()
    else()
      message(STATUS
        "MSVC -> forcing use of dynamically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
        endif()
      endforeach()
    endif()
endif()

find_package(Qt5 COMPONENTS Network REQUIRED)

if(NOT WIN32)

find_library(PAHO_MQTT3C paho-mqtt3c REQUIRED)
find_library(PAHO_MQTT3A paho-mqtt3a REQUIRED)
find_library(PAHO_MQTT3CS paho-mqtt3cs REQUIRED)
find_library(PAHO_MQTT3AS paho-mqtt3as REQUIRED)
find_library(PAHO_MQTTPP3 paho-mqttpp3 REQUIRED)

endif()

add_compile_options(
    -Wall
    -O3
    )
if(MSVC)
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


configure_file(
    "${PROJECT_CONFIG_DIR}/version.h"
    "${PROJECT_HEADER_DIR}/version.h"
    )

set(PROJECT_SOURCE_FILES
    "${PROJECT_SRC_DIR}/config.cpp"
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
    "${PROJECT_HEADER_DIR}/config.h"
    )

add_definitions(-DMUONDETECTOR_LIBRARY_EXPORT)

add_library(muondetector STATIC ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})
target_include_directories(muondetector PUBLIC
    ${PROJECT_HEADER_DIR}
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>
    )

if(WIN32)
target_link_directories(muondetector PUBLIC
    "${MQTT_C_DIR}/lib"
    "${MQTT_CPP_DIR}/lib"
    "${QWT_DIR}/lib"
    "${CRYPTOPP_DIR}/lib"
    )
target_link_libraries(muondetector qwt.lib Qt5::Network paho-mqtt3c.lib paho-mqtt3a.lib paho-mqtt3cs.lib paho-mqtt3as.lib paho-mqttpp3.lib cryptlib.lib)
target_include_directories(muondetector PUBLIC
    ${CRYPTOPP_DIR}/include/
    $<BUILD_INTERFACE:${QWT_DIR}/include>
    $<BUILD_INTERFACE:${MQTT_C_DIR}/include>
    $<INSTALL_INTERFACE:${MQTT_C_DIR}/include>
    $<BUILD_INTERFACE:${MQTT_CPP_DIR}/include/mqtt>
    $<INSTALL_INTERFACE:${MQTT_CPP_DIR}/include/mqtt>
    ${Qt5Network_INCLUDE_DIRS}
    )
else()
target_link_libraries(muondetector Qt5::Network paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3)
endif()

install(TARGETS muondetector DESTINATION lib COMPONENT library)
install(FILES ${PROJECT_HEADER_FILES} DESTINATION include COMPONENT library)

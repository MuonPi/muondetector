cmake_minimum_required(VERSION 3.10)
project(muondetector-daemon LANGUAGES CXX C)

string(TIMESTAMP PROJECT_DATE_STRING "%b %d, %Y")

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
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

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")

find_package(Qt5 COMPONENTS Network SerialPort REQUIRED)

if(NOT WIN32) # added to make program editable in qt-creator on windows

find_library(PAHO_MQTT3C paho-mqtt3c REQUIRED)
find_library(PAHO_MQTT3A paho-mqtt3a REQUIRED)
find_library(PAHO_MQTT3CS paho-mqtt3cs REQUIRED)
find_library(PAHO_MQTT3AS paho-mqtt3as REQUIRED)
find_library(PAHO_MQTTPP3 paho-mqttpp3 REQUIRED)

find_library(CRYPTOPP crypto++ REQUIRED)
find_library(CONFIGPP config++ REQUIRED)
find_library(PIGPIOD_IF2 pigpiod_if2 REQUIRED)
find_library(RT rt REQUIRED)

endif()

find_library(MUONDETECTOR_LIB NAMES muondetector HINTS "${CMAKE_CURRENT_BINARY_DIR}/../lib/" REQUIRED)

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
    "${PROJECT_SRC_DIR}/qtserialublox.cpp"
    "${PROJECT_SRC_DIR}/qtserialublox_processmessages.cpp"
    "${PROJECT_SRC_DIR}/pigpiodhandler.cpp"
    "${PROJECT_SRC_DIR}/daemon.cpp"
    "${PROJECT_SRC_DIR}/custom_io_operators.cpp"
    "${PROJECT_SRC_DIR}/filehandler.cpp"
    "${PROJECT_SRC_DIR}/calibration.cpp"
    "${PROJECT_SRC_DIR}/gpio_mapping.cpp"
    "${PROJECT_SRC_DIR}/logengine.cpp"
    "${PROJECT_SRC_DIR}/geohash.cpp"
    "${PROJECT_SRC_DIR}/tdc7200.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cbusses.c"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/adafruit_ssd1306/adafruit_ssd1306.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/ads1115/ads1115.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/bme280/bme280.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/bmp180/bmp180.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/eeprom24aa02/eeprom24aa02.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/hmc5883/hmc5883.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/lm75/lm75.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/mcp4728/mcp4728.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/pca9536/pca9536.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/sht21/sht21.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/ubloxi2c/ubloxi2c.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/x9119/x9119.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/Adafruit_GFX.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/i2cdevice.cpp"
    "${PROJECT_SRC_DIR}/i2c/i2cdevices/glcdfont.c"
    )

set(DAEMON_HEADER_FILES
    "${PROJECT_HEADER_DIR}/unixtime_from_gps.h"
    "${PROJECT_HEADER_DIR}/time_from_rtc.h"
    "${PROJECT_HEADER_DIR}/qtserialublox.h"
    "${PROJECT_HEADER_DIR}/daemon.h"
    "${PROJECT_HEADER_DIR}/pigpiodhandler.h"
    "${PROJECT_HEADER_DIR}/custom_io_operators.h"
    "${PROJECT_HEADER_DIR}/filehandler.h"
    "${PROJECT_HEADER_DIR}/calibration.h"
    "${PROJECT_HEADER_DIR}/logparameter.h"
    "${PROJECT_HEADER_DIR}/gpio_mapping.h"
    "${PROJECT_HEADER_DIR}/tdc7200.h"
    "${PROJECT_HEADER_DIR}/logengine.h"
    "${PROJECT_HEADER_DIR}/geohash.h"
    "${PROJECT_HEADER_DIR}/i2c/addresses.h"
    "${PROJECT_HEADER_DIR}/i2c/custom_i2cdetect.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cbusses.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/adafruit_ssd1306/adafruit_ssd1306.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/ads1015/ads1015.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/ads1115/ads1115.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/bme280/bme280.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/bmp180/bmp180.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/eeprom24aa02/eeprom24aa02.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/hmc5883/hmc5883.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/lm75/lm75.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/mcp4728/mcp4728.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/pca9536/pca9536.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/sht21/sht21.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/ubloxi2c/ubloxi2c.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/x9119/x9119.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/Adafruit_GFX.h"
    "${PROJECT_HEADER_DIR}/i2c/i2cdevices/i2cdevice.h"
    "${PROJECT_HEADER_DIR}/i2c/linux/i2c-dev.h"
    )

set(LOGIN_SOURCE_FILES
    "${PROJECT_SRC_DIR}/muondetector-login.cpp"
    )

set(LOGIN_INSTALL_FILES
    "${PROJECT_CONFIG_DIR}/muondetector-login"
    )

set(DAEMON_INSTALL_FILES
    "${PROJECT_CONFIG_DIR}/muondetector.conf"
    )

configure_file(
    "${PROJECT_CONFIG_DIR}/muondetector-daemon.1"
    "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1"
    )
configure_file(
    "${PROJECT_CONFIG_DIR}/muondetector-login.1"
    "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login.1"
    )


add_executable(muondetector-login ${LOGIN_SOURCE_FILES})

target_include_directories(muondetector-login PUBLIC
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_include_directories(muondetector-login PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>)

target_link_directories(muondetector-login PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../lib/")
target_link_libraries(muondetector-login
    Qt5::Network Qt5::SerialPort
    crypto++
    paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3
    muondetector
    )

add_executable(muondetector-daemon ${DAEMON_SOURCE_FILES} ${DAEMON_HEADER_FILES})

target_include_directories(muondetector-daemon PUBLIC
    $<BUILD_INTERFACE:${PROJECT_HEADER_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)

target_include_directories(muondetector-daemon PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>
    )

target_link_directories(muondetector-daemon PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../lib/")
target_link_libraries(muondetector-daemon
    Qt5::Network Qt5::SerialPort
    crypto++
    pigpiod_if2
    rt
    config++
    paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3
    muondetector
    )


if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_command(TARGET muondetector-daemon POST_BUILD
            COMMAND ${CMAKE_STRIP} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/muondetector-daemon")
    add_custom_command(TARGET muondetector-login POST_BUILD
            COMMAND ${CMAKE_STRIP} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/muondetector-login")
endif ()




include(GNUInstallDirs)

add_custom_target(changelog ALL COMMAND gzip -cn9 "${PROJECT_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")
add_custom_target(manpage ALL COMMAND gzip -cn9 "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1" > "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1.gz")
add_custom_target(manpage-login ALL COMMAND gzip -cn9 "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login.1" > "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login.1.gz")

install(TARGETS muondetector-daemon DESTINATION bin COMPONENT daemon)
install(TARGETS muondetector-login DESTINATION lib/muondetector/bin COMPONENT daemon)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT daemon)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1.gz" DESTINATION "share/man/man1/" COMPONENT daemon)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login.1.gz" DESTINATION "share/man/man1/" COMPONENT daemon)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login-helper.1.gz" DESTINATION "share/man/man1/" COMPONENT daemon)
install(FILES "${PROJECT_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT daemon)
install(FILES ${DAEMON_INSTALL_FILES} DESTINATION "/etc/muondetector/" COMPONENT daemon)
install(FILES "${PROJECT_CONFIG_DIR}/muondetector-daemon.service" DESTINATION "/lib/systemd/system" COMPONENT daemon)
install(FILES "${PROJECT_CONFIG_DIR}/pigpiod.conf" DESTINATION "/etc/systemd/system/pigpiod.service.d/" COMPONENT daemon)
install(PROGRAMS ${LOGIN_INSTALL_FILES} DESTINATION bin COMPONENT daemon)



set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "pigpiod")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_CONFIG_DIR}/license")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_CONFIG_DIR}/preinst;${PROJECT_CONFIG_DIR}/postinst;${PROJECT_CONFIG_DIR}/prerm;${PROJECT_CONFIG_DIR}/conffiles")
set(CPACK_PACKAGE_VENDOR "MuonPi.org")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/MuonPi/muondetector")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Daemon that controls the muon detector board")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION " It opens serial and i2c connections to the muondetector board.
 It runs in the background and sends the data to the central server.
 It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MuonPi <developer@muonpi.org>")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../packages/")

include(CPack)

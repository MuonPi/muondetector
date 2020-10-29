cmake_minimum_required(VERSION 3.10)
project(muondetector-daemon LANGUAGES CXX C)


set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
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

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")

find_package(Qt5 COMPONENTS Network SerialPort REQUIRED)

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
    "${PROJECT_CONFIG_DIR}/muondetector-login.sh"
    )

set(DAEMON_INSTALL_FILES
    "${PROJECT_CONFIG_DIR}/ubx_rates_config.cfg"
    )

set(LIBCRYPTO_LIBRARIES "crypto++")
set(LIBPIGPIOD_LIBRARIES "pigpiod_if2")
set(LIBRT_LIBRARIES "rt")
set(LIBCONFIG_LIBRARIES "config++")
set(LIBPAHO_MQTT_LIBRARIES "paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3")




add_executable(muondetector-login ${LOGIN_SOURCE_FILES})

target_include_directories(muondetector-login PUBLIC
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)
target_include_directories(muondetector-login PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>)

target_link_libraries(muondetector-login
    Qt5::Network Qt5::SerialPort
    ${LIBCRYPTO_LIBRARIES}
    paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3
    "${CMAKE_CURRENT_BINARY_DIR}/../lib/libmuondetector.a"
    )

add_executable(muondetector-daemon ${DAEMON_SOURCE_FILES} ${DAEMON_HEADER_FILES})

target_include_directories(muondetector-daemon PUBLIC
    $<BUILD_INTERFACE:${PROJECT_HEADER_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>)

target_include_directories(muondetector-daemon PUBLIC
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<INSTALL_INTERFACE:include/mqtt>
    )

target_link_libraries(muondetector-daemon
    Qt5::Network Qt5::SerialPort
    ${LIBCRYPTO_LIBRARIES}
    ${LIBPIGPIOD_LIBRARIES}
    ${LIBRT_LIBRARIES}
    ${LIBCONFIG_LIBRARIES}
    paho-mqtt3c paho-mqtt3a paho-mqtt3cs paho-mqtt3as paho-mqttpp3
    "${CMAKE_CURRENT_BINARY_DIR}/../lib/libmuondetector.a"
    )

install(FILES ${DAEMON_INSTALL_FILES} DESTINATION etc)
install(TARGETS muondetector-daemon muondetector-login DESTINATION bin)

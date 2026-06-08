set(MUONDETECTOR_DAEMON_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/daemon/src")
set(MUONDETECTOR_DAEMON_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/daemon/config")
set(MUONDETECTOR_DAEMON_BINARY_DIR "${CMAKE_BINARY_DIR}/daemon")


configure_file(
    "${MUONDETECTOR_LIBRARY_DIR}/config/version.h.in"
    "${MUONDETECTOR_DAEMON_BINARY_DIR}/version.h"
    )


find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1)

find_library(LIBCONFIG
    names libconfig++ libconfigpp config++ configpp libconfig config
    REQUIRED
)

find_library(MOSQUITTO
    names mosquitto
    REQUIRED
)

find_library(RT
    NAMES rt
    REQUIRED
)

find_library(GPIOD
    names gpiod libgpiod
    REQUIRED
)

set(MUONDETECTOR_I2C_SOURCE_FILES
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/Adafruit_GFX.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/adafruit_ssd1306.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/ads1115.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/bme280.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/bmp180.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/eeprom24aa02.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/hmc5883.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/lm75.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/mcp4728.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/pca9536.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/sht21.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/sht31.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/tca9546a.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/ubloxi2c.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/x9119.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/mic184.cpp"

    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/i2cdevice.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/i2cutil.cpp"

    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2cdevices.cpp"
    )

set(MUONDETECTOR_DAEMON_SOURCE_FILES
    "${MUONDETECTOR_DAEMON_SRC_DIR}/app/main.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/app/config_parser.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bus.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/logging/logger.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/scheduler.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/system_builder.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bindings.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/maintenance.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/thread_pool.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/component.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/factories/component_factory.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/registries/component_manager.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/factories/device_factory.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/registries/data_store.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/registries/sink_manager.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/libsecret/credentials.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/network/tcpserver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/sds011/sds011.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/sds011/sds011_parser.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/ublox/serialublox.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/ublox/ubx_parser.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/ublox/message_processor.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/tcp_sink.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/file_sink.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/mqtt_sink.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/tcp_source.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/temp_source.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/tcp_command_decoder.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/ads1115_driver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/mcp4728_driver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/eeprom24aa02_driver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/pca9536_driver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/adafruit_ssd1306_driver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/gpio_driver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/gpio_ratebuffer.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/ublox_ratebuffer.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/calibration.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/kalman_gnss_filter.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/geoposmanager.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/geohash.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/components/logengine.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/components/log_parameter_processor.cpp"

    "${MUONDETECTOR_I2C_SOURCE_FILES}"
    "${MUONDETECTOR_SPI_SOURCE_FILES}"
    )


set(MUONDETECTOR_I2C_HEADER_FILES
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/Adafruit_GFX.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/adafruit_ssd1306.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/ads1015.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/ads1115.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/bme280.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/bmp180.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/eeprom24aa02.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/glcdfont.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/hmc5883.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/lm75.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/mcp4728.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/pca9536.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/sht21.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/sht31.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/tca9546a.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/ubloxi2c.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/x9119.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/mic184.h"

    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/i2cdevice.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2c/i2cutil.h"

    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/device_types.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/i2cdevices.h"
    )

# set(MUONDETECTOR_SPI_HEADER_FILES
#     "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/spi/tdc7200.h"

#     "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/spidevices.h"
#     )

set(MUONDETECTOR_DAEMON_HEADER_FILES
    "${MUONDETECTOR_DAEMON_SRC_DIR}/app/config_parser.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bus.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/logging/logger.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/scheduler.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/task.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/system_builder.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bindings.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/maintenance.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/thread_pool.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/component.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/factories/component_factory.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/registries/component_manager.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/factories/device_factory.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/registries/device_registry.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/registries/data_store.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/libsecret/credentials.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/network/tcpserver.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/sds011/sds011.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/sds011/sds011_parser.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/ublox/serialublox.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/ublox/ubx_parser.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/hardware/ublox/message_processor.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/sink.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/tcp_sink.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/file_sink.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/mqtt_sink.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/source.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/tcp_source.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/temp_source.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/tcp_command_decoder.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/ads1115_driver.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/mcp4728_driver.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/eeprom24aa02_driver.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/pca9536_driver.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/adafruit_ssd1306_driver.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/drivers/gpio_driver.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/gpio_ratebuffer.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/ublox_ratebuffer.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/calibration.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/gpio_mapping.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/logparameter.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/kalman_gnss_filter.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/geoposmanager.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/utility/geohash.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/components/log_parameter_processor.h"

    "${MUONDETECTOR_I2C_HEADER_FILES}"
    "${MUONDETECTOR_SPI_HEADER_FILES}"
    )

set(MUONDETECTOR_LOGIN_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/credentials_utility/muondetector-login.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/logging/logger.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/thread_pool.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bus.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/libsecret/credentials.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/mqtt_sink.cpp"
    )

set(MUONDETECTOR_LOGIN_INSTALL_FILES
    "${MUONDETECTOR_DAEMON_CONFIG_DIR}/muondetector-login"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/logging/logger.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/thread_pool.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bus.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/libsecret/credentials.h"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/mqtt_sink.h"
    )

set(MUONDETECTOR_DAEMON_INSTALL_FILES
    "${MUONDETECTOR_DAEMON_CONFIG_DIR}/muondetector.conf"
    "${MUONDETECTOR_DAEMON_CONFIG_DIR}/hardware.conf"
    "${MUONDETECTOR_DAEMON_CONFIG_DIR}/components.conf"
    )

configure_file(
    "${MUONDETECTOR_DAEMON_CONFIG_DIR}/muondetector-daemon.1"
    "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1"
    )


add_executable(muondetector-login ${MUONDETECTOR_LOGIN_SOURCE_FILES} ${MUONDETECTOR_LOGIN_HEADER_FILES})

set_target_properties(muondetector-login PROPERTIES POSITION_INDEPENDENT_CODE 1)

target_include_directories(muondetector-login PUBLIC
    $<BUILD_INTERFACE:${MUONDETECTOR_DAEMON_SRC_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>
    # $<BUILD_INTERFACE:${SECRET_INCLUDE_DIR}/libsecret-1>
)

target_link_libraries(muondetector-login
    ${MOSQUITTO}
    PkgConfig::LIBSECRET
    muondetector-shared
    muondetector-protocol
)

add_executable(muondetector-daemon ${MUONDETECTOR_DAEMON_SOURCE_FILES} ${MUONDETECTOR_DAEMON_HEADER_FILES})

set_target_properties(muondetector-daemon PROPERTIES POSITION_INDEPENDENT_CODE 1)


target_include_directories(muondetector-daemon PUBLIC
    $<BUILD_INTERFACE:${MUONDETECTOR_DAEMON_SRC_DIR}>
    $<BUILD_INTERFACE:${MUONDETECTOR_DAEMON_BINARY_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CAPNP_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${SECRET_INCLUDE_DIR}/libsecret-1>
    # $<BUILD_INTERFACE:${GLIB_INCLUDE_DIR}/glib-2.0>
)

target_link_libraries(muondetector-daemon PRIVATE
    ${LIBCONFIG}
    ${MOSQUITTO}
    ${GPIOD}
    ${RT}
    ${CAPNP_LIBRARIES}
    ${CAPNP_KJ_LIBRARIES}
    muondetector-shared
    muondetector-protocol
    PkgConfig::LIBSECRET
    PkgConfig::GLIB
    Threads::Threads
)

if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_command(TARGET muondetector-daemon POST_BUILD
            COMMAND ${CMAKE_STRIP} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/muondetector-daemon")
endif ()

######################################################################################################
# PACKAGING
######################################################################################################

if (${MUONDETECTOR_ON_RASPBERRYPI})
    include(GNUInstallDirs)

    add_custom_target(prep-daemon ALL COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/daemon")
    add_custom_target(changelog-daemon ALL COMMAND gzip -cn9 "${MUONDETECTOR_DAEMON_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/daemon/changelog.gz")
    add_dependencies(changelog-daemon prep-daemon)
    add_custom_target(manpage-daemon ALL COMMAND gzip -cn9 "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1" > "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1.gz")
    # add_custom_target(manpage-login ALL COMMAND gzip -cn9 "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login.1" > "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login.1.gz")

    install(TARGETS muondetector-daemon DESTINATION bin COMPONENT daemon)
    # install(TARGETS muondetector-login DESTINATION lib/muondetector/bin COMPONENT daemon)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/daemon/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}-daemon" COMPONENT daemon)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-daemon.1.gz" DESTINATION "share/man/man1/" COMPONENT daemon)
    # install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-login.1.gz" DESTINATION "share/man/man1/" COMPONENT daemon)
    install(FILES "${MUONDETECTOR_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}-daemon" COMPONENT daemon)
    install(FILES ${MUONDETECTOR_DAEMON_INSTALL_FILES} DESTINATION "/etc/muondetector/" COMPONENT daemon)
    install(FILES "${MUONDETECTOR_DAEMON_CONFIG_DIR}/muondetector-daemon.service" DESTINATION "/lib/systemd/system" COMPONENT daemon)
    install(PROGRAMS ${MUONDETECTOR_LOGIN_INSTALL_FILES} DESTINATION bin COMPONENT daemon)

    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${MUONDETECTOR_DAEMON_CONFIG_DIR}/preinst;${MUONDETECTOR_DAEMON_CONFIG_DIR}/postinst;${MUONDETECTOR_DAEMON_CONFIG_DIR}/prerm;${MUONDETECTOR_DAEMON_CONFIG_DIR}/conffiles")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION " Daemon that controls the muon detector board.
    It opens serial and i2c connections to the muondetector board.
    It runs in the background and sends the data to the central server.
    It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
    set(CPACK_DEBIAN_PACKAGE_NAME "muondetector-daemon")
endif()
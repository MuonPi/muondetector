set(MUONDETECTOR_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library")
set(MUONDETECTOR_LIBRARY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/src")
set(MUONDETECTOR_LIBRARY_BINARY_DIR "${CMAKE_BINARY_DIR}/library")


configure_file(
    "${MUONDETECTOR_LIBRARY_DIR}/config/version.h.in"
    "${MUONDETECTOR_LIBRARY_BINARY_DIR}/version.h"
    )


find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1)


# Add Boost
find_package(Boost CONFIG REQUIRED)
find_package(Threads REQUIRED)


set(MUONDETECTOR_LIBRARY_SOURCE_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/libsecret/credentials.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/capnp/capnp_codec.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/config.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpconnection.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/networkdiscovery.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/histogram.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox/ublox_structs.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.cpp"
    )

set(MUONDETECTOR_LIBRARY_EVENT_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/adc_mode_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/adc_trace_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/ads1115_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/bias_switch_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/bias_voltage_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/calib_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/event_trigger_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/gain_switch_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/gpio_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/gpio_inhibit_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/gpio_rate_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/i2c_stats_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/temperature_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/mqtt_status_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/pca_switch_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/polarity_switch_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/preamp_switch_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/spi_stats_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/tcp_packet_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/mcp4728_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/threshold_setting_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/ubx_event.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/events/version_event.h"
)

set(MUONDETECTOR_LIBRARY_CMD_FILES
)

set(MUONDETECTOR_LIBRARY_HEADER_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/libsecret/credentials.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/capnp/capnp_codec.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/gpio_pin_definitions.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/histogram.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpconnection.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpmessage_keys.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/networkdiscovery.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox/ublox_messages.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox/ublox_structs.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/muondetector_structs.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/config.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.h"
    )

set(MUONDETECTOR_COMMANDS_HEADER_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/bias_voltage_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/calibration_save_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/pca_switch_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_default_config_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_msg_poll_rate_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_reset_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/bias_switch_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/event_trigger_selection_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/polarity_switch_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_min_cno_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_msg_rate_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_save_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/burst_sampling_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/gain_switch_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/preamp_switch_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_min_max_sv_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_protocol_selection_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_set_aop_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/calibration_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/mqtt_inhibit_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/threshold_setting_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_msg_poll_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_rate_cmd.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/commands/ubx_version_dependent_msg_rate_cmd.h"
)

add_library(muondetector-shared OBJECT ${MUONDETECTOR_LIBRARY_SOURCE_FILES} ${MUONDETECTOR_LIBRARY_HEADER_FILES})
add_library(muondetector-static STATIC ${MUONDETECTOR_LIBRARY_SOURCE_FILES} ${MUONDETECTOR_LIBRARY_HEADER_FILES}
)
target_compile_definitions(muondetector-shared PUBLIC MUONDETECTOR_LIBRARY_EXPORT)
target_compile_definitions(muondetector-static PUBLIC MUONDETECTOR_LIBRARY_EXPORT)
set_target_properties(muondetector-shared PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(muondetector-shared PUBLIC
    "${MUONDETECTOR_LIBRARY_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network"
    "${MUONDETECTOR_LIBRARY_BINARY_DIR}"
    "${Boost_INCLUDE_DIRS}"
    )

target_include_directories(muondetector-static PUBLIC
    "${MUONDETECTOR_LIBRARY_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network"
    "${MUONDETECTOR_LIBRARY_BINARY_DIR}"
    "${Boost_INCLUDE_DIRS}"
    )

target_link_libraries(muondetector-shared PUBLIC
    muondetector-protocol
    PkgConfig::LIBSECRET
    PkgConfig::GLIB
)

target_link_libraries(muondetector-static PUBLIC
    muondetector-protocol
    PkgConfig::LIBSECRET
    PkgConfig::GLIB
)
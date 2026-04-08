set(MUONDETECTOR_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library")
set(MUONDETECTOR_LIBRARY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/src")


configure_file(
    "${MUONDETECTOR_LIBRARY_DIR}/config/version.h"
    "${MUONDETECTOR_LIBRARY_DIR}/version.h"
    )

set(MUONDETECTOR_LIBRARY_SOURCE_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/config.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpconnection.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpmessage.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/histogram.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox_structs.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/networkdiscovery.cpp"
    )

set(MUONDETECTOR_LIBRARY_HEADER_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/gpio_pin_definitions.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/histogram.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpconnection.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpmessage.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpmessage_keys.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox_messages.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox_structs.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/muondetector_structs.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/config.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/networkdiscovery.h"
    )

if (MUONDETECTOR_BUILD_DAEMON)
set(MUONDETECTOR_LIBRARY_MQTT_SOURCE_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/mqtt/mqtthandler.cpp"
    )
set(MUONDETECTOR_LIBRARY_MQTT_HEADER_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/mqtt/mqtthandler.h"
    )

add_library(muondetector-shared-mqtt OBJECT ${MUONDETECTOR_LIBRARY_MQTT_SOURCE_FILES} ${MUONDETECTOR_LIBRARY_MQTT_HEADER_FILES})
target_compile_definitions(muondetector-shared-mqtt PUBLIC MUONDETECTOR_LIBRARY_EXPORT)
set_target_properties(muondetector-shared-mqtt PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(muondetector-shared-mqtt PUBLIC
    "${MUONDETECTOR_LIBRARY_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/mqtt"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network"
    )
endif ()

add_library(muondetector-shared OBJECT ${MUONDETECTOR_LIBRARY_SOURCE_FILES} ${MUONDETECTOR_LIBRARY_HEADER_FILES})
target_compile_definitions(muondetector-shared PUBLIC MUONDETECTOR_LIBRARY_EXPORT)
set_target_properties(muondetector-shared PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_include_directories(muondetector-shared PUBLIC
    "${MUONDETECTOR_LIBRARY_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/mqtt"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network"
    )

set(MUONDETECTOR_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library")
set(MUONDETECTOR_LIBRARY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/src")


configure_file(
    "${MUONDETECTOR_LIBRARY_DIR}/config/version.h"
    "${MUONDETECTOR_LIBRARY_DIR}/version.h"
    )


set(FOUND_LIBATOMIC TRUE)
find_package(CapnProto REQUIRED)
add_definitions(${CAPNP_DEFINITIONS})
file(GLOB CAPNP_FILES
    ${MUONDETECTOR_LIBRARY_DIR}/src/capnp/*.capnp
)
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS ${CAPNP_FILES})
get_filename_component(CAPNP_INCLUDE_DIR ${CAPNP_HDRS} DIRECTORY)


set(MUONDETECTOR_LIBRARY_SOURCE_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/config.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpconnection.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/histogram.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox_structs.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/networkdiscovery.cpp"
    "${CAPNP_SRCS}"
    )

set(MUONDETECTOR_LIBRARY_HEADER_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/gpio_pin_definitions.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/histogram.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpconnection.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/tcpmessage_keys.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox_messages.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/ublox_structs.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/muondetector_structs.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/config.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/data/custom_io_operators.h"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/network/networkdiscovery.h"
    "${CAPNP_HDRS}"
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
    "${CAPNP_INCLUDE_DIR}"
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
    "${CAPNP_INCLUDE_DIR}"
    )

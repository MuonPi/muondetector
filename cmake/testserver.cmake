set(TESTSERVER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testserver/src")

find_library(MOSQUITTO
    names mosquitto
    REQUIRED
)
find_library(CRYPTOPP
    names cryptopp crypto++
    REQUIRED
)
find_library(RT
    NAMES rt
    REQUIRED
)

set(TESTSERVER_SOURCE_FILES
    "${TESTSERVER_SRC_DIR}/main.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/logging/logger.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/thread_pool.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bus.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/component.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/tcp_sink.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/tcp_source.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/network/tcpserver.cpp"
    $<TARGET_OBJECTS:muondetector-shared>
)

set(TESTSERVER_INCLUDE_FILES
    "${MUONDETECTOR_DAEMON_HEADER_DIR}/network/tcpserver.h"
)


add_executable(testserver ${TESTSERVER_SOURCE_FILES} ${TESTSERVER_HEADER_FILES})

set_target_properties(testserver PROPERTIES POSITION_INDEPENDENT_CODE 1)


target_include_directories(testserver PUBLIC
    $<BUILD_INTERFACE:${MUONDETECTOR_DAEMON_SRC_DIR}>
    $<BUILD_INTERFACE:${TESTSERVER_SRC_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CAPNP_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
)


target_link_libraries(testserver PRIVATE
    ${MOSQUITTO}
    ${CRYPTOPP}
    # ${RT}
    ${CAPNP_LIBRARIES}
    ${CAPNP_KJ_LIBRARIES}
    muondetector-shared
    muondetector-protocol
    Threads::Threads
)


if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_command(TARGET testserver POST_BUILD
            COMMAND ${CMAKE_STRIP} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testserver")
endif ()

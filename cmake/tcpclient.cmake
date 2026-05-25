set(TCPCLIENT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tcpclient_standalone/src")

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

set(TCPCLIENT_SOURCE_FILES
    "${TCPCLIENT_SRC_DIR}/main.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/logging/logger.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/thread_pool.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bus.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/tcp_sink.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/component.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/tcp_source.cpp"
    $<TARGET_OBJECTS:muondetector-shared>
)


add_executable(tcpclient ${TCPCLIENT_SOURCE_FILES} ${TCPCLIENT_HEADER_FILES})

set_target_properties(tcpclient PROPERTIES POSITION_INDEPENDENT_CODE 1)


target_include_directories(tcpclient PUBLIC
    $<BUILD_INTERFACE:${MUONDETECTOR_DAEMON_SRC_DIR}>
    $<BUILD_INTERFACE:${TCPCLIENT_SRC_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CAPNP_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
)


target_link_libraries(tcpclient PRIVATE
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
    add_custom_command(TARGET tcpclient POST_BUILD
            COMMAND ${CMAKE_STRIP} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tcpclient")
endif ()

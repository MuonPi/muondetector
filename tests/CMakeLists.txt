set(MUONDETECTOR_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(MUONDETECTOR_TCP_TEST_COMMON_SOURCES
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/thread_pool.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/core/event_bus.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/network/tcpserver.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sinks/tcp_sink.cpp"
    "${MUONDETECTOR_DAEMON_SRC_DIR}/sources/tcp_source.cpp"
    $<TARGET_OBJECTS:muondetector-shared>
)

function(add_muondetector_tcp_test target_name test_source test_name)
    add_executable(${target_name}
        "${MUONDETECTOR_TESTS_DIR}/${test_source}"
        ${MUONDETECTOR_TCP_TEST_COMMON_SOURCES}
    )

    target_include_directories(${target_name} PRIVATE
        "${MUONDETECTOR_DAEMON_SRC_DIR}"
        "${MUONDETECTOR_LIBRARY_SRC_DIR}"
        "${MUONDETECTOR_LIBRARY_SRC_DIR}/network"
        "${CAPNP_INCLUDE_DIR}"
    )

    target_link_libraries(${target_name} PRIVATE
        ${CAPNP_LIBRARIES}
        ${CAPNP_KJ_LIBRARIES}
        pthread
    )

    add_test(NAME ${test_name} COMMAND ${target_name})
endfunction()

add_muondetector_tcp_test(muondetector-tcp-transmit-test tcp_transmit_test.cpp tcp-transmit)
add_muondetector_tcp_test(muondetector-tcp-malformed-checksum-test tcp_malformed_checksum_test.cpp tcp-malformed-checksum)
add_muondetector_tcp_test(muondetector-tcp-idle-cleanup-test tcp_idle_cleanup_test.cpp tcp-idle-cleanup)
add_muondetector_tcp_test(muondetector-tcp-multi-client-isolation-test tcp_multi_client_isolation_test.cpp tcp-multi-client-isolation)
add_muondetector_tcp_test(muondetector-tcp-partial-reassembly-test tcp_partial_reassembly_test.cpp tcp-partial-reassembly)
add_muondetector_tcp_test(muondetector-tcp-bad-magic-test tcp_bad_magic_test.cpp tcp-bad-magic)
add_muondetector_tcp_test(muondetector-tcp-bad-version-test tcp_bad_version_test.cpp tcp-bad-version)
add_muondetector_tcp_test(muondetector-tcp-oversized-payload-test tcp_oversized_payload_test.cpp tcp-oversized-payload)
add_muondetector_tcp_test(muondetector-tcp-heartbeat-ping-test tcp_heartbeat_ping_test.cpp tcp-heartbeat-ping)
add_muondetector_tcp_test(muondetector-tcp-burst-delivery-test tcp_burst_delivery_test.cpp tcp-burst-delivery)

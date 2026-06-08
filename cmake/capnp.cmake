# ============================================================
# capnp.cmake
# Drop-in Cap'n Proto + YAML generator integration
# ============================================================

# Usage in main CMakeLists.txt:
#
#   include(cmake/capnp.cmake)
#   target_link_libraries(muondetector-shared PUBLIC muondetector-protocol)
#
# ============================================================

set(CAPNP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(CAPNP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CAPNP_SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/library/src/capnp")
set(CAPNP_GENERATOR_SCRIPT "${CMAKE_SOURCE_DIR}/tools/generate_capnp.py")

set(CAPNP_BUILD_SUBDIR "generated/capnp")
set(CAPNP_BUILD_DIRECTORY "${CMAKE_BINARY_DIR}/${CAPNP_BUILD_SUBDIR}")

set(GENERATED_PROTOCOL_CAPNP "${CAPNP_BUILD_DIRECTORY}/protocol.capnp")
file(MAKE_DIRECTORY "${CAPNP_BUILD_DIRECTORY}")

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------
set(FOUND_LIBATOMIC TRUE)

if(CMAKE_CROSSCOMPILING)
    set(HOST_CAPNP_EXECUTABLE /usr/local/bin/capnp CACHE FILEPATH "Host capnp compiler executable")
    set(HOST_CAPNPC_CXX_EXECUTABLE /usr/local/bin/capnpc-c++ CACHE FILEPATH "Host capnp C++ plugin executable")
    set(CAPNP_EXECUTABLE "${HOST_CAPNP_EXECUTABLE}" CACHE FILEPATH "" FORCE)
    set(CAPNPC_CXX_EXECUTABLE "${HOST_CAPNPC_CXX_EXECUTABLE}" CACHE FILEPATH "" FORCE)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(CapnProto QUIET)

if(CMAKE_CROSSCOMPILING)
    set(CAPNP_EXECUTABLE "${HOST_CAPNP_EXECUTABLE}" CACHE FILEPATH "" FORCE)
    set(CAPNPC_CXX_EXECUTABLE "${HOST_CAPNPC_CXX_EXECUTABLE}" CACHE FILEPATH "" FORCE)

    if(NOT EXISTS "${HOST_CAPNP_EXECUTABLE}")
        message(FATAL_ERROR
            "Host capnp compiler not found at ${HOST_CAPNP_EXECUTABLE}. "
            "Install the host capnproto package, or pass -DHOST_CAPNP_EXECUTABLE=/path/to/capnp."
        )
    endif()
    if(NOT EXISTS "${HOST_CAPNPC_CXX_EXECUTABLE}")
        message(FATAL_ERROR
            "Host capnp C++ plugin not found at ${HOST_CAPNPC_CXX_EXECUTABLE}. "
            "Install the matching host capnproto C++ plugin, or pass "
            "-DHOST_CAPNPC_CXX_EXECUTABLE=/path/to/capnpc-c++."
        )
    endif()

    execute_process(
        COMMAND "${HOST_CAPNP_EXECUTABLE}" --version
        OUTPUT_VARIABLE HOST_CAPNP_VERSION_OUTPUT
        ERROR_VARIABLE HOST_CAPNP_VERSION_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE HOST_CAPNP_VERSION_RESULT
    )
    if(NOT HOST_CAPNP_VERSION_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to run host capnp compiler ${HOST_CAPNP_EXECUTABLE}: "
            "${HOST_CAPNP_VERSION_ERROR}"
        )
    endif()

    string(REGEX MATCH "[0-9]+(\\.[0-9]+)+" HOST_CAPNP_VERSION "${HOST_CAPNP_VERSION_OUTPUT}")
    message(STATUS "Host capnp compiler: ${HOST_CAPNP_EXECUTABLE} (${HOST_CAPNP_VERSION_OUTPUT})")
    message(STATUS "Host capnp C++ plugin: ${HOST_CAPNPC_CXX_EXECUTABLE}")
    message(STATUS "Target CapnProto package version: ${CapnProto_VERSION}")

    if(CapnProto_VERSION AND HOST_CAPNP_VERSION
        AND NOT HOST_CAPNP_VERSION VERSION_EQUAL CapnProto_VERSION
    )
        message(FATAL_ERROR
            "Cap'n Proto version mismatch for cross-compilation. "
            "The host capnp compiler is ${HOST_CAPNP_VERSION}, but the target sysroot "
            "Cap'n Proto headers/libraries are ${CapnProto_VERSION}. Generated Cap'n Proto "
            "C++ requires an exact version match. Install/build a host capnp compiler with "
            "version ${CapnProto_VERSION} and pass -DHOST_CAPNP_EXECUTABLE=/path/to/capnp, "
            "or update the sysroot Cap'n Proto dev package to match the host compiler."
        )
    endif()
endif()

if(WIN32 AND NOT CapnProto_FOUND)
    message(STATUS "Building Cap'n Proto from source (Windows)")

    include(FetchContent)
    FetchContent_Declare(capnproto
        GIT_REPOSITORY https://github.com/capnproto/capnproto.git
        GIT_TAG v1.4.0
    )
    FetchContent_MakeAvailable(capnproto)

    set(CAPNP_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/capnproto-build")
    set(CAPNP_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/capnproto-install")
    file(MAKE_DIRECTORY "${CAPNP_BUILD_DIR}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-G" "Ninja"
            "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_INSTALL_PREFIX=${CAPNP_INSTALL_DIR}"
            "-DBUILD_TESTING=OFF"
            "${capnproto_SOURCE_DIR}/c++"
        WORKING_DIRECTORY "${CAPNP_BUILD_DIR}"
        RESULT_VARIABLE CAPNP_CONFIG_RESULT
    )
    if(NOT CAPNP_CONFIG_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to configure Cap'n Proto")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${CAPNP_BUILD_DIR}" --config Release -j
        RESULT_VARIABLE CAPNP_BUILD_RESULT
    )
    if(NOT CAPNP_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build Cap'n Proto")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${CAPNP_BUILD_DIR}"
        RESULT_VARIABLE CAPNP_INSTALL_RESULT
    )
    if(NOT CAPNP_INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to install Cap'n Proto")
    endif()

    # Point find_package at our freshly built install
    set(CapnProto_DIR "${CAPNP_INSTALL_DIR}/lib/cmake/CapnProto")
endif()

add_definitions(${CAPNP_DEFINITIONS})

# ------------------------------------------------------------
# Input / Generated Files
# ------------------------------------------------------------
set(PROTOCOL_YAML "${CAPNP_SOURCE_DIRECTORY}/protocol.yaml")

# ------------------------------------------------------------
# Generate protocol.capnp at configure time
# This avoids build-time ordering issues with capnp_generate_cpp()
# ------------------------------------------------------------
message(STATUS "Generating protocol.capnp from protocol.yaml")

execute_process(
    COMMAND
        ${Python3_EXECUTABLE}
        "${CAPNP_GENERATOR_SCRIPT}"
        "${PROTOCOL_YAML}"
        "${GENERATED_PROTOCOL_CAPNP}"
    RESULT_VARIABLE GEN_RESULT
)

if(NOT GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate protocol.capnp")
endif()

# ------------------------------------------------------------
# Reconfigure automatically if YAML changes
# ------------------------------------------------------------
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROTOCOL_YAML}"
)

# ------------------------------------------------------------
# Collect static .capnp files
# ------------------------------------------------------------
file(GLOB STATIC_CAPNP_FILES
    CONFIGURE_DEPENDS
    "${CAPNP_SOURCE_DIRECTORY}/*.capnp"
)

# Remove protocol.capnp if someone manually placed one there
list(FILTER STATIC_CAPNP_FILES EXCLUDE REGEX ".*/protocol\\.capnp$")

# Final schema list
set(ALL_CAPNP_FILES
    ${STATIC_CAPNP_FILES}
    ${GENERATED_PROTOCOL_CAPNP}
    # ${GENERATED_PROTOCOL_CAPNP_FOR_CAPNP}
)

message(STATUS "Generating CapnProto Files" ${ALL_CAPNP_FILES})

# ------------------------------------------------------------
# Generate C++ from schemas
# ------------------------------------------------------------
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS ${ALL_CAPNP_FILES})

message(STATUS "CAPNP_SRCS" ${CAPNP_SRCS})
message(STATUS "CAPNP_HDRS" ${CAPNP_HDRS})

# Generated headers usually live beside generated sources
get_filename_component(CAPNP_INCLUDE_DIR "${CAPNP_HDRS}" DIRECTORY)

# ------------------------------------------------------------
# Protocol library
# ------------------------------------------------------------
add_library(muondetector-protocol STATIC
    ${CAPNP_SRCS}
    ${CAPNP_HDRS}
)

set(CAPNP_INCLUDE_DIRS
    ${CAPNP_SOURCE_DIRECTORY}
    ${CMAKE_BINARY_DIR}/library/src/capnp
    ${CMAKE_BINARY_DIR}/generated/capnp
)

target_include_directories(muondetector-protocol PUBLIC
    ${CAPNP_INCLUDE_DIRS}
)

target_link_libraries(muondetector-protocol PUBLIC
    CapnProto::capnp
    CapnProto::kj
)

target_compile_features(muondetector-protocol PUBLIC cxx_std_20)

source_group("Capnp Schemas" FILES ${STATIC_CAPNP_FILES})
source_group("Generated" FILES
    ${GENERATED_PROTOCOL_CAPNP}
    ${CAPNP_SRCS}
    ${CAPNP_HDRS}
)
message(STATUS "Cap'n Proto protocol library ready: muondetector-protocol")

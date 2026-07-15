# ============================================================
# capnp.cmake
# Unified Cap'n Proto + YAML generator integration
# ============================================================

set(CAPNP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(CAPNP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

get_filename_component(MUONDETECTOR_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/.."
    ABSOLUTE
)

set(CAPNP_SOURCE_DIRECTORY
    "${MUONDETECTOR_ROOT}/library/src/capnp"
)

set(CAPNP_GENERATOR_SCRIPT
    "${MUONDETECTOR_ROOT}/tools/generate_capnp.py"
)

set(CAPNP_BUILD_SUBDIR "generated/capnp")
set(CAPNP_BUILD_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CAPNP_BUILD_SUBDIR}")

set(GENERATED_PROTOCOL_CAPNP "${CAPNP_BUILD_DIRECTORY}/protocol.capnp")

file(MAKE_DIRECTORY "${CAPNP_BUILD_DIRECTORY}")

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------
set(FOUND_LIBATOMIC TRUE)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(CapnProto QUIET)

# ------------------------------------------------------------
# Cross compilation: use host capnp tools, target CapnProto libs
# ------------------------------------------------------------
if(CMAKE_CROSSCOMPILING)
    set(HOST_CAPNP_EXECUTABLE /usr/local/bin/capnp
        CACHE FILEPATH "Host capnp compiler executable"
    )

    set(HOST_CAPNPC_CXX_EXECUTABLE /usr/local/bin/capnpc-c++
        CACHE FILEPATH "Host capnp C++ plugin executable"
    )

    if(NOT EXISTS "${HOST_CAPNP_EXECUTABLE}")
        message(FATAL_ERROR
            "Host capnp compiler not found at ${HOST_CAPNP_EXECUTABLE}. "
            "Pass -DHOST_CAPNP_EXECUTABLE=/path/to/capnp."
        )
    endif()

    if(NOT EXISTS "${HOST_CAPNPC_CXX_EXECUTABLE}")
        message(FATAL_ERROR
            "Host capnp C++ plugin not found at ${HOST_CAPNPC_CXX_EXECUTABLE}. "
            "Pass -DHOST_CAPNPC_CXX_EXECUTABLE=/path/to/capnpc-c++."
        )
    endif()

    set(CAPNP_EXECUTABLE "${HOST_CAPNP_EXECUTABLE}" CACHE FILEPATH "" FORCE)
    set(CAPNPC_CXX_REAL_EXECUTABLE "${HOST_CAPNPC_CXX_EXECUTABLE}")

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
            "Cap'n Proto version mismatch. Host compiler is ${HOST_CAPNP_VERSION}, "
            "target package is ${CapnProto_VERSION}."
        )
    endif()
endif()

# ------------------------------------------------------------
# Windows: build host capnp tools from source
# ------------------------------------------------------------
if(WIN32 AND NOT CMAKE_CROSSCOMPILING AND NOT CapnProto_FOUND)
    message(STATUS "Building Cap'n Proto from source for Windows")

    include(FetchContent)

    FetchContent_Declare(capnproto
        GIT_REPOSITORY https://github.com/capnproto/capnproto.git
        GIT_TAG v1.4.0
    )

    FetchContent_MakeAvailable(capnproto)

    set(CAPNP_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/capnproto-build")
    file(MAKE_DIRECTORY "${CAPNP_BUILD_DIR}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -G Ninja
            "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DBUILD_TESTING=OFF"
            "${capnproto_SOURCE_DIR}/c++"
        WORKING_DIRECTORY "${CAPNP_BUILD_DIR}"
        RESULT_VARIABLE CAPNP_CONFIG_RESULT
    )

    if(NOT CAPNP_CONFIG_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to configure host Cap'n Proto tools")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${CAPNP_BUILD_DIR}" --config Release
        RESULT_VARIABLE CAPNP_BUILD_RESULT
    )

    if(NOT CAPNP_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build host Cap'n Proto tools")
    endif()

    find_program(CAPNP_EXECUTABLE
        NAMES capnp.exe capnp
        PATHS
            "${CAPNP_BUILD_DIR}"
            "${CAPNP_BUILD_DIR}/src/capnp"
            "${CAPNP_BUILD_DIR}/src/capnp/output/bin"
            "${CAPNP_BUILD_DIR}/c++/src/capnp/output/bin"
        NO_DEFAULT_PATH
        REQUIRED
    )

    find_program(CAPNPC_CXX_REAL_EXECUTABLE
        NAMES capnpc-c++.exe capnpc-c++
        PATHS
            "${CAPNP_BUILD_DIR}"
            "${CAPNP_BUILD_DIR}/src/capnp"
            "${CAPNP_BUILD_DIR}/src/capnp/output/bin"
            "${CAPNP_BUILD_DIR}/c++/src/capnp/output/bin"
        NO_DEFAULT_PATH
        REQUIRED
    )

    message(STATUS "Using capnp: ${CAPNP_EXECUTABLE}")
    message(STATUS "Using capnpc-c++: ${CAPNPC_CXX_REAL_EXECUTABLE}")

    if(NOT EXISTS "${CAPNP_EXECUTABLE}")
        message(FATAL_ERROR "capnp.exe was not built at ${CAPNP_EXECUTABLE}")
    endif()

    if(NOT EXISTS "${CAPNPC_CXX_REAL_EXECUTABLE}")
        message(FATAL_ERROR "capnpc-c++.exe was not built at ${CAPNPC_CXX_REAL_EXECUTABLE}")
    endif()

    if(TARGET capnp AND NOT TARGET CapnProto::capnp)
        add_library(CapnProto::capnp ALIAS capnp)
    endif()

    if(TARGET kj AND NOT TARGET CapnProto::kj)
        add_library(CapnProto::kj ALIAS kj)
    endif()
endif()

# ------------------------------------------------------------
# Normal native builds: use package-provided tools
# ------------------------------------------------------------
if(NOT CMAKE_CROSSCOMPILING AND NOT WIN32)
    if(NOT CAPNP_EXECUTABLE)
        find_program(CAPNP_EXECUTABLE capnp REQUIRED)
    endif()

    if(NOT CAPNPC_CXX_REAL_EXECUTABLE)
        find_program(CAPNPC_CXX_REAL_EXECUTABLE capnpc-c++ REQUIRED)
    endif()
endif()

if(NOT CAPNP_EXECUTABLE)
    message(FATAL_ERROR "CAPNP_EXECUTABLE is not set")
endif()

if(NOT CAPNPC_CXX_REAL_EXECUTABLE)
    message(FATAL_ERROR "CAPNPC_CXX_REAL_EXECUTABLE is not set")
endif()

get_filename_component(CAPNPC_CXX_DIR "${CAPNPC_CXX_REAL_EXECUTABLE}" DIRECTORY)

if(NOT TARGET CapnProto::capnp OR NOT TARGET CapnProto::kj)
    message(FATAL_ERROR
        "CapnProto library targets are missing. Need CapnProto::capnp and CapnProto::kj."
    )
endif()

add_definitions(${CAPNP_DEFINITIONS})

# ------------------------------------------------------------
# Generate protocol.capnp from YAML at configure time
# ------------------------------------------------------------
set(PROTOCOL_YAML "${CAPNP_SOURCE_DIRECTORY}/protocol.yaml")

message(STATUS "Generating protocol.capnp from protocol.yaml")

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CAPNP_GENERATOR_SCRIPT}"
        "${PROTOCOL_YAML}"
        "${GENERATED_PROTOCOL_CAPNP}"
    RESULT_VARIABLE GEN_RESULT
)

if(NOT GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate protocol.capnp")
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PROTOCOL_YAML}"
)

# ------------------------------------------------------------
# Collect all schemas
# ------------------------------------------------------------
file(GLOB STATIC_CAPNP_FILES
    CONFIGURE_DEPENDS
    "${CAPNP_SOURCE_DIRECTORY}/*.capnp"
)

list(FILTER STATIC_CAPNP_FILES EXCLUDE REGEX ".*/protocol\\.capnp$")

set(ALL_CAPNP_FILES
    ${STATIC_CAPNP_FILES}
    ${GENERATED_PROTOCOL_CAPNP}
)

# ------------------------------------------------------------
# Unified Cap'n Proto C++ generation
# ------------------------------------------------------------
set(CAPNP_SRCS)
set(CAPNP_HDRS)

foreach(CAPNP_SCHEMA ${ALL_CAPNP_FILES})
    file(RELATIVE_PATH CAPNP_RELATIVE_TO_BUILD
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CAPNP_SCHEMA}"
    )

    if(NOT CAPNP_RELATIVE_TO_BUILD MATCHES "^\\.\\.")
        set(CAPNP_SRC_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
        set(CAPNP_RELATIVE_SCHEMA "${CAPNP_RELATIVE_TO_BUILD}")
    else()
        set(CAPNP_SRC_PREFIX "${MUONDETECTOR_ROOT}")

        file(RELATIVE_PATH CAPNP_RELATIVE_SCHEMA
            "${MUONDETECTOR_ROOT}"
            "${CAPNP_SCHEMA}"
        )
    endif()

    set(CAPNP_GENERATED_CPP
        "${CMAKE_CURRENT_BINARY_DIR}/${CAPNP_RELATIVE_SCHEMA}.c++"
    )

    set(CAPNP_GENERATED_H
        "${CMAKE_CURRENT_BINARY_DIR}/${CAPNP_RELATIVE_SCHEMA}.h"
    )

    get_filename_component(CAPNP_GENERATED_DIR "${CAPNP_GENERATED_CPP}" DIRECTORY)
    file(MAKE_DIRECTORY "${CAPNP_GENERATED_DIR}")

    add_custom_command(
        OUTPUT
            "${CAPNP_GENERATED_CPP}"
            "${CAPNP_GENERATED_H}"
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "PATH=${CAPNPC_CXX_DIR};$ENV{PATH}"
            "${CAPNP_EXECUTABLE}"
            compile
            -oc++:.
            "--src-prefix=${CAPNP_SRC_PREFIX}"
            "-I${MUONDETECTOR_ROOT}"
            "-I${CAPNP_SOURCE_DIRECTORY}"
            "-I${CAPNP_BUILD_DIRECTORY}"
            "${CAPNP_SCHEMA}"
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS
            "${CAPNP_SCHEMA}"
        COMMENT
            "Generating C++ from ${CAPNP_SCHEMA}"
        VERBATIM
    )

    list(APPEND CAPNP_SRCS "${CAPNP_GENERATED_CPP}")
    list(APPEND CAPNP_HDRS "${CAPNP_GENERATED_H}")
endforeach()

# ------------------------------------------------------------
# Protocol library
# ------------------------------------------------------------
add_library(muondetector-protocol STATIC
    ${CAPNP_SRCS}
    ${CAPNP_HDRS}
)

target_include_directories(muondetector-protocol PUBLIC
    "${CAPNP_SOURCE_DIRECTORY}"
    "${CAPNP_BUILD_DIRECTORY}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/library/src/capnp"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/capnp"
)

target_link_libraries(muondetector-protocol PUBLIC
    CapnProto::capnp
    CapnProto::kj
)

target_compile_features(muondetector-protocol PUBLIC cxx_std_20)

source_group("Capnp Schemas" FILES ${ALL_CAPNP_FILES})
source_group("Generated" FILES ${CAPNP_SRCS} ${CAPNP_HDRS})

message(STATUS "Cap'n Proto protocol library ready: muondetector-protocol")

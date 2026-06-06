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

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(CapnProto REQUIRED)
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
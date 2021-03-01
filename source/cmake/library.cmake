set(MUONDETECTOR_LIBRARY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/src")
set(MUONDETECTOR_LIBRARY_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/include")
set(MUONDETECTOR_LIBRARY_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/config")
set(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/include/")

if(WIN32)
    set(QWT_DIR "C:/Qwt-6.1.4")
    set(OPENSSL_DIR "C:/Qt/Tools/OpenSSL/Win_x64")
    set(CRYPTOPP_DIR "C:/cryptopp")
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019_64/lib/cmake/Qt5QuickCompiler")
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019_64/lib/cmake/Qt5")

    if (MSVC)
        if("${MSVC_RUNTIME}" STREQUAL "")
            set(MSVC_RUNTIME "static")
        endif()
            # Set compiler options.
        set(variables
          CMAKE_C_FLAGS_DEBUG
          CMAKE_C_FLAGS_MINSIZEREL
          CMAKE_C_FLAGS_RELEASE
          CMAKE_C_FLAGS_RELWITHDEBINFO
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_CXX_FLAGS_MINSIZEREL
          CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_RELWITHDEBINFO
        )
        if(${MSVC_RUNTIME} STREQUAL "static")
          message(STATUS
            "MSVC -> forcing use of statically-linked runtime."
          )
          foreach(variable ${variables})
            if(${variable} MATCHES "/MD")
              string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
            endif()
          endforeach()
        else()
          message(STATUS
            "MSVC -> forcing use of dynamically-linked runtime."
          )
          foreach(variable ${variables})
            if(${variable} MATCHES "/MT")
              string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
            endif()
          endforeach()
        endif()
    endif()

endif()

find_package(Qt5 COMPONENTS Network REQUIRED)


configure_file(
    "${MUONDETECTOR_LIBRARY_CONFIG_DIR}/version.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/version.h"
    )

set(MUONDETECTOR_LIBRARY_SOURCE_FILES
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/config.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/tcpconnection.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/tcpmessage.cpp"
    "${MUONDETECTOR_LIBRARY_SRC_DIR}/mqtthandler.cpp"
    )

set(MUONDETECTOR_LIBRARY_HEADER_FILES
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/muondetector_shared_global.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/gpio_pin_definitions.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/histogram.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/tcpconnection.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/tcpmessage.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/mqtthandler.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/tcpmessage_keys.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/ublox_messages.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/ubx_msg_key_name_map.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/ublox_structs.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/muondetector_structs.h"
    "${MUONDETECTOR_LIBRARY_HEADER_DIR}/config.h"
    )

add_library(muondetector-shared OBJECT ${MUONDETECTOR_LIBRARY_SOURCE_FILES} ${MUONDETECTOR_LIBRARY_HEADER_FILES})

target_compile_definitions(muondetector-shared PUBLIC MUONDETECTOR_LIBRARY_EXPORT)

set_target_properties(muondetector-shared PROPERTIES POSITION_INDEPENDENT_CODE 1)

target_include_directories(muondetector-shared PUBLIC
    ${MUONDETECTOR_LIBRARY_HEADER_DIR}
    )

if(WIN32)
target_include_directories(muondetector-shared PUBLIC
    ${CRYPTOPP_DIR}/include/
    $<BUILD_INTERFACE:${QWT_DIR}/include>
    ${Qt5Network_INCLUDE_DIRS}
    )
else()
target_link_libraries(muondetector-shared Qt5::Network mosquitto)
endif()

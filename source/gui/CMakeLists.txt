cmake_minimum_required(VERSION 3.10)
project(muondetector-gui LANGUAGES CXX)

string(TIMESTAMP PROJECT_DATE_STRING "%b %d, %Y")

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
set(PROJECT_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qml")
set(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../library/include/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/version.cmake")

if(${MUONDETECTOR_BUILD_TIDY})
  set(CMAKE_CXX_CLANG_TIDY
      clang-tidy;
      -header-filter=^global;
      -checks=-*,readability-*,bugprone-*,performace-*,clang-analyzer-*,modernize-*,hicpp-*;
      )
endif(${MUONDETECTOR_BUILD_TIDY})

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)

set(QWT_DIR "C:/Qwt-6.1.4")
set(OPENSSL_DIR "C:/Qt/Tools/OpenSSL/Win_x64")
set(MQTT_CPP_DIR "C:/paho-mqtt-cpp-1.1.0-win64")
set(MQTT_C_DIR "C:/eclipse-paho-mqtt-c-win64-1.3.6")
set(CRYPTOPP_DIR "C:/cryptopp")
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019_64/lib/cmake/Qt5QuickCompiler")
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.15.1/msvc2019_64/lib/cmake/Qt5")

else()

set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/")

endif()

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

find_package(Qt5 COMPONENTS Network Svg Widgets Gui Quick QuickWidgets Qml REQUIRED)

if(NOT WIN32)

find_library(PAHO_MQTT3C paho-mqtt3c REQUIRED)
find_library(PAHO_MQTT3A paho-mqtt3a REQUIRED)
find_library(PAHO_MQTT3CS paho-mqtt3cs REQUIRED)
find_library(PAHO_MQTT3AS paho-mqtt3as REQUIRED)
find_library(PAHO_MQTTPP3 paho-mqttpp3 REQUIRED)
find_library(CRYPTOPP crypto++ REQUIRED)
find_library(QWT_QT5 qwt-qt5 REQUIRED)

endif()

find_library(MUONDETECTOR_LIB NAMES muondetector HINTS "${CMAKE_CURRENT_BINARY_DIR}/../lib/" REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")


add_compile_options(
    -Wall
    -O3
    )
if (MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
else()

add_compile_options(
    -Wextra
    -Wshadow
    -Wpedantic
    )

endif()

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")
add_compile_options(
    -mthumb
    -mthumb-interwork
    -march=armv7-a
    )
endif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "armv7l")

set(PROJECT_SOURCE_FILES
    "${PROJECT_SRC_DIR}/calibform.cpp"
    "${PROJECT_SRC_DIR}/calibscandialog.cpp"
    "${PROJECT_SRC_DIR}/custom_histogram_widget.cpp"
    "${PROJECT_SRC_DIR}/custom_plot_widget.cpp"
    "${PROJECT_SRC_DIR}/gnssposwidget.cpp"
    "${PROJECT_SRC_DIR}/gpssatsform.cpp"
    "${PROJECT_SRC_DIR}/histogramdataform.cpp"
    "${PROJECT_SRC_DIR}/i2cform.cpp"
    "${PROJECT_SRC_DIR}/logplotswidget.cpp"
    "${PROJECT_SRC_DIR}/main.cpp"
    "${PROJECT_SRC_DIR}/mainwindow.cpp"
    "${PROJECT_SRC_DIR}/map.cpp"
    "${PROJECT_SRC_DIR}/parametermonitorform.cpp"
    "${PROJECT_SRC_DIR}/plotcustom.cpp"
    "${PROJECT_SRC_DIR}/scanform.cpp"
    "${PROJECT_SRC_DIR}/settings.cpp"
    "${PROJECT_SRC_DIR}/spiform.cpp"
    "${PROJECT_SRC_DIR}/status.cpp"
    )

set(PROJECT_HEADER_FILES
    "${PROJECT_HEADER_DIR}/calibform.h"
    "${PROJECT_HEADER_DIR}/calibscandialog.h"
    "${PROJECT_HEADER_DIR}/custom_histogram_widget.h"
    "${PROJECT_HEADER_DIR}/custom_plot_widget.h"
    "${PROJECT_HEADER_DIR}/gnssposwidget.h"
    "${PROJECT_HEADER_DIR}/gpssatsform.h"
    "${PROJECT_HEADER_DIR}/histogramdataform.h"
    "${PROJECT_HEADER_DIR}/i2cform.h"
    "${PROJECT_HEADER_DIR}/logplotswidget.h"
    "${PROJECT_HEADER_DIR}/mainwindow.h"
    "${PROJECT_HEADER_DIR}/map.h"
    "${PROJECT_HEADER_DIR}/parametermonitorform.h"
    "${PROJECT_HEADER_DIR}/plotcustom.h"
    "${PROJECT_HEADER_DIR}/scanform.h"
    "${PROJECT_HEADER_DIR}/settings.h"
    "${PROJECT_HEADER_DIR}/spiform.h"
    "${PROJECT_HEADER_DIR}/status.h"
    )
set(PROJECT_UI_FILES
    "${PROJECT_UI_DIR}/calibform.ui"
    "${PROJECT_UI_DIR}/calibscandialog.ui"
    "${PROJECT_UI_DIR}/gnssposwidget.ui"
    "${PROJECT_UI_DIR}/gpssatsform.ui"
    "${PROJECT_UI_DIR}/histogramdataform.ui"
    "${PROJECT_UI_DIR}/i2cform.ui"
    "${PROJECT_UI_DIR}/logplotswidget.ui"
    "${PROJECT_UI_DIR}/mainwindow.ui"
    "${PROJECT_UI_DIR}/map.ui"
    "${PROJECT_UI_DIR}/parametermonitorform.ui"
    "${PROJECT_UI_DIR}/scanform.ui"
    "${PROJECT_UI_DIR}/settings.ui"
    "${PROJECT_UI_DIR}/spiform.ui"
    "${PROJECT_UI_DIR}/status.ui"
    )

set(PROJECT_RESOURCE_FILES "")

if (WIN32)
set(PROJECT_RESOURCE_FILES
    "${PROJECT_RESOURCE_FILES}"
    "${PROJECT_RES_DIR}/windows.rc"
    )
configure_file(
    "${PROJECT_RES_DIR}/res/muon.ico"
    "${CMAKE_CURRENT_BINARY_DIR}/muon.ico"
    )
else()
configure_file(
    "${PROJECT_CONFIG_DIR}/muondetector-gui.1"
    "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1"
    )
endif()

find_package(Qt5QuickCompiler)

if(Qt5QuickCompiler_FOUND)

qtquick_compiler_add_resources(qml_QRC "${PROJECT_RES_DIR}/resources.qrc")

else()

qt5_add_resources(qml_QRC "${PROJECT_RES_DIR}/resources.qrc")

endif()

add_executable(muondetector-gui ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES} ${PROJECT_UI_FILES} ${PROJECT_RESOURCE_FILES} ${qml_QRC})

target_include_directories(muondetector-gui PUBLIC
    $<BUILD_INTERFACE:${PROJECT_HEADER_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>
    $<BUILD_INTERFACE:/usr/local/include/mqtt>
    $<BUILD_INTERFACE:/usr/include/qwt>
    $<INSTALL_INTERFACE:include/mqtt>
    )
target_link_directories(muondetector-gui PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../lib/")

if(WIN32)
add_definitions(-DQWT_DLL)

target_include_directories(muondetector-gui PUBLIC
    ${OPENSSL_DIR}/include/
    $<BUILD_INTERFACE:${CRYPTOPP_DIR}/include>
    $<INSTALL_INTERFACE:${CRYPTOPP_DIR}/include>
    $<BUILD_INTERFACE:${QWT_DIR}/include>
    $<INSTALL_INTERFACE:${QWT_DIR}/include>
    $<BUILD_INTERFACE:${MQTT_C_DIR}/include>
    $<INSTALL_INTERFACE:${MQTT_C_DIR}/include>
    $<BUILD_INTERFACE:${MQTT_CPP_DIR}/include/mqtt>
    $<INSTALL_INTERFACE:${MQTT_CPP_DIR}/include/mqtt>
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Svg_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5QuickWidgets_INCLUDE_DIRS}
    ${Qt5Qml_INCLUDE_DIRS}
    )
target_link_directories(muondetector-gui PUBLIC
    "${MQTT_CPP_DIR}/lib/"
    "${MQTT_C_DIR}/lib/"
    "${QWT_DIR}/lib/"
    "${OPENSSL_DIR}/lib/"
    "${CRYPTOPP_DIR}/lib/"
    )

target_link_libraries(muondetector-gui
    Qt5::Network Qt5::Svg Qt5::Widgets Qt5::Gui Qt5::Quick Qt5::QuickWidgets Qt5::Qml
    muondetector
    paho-mqtt3c.lib
    paho-mqtt3a.lib
    paho-mqtt3cs.lib
    paho-mqtt3as.lib
    qwt.lib
    cryptlib.lib
    paho-mqttpp3.lib
    )

else()

target_link_libraries(muondetector-gui
    Qt5::Network Qt5::Svg Qt5::Widgets Qt5::Gui Qt5::Quick Qt5::QuickWidgets Qt5::Qml
    muondetector
    pthread
    paho-mqtt3c
    paho-mqtt3a
    paho-mqtt3cs
    paho-mqtt3as
    crypto++
    qwt-qt5
    paho-mqttpp3
    )

endif()


install(FILES "${PROJECT_CONFIG_DIR}/muon.ico" DESTINATION "share/pixmaps/" COMPONENT gui)
install(FILES "${PROJECT_CONFIG_DIR}/muondetector-gui.desktop" DESTINATION "share/applications/" COMPONENT gui)
install(TARGETS muondetector-gui DESTINATION bin COMPONENT gui)

if(WIN32)

include("${PROJECT_SOURCE_DIR}/../cmake/Windeployqt.cmake")
set(windeploy_options
    --qmldir "${PROJECT_QML_DIR}"
    -opengl
    -printsupport
) # additional options for windeployqt.exe


windeployqt(muondetector-gui "${PROJECT_BINARY_DIR}/bin" "${windeploy_options}")
# create a list of files to copy
set( THIRD_PARTY_DLLS
   "${OPENSSL_DIR}/bin/libcrypto-1_1-x64.dll"
   "${OPENSSL_DIR}/bin/libssl-1_1-x64.dll"
   "${QWT_DIR}/lib/qwt.dll"
   "${MQTT_CPP_DIR}/lib/paho-mqttpp3.dll"
   "${MQTT_C_DIR}/lib/paho-mqtt3a.dll"
   "${MQTT_C_DIR}/lib/paho-mqtt3as.dll"
   "${MQTT_C_DIR}/lib/paho-mqtt3c.dll"
   "${MQTT_C_DIR}/lib/paho-mqtt3cs.dll"
)

# do the copying
foreach( file_i ${THIRD_PARTY_DLLS})
    add_custom_command(
            TARGET muondetector-gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${file_i}"
                    "${PROJECT_BINARY_DIR}/../bin/"
    )
    install(FILES "${file_i}" DESTINATION "bin" COMPONENT gui)
endforeach( file_i )


set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_CONFIG_DIR}/description.txt")
set(CPACK_NSIS_MODIFY_PATH ON)
include(InstallRequiredSystemLibraries)

#set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

# There is a bug in NSI that does not handle full UNIX paths properly.
# Make sure there is at least one set of four backlashes.
#set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\muondetector-gui.exe")
set(CPACK_NSIS_DISPLAY_NAME "Muondetector gui")
set(CPACK_NSIS_HELP_LINK "https://muonpi.org")
set(CPACK_NSIS_URL_INFO_ABOUT "https://muonpi.org")
set(CPACK_NSIS_CONTACT "support@muonpi.org")
set(CPACK_NSIS_MUI_ICON "${PROJECT_RES_DIR}/res/muon.ico")
set(CPACK_NSIS_MUI_UNIICON "${PROJECT_RES_DIR}/res/muon.ico")
set(CPACK_PACKAGE_EXECUTABLES "muondetector-gui" "muondetector-gui")

else()

include(GNUInstallDirs)

add_custom_target(changelog ALL COMMAND gzip -cn9 "${PROJECT_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")
add_custom_target(manpage ALL COMMAND gzip -cn9 "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1" > "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1.gz")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT gui)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1.gz" DESTINATION "share/man/man1/" COMPONENT gui)
install(FILES "${PROJECT_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT gui)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libpaho-mqttpp | paho-mqtt-cpp, qml-module-qtpositioning (>=5), qml-module-qtlocation (>=5), qml-module-qtquick2 (>=5), qml-module-qtquick-layouts (>=5), qml-module-qtquick-controls2 (>=5), qml-module-qtquick-controls (>=5), qml-module-qtquick-templates2 (>=5)")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/MuonPi/muondetector")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION " It connects to muondetector-daemon via TCP. It is based on Qt and C++.
 It lets you change the settings for the muondetector hardware and
 uses qml for displaying the current position on the map if connected
 the muondetector-daemon.
 It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MuonPi <developer@muonpi.org>")

endif()
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_CONFIG_DIR}/license")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../packages/")
set(CPACK_PACKAGE_VENDOR "MuonPi.org")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GUI for monitoring and controlling the muondetector-daemon.")

include(CPack)

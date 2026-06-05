set(MUONDETECTOR_GUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/src")
set(MUONDETECTOR_GUI_UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/src")
set(MUONDETECTOR_GUI_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/")
set(MUONDETECTOR_GUI_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/include")
set(MUONDETECTOR_GUI_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/config")
set(MUONDETECTOR_GUI_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/qml")
set(MUONDETECTOR_GUI_APPLE_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gui/res")


find_package(Qt6 REQUIRED COMPONENTS
    Network Svg Widgets Gui Quick QuickWidgets Qml Positioning
)
qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)
    find_library(MOSQUITTO
        NAMES mosquitto
        HINTS ${MOSQUITTO_DIR}
        REQUIRED
    )
    if(MOSQUITTO)
        message(STATUS "Mosquitto found: ${MOSQUITTO}")
    endif()
endif()


set(MUONDETECTOR_GUI_SOURCE_FILES
    "${MUONDETECTOR_GUI_SOURCE_DIR}/calibform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/calibscandialog.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/custom_histogram_widget.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/custom_plot_widget.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/gnssposwidget.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/gnssinfoform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/histogramdataform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/i2cform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/logplotswidget.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/main.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/mainwindow.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/map.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/parametermonitorform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/plotcustom.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/scanform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/ubloxsettingsform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/spiform.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/status.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/delegates/itemdeletedelegate.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/histogram_series_data.cpp"
    "${MUONDETECTOR_GUI_SOURCE_DIR}/curve_series_data.cpp"
    )

set(MUONDETECTOR_GUI_HEADER_FILES
    "${MUONDETECTOR_GUI_HEADER_DIR}/calibform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/calibscandialog.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/custom_histogram_widget.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/custom_plot_widget.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/gnssposwidget.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/gnssinfoform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/histogramdataform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/i2cform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/logplotswidget.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/mainwindow.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/map.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/parametermonitorform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/plotcustom.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/scanform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/ubloxsettingsform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/spiform.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/status.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/delegates/itemdeletedelegate.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/histogram_series_data.h"
    "${MUONDETECTOR_GUI_HEADER_DIR}/curve_series_data.h"
    )
set(MUONDETECTOR_GUI_UI_FILES
    "${MUONDETECTOR_GUI_UI_DIR}/calibform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/calibscandialog.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/gnssposwidget.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/gnssinfoform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/histogramdataform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/i2cform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/logplotswidget.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/mainwindow.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/map.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/parametermonitorform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/scanform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/ubloxsettingsform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/spiform.ui"
    "${MUONDETECTOR_GUI_UI_DIR}/status.ui"
    )

set(MUONDETECTOR_GUI_RESOURCE_FILES "")

if (WIN32)
set(MUONDETECTOR_GUI_RESOURCE_FILES
    "${MUONDETECTOR_GUI_RESOURCE_FILES}"
    "${MUONDETECTOR_GUI_RES_DIR}/windows.rc"
    )
configure_file(
    "${MUONDETECTOR_GUI_RES_DIR}/res/muon.ico"
    "${CMAKE_CURRENT_BINARY_DIR}/muon.ico"
    )
else()

configure_file(
    "${MUONDETECTOR_GUI_CONFIG_DIR}/muondetector-gui.1"
    "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1"
    )

endif()

qt_add_resources(qml_QRC "${MUONDETECTOR_GUI_RES_DIR}/resources.qrc")
# find_package(Qt5 COMPONENTS Network Svg Widgets Gui Quick QuickWidgets Qml REQUIRED)
# find_package(Qt5QuickCompiler)

# if(Qt5QuickCompiler_FOUND)

# qtquick_compiler_add_resources(qml_QRC "${MUONDETECTOR_GUI_RES_DIR}/resources.qrc")

# else()

# qt5_add_resources(qml_QRC "${MUONDETECTOR_GUI_RES_DIR}/resources.qrc")

# endif()

if(APPLE)

  set(MACOSX_BUNDLE_ICON_FILE muon.icns)
  set(myApp_ICON ${MUONDETECTOR_GUI_APPLE_RES_DIR}/muon.icns)
  set_source_files_properties(${myApp_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

qt_add_executable(muondetector-gui MACOSX_BUNDLE
    ${myApp_ICON}
    ${MUONDETECTOR_GUI_SOURCE_FILES}
    ${MUONDETECTOR_GUI_HEADER_FILES}
    ${MUONDETECTOR_GUI_UI_FILES}
    ${MUONDETECTOR_GUI_RESOURCE_FILES}
    ${qml_QRC}
)
#   add_executable(muondetector-gui MACOSX_BUNDLE ${myApp_ICON} ${MUONDETECTOR_GUI_SOURCE_FILES} ${MUONDETECTOR_GUI_HEADER_FILES} ${MUONDETECTOR_GUI_UI_FILES} ${MUONDETECTOR_GUI_RESOURCE_FILES} ${qml_QRC} )  

  set_target_properties(muondetector-gui PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${MUONDETECTOR_GUI_APPLE_RES_DIR}/Info.plist.in)

elseif(WIN32)

# add_executable(muondetector-gui WIN32 ${MUONDETECTOR_GUI_SOURCE_FILES} ${MUONDETECTOR_GUI_HEADER_FILES} ${MUONDETECTOR_GUI_UI_FILES} ${MUONDETECTOR_GUI_RESOURCE_FILES} ${qml_QRC})

  qt_add_executable(muondetector-gui
    WIN32
    ${MUONDETECTOR_GUI_SOURCE_FILES}
    ${MUONDETECTOR_GUI_HEADER_FILES}
    ${MUONDETECTOR_GUI_UI_FILES}
    ${MUONDETECTOR_GUI_RESOURCE_FILES}
    ${qml_QRC}
  )
else()

# add_executable(muondetector-gui ${MUONDETECTOR_GUI_SOURCE_FILES} ${MUONDETECTOR_GUI_HEADER_FILES} ${MUONDETECTOR_GUI_UI_FILES} ${MUONDETECTOR_GUI_RESOURCE_FILES} ${qml_QRC})

  qt_add_executable(muondetector-gui
    ${MUONDETECTOR_GUI_SOURCE_FILES}
    ${MUONDETECTOR_GUI_HEADER_FILES}
    ${MUONDETECTOR_GUI_UI_FILES}
    ${MUONDETECTOR_GUI_RESOURCE_FILES}
    ${qml_QRC}
  )
endif()

set(MUONDETECTOR_DEPENDENCIES
    muondetector-shared
)
if (BUILDING_BUNDLED_QWT)
    set(MUONDETECTOR_DEPENDENCIES
        ${MUONDETECTOR_DEPENDENCIES}
        qwt
    )
endif()

add_dependencies(muondetector-gui ${MUONDETECTOR_DEPENDENCIES})

set_target_properties(muondetector-gui PROPERTIES POSITION_INDEPENDENT_CODE 1)

target_include_directories(muondetector-gui PUBLIC
    $<BUILD_INTERFACE:${MUONDETECTOR_GUI_HEADER_DIR}>
    $<BUILD_INTERFACE:${LIBRARY_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${MOSQUITTO_DIR}>
    $<BUILD_INTERFACE:${QWT_INCLUDE_DIR}>
    #for OSX
    $<BUILD_INTERFACE:/usr/local/include>
)

if(WIN32)

    target_link_libraries(muondetector-gui PRIVATE
        Qt6::Network Qt6::Svg Qt6::Widgets Qt6::Gui Qt6::Quick Qt6::QuickWidgets Qt6::Qml Qt6::Positioning
        ${QWT_LIBRARY}
        muondetector-shared
        protocol
        pthread
        ${MOSQUITTO}
    )

elseif(APPLE)

    target_link_libraries(muondetector-gui PRIVATE
        Qt6::Network Qt6::Svg Qt6::Widgets Qt6::Gui Qt6::Quick Qt6::QuickWidgets Qt6::Qml Qt6::Positioning
        ${QWT_LIBRARY}
        muondetector-shared
        protocol
        pthread
    )

else()

    target_link_libraries(muondetector-gui PRIVATE
        Qt6::Network Qt6::Svg Qt6::Widgets Qt6::Gui Qt6::Quick Qt6::QuickWidgets Qt6::Qml Qt6::Positioning
        ${QWT_LIBRARY}
        muondetector-shared
        muondetector-protocol
        pthread
    )

endif()

######################################################################################################
# PACKAGING
######################################################################################################

install(TARGETS muondetector-gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)


if(PACKAGING_MODE)
    message(STATUS "Packaging mode: disabling Qt deploy")
    set(QT_SKIP_AUTO_DEPLOY ON)
    install(FILES ${CMAKE_SOURCE_DIR}/desktop/muondetector-gui.metainfo.xml
        DESTINATION share/metainfo
    )
    install(FILES muondetector-gui.desktop
        DESTINATION share/applications
        COMPONENT gui
    )
    install(FILES muon.ico DESTINATION share/pixmaps/)
else()
    qt_generate_deploy_app_script(
        TARGET muondetector-gui
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

if(WIN32)

    # QWT (manual dependency - Qt does NOT handle this)
    install(FILES "${QWT_DIR}/lib/qwt.dll"
        DESTINATION bin
    )

    # MSVC/MinGW runtime (ONLY if you really need them)
    install(FILES
        "${SDKROOT}/bin/libwinpthread-1.dll"
        "${SDKROOT}/bin/libstdc++-6.dll"
        "${SDKROOT}/bin/libgcc_s_seh-1.dll"
        DESTINATION bin
    )

    # OpenSSL (still manual unless Qt ships it)
    install(FILES
        "${QTTOOLS}/OpenSSL/Win_x64/bin/libssl-1_1-x64.dll"
        "${QTTOOLS}/OpenSSL/Win_x64/bin/libcrypto-1_1-x64.dll"
        DESTINATION bin
    )

endif()

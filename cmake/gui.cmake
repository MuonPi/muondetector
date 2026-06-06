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
  # add_executable(muondetector-gui MACOSX_BUNDLE ${myApp_ICON} ${MUONDETECTOR_GUI_SOURCE_FILES} ${MUONDETECTOR_GUI_HEADER_FILES} ${MUONDETECTOR_GUI_UI_FILES} ${MUONDETECTOR_GUI_RESOURCE_FILES} ${qml_QRC} )  

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
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Windeployqt.cmake")
    set(windeploy_options
        --qmldir "${MUONDETECTOR_GUI_QML_DIR}"
        -opengl
        -printsupport
    ) # additional options for windeployqt.exe

    windeployqt(muondetector-gui "${PROJECT_BINARY_DIR}/bin" "${windeploy_options}")
    # create a list of files to copy
    message(STATUS "sdk: ${SDKROOT}")
    set( COPY_DLLS
    "${QWT_DIR}/lib/qwt.dll"
    "${SDKROOT}/bin/libwinpthread-1.dll"
    "${SDKROOT}/bin/libstdc++-6.dll"
    "${SDKROOT}/bin/libgcc_s_seh-1.dll"
    "${QTTOOLS}/OpenSSL/Win_x64/bin/libssl-1_1-x64.dll"
    )

    # do the copying
    foreach( file_i ${COPY_DLLS})
        add_custom_command(
                TARGET muondetector-gui POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        "${file_i}"
                        "${PROJECT_BINARY_DIR}/output/bin/"
        )
        install(FILES "${file_i}" DESTINATION "bin" COMPONENT gui)
    endforeach( file_i )

    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${MUONDETECTOR_GUI_CONFIG_DIR}/description.txt")
    set(CPACK_NSIS_MODIFY_PATH ON)

    set(CPACK_PACKAGE_NAME "muondetector-gui")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

    include(InstallRequiredSystemLibraries)

    # There is a bug in NSI that does not handle full UNIX paths properly.
    # Make sure there is at least one set of four backlashes.
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\muondetector-gui.exe")
    set(CPACK_NSIS_DISPLAY_NAME "Muondetector gui")
    set(CPACK_NSIS_HELP_LINK "https://muonpi.org")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://muonpi.org")
    set(CPACK_NSIS_CONTACT "support@muonpi.org")
    set(CPACK_NSIS_MUI_ICON "${MUONDETECTOR_GUI_RES_DIR}/res/muon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${MUONDETECTOR_GUI_RES_DIR}/res/muon.ico")
    set(CPACK_PACKAGE_EXECUTABLES "muondetector-gui" "muondetector-gui")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GUI for monitoring and controlling the muondetector-daemon.")

    target_link_libraries(muondetector-gui PRIVATE
        Qt6::Network Qt6::Svg Qt6::Widgets Qt6::Gui Qt6::Quick Qt6::QuickWidgets Qt6::Qml Qt6::Positioning
        Qwt::Qwt
        muondetector-shared
        protocol
        pthread
        ${MOSQUITTO}
    )

elseif(APPLE)

    target_link_libraries(muondetector-gui PRIVATE
        Qt6::Network Qt6::Svg Qt6::Widgets Qt6::Gui Qt6::Quick Qt6::QuickWidgets Qt6::Qml Qt6::Positioning
        Qwt::Qwt
        muondetector-shared
        protocol
        pthread
    )

else()

    target_link_libraries(muondetector-gui PRIVATE
        Qt6::Network Qt6::Svg Qt6::Widgets Qt6::Gui Qt6::Quick Qt6::QuickWidgets Qt6::Qml Qt6::Positioning
        Qwt::Qwt
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
    install(FILES ${CMAKE_SOURCE_DIR}/gui/config/muondetector-gui.metainfo.xml
        DESTINATION share/metainfo
    )
    install(FILES ${CMAKE_SOURCE_DIR}/gui/config/muondetector-gui.desktop
        DESTINATION share/applications
        COMPONENT gui
    )
    install(FILES ${CMAKE_SOURCE_DIR}/gui/config/muon.ico DESTINATION share/pixmaps/)
else()
    include(GNUInstallDirs)
    add_custom_target(prep-gui ALL COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/gui")
    add_custom_target(changelog-gui ALL COMMAND gzip -cn9 "${MUONDETECTOR_GUI_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/gui/changelog.gz")
    add_dependencies(changelog-gui prep-gui)
    add_custom_target(manpage-gui ALL COMMAND gzip -cn9 "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1" > "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1.gz")

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gui/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}-gui" COMPONENT gui)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/muondetector-gui.1.gz" DESTINATION "share/man/man1/" COMPONENT gui)
    install(FILES "${MUONDETECTOR_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}-gui" COMPONENT gui)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "qml-module-qtpositioning (>=6), qml-module-qtlocation (>=6), qml-module-qtquick2 (>=6), qml-module-qtquick-layouts (>=6), qml-module-qtquick-controls2 (>=6), qml-module-qtquick-controls (>=6), qml-module-qtquick-templates2 (>=6)")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION " GUI for monitoring and controlling the muondetector-daemon.
    It connects to muondetector-daemon via TCP. It is based on Qt and C++.
    It lets you change the settings for the muondetector hardware and
    uses qml for displaying the current position on the map if connected
    the muondetector-daemon.
    It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
    set(CPACK_DEBIAN_PACKAGE_NAME "muondetector-gui")

#    if(Qt6_VERSION VERSION_GREATER_EQUAL 6.5)
#        qt_generate_deploy_app_script(
#            TARGET muondetector-gui
#            OUTPUT_SCRIPT deploy_script
#            NO_UNSUPPORTED_PLATFORM_ERROR
#        )
#    else()
#        qt_generate_deploy_app_script(
#            TARGET muondetector-gui
#            FILENAME_VARIABLE deploy_script
#        )
#    endif()
#    install(SCRIPT ${deploy_script})
endif()

if(WIN32 OR APPLE)
    qt_generate_deploy_app_script(
        TARGET muondetector-gui
        OUTPUT_SCRIPT deploy_script
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

#Find QWT_LIBARY
if(APPLE)
    find_library(QWT_LIBRARY
        NAMES qwt
        HINTS /opt/local/libexec/qt6/lib
        QUIET)
    if(QWT_LIBRARY)
        include_directories(${QWT_LIBRARY}/Headers)
        link_libraries(${QWT_LIBRARY})
        message(STATUS "QWT_LIBRARY found: ${QWT_LIBRARY}")
    endif()
elseif(WIN32)
    find_library(QWT_LIBRARY
        NAMES qwt
        HINTS ${QWT_DIR}/lib
        QUIET)
        if(QWT_LIBRARY)
            message(STATUS "QWT_LIBRARY found: ${QWT_LIBRARY}")
        endif()
else()
    find_library(
        QWT_LIBRARY
        NAMES qwt qwt-qt6
        QUIET
    )
    if (QWT_LIBRARY)
        find_path(QWT_INCLUDE_DIR
            NAMES qwt_plot.h
            PATH_SUFFIXES qwt
            QUIET
        )
        if (QWT_INCLUDE_DIR)
            message(STATUS "Found QWT_INCLUDE_DIR " ${QWT_INCLUDE_DIR})
        else()
            message(STATUS "Failed to find QWT_INCLUDE_DIR, guessing /usr/include/qwt")
        endif()
    endif()
endif()


if(QWT_LIBRARY)

    message(STATUS "Using system Qwt")

else()
    message(STATUS "Building Qwt from source")
    set(BUILDING_BUNDLED_QWT true)

    # 1. ensure tool exists AND IS QT6!
    find_program(QT_QMAKE_EXECUTABLE
        NAMES qmake6 qmake-qt6
        REQUIRED
    )

    # 2. define install prefix
    set(QWT_PREFIX ${CMAKE_BINARY_DIR}/external/qwt)

    # 3. build external project

    find_program(QT_QMAKE_EXECUTABLE qmake REQUIRED)
    message(STATUS "QMAKE = ${QT_QMAKE_EXECUTABLE}")

    include(ExternalProject)

    set(QWT_PREFIX ${CMAKE_BINARY_DIR}/external/qwt)

    ExternalProject_Add(qwt_ext
        GIT_REPOSITORY https://git.code.sf.net/p/qwt/git
        GIT_TAG v6.3.0

        SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/qwt-src
        BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/qwt-build

        UPDATE_DISCONNECTED 0

        CONFIGURE_COMMAND
            ${QT_QMAKE_EXECUTABLE} <SOURCE_DIR>/qwt.pro
            QWT_INSTALL_PREFIX=${QWT_PREFIX}
            QWT_INSTALL_LIBS=${QWT_PREFIX}/lib
            QWT_INSTALL_HEADERS=${QWT_PREFIX}/include

        BUILD_COMMAND make
        INSTALL_COMMAND ""
    )

    # 4. force build trigger
    add_custom_target(qwt ALL DEPENDS qwt_ext)

    # 5. define IMPORTED target AFTER prefix exists
    # add_library(Qwt::Qwt SHARED IMPORTED GLOBAL)

    # set_target_properties(Qwt::Qwt PROPERTIES
    #     IMPORTED_LOCATION ${QWT_PREFIX}/lib/libqwt.so
    #     INTERFACE_INCLUDE_DIRECTORIES ${QWT_PREFIX}/include
    # )
    set(QWT_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/qwt-src/src)
    set(QWT_LIBRARY     ${CMAKE_BINARY_DIR}/_deps/qwt-build/lib/libqwt.so)

    add_library(Qwt::Qwt UNKNOWN IMPORTED)

    set_target_properties(Qwt::Qwt PROPERTIES
        IMPORTED_LOCATION ${QWT_PREFIX}/lib/libqwt.so
        INTERFACE_INCLUDE_DIRECTORIES ${QWT_PREFIX}/include
    )

    add_dependencies(Qwt::Qwt qwt_ext)
endif()
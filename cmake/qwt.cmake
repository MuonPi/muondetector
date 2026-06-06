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
    set(BUILDING_BUNDLED_QWT true)
    message(STATUS "Building Qwt from source")

    include(FetchContent)

    FetchContent_Declare(qwt
        GIT_REPOSITORY https://git.code.sf.net/p/qwt/git
        GIT_TAG v6.3.0
    )

    FetchContent_MakeAvailable(qwt)

    set(QWT_BUILD_DIR ${CMAKE_BINARY_DIR}/_deps/qwt-build)

    find_program(QMAKE_EXECUTABLE
        NAMES qmake6 qmake-qt6
        REQUIRED
    )

    add_custom_command(
        OUTPUT ${QWT_BUILD_DIR}/lib/libqwt.so
        COMMAND ${CMAKE_COMMAND} -E make_directory ${QWT_BUILD_DIR}

        COMMAND ${QMAKE_EXECUTABLE}
                ${qwt_SOURCE_DIR}/qwt.pro
                CONFIG+=release

        COMMAND make -j

        WORKING_DIRECTORY ${QWT_BUILD_DIR}
        DEPENDS ${qwt_SOURCE_DIR}
        COMMENT "Building Qwt via qmake"
    )

    add_custom_target(qwt ALL
        DEPENDS ${QWT_BUILD_DIR}/lib/libqwt.so
    )

    add_library(Qwt::Qwt SHARED IMPORTED GLOBAL)

    set_target_properties(Qwt::Qwt PROPERTIES
        IMPORTED_LOCATION ${QWT_BUILD_DIR}/lib/libqwt.so
        INTERFACE_INCLUDE_DIRECTORIES ${qwt_SOURCE_DIR}/src
    )

    add_dependencies(Qwt::Qwt qwt)
endif()
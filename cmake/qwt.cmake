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
        NAMES qwt qwt6
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

    set(QWT_BUILD_EXAMPLES OFF CACHE BOOL "")
    set(QWT_BUILD_TESTS OFF CACHE BOOL "")

    FetchContent_MakeAvailable(qwt)

    set(QWT_BUILD_DIR ${CMAKE_BINARY_DIR}/_deps/qwt-build)

    if (WIN32)
        find_program(QMAKE_EXECUTABLE
            NAMES qmake6 qmake-qt6 qmake6.exe
            REQUIRED
        )

        message(STATUS "Found QMAKE_EXECUTABLE " ${QMAKE_EXECUTABLE})

        add_custom_command(
            OUTPUT ${QWT_BUILD_DIR}/lib/qwt.dll ${QWT_BUILD_DIR}/lib/libqwt.a
            COMMAND ${CMAKE_COMMAND} -E make_directory ${QWT_BUILD_DIR}
            COMMAND ${CMAKE_COMMAND} -E env
                "PATH=C:/Qt/Tools/llvm-mingw1706_64/bin;$ENV{PATH}"
                QMAKESPEC=win32-clang-g++

                ${QMAKE_EXECUTABLE}
                ${qwt_SOURCE_DIR}/qwt.pro
                CONFIG+=release


            COMMAND "${MINGW_MAKE}" -j ${CPU_CORE_SUFFIX}

            WORKING_DIRECTORY ${QWT_BUILD_DIR}
            DEPENDS ${qwt_SOURCE_DIR}
            COMMENT "Building Qwt via qmake"
        )

        add_custom_target(qwt ALL
            DEPENDS ${QWT_BUILD_DIR}/lib/qwt.dll  ${QWT_BUILD_DIR}/lib/libqwt.a
        )

        add_library(Qwt::Qwt SHARED IMPORTED GLOBAL)
        set_target_properties(Qwt::Qwt PROPERTIES
            IMPORTED_LOCATION "${QWT_BUILD_DIR}/lib/qwt.dll"
            IMPORTED_IMPLIB   "${QWT_BUILD_DIR}/lib/libqwt.a"
            INTERFACE_INCLUDE_DIRECTORIES ${qwt_SOURCE_DIR}/src
        )
        add_dependencies(Qwt::Qwt qwt)
    else()
        find_program(QMAKE_EXECUTABLE
            NAMES qmake6 qmake-qt6
            REQUIRED
        )

        # If build on raspberry pi: 1 core is enough (don't overflow RAM)
        # Else: as many as possible
        if (MUONDETECTOR_ON_RASPBERRYPI)
            set(CPU_CORE_SUFFIX -j1)
        else()
            set(CPU_CORE_SUFFIX -j)
        endif()

        add_custom_command(
            OUTPUT ${QWT_BUILD_DIR}/lib/libqwt.so
            COMMAND ${CMAKE_COMMAND} -E make_directory ${QWT_BUILD_DIR}

            COMMAND ${QMAKE_EXECUTABLE}
                    ${qwt_SOURCE_DIR}/qwt.pro
                    CONFIG+=release

            COMMAND make ${CPU_CORE_SUFFIX}

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
endif()
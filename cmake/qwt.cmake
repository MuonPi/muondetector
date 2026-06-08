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
    set(QWT_SEARCH_DIRS
        "${CMAKE_SYSROOT}/usr/lib"
        "${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf"
        "${CMAKE_SYSROOT}/usr/local/lib"
        "${CMAKE_SYSROOT}/usr/local/lib/arm-linux-gnueabihf"
        "${CMAKE_SYSROOT}/opt/qwt/lib"
    )

    find_library(
        QWT_LIBRARY
        NAMES qwt qwt-qt6 qwt6
        # When cross-compiling, search only inside the sysroot
        HINTS ${QWT_SEARCH_DIRS}
        QUIET
    )

    if(NOT QWT_LIBRARY)
        file(GLOB QWT_VERSIONED_LIBRARY_CANDIDATES
            LIST_DIRECTORIES false
            "${CMAKE_SYSROOT}/usr/lib/libqwt*.so*"
            "${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf/libqwt*.so*"
            "${CMAKE_SYSROOT}/usr/local/lib/libqwt*.so*"
            "${CMAKE_SYSROOT}/usr/local/lib/arm-linux-gnueabihf/libqwt*.so*"
            "${CMAKE_SYSROOT}/opt/qwt/lib/libqwt*.so*"
        )
        if(QWT_VERSIONED_LIBRARY_CANDIDATES)
            list(GET QWT_VERSIONED_LIBRARY_CANDIDATES 0 QWT_LIBRARY)
        endif()
    endif()

    if (QWT_LIBRARY)
        find_path(QWT_INCLUDE_DIR
            NAMES qwt_plot.h
            HINTS
                "${CMAKE_SYSROOT}/usr/include"
                "${CMAKE_SYSROOT}/usr/local/include"
                "${CMAKE_SYSROOT}/opt/qwt/include"
            PATH_SUFFIXES qwt qwt6 qwt-qt6
            QUIET
        )
        if (QWT_INCLUDE_DIR)
            message(STATUS "Found QWT_INCLUDE_DIR " ${QWT_INCLUDE_DIR})
        else()
            message(STATUS "Failed to find QWT_INCLUDE_DIR")
        endif()
    endif()
endif()


if(QWT_LIBRARY AND EXISTS "${QWT_INCLUDE_DIR}")

    message(STATUS "Using system Qwt: ${QWT_LIBRARY}")
    if(NOT TARGET Qwt::Qwt)
        add_library(Qwt::Qwt UNKNOWN IMPORTED GLOBAL)
        set_target_properties(Qwt::Qwt PROPERTIES
            IMPORTED_LOCATION "${QWT_LIBRARY}"
        )
        if(QWT_INCLUDE_DIR)
            set_target_properties(Qwt::Qwt PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${QWT_INCLUDE_DIR}"
            )
        endif()
    endif()

else()
    message(STATUS "FetchContent_Declare")
    include(FetchContent)
    FetchContent_Declare(qwt
        GIT_REPOSITORY https://git.code.sf.net/p/qwt/git
        GIT_TAG v6.3.0
    )
    FetchContent_MakeAvailable(qwt)

    set(BUILDING_BUNDLED_QWT true)
    message(STATUS "Building Qwt from source")


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
                "PATH={TOOLCHAIN_PATH};$ENV{PATH}"
                QMAKESPEC=win32-clang-g++
                QMAKE_CC=${TOOLCHAIN_PATH}/clang.exe
                QMAKE_CXX=${TOOLCHAIN_PATH}/clang++.exe

                ${QMAKE_EXECUTABLE}
                ${qwt_SOURCE_DIR}/qwt.pro
                CONFIG+=release

            COMMAND ${CMAKE_COMMAND} -E env
                "PATH={TOOLCHAIN_PATH};$ENV{PATH}"
                ${TOOLCHAIN_PATH}/mingw32-make.exe -j 24

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

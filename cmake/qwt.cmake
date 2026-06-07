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

elseif(WIN32)
    message(STATUS "Building Qwt from source (Windows/qmake)")

    include(FetchContent)
    FetchContent_Declare(qwt
        GIT_REPOSITORY https://git.code.sf.net/p/qwt/git
        GIT_TAG v6.3.0
    )
    FetchContent_MakeAvailable(qwt)

    set(QWT_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/qwt-build")
    set(QWT_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/qwt-install")
    file(MAKE_DIRECTORY "${QWT_BUILD_DIR}")

    find_program(QMAKE_EXECUTABLE
        NAMES qmake6 qmake
        HINTS "${Qt6_DIR}/../../../bin"
        REQUIRED
    )

    # Configure with qmake
    execute_process(
        COMMAND "${QMAKE_EXECUTABLE}"
            "${qwt_SOURCE_DIR}/qwt.pro"
            "CONFIG+=release"
            "QWT_INSTALL_PREFIX=${QWT_INSTALL_DIR}"
        WORKING_DIRECTORY "${QWT_BUILD_DIR}"
        RESULT_VARIABLE QWT_QMAKE_RESULT
    )
    if(NOT QWT_QMAKE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to run qmake for Qwt")
    endif()

    # Build with ninja (qmake generates a Makefile but we can use mingw32-make)
    find_program(MINGW_MAKE
        NAMES mingw32-make make
        HINTS "C:/Qt/Tools/llvm-mingw1706_64/bin"
              "C:/Qt/Tools/mingw1310_64/bin"
        REQUIRED
    )

    execute_process(
        COMMAND "${MINGW_MAKE}" -j4
        WORKING_DIRECTORY "${QWT_BUILD_DIR}"
        RESULT_VARIABLE QWT_BUILD_RESULT
    )
    if(NOT QWT_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build Qwt")
    endif()

    execute_process(
        COMMAND "${MINGW_MAKE}" install
        WORKING_DIRECTORY "${QWT_BUILD_DIR}"
        RESULT_VARIABLE QWT_INSTALL_RESULT
    )
    if(NOT QWT_INSTALL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to install Qwt")
    endif()

    add_library(Qwt::Qwt SHARED IMPORTED GLOBAL)
    set_target_properties(Qwt::Qwt PROPERTIES
        IMPORTED_LOCATION "${QWT_INSTALL_DIR}/lib/qwt.dll"
        IMPORTED_IMPLIB   "${QWT_INSTALL_DIR}/lib/libqwt.dll.a"
        INTERFACE_INCLUDE_DIRECTORIES "${QWT_INSTALL_DIR}/include"
    )

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
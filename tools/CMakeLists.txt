cmake_minimum_required(VERSION 3.10)

project(readers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ltr390_reader
    ltr390_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/ltr390uv01.cpp
)

target_include_directories(ltr390_reader PRIVATE
    .
    ../daemon/src
)

add_executable(as7331_reader
    as7331_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/as7331.cpp
)

target_include_directories(as7331_reader PRIVATE
    .
    ../daemon/src
)

add_executable(as7343_reader
    as7343_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/as7343.cpp
)

target_include_directories(as7343_reader PRIVATE
    .
    ../daemon/src
)
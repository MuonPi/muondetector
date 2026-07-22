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
    ../daemon/src/hardware
)

add_executable(as7331_reader
    as7331_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/as7331.cpp
)

target_include_directories(as7331_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(as7343_reader
    as7343_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/as7343.cpp
)

target_include_directories(as7343_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(bme280_reader
    bme280_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/bme280.cpp
)

target_include_directories(bme280_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(sht31_reader
    sht31_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/sht31.cpp
)

target_include_directories(sht31_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(sht21_reader
    sht21_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/sht21.cpp
)

target_include_directories(sht21_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(lm75_reader
    lm75_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/lm75.cpp
)

target_include_directories(lm75_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(mic184_reader
    mic184_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/mic184.cpp
)

target_include_directories(mic184_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

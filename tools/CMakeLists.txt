cmake_minimum_required(VERSION 3.10)

project(readers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET muondetector-hardware)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/hardware.cmake")
endif()

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

add_executable(gy521_reader
    gy521_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/mpu6050.cpp
)

target_include_directories(gy521_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(hmc5883_reader
    hmc5883_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/hmc5883.cpp
)

target_include_directories(hmc5883_reader PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

add_executable(qmc5883_reader
    qmc5883_reader.cpp
    ../daemon/src/hardware/i2c/i2cdevice.cpp
    ../daemon/src/hardware/i2c/qmc5883.cpp
)

target_include_directories(qmc5883_reader PRIVATE
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

add_executable(ook_sender
    ook_sender.cpp
)

target_include_directories(ook_sender PRIVATE
    .
    ../daemon/src
    ../daemon/src/hardware
)

target_link_libraries(ook_sender PRIVATE
    muondetector-hardware
)

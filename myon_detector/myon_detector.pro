QT -= gui
QT += core
QT += network
QT += serialport

CONFIG += c++11 console
CONFIG -= app_bundle
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../shared  \
    src

LIBS += -lwiringPi

SOURCES += src/main.cpp \
    src/custom_io_operators.cpp \
    src/gnsssatellite.cpp \
    ../shared/tcpconnection.cpp \
    src/qtserialublox.cpp \
    src/qtserialublox_processmessages.cpp \
    src/demon.cpp \
    src/i2c/i2cdevices.cpp \
    src/i2c/custom_i2cdetect.c \
    src/i2c/i2cbusses.c \
    ../shared/i2cproperty.cpp

HEADERS += \
    src/custom_io_operators.h \
    src/gnsssatellite.h \
    src/unixtime_from_gps.h \
    src/structs_and_defines.h \
    src/time_from_rtc.h \
    ../shared/tcpconnection.h \
    src/qtserialublox.h \
    src/demon.h \
    src/i2c/addresses.h \
    src/i2c/custom_i2cdetect.h \
    src/i2c/i2cbusses.h \
    src/i2c/i2cdevices.h \
    src/i2c/linux/i2c-dev.h \
    ../shared/i2cproperty.h

OBJECTS_DIR += created_files

MOC_DIR += created_files

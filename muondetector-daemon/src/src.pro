QT -= gui
QT += core
QT += network
QT += serialport
VERSION = 1.0.3
CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = muondetector-daemon
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DESTDIR = ../bin
UI_DIR = ../build/ui
MOC_DIR = ../build/moc
RCC_DIR = ../build/rcc
OBJECTS_DIR = ../build/o/unix

INCLUDEPATH +=  . \
    main \
    main/i2c

INCLUDEPATH += ../../muondetector-shared/src/

LIBS += -L/usr/lib/muondetector-shared -lmuondetector-shared
DEPENDPATH += /usr/lib/muondetector-shared

LIBS += -lwiringPi \
        -lpigpiod_if2 \
        -lcrypto++ \
        -lrt

SOURCES += main/main.cpp \
    main/gnsssatellite.cpp \
    main/qtserialublox.cpp \
    main/qtserialublox_processmessages.cpp \
    main/i2c/i2cdevices.cpp \
#    main/i2c/custom_i2cdetect.c \
    main/i2c/i2cbusses.c \
    main/i2c/Adafruit_GFX.cpp \
    main/pigpiodhandler.cpp \
    main/daemon.cpp \
    main/custom_io_operators.cpp \
    main/filehandler.cpp \
    main/calibration.cpp \
    main/i2c/glcdfont.c \
    main/gpio_mapping.cpp


HEADERS += \
#    main/gnsssatellite.h \
    main/unixtime_from_gps.h \
    main/time_from_rtc.h \
    main/qtserialublox.h \
    main/i2c/addresses.h \
#    main/i2c/custom_i2cdetect.h \
    main/i2c/i2cbusses.h \
    main/i2c/i2cdevices.h \
    main/i2c/Adafruit_GFX.h \
#   main/i2c/linux/i2c-dev.h \
    main/pigpiodhandler.h \
    main/daemon.h \
#    main/ublox_structs.h \
    main/custom_io_operators.h \
    main/filehandler.h \
    main/calibration.h \
    main/logparameter.h \
    main/gpio_mapping.h

DISTFILES += ubx_rates_config.cfg

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

DESTDIR = $$PWD/bin
UI_DIR = $$PWD/build/ui
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
OBJECTS_DIR = $$PWD/build/o/unix

INCLUDEPATH +=  $$PWD \
    $$PWD/main \
    $$PWD/src/i2c

INCLUDEPATH += $$PWD/../muondetector-shared/src/

#LIBS += -L/usr/lib/muondetector-shared -lmuondetector-shared
LIBS += -L/usr/lib/ -lmuondetector-shared
#DEPENDPATH += /usr/lib/muondetector-shared

LIBS += -lpigpiod_if2 \
        -lcrypto++ \
        -lrt

SOURCES += $$PWD/src/main.cpp \
#    $$PWD/src/gnsssatellite.cpp \
    $$PWD/src/qtserialublox.cpp \
    $$PWD/src/qtserialublox_processmessages.cpp \
#    $$PWD/src/i2c/custom_i2cdetect.c \
    $$PWD/src/i2c/i2cbusses.c \
    $$PWD/src/pigpiodhandler.cpp \
    $$PWD/src/daemon.cpp \
    $$PWD/src/custom_io_operators.cpp \
    $$PWD/src/filehandler.cpp \
    $$PWD/src/calibration.cpp \
    $$PWD/src/gpio_mapping.cpp \
    $$PWD/src/i2c/i2cdevices/adafruit_ssd1306/adafruit_ssd1306.cpp \
    $$PWD/src/i2c/i2cdevices/ads1115/ads1115.cpp \
    $$PWD/src/i2c/i2cdevices/bme280/bme280.cpp \
    $$PWD/src/i2c/i2cdevices/bmp180/bmp180.cpp \
    $$PWD/src/i2c/i2cdevices/eeprom24aa02/eeprom24aa02.cpp \
    $$PWD/src/i2c/i2cdevices/hmc5883/hmc5883.cpp \
    $$PWD/src/i2c/i2cdevices/lm75/lm75.cpp \
    $$PWD/src/i2c/i2cdevices/mcp4728/mcp4728.cpp \
    $$PWD/src/i2c/i2cdevices/pca9536/pca9536.cpp \
    $$PWD/src/i2c/i2cdevices/sht21/sht21.cpp \
    $$PWD/src/i2c/i2cdevices/ubloxi2c/ubloxi2c.cpp \
    $$PWD/src/i2c/i2cdevices/x9119/x9119.cpp \
    $$PWD/src/i2c/i2cdevices/Adafruit_GFX.cpp \
    $$PWD/src/i2c/i2cdevices/i2cdevice.cpp \
    $$PWD/src/i2c/i2cdevices/glcdfont.c \
    $$PWD/src/tdc7200.cpp


HEADERS += \
#    $$PWD/src/gnsssatellite.h \
    $$PWD/src/unixtime_from_gps.h \
    $$PWD/src/time_from_rtc.h \
    $$PWD/src/qtserialublox.h \
    $$PWD/src/i2c/addresses.h \
#    $$PWD/src/i2c/custom_i2cdetect.h \
    $$PWD/src/i2c/i2cbusses.h \
    $$PWD/src/i2c/i2cdevices.h \
#   $$PWD/src/i2c/linux/i2c-dev.h \
    $$PWD/src/pigpiodhandler.h \
    $$PWD/src/daemon.h \
#    $$PWD/src/ublox_structs.h \
    $$PWD/src/custom_io_operators.h \
    $$PWD/src/filehandler.h \
    $$PWD/src/calibration.h \
    $$PWD/src/logparameter.h \
    $$PWD/src/gpio_mapping.h \
    $$PWD/src/i2c/i2cdevices/adafruit_ssd1306/adafruit_ssd1306.h \
    $$PWD/src/i2c/i2cdevices/ads1015/ads1015.h \
    $$PWD/src/i2c/i2cdevices/ads1115/ads1115.h \
    $$PWD/src/i2c/i2cdevices/bme280/bme280.h \
    $$PWD/src/i2c/i2cdevices/bmp180/bmp180.h \
    $$PWD/src/i2c/i2cdevices/eeprom24aa02/eeprom24aa02.h \
    $$PWD/src/i2c/i2cdevices/hmc5883/hmc5883.h \
    $$PWD/src/i2c/i2cdevices/lm75/lm75.h \
    $$PWD/src/i2c/i2cdevices/mcp4728/mcp4728.h \
    $$PWD/src/i2c/i2cdevices/pca9536/pca9536.h \
    $$PWD/src/i2c/i2cdevices/sht21/sht21.h \
    $$PWD/src/i2c/i2cdevices/ubloxi2c/ubloxi2c.h \
    $$PWD/src/i2c/i2cdevices/x9119/x9119.h \
    $$PWD/src/i2c/i2cdevices/Adafruit_GFX.h \
    $$PWD/src/i2c/i2cdevices/i2cdevice.h \
    $$PWD/src/tdc7200.h

DISTFILES += $$PWD/ubx_rates_config.cfg

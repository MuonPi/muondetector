#-------------------------------------------------
#
# Project created by QtCreator 2018-05-05T16:09:13
#
#-------------------------------------------------
win32{
include ( C:/qwt-6.1.3/features/qwt.prf )
}
VERSION = 1.1.1
QT       += core \
          widgets \
          gui \
          network
QT       += svg
QT       += quickwidgets
QT       += quick
QT       += qml

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += qwt
CONFIG += c++11
CONFIG += qtquickcompiler

#QMAKE_CXXFLAGS += -mthumb
#QMAKE_CXXFLAGS += -mthumb-interwork
#QMAKE_CXXFLAGS += -marm

TARGET = muondetector-gui
TEMPLATE = app
CONFIG += warn_on
CONFIG += release

linux { #assumes that it is compiled on raspberry pi!!! if not the case delete
    contains(QMAKE_HOST.arch, arm.*):{
        QMAKE_CXXFLAGS += -mthumb
        QMAKE_CXXFLAGS += -mthumb-interwork
        QMAKE_CXXFLAGS += -march=armv7-a
    }
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DESTDIR = bin
UI_DIR = build/ui
MOC_DIR = build/moc
RCC_DIR = build/rcc
unix:OBJECTS_DIR = build/o/unix
win32:OBJECTS_DIR = build/o/win32
macx:OBJECTS_DIR = build/o/mac

INCLUDEPATH += \
    $$PWD/src/main \
    $$PWD/qml

INCLUDEPATH += $$PWD/../muondetector-shared/src/

INCLUDEPATH += /usr/local/qwt-6.1.3/include/
INCLUDEPATH += /usr/local/include/qwt/

#unix:INCLUDEPATH += /usr/lib/muondetector-shared

unix:LIBS += -L/usr/lib -L/usr/local/lib -L/usr/local/qwt-6.1.3/lib -lqwt-qt5
else:unix:LIBS += -L/usr/local/qwt-6.1.3/lib -lqwt-qt5
else:unix:LIBS += -L/usr/lib/muondetector-gui -lqwt-qt5
else:unix:LIBS += "$$PWD/../lib/libqwt.so.6.1.3"
else:unix:LIBS += -L/usr/lib/ -lqwt-qt5
unix:LIBS += -L/usr/lib/muondetector-shared -lmuondetector-shared
win32:LIBS += -L./lib -lmuondetector-shared1

#QT5_ADD_RESOURCES(RESOURCES resources.qrc)

FORMS += \
    $$PWD/src/main/calibform.ui \
    $$PWD/src/main/gpssatsform.ui \
    $$PWD/src/main/histogramdataform.ui \
    $$PWD/src/main/i2cform.ui \
    $$PWD/src/main/mainwindow.ui \
    $$PWD/src/main/map.ui \
    $$PWD/src/main/settings.ui \
    $$PWD/src/main/spiform.ui \
    $$PWD/src/main/status.ui \
    $$PWD/src/main/parametermonitorform.ui \
    src/main/calibscandialog.ui \
    src/main/logplotswidget.ui

HEADERS += \
    $$PWD/src/main/calibform.h \
    $$PWD/src/main/custom_histogram_widget.h \
    $$PWD/src/main/custom_plot_widget.h \
    $$PWD/src/main/gpssatsform.h \
    $$PWD/src/main/histogramdataform.h \
    $$PWD/src/main/i2cform.h \
    $$PWD/src/main/mainwindow.h \
    $$PWD/src/main/map.h \
    $$PWD/src/main/plotcustom.h \
    $$PWD/src/main/settings.h \
    $$PWD/src/main/spiform.h \
    $$PWD/src/main/status.h \
    $$PWD/src/main/parametermonitorform.h \
    src/main/calibscandialog.h \
    src/main/logplotswidget.h

SOURCES += \
    $$PWD/src/main/calibform.cpp \
    $$PWD/src/main/custom_histogram_widget.cpp \
    $$PWD/src/main/custom_plot_widget.cpp \
    $$PWD/src/main/gpssatsform.cpp \
    $$PWD/src/main/histogramdataform.cpp \
    $$PWD/src/main/i2cform.cpp \
    $$PWD/src/main/main.cpp \
    $$PWD/src/main/mainwindow.cpp \
    $$PWD/src/main/map.cpp \
    $$PWD/src/main/plotcustom.cpp \
    $$PWD/src/main/settings.cpp \
    $$PWD/src/main/spiform.cpp \
    $$PWD/src/main/status.cpp   \
    $$PWD/src/main/parametermonitorform.cpp \
    src/main/calibscandialog.cpp \
    src/main/logplotswidget.cpp

RESOURCES += \
    resources.qrc

DISTFILES += \
    res/mymap.qml \
    res/icon.ico

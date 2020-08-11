#-------------------------------------------------
#
# Project created by QtCreator 2018-05-05T16:09:13
#
#-------------------------------------------------
win32{
include ( C:/qwt-6.1.4/features/qwt.prf )
}
VERSION = 1.2.2
DEFINES += VERSION_STRING=\\\"v$${VERSION}\\\"
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
    $$PWD/src \
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
    $$PWD/src/calibform.ui \
    $$PWD/src/gpssatsform.ui \
    $$PWD/src/histogramdataform.ui \
    $$PWD/src/i2cform.ui \
    $$PWD/src/mainwindow.ui \
    $$PWD/src/map.ui \
    $$PWD/src/settings.ui \
    $$PWD/src/spiform.ui \
    $$PWD/src/status.ui \
    $$PWD/src/parametermonitorform.ui \
    $$PWD/src/calibscandialog.ui \
    $$PWD/src/logplotswidget.ui \
    $$PWD/src/gnssposwidget.ui \
    $$PWD/src/scanform.ui

HEADERS += \
    $$PWD/src/calibform.h \
    $$PWD/src/custom_histogram_widget.h \
    $$PWD/src/custom_plot_widget.h \
    $$PWD/src/gpssatsform.h \
    $$PWD/src/histogramdataform.h \
    $$PWD/src/i2cform.h \
    $$PWD/src/mainwindow.h \
    $$PWD/src/map.h \
    $$PWD/src/plotcustom.h \
    $$PWD/src/settings.h \
    $$PWD/src/spiform.h \
    $$PWD/src/status.h \
    $$PWD/src/parametermonitorform.h \
    $$PWD/src/calibscandialog.h \
    $$PWD/src/logplotswidget.h \
    $$PWD/src/gnssposwidget.h \
    $$PWD/src/scanform.h

SOURCES += \
    $$PWD/src/calibform.cpp \
    $$PWD/src/custom_histogram_widget.cpp \
    $$PWD/src/custom_plot_widget.cpp \
    $$PWD/src/gpssatsform.cpp \
    $$PWD/src/histogramdataform.cpp \
    $$PWD/src/i2cform.cpp \
    $$PWD/src/main.cpp \
    $$PWD/src/mainwindow.cpp \
    $$PWD/src/map.cpp \
    $$PWD/src/plotcustom.cpp \
    $$PWD/src/settings.cpp \
    $$PWD/src/spiform.cpp \
    $$PWD/src/status.cpp   \
    $$PWD/src/parametermonitorform.cpp \
    $$PWD/src/calibscandialog.cpp \
    $$PWD/src/logplotswidget.cpp \
    $$PWD/src/gnssposwidget.cpp \
    $$PWD/src/scanform.cpp

RESOURCES += \
    resources.qrc

win32:RC_ICONS = res/muon.ico

DISTFILES += \
    res/mymap.qml \
    res/icon.ico

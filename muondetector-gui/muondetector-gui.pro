#-------------------------------------------------
#
# Project created by QtCreator 2018-05-05T16:09:13
#
#-------------------------------------------------
win32{
include ( C:/qwt-6.1.3/features/qwt.prf )
}
VERSION = 1.0.3
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
    src/main \
    qml

INCLUDEPATH += $$PWD/../muondetector-shared/src/

INCLUDEPATH += /usr/local/qwt-6.1.3/include/
INCLUDEPATH += /usr/local/include/qwt/

#unix:INCLUDEPATH += /usr/lib/muondetector-shared

unix:LIBS += -L/usr/lib -L/usr/local/lib -L/usr/local/qwt-6.1.3/lib -lqwt-qt5
else:unix:LIBS += -L/usr/local/qwt-6.1.3/lib -lqwt
else:unix:LIBS += -L/usr/lib/muondetector-gui -lqwt
else:unix:LIBS += "$$PWD/../lib/libqwt.so.6.1.3"
else:unix:LIBS += -L/usr/lib/ -lqwt
unix:LIBS += -L/usr/lib/muondetector-shared -lmuondetector-shared
win32:LIBS += -L./lib -lmuondetector-shared1

#QT5_ADD_RESOURCES(RESOURCES resources.qrc)

FORMS += \
    src/main/calibform.ui \
    src/main/gpssatsform.ui \
    src/main/histogramdataform.ui \
    src/main/i2cform.ui \
    src/main/mainwindow.ui \
    src/main/map.ui \
    src/main/settings.ui \
    src/main/status.ui

HEADERS += \
    src/main/calibform.h \
    src/main/custom_histogram_widget.h \
    src/main/custom_plot_widget.h \
    src/main/gpssatsform.h \
    src/main/histogramdataform.h \
    src/main/i2cform.h \
    src/main/mainwindow.h \
    src/main/map.h \
    src/main/plotcustom.h \
    src/main/settings.h \
    src/main/status.h

SOURCES += \
    src/main/calibform.cpp \
    src/main/custom_histogram_widget.cpp \
    src/main/custom_plot_widget.cpp \
    src/main/gpssatsform.cpp \
    src/main/histogramdataform.cpp \
    src/main/i2cform.cpp \
    src/main/main.cpp \
    src/main/mainwindow.cpp \
    src/main/map.cpp \
    src/main/plotcustom.cpp \
    src/main/settings.cpp \
    src/main/status.cpp

RESOURCES += \
    resources.qrc

DISTFILES += \
    res/mymap.qml \
    res/icon.ico

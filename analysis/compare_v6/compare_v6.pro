QT       += core gui
#ATTENTION: don't use files with "< or >" in the name it will break the program, thank you!
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = compare_v6

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += DO_NOT_USE_COMPARE_MAIN_CPP
DEFINES += PATH_ROLE 42
# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    compare_v6.cpp \
    filemodel.cpp \
    main.cpp \
    mainwindow.cpp \
    selectionmodel.cpp \
    treeitem.cpp

HEADERS += \
    filemodel.h \
    mainwindow.h \
    selectionmodel.h \
    treeitem.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

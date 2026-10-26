QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = SimpleEditor
TEMPLATE = app
CONFIG  += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    codeeditor.cpp \
    finddialog.cpp \
    replacedialog.cpp \
    aboutdialog.cpp

HEADERS += \
    mainwindow.h \
    codeeditor.h \
    finddialog.h \
    replacedialog.h \
    aboutdialog.h

FORMS += \
    forms/mainwindow.ui \
    forms/finddialog.ui \
    forms/replacedialog.ui \
    forms/aboutdialog.ui

RESOURCES += icons.qrc

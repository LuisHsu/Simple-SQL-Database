#-------------------------------------------------
#
# Project created by QtCreator 2016-05-19T04:20:36
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DBProject
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    createdbdialog.cpp \
    filesystem.cpp \
    createtabledialog.cpp \
    loadtabledialog.cpp

HEADERS  += mainwindow.h \
    createdbdialog.h \
    filesystem.h \
    createtabledialog.h \
    loadtabledialog.h

FORMS    += mainwindow.ui \
    createdbdialog.ui \
    createtabledialog.ui \
    loadtabledialog.ui

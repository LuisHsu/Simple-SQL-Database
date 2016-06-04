QT += core

CONFIG += c++11

TARGET = Dumper
CONFIG += console

TEMPLATE = app

SOURCES += main.cpp \
    ../Program/filesystem.cpp

HEADERS += \
    ../Program/filesystem.h

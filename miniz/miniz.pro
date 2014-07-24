#-------------------------------------------------
#
# Project created by QtCreator 2014-07-24T14:42:34
#
#-------------------------------------------------

QT       -= core gui

TARGET = miniz
TEMPLATE = lib
CONFIG += staticlib

QMAKE_CXXFLAGS += -fno-strict-aliasing

SOURCES += miniz.cpp

HEADERS += \
    miniz.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}

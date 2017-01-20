#-------------------------------------------------
#
# Project created by QtCreator 2013-09-20T09:00:16
#
#-------------------------------------------------

QT += core gui sql multimedia network widgets

TARGET = BreakMusic
TEMPLATE = app

win32: RC_FILE = BreakMusic.rc
#win32: CONFIG += static

SOURCES += main.cpp\
        mainwindow.cpp \
    databasedialog.cpp \
    bmipcserver.cpp \
    bmsettings.cpp \
    pltablemodel.cpp \
    plitemdelegate.cpp \
    dbtablemodel.cpp \
    dbupdatethread.cpp \
    dbitemdelegate.cpp \
    abstractaudiobackend.cpp \
    audiobackendgstreamer.cpp

HEADERS  += mainwindow.h \
    databasedialog.h \
    bmipcserver.h \
    bmsettings.h \
    pltablemodel.h \
    plitemdelegate.h \
    dbtablemodel.h \
    dbupdatethread.h \
    dbitemdelegate.h \
    abstractaudiobackend.h \
    audiobackendgstreamer.h

FORMS    += mainwindow.ui \
    databasedialog.ui

RESOURCES += \
    resources.qrc

unix: QT_CONFIG -= no-pkg-config
unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += taglib_c
unix: PKGCONFIG += gstreamer-1.0
#unix: PKGCONFIG += fftw3

win32: CONFIG += link_pkgconfig
win32: PKGCONFIG += taglib_c
win32: PKGCONFIG += gstreamer-1.0

unix {
    iconfiles.files += icons/bmicon64x64.png
    iconfiles.path = /usr/share/pixmaps
    desktopfiles.files += breakmusic.desktop
    desktopfiles.path = /usr/share/applications
    binaryfiles.files += BreakMusic
    binaryfiles.path = /usr/bin
    INSTALLS += binaryfiles iconfiles desktopfiles
}

DISTFILES += \
    bmicon.ico \
    BreakMusic.rc \
    LICENSE


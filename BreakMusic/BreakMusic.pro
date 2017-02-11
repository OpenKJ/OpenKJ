#-------------------------------------------------
#
# Project created by QtCreator 2013-09-20T09:00:16
#
#-------------------------------------------------

QT += core gui sql multimedia network widgets

TARGET = BreakMusic
TEMPLATE = app

win32: RC_FILE = BreakMusic.rc

SOURCES += main.cpp\
    mainwindow.cpp \
    bmipcserver.cpp \
    bmsettings.cpp \
    abstractaudiobackend.cpp \
    audiobackendgstreamer.cpp \
    tagreader.cpp \
    bmdbdialog.cpp \
    bmdbupdatethread.cpp \
    bmdbitemdelegate.cpp \
    bmdbtablemodel.cpp \
    bmpltablemodel.cpp \
    bmplitemdelegate.cpp

HEADERS  += mainwindow.h \
    bmipcserver.h \
    bmsettings.h \
    abstractaudiobackend.h \
    audiobackendgstreamer.h \
    tagreader.h \
    bmdbdialog.h \
    bmdbupdatethread.h \
    bmdbitemdelegate.h \
    bmdbtablemodel.h \
    bmpltablemodel.h \
    bmplitemdelegate.h

FORMS    += mainwindow.ui \
    bmdbdialog.ui

RESOURCES += \
    resources.qrc

unix:!macx {
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-1.0
    iconfiles.files += icons/bmicon64x64.png
    iconfiles.path = /usr/share/pixmaps
    desktopfiles.files += breakmusic.desktop
    desktopfiles.path = /usr/share/applications
    binaryfiles.files += BreakMusic
    binaryfiles.path = /usr/bin
    INSTALLS += binaryfiles iconfiles desktopfiles
}

macx: {
    LIBS += -F/Library/Frameworks -framework GStreamer
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    ICON = icons/BreakMusic.icns
}

win32 {
    ## Windows common build here

    !contains(QMAKE_TARGET.arch, x86_64) {
        ## Windows x86 (32bit) specific build here
        INCLUDEPATH += C:\gstreamer\1.0\x86\include\gstreamer-1.0
        INCLUDEPATH += C:\gstreamer\1.0\x86\include\glib-2.0
        INCLUDEPATH += C:\gstreamer\1.0\x86\lib\glib-2.0\include
        INCLUDEPATH += C:\gstreamer\1.0\x86\include\glib-2.0\gobject
        LIBS += -LC:\gstreamer\1.0\x86\lib -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0
    } else {
        ## Windows x64 (64bit) specific build here
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\include\gstreamer-1.0
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\include\glib-2.0
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\lib\glib-2.0\include
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\include\glib-2.0\gobject
        LIBS += -LC:\gstreamer\1.0\x86_64\lib -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0
    }
}

DISTFILES += \
    bmicon.ico \
    BreakMusic.rc \
    LICENSE


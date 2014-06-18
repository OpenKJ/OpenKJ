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
    songdbtablemodel.cpp \
    playlisttablemodel.cpp \
    khipc.cpp \
    sourcedirtablemodel.cpp \
    databasedialog.cpp \
    databaseupdatethread.cpp \
    fader.cpp \
    bmipcserver.cpp \
    bmplaylist.cpp \
    bmsong.cpp \
    bmsourcedir.cpp \
    bmsettings.cpp \
    bmabstractaudiobackend.cpp \
    bmaudiobackendgstreamer.cpp

HEADERS  += mainwindow.h \
    songdbtablemodel.h \
    playlisttablemodel.h \
    khipc.h \
    sourcedirtablemodel.h \
    databasedialog.h \
    databaseupdatethread.h \
    fader.h \
    bmipcserver.h \
    bmplaylist.h \
    bmsong.h \
    bmsourcedir.h \
    bmsettings.h \
    bmabstractaudiobackend.h \
    bmaudiobackendgstreamer.h

FORMS    += mainwindow.ui \
    databasedialog.ui

RESOURCES += \
    resources.qrc

unix: QT_CONFIG -= no-pkg-config
unix: CONFIG += link_pkgconfig

unix: PKGCONFIG += taglib_c
unix: PKGCONFIG += gstreamer-0.10

win32: INCLUDEPATH += "C:\taglib\include\taglib"
win32: LIBS += -L"C:\taglib\taglib\taglib\Release" -ltag

win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\include\gstreamer-0.10"
win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\include\glib-2.0"
win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\lib\glib-2.0\include"
win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\include\libxml2"
win32: LIBS+= -L"C:\gstreamer-sdk\0.10\x86\lib" -lgstreamer-0.10 -lglib-2.0 -lgobject-2.0

unix {
    iconfiles.files += icons/bmicon64x64.png
    iconfiles.path = /usr/share/pixmaps
    desktopfiles.files += breakmusic.desktop
    desktopfiles.path = /usr/share/applications
    binaryfiles.files += BreakMusic
    binaryfiles.path = /usr/bin
    INSTALLS += binaryfiles iconfiles desktopfiles
}

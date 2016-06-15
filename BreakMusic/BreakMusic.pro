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
    databasedialog.cpp \
    bmipcserver.cpp \
    bmsettings.cpp \
    bmabstractaudiobackend.cpp \
    pltablemodel.cpp \
    plitemdelegate.cpp \
    dbtablemodel.cpp \
    dbupdatethread.cpp \
    dbitemdelegate.cpp \
    audiobackendqtmultimedia.cpp \
    smbPitchShift/smbPitchShift.fftw3.cpp \
    audioprocproxyiodevice.cpp

HEADERS  += mainwindow.h \
    databasedialog.h \
    bmipcserver.h \
    bmsettings.h \
    bmabstractaudiobackend.h \
    pltablemodel.h \
    plitemdelegate.h \
    dbtablemodel.h \
    dbupdatethread.h \
    dbitemdelegate.h \
    audiobackendqtmultimedia.h \
    smbPitchShift/smbPitchShift.fftw3.h \
    audioprocproxyiodevice.h

FORMS    += mainwindow.ui \
    databasedialog.ui

RESOURCES += \
    resources.qrc

unix: QT_CONFIG -= no-pkg-config
unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += taglib_c
unix: PKGCONFIG += fftw3

win32: INCLUDEPATH += "K:\k\include\taglib"
win32: LIBS += -ltag

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

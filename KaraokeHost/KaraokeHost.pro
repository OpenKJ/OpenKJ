#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT       += core gui sql network opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: CONFIG += console

TARGET = KaraokeHost
TEMPLATE = app

#DEFINES += USE_FMOD
DEFINES += USE_GSTREAMER
# On Linux platforms QMediaPlayer uses gstreamer as its base.  You can not
# load both backends due to conflicts.
#DEFINES += USE_QMEDIAPLAYER

SOURCES += main.cpp\
        mainwindow.cpp \
    queuetablemodel.cpp \
    rotationtablemodel.cpp \
    songdbtablemodel.cpp \
    libCDG/src/libCDG_Frame_Image.cpp \
    libCDG/src/libCDG_Color.cpp \
    libCDG/src/CDG_Frame_Image.cpp \
    libCDG/src/libCDG.cpp \
    databasedialog.cpp \
    sourcedirtablemodel.cpp \
    dbupdatethread.cpp \
    settingsdialog.cpp \
    songdbloadthread.cpp \
    khqueuesong.cpp \
    khsinger.cpp \
    khsong.cpp \
    khregularsinger.cpp \
    khregularsong.cpp \
    khipcclient.cpp \
    khabstractaudiobackend.cpp \
    khzip.cpp \
    qglcanvas.cpp \
    cdgwindow.cpp \
    khsettings.cpp \
    regularsingersdialog.cpp \
    regularsingermodel.cpp \
    scrolltext.cpp \
    regularexportdialog.cpp \
    regularimportdialog.cpp \
    khrequestsdialog.cpp \
    requeststablemodel.cpp \
    cdgpreviewdialog.cpp \
    khdb.cpp \
    dlgkeychange.cpp

HEADERS  += mainwindow.h \
    queuetablemodel.h \
    rotationtablemodel.h \
    songdbtablemodel.h \
    libCDG/include/libCDG.h \
    libCDG/include/libCDG_Frame_Image.h \
    libCDG/include/libCDG_Color.h \
    libCDG/include/CDG_Frame_Image.h \
    databasedialog.h \
    sourcedirtablemodel.h \
    dbupdatethread.h \
    settingsdialog.h \
    songdbloadthread.h \
    khqueuesong.h \
    khsinger.h \
    khsong.h \
    khregularsinger.h \
    khregularsong.h \
    khipcclient.h \
    khabstractaudiobackend.h \
    khzip.h \
    qglcanvas.h \
    cdgwindow.h \
    khsettings.h \
    regularsingersdialog.h \
    regularsingermodel.h \
    scrolltext.h \
    regularexportdialog.h \
    regularimportdialog.h \
    khrequestsdialog.h \
    requeststablemodel.h \
    cdgpreviewdialog.h \
    khdb.h \
    dlgkeychange.h

FORMS    += mainwindow.ui \
    databasedialog.ui \
    settingsdialog.ui \
    cdgwindow.ui \
    regularsingersdialog.ui \
    regularexportdialog.ui \
    regularimportdialog.ui \
    khrequestsdialog.ui \
    cdgpreviewdialog.ui \
    dlgkeychange.ui

unix: CONFIG += link_pkgconfig
#unix: PKGCONFIG += phonon4qt5

win32: INCLUDEPATH += "/usr/i686-w64-mingw32/sys-root/mingw/include/gstreamer-1.0/"
win32: INCLUDEPATH += "/usr/i686-w64-mingw32/sys-root/mingw/include/glib-2.0/"
win32: INCLUDEPATH += "/usr/i686-w64-mingw32/sys-root/mingw/lib/glib-2.0/include/"
win32: LIBS+= -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0
unix: LIBS += -ltag -lminizip
win32: LIBS += -lminizip -ltag.dll
# win32: LIBS += -lminizip

contains(DEFINES, USE_GSTREAMER) {
    message("USE_GSTREAMER defined, building GStreamer audio backend")
    PKGCONFIG += gstreamer-1.0
    HEADERS += khaudiobackendgstreamer.h
    SOURCES += khaudiobackendgstreamer.cpp
}

contains(DEFINES, USE_QMEDIAPLAYER) {
        QT += multimedia
        HEADERS += khaudiobackendqmediaplayer.h
        SOURCES += khaudiobackendqmediaplayer.cpp
}

contains(DEFINES, USE_FMOD) {
        # If building on win32/64 you'll need to fix the paths
        message("USE_FMOD defined, building with Fmod audio backend (http://www.fmod.org) support")
	message("Please note that, while free for non-commercial use, FMOD is NOT open source")
        win32: INCLUDEPATH += "/home/isaac/.wine/drive_c/Program Files (x86)/FMOD SoundSystem/FMOD Programmers API Windows/api/inc"
	HEADERS += khaudiobackendfmod.h
	SOURCES += khaudiobackendfmod.cpp
        win32: LIBS += -L"/home/isaac/.wine/drive_c/Program Files (x86)/FMOD SoundSystem/FMOD Programmers API Windows/api/lib" -lfmodex
	unix {
		contains(QMAKE_HOST.arch, x86_64) {
			message("64bit UNIX/Linux platform detected, linking fmodex64")
			LIBS += -lfmodex64
		} else {
			message("UNIX/Linux platform does not appear to be 64bit, linking fmodex")
			LIBS += -lfmodex
		}
	}
}

RESOURCES += \
    resources.qrc

INCLUDEPATH += $$PWD/../Cdg2
DEPENDPATH += $$PWD/../Cdg2

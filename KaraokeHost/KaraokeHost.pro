#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT       += core gui sql network opengl multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KaraokeHost
TEMPLATE = app


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
    khrotation.cpp \
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
    regularsingermodel.cpp

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
    khrotation.h \
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
    regularsingermodel.h

FORMS    += mainwindow.ui \
    databasedialog.ui \
    settingsdialog.ui \
    cdgwindow.ui \
    regularsingersdialog.ui

unix: CONFIG += link_pkgconfig
#unix: PKGCONFIG += phonon4qt5

    LIBS += -ltag -lminizip

contains(DEFINES, USE_FMOD) {
	message("USE_FMOD defined, building with FMOD API (http://www.fmod.org) support")
	message("Please note that, while free for non-commercial use, FMOD is NOT open source")
        win32: INCLUDEPATH += "/home/isaac/devel/QT/KSP/fmod-win32/fmod/inc/"
	HEADERS += khaudiobackendfmod.h
	SOURCES += khaudiobackendfmod.cpp

        win32: LIBS += -L"/home/isaac/devel/QT/KSP/fmod-win32/fmod/lib/" -lfmodex
	unix {
		contains(QMAKE_HOST.arch, x86_64) {
			message("64bit UNIX/Linux platform detected, linking fmodex64")
			LIBS += -lfmodex64
		} else {
			message("UNIX/Linux platform does not appear to be 64bit, linking fmodex")
			LIBS += -lfmodex
		}
	}
} else {
        QT += multimedia
        HEADERS += khaudiobackendqmediaplayer.h
        SOURCES += khaudiobackendqmediaplayer.cpp
}

RESOURCES += \
    resources.qrc

INCLUDEPATH += $$PWD/../Cdg2
DEPENDPATH += $$PWD/../Cdg2

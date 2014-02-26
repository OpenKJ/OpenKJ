#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT       += core gui sql widgets network multimedia opengl

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
    khaudiobackendqmediaplayer.cpp \
    khzip.cpp \
    qglcanvas.cpp \
    cdgwindow.cpp \
    khsettings.cpp

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
    khaudiobackendqmediaplayer.h \
    khzip.h \
    qglcanvas.h \
    cdgwindow.h \
    khsettings.h

FORMS    += mainwindow.ui \
    databasedialog.ui \
    settingsdialog.ui \
    cdgwindow.ui

unix: CONFIG += link_pkgconfig
#unix: PKGCONFIG += phonon4qt5

unix: LIBS += -ltag -L$$_PRO_FILE_PWD_/fmod-linux/fmod/lib -lfmodex64 -lminizip

win32: LIBS += -L"$$_PRO_FILE_PWD_/taglib-win32/lib" -ltag
win32: INCLUDEPATH += ./taglib-win32/include
win32: INCLUDEPATH += "C:\Users\nunya\Downloads\boost_1_54_0\boost_1_54_0"

contains(DEFINES, USE_FMOD) {
	message("USE_FMOD defined, building with FMOD API (http://www.fmod.org) support")
	message("Please note that, while free for non-commercial use, FMOD is NOT open source")
	HEADERS += khaudiobackendfmod.h
	SOURCES += khaudiobackendfmod.cpp

	win32: LIBS += -lfmodex
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

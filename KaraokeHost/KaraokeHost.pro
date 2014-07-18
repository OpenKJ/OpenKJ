#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT += core gui sql network widgets multimedia

unix: DEFINES += USE_GL
#win32: DEFINES += USE_GL

win32: RC_FILE = KaraokeHost.rc

contains(DEFINES, USE_GL) {
    QT += opengl
}

TARGET = KaraokeHost
TEMPLATE = app

DEFINES += USE_GSTREAMER

SOURCES += main.cpp\
    mainwindow.cpp \
    queuetablemodel.cpp \
    rotationtablemodel.cpp \
    songdbtablemodel.cpp \
    libCDG/src/libCDG_Frame_Image.cpp \
    libCDG/src/libCDG_Color.cpp \
    libCDG/src/CDG_Frame_Image.cpp \
    libCDG/src/libCDG.cpp \
    sourcedirtablemodel.cpp \
    dbupdatethread.cpp \
    songdbloadthread.cpp \
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
    khsettings.cpp \
    regularsingermodel.cpp \
    scrolltext.cpp \
    requeststablemodel.cpp \
    khdb.cpp \
    dlgkeychange.cpp \
    dlgcdgpreview.cpp \
    dlgdatabase.cpp \
    dlgrequests.cpp \
    dlgregularexport.cpp \
    dlgregularimport.cpp \
    dlgregularsingers.cpp \
    dlgsettings.cpp \
    dlgcdg.cpp \
    dlgdurationscan.cpp \
    khaudiorecorder.cpp \
    dbtablemodel.cpp \
    queuemodel.cpp \
    rotationmodel.cpp \
    rotationitemdelegate.cpp \
    dbitemdelegate.cpp

HEADERS  += mainwindow.h \
    queuetablemodel.h \
    rotationtablemodel.h \
    songdbtablemodel.h \
    libCDG/include/libCDG.h \
    libCDG/include/libCDG_Frame_Image.h \
    libCDG/include/libCDG_Color.h \
    libCDG/include/CDG_Frame_Image.h \
    sourcedirtablemodel.h \
    dbupdatethread.h \
    songdbloadthread.h \
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
    khsettings.h \
    regularsingermodel.h \
    scrolltext.h \
    requeststablemodel.h \
    khdb.h \
    dlgkeychange.h \
    dlgcdgpreview.h \
    dlgdatabase.h \
    dlgrequests.h \
    dlgregularexport.h \
    dlgregularimport.h \
    dlgregularsingers.h \
    dlgsettings.h \
    dlgcdg.h \
    dlgdurationscan.h \
    khaudiorecorder.h \
    dbtablemodel.h \
    queuemodel.h \
    rotationmodel.h \
    rotationitemdelegate.h \
    dbitemdelegate.h

FORMS    += mainwindow.ui \
    dlgkeychange.ui \
    dlgcdgpreview.ui \
    dlgdatabase.ui \
    dlgrequests.ui \
    dlgregularexport.ui \
    dlgregularimport.ui \
    dlgregularsingers.ui \
    dlgsettings.ui \
    dlgcdg.ui \
    dlgdurationscan.ui

unix: CONFIG += link_pkgconfig

contains(DEFINES, USE_GSTREAMER) {
    message("USE_GSTREAMER defined, building GStreamer audio backend")
    unix: PKGCONFIG += gstreamer-0.10
    win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\include\gstreamer-0.10"
    win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\include\glib-2.0"
    win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\lib\glib-2.0\include"
    win32: INCLUDEPATH += "C:\gstreamer-sdk\0.10\x86\include\libxml2"
    win32: LIBS+= -L"C:\gstreamer-sdk\0.10\x86\lib" -lgstreamer-0.10 -lglib-2.0 -lgobject-2.0
    macx: INCLUDEPATH += "/Library/Frameworks/GStreamer.framework/Headers"
    macx: LIBS += -L"/Library/Frameworks/GStreamer.framework/Libraries" -lgstreamer-0.10 -lglib-2.0 -lgobject-2.0
    HEADERS += khaudiobackendgstreamer.h
    SOURCES += khaudiobackendgstreamer.cpp
}

RESOURCES += \
    resources.qrc

unix {
    iconfiles.files += Icons/khicon-64x64.png
    iconfiles.path = /usr/share/pixmaps
    desktopfiles.files += karaokehost.desktop
    desktopfiles.path = /usr/share/applications
    binaryfiles.files += KaraokeHost
    binaryfiles.path = /usr/bin
    INSTALLS += binaryfiles iconfiles desktopfiles
}

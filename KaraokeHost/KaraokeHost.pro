#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT += core gui sql network widgets multimedia KArchive ThreadWeaver

#CONFIG += console

unix: DEFINES += USE_GL
#win32: DEFINES += USE_GL

win32: RC_FILE = KaraokeHost.rc

contains(DEFINES, USE_GL) {
    QT += opengl
}

TARGET = KaraokeHost
TEMPLATE = app

#DEFINES += USE_GSTREAMER
#DEFINES += USE_QMEDIAPLAYER
DEFINES += USE_QTMULTIMEDIA

SOURCES += main.cpp\
    mainwindow.cpp \
    libCDG/src/libCDG_Frame_Image.cpp \
    libCDG/src/libCDG_Color.cpp \
    libCDG/src/libCDG.cpp \
    sourcedirtablemodel.cpp \
    dbupdatethread.cpp \
    khipcclient.cpp \
    khabstractaudiobackend.cpp \
    khaudiobackendqmediaplayer.cpp \
    khsettings.cpp \
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
    dbitemdelegate.cpp \
    queueitemdelegate.cpp \
    regitemdelegate.cpp \
    okarchive.cpp \
    cdgvideosurface.cpp \
    cdgvideowidget.cpp \
    imagewidget.cpp \
    khaudiobackendqtmultimedia.cpp

HEADERS  += mainwindow.h \
    libCDG/include/libCDG.h \
    libCDG/include/libCDG_Frame_Image.h \
    libCDG/include/libCDG_Color.h \
    sourcedirtablemodel.h \
    dbupdatethread.h \
    khipcclient.h \
    khabstractaudiobackend.h \
    khaudiobackendqmediaplayer.h \
    khsettings.h \
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
    dbitemdelegate.h \
    queueitemdelegate.h \
    regitemdelegate.h \
    okarchive.h \
    cdgvideosurface.h \
    cdgvideowidget.h \
    imagewidget.h \
    khaudiobackendqtmultimedia.h

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

#win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../miniz/release/ -lminiz
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../miniz/debug/ -lminiz
#else:unix: LIBS += -L$$OUT_PWD/../miniz/ -lminiz

#INCLUDEPATH += $$PWD/../miniz
#DEPENDPATH += $$PWD/../miniz

#win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../miniz/release/libminiz.a
#else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../miniz/debug/libminiz.a
#else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../miniz/release/miniz.lib
#else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../miniz/debug/miniz.lib
#else:unix: PRE_TARGETDEPS += $$OUT_PWD/../miniz/libminiz.a

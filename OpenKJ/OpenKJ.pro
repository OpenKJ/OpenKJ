#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT += core gui sql network widgets multimedia concurrent svg

unix: DEFINES += USE_GL

win32: RC_ICONS = Icons/okjicon.ico

contains(DEFINES, USE_GL) {
    QT += opengl
}

TARGET = OpenKJ 
TEMPLATE = app

SOURCES += main.cpp\
    mainwindow.cpp \
    libCDG/src/libCDG_Frame_Image.cpp \
    libCDG/src/libCDG_Color.cpp \
    libCDG/src/libCDG.cpp \
    sourcedirtablemodel.cpp \
    dbupdatethread.cpp \
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
    abstractaudiobackend.cpp \
    audiobackendgstreamer.cpp \
    tagreader.cpp \
    bmdbitemdelegate.cpp \
    bmdbtablemodel.cpp \
    bmdbupdatethread.cpp \
    bmpltablemodel.cpp \
    bmplitemdelegate.cpp \
    settings.cpp \
    bmdbdialog.cpp \
    miniz.c

HEADERS  += mainwindow.h \
    libCDG/include/libCDG.h \
    libCDG/include/libCDG_Frame_Image.h \
    libCDG/include/libCDG_Color.h \
    sourcedirtablemodel.h \
    dbupdatethread.h \
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
    abstractaudiobackend.h \
    audiobackendgstreamer.h \
    tagreader.h \
    bmdbitemdelegate.h \
    bmdbtablemodel.h \
    bmdbupdatethread.h \
    bmplitemdelegate.h \
    bmpltablemodel.h \
    settings.h \
    bmdbdialog.h

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
    bmdbdialog.ui

RESOURCES += resources.qrc

unix:!macx {
    isEmpty(DESTDIR) {
      DESTDIR = /usr
    }
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0
    iconfiles.files += Icons/okjicon.svg
    iconfiles.path = $$DESTDIR/share/pixmaps
    desktopfiles.files += openkj.desktop
    desktopfiles.path = $$DESTDIR/share/applications
    binaryfiles.files += OpenKJ
    binaryfiles.path = $$DESTDIR/bin
    INSTALLS += binaryfiles iconfiles desktopfiles
}

macx: {
    LIBS += -F/Library/Frameworks -framework GStreamer
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    ICON = Icons/OpenKJ.icns
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

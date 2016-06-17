#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT += core gui sql network widgets multimedia KArchive ThreadWeaver

unix: DEFINES += USE_GL
#win32: DEFINES += USE_GL

win32: RC_FILE = KaraokeHost.rc
#win32: LIBS += -L"K:/fftw" -lfftw3-3


contains(DEFINES, USE_GL) {
    QT += opengl
}

TARGET = KaraokeHost
TEMPLATE = app

DEFINES += USE_QTMULTIMEDIA

win32: INCLUDEPATH += "K:/fftw"

SOURCES += main.cpp\
    mainwindow.cpp \
    libCDG/src/libCDG_Frame_Image.cpp \
    libCDG/src/libCDG_Color.cpp \
    libCDG/src/libCDG.cpp \
    sourcedirtablemodel.cpp \
    dbupdatethread.cpp \
    khipcclient.cpp \
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
    audioprocproxyiodevice.cpp \
    smbPitchShift/smbPitchShift.fftw3.cpp \
    abstractaudiobackend.cpp \
    audiobackendqtmultimedia.cpp

HEADERS  += mainwindow.h \
    libCDG/include/libCDG.h \
    libCDG/include/libCDG_Frame_Image.h \
    libCDG/include/libCDG_Color.h \
    sourcedirtablemodel.h \
    dbupdatethread.h \
    khipcclient.h \
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
    audioprocproxyiodevice.h \
    smbPitchShift/smbPitchShift.fftw3.h \
    audiobackendqtmultimedia.h \
    abstractaudiobackend.h

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
unix: PKGCONFIG += fftw3


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


win32: LIBS += -L"K:/fftw-3.3.4/fftw-3.3-libs/Release/" -llibfftw-3.3

win32: INCLUDEPATH += "K:/fftw-3.3.4/api"
win32: DEPENDPATH += "K:/fftw-3.3.4/api"

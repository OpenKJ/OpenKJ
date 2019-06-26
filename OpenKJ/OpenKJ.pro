#-------------------------------------------------
#
# Project created by QtCreator 2013-01-07T17:48:44
#
#-------------------------------------------------

QT += core gui sql network widgets multimedia concurrent svg printsupport

unix:DISTVER = $$system(cat /etc/os-release |grep VERSION_ID |cut -d'=' -f2 | sed -e \'s/^\"//\' -e \'s/\"$//\')
message($$DISTVER)

unix:!macx {
    isEmpty(PREFIX) {
      PREFIX=/usr
    }
#    equals(DISTVER, "16.04")|equals(DISTVER, "7") {
#        DEFINES += STATIC_TAGLIB
#        message("Out of date Linux distro detected, using built in taglib instead of OS package")
#    } else {
        message("Using OS packages for taglib")
        PKGCONFIG += taglib taglib-extras
#    }
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-audio-1.0 gstreamer-pbutils-1.0 gstreamer-controller-1.0
    iconfiles.files += Icons/okjicon.svg
    iconfiles.path = $$PREFIX/share/pixmaps
    desktopfiles.files += openkj.desktop
    desktopfiles.path = $$PREFIX/share/applications
    binaryfiles.files += OpenKJ
    binaryfiles.path = $$PREFIX/bin
    INSTALLS += binaryfiles iconfiles desktopfiles
    DEFINES += USE_GL
}

macx: {
    LIBS += -F/Library/Frameworks -framework GStreamer
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    ICON = Icons/OpenKJ.icns
    DEFINES += STATIC_TAGLIB
}

win32 {
    ## Windows common build here
    DEFINES += STATIC_TAGLIB
    RC_ICONS = Icons/okjicon.ico
    !contains(QMAKE_TARGET.arch, x86_64) {
        ## Windows x86 (32bit) specific build here
        INCLUDEPATH += C:\gstreamer\1.0\x86\include\gstreamer-1.0
        INCLUDEPATH += C:\gstreamer\1.0\x86\include\glib-2.0
        INCLUDEPATH += C:\gstreamer\1.0\x86\lib\glib-2.0\include
        INCLUDEPATH += C:\gstreamer\1.0\x86\include\glib-2.0\gobject
        LIBS += -LC:\gstreamer\1.0\x86\lib -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0 -lgstapp-1.0 -lgstaudio-1.0 -lgstpbutils-1.0 -lgstcontroller-1.0
    } else {
        ## Windows x64 (64bit) specific build here
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\include\gstreamer-1.0
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\include\glib-2.0
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\lib\glib-2.0\include
        INCLUDEPATH += C:\gstreamer\1.0\x86_64\include\glib-2.0\gobject
        LIBS += -LC:\gstreamer\1.0\x86_64\lib -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0 -lgstapp-1.0 -lgstaudio-1.0 -lgstpbutils-1.0 -lgstcontroller-1.0
    }
}

contains(DEFINES, USE_GL) {
    QT += opengl
}

TARGET = OpenKJ 
TEMPLATE = app

contains(DEFINES, STATIC_TAGLIB) {
    DEFINES += TAGLIB_STATIC
    INCLUDEPATH += taglib
    INCLUDEPATH += taglib/toolkit
    INCLUDEPATH += taglib/mpeg/id3v2
    INCLUDEPATH += taglib/mpeg/id3v2/frames
    INCLUDEPATH += taglib/mpeg/id3v1
    INCLUDEPATH += taglib/mod
    INCLUDEPATH += taglib/ogg
    INCLUDEPATH += taglib/ogg/flac
    INCLUDEPATH += taglib/ogg/opus
    INCLUDEPATH += taglib/ogg/speex
    INCLUDEPATH += taglib/ogg/vorbis
    INCLUDEPATH += taglib/flac
    INCLUDEPATH += taglib/ape
    INCLUDEPATH += taglib/riff
    INCLUDEPATH += taglib/asf
    INCLUDEPATH += taglib/mpeg
    INCLUDEPATH += taglib/riff
    INCLUDEPATH += taglib/riff/aiff
    INCLUDEPATH += taglib/riff/wav
    INCLUDEPATH += taglib/it
    INCLUDEPATH += taglib/mp4
    INCLUDEPATH += taglib/mpc
    INCLUDEPATH += taglib/s3m
    INCLUDEPATH += taglib/trueaudio
    INCLUDEPATH += taglib/wavpack
    INCLUDEPATH += taglib/xm
}
# fix macOS build after upgrading xcode
QMAKE_MAC_SDK = MacOSX10.13
QMAKE_MAC_SDK.macosx.version = 10.13

VERSION = 1.5.62
message($$VERSION)
QMAKE_TARGET_COMPANY = OpenKJ.org
QMAKE_TARGET_PRODUCT = OpenKJ
QMAKE_TARGET_DESCRIPTION = OpenKJ karaoke hosting software
DEFINES += OKJ_UNSTABLE


unix: BLDDATE = $$system(date -R)
win32: BLDDATE = $$system(date /t)
DEFINES += BUILD_DATE=__DATE__

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
    dlgcustompatterns.cpp \
    custompatternsmodel.cpp \
    audiorecorder.cpp \
    okjsongbookapi.cpp \
    dlgdbupdate.cpp \
    dlgbookcreator.cpp \
    dlgeq.cpp \
    audiofader.cpp \
    customlineedit.cpp \
    updatechecker.cpp \
    volslider.cpp \
    dlgaddsinger.cpp \
    ticker.cpp \
    songshop.cpp \
    dlgsongshop.cpp \
    songshopmodel.cpp \
    shopsortfilterproxymodel.cpp \
    simplecrypt.cpp \
    dlgsongshoppurchase.cpp \
    dlgsetpassword.cpp \
    dlgpassword.cpp \
    dlgpurchaseprogress.cpp \
    karaokefileinfo.cpp \
    dlgeditsong.cpp \
    soundfxbutton.cpp \
    durationlazyupdater.cpp \
    idledetect.cpp \
    dlgdebugoutput.cpp

contains(DEFINES, STATIC_TAGLIB) {
    SOURCES += taglib/ape/apefile.cpp \
    taglib/ape/apefooter.cpp \
    taglib/ape/apeitem.cpp \
    taglib/ape/apeproperties.cpp \
    taglib/ape/apetag.cpp \
    taglib/asf/asfattribute.cpp \
    taglib/asf/asffile.cpp \
    taglib/asf/asfpicture.cpp \
    taglib/asf/asfproperties.cpp \
    taglib/asf/asftag.cpp \
    taglib/flac/flacfile.cpp \
    taglib/flac/flacmetadatablock.cpp \
    taglib/flac/flacpicture.cpp \
    taglib/flac/flacproperties.cpp \
    taglib/flac/flacunknownmetadatablock.cpp \
    taglib/it/itfile.cpp \
    taglib/it/itproperties.cpp \
    taglib/mod/modfile.cpp \
    taglib/mod/modfilebase.cpp \
    taglib/mod/modproperties.cpp \
    taglib/mod/modtag.cpp \
    taglib/mp4/mp4atom.cpp \
    taglib/mp4/mp4coverart.cpp \
    taglib/mp4/mp4file.cpp \
    taglib/mp4/mp4item.cpp \
    taglib/mp4/mp4properties.cpp \
    taglib/mp4/mp4tag.cpp \
    taglib/mpc/mpcfile.cpp \
    taglib/mpc/mpcproperties.cpp \
    taglib/mpeg/id3v1/id3v1genres.cpp \
    taglib/mpeg/id3v1/id3v1tag.cpp \
    taglib/mpeg/id3v2/frames/attachedpictureframe.cpp \
    taglib/mpeg/id3v2/frames/chapterframe.cpp \
    taglib/mpeg/id3v2/frames/commentsframe.cpp \
    taglib/mpeg/id3v2/frames/eventtimingcodesframe.cpp \
    taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.cpp \
    taglib/mpeg/id3v2/frames/ownershipframe.cpp \
    taglib/mpeg/id3v2/frames/podcastframe.cpp \
    taglib/mpeg/id3v2/frames/popularimeterframe.cpp \
    taglib/mpeg/id3v2/frames/privateframe.cpp \
    taglib/mpeg/id3v2/frames/relativevolumeframe.cpp \
    taglib/mpeg/id3v2/frames/synchronizedlyricsframe.cpp \
    taglib/mpeg/id3v2/frames/tableofcontentsframe.cpp \
    taglib/mpeg/id3v2/frames/textidentificationframe.cpp \
    taglib/mpeg/id3v2/frames/uniquefileidentifierframe.cpp \
    taglib/mpeg/id3v2/frames/unknownframe.cpp \
    taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.cpp \
    taglib/mpeg/id3v2/frames/urllinkframe.cpp \
    taglib/mpeg/id3v2/id3v2extendedheader.cpp \
    taglib/mpeg/id3v2/id3v2footer.cpp \
    taglib/mpeg/id3v2/id3v2frame.cpp \
    taglib/mpeg/id3v2/id3v2framefactory.cpp \
    taglib/mpeg/id3v2/id3v2header.cpp \
    taglib/mpeg/id3v2/id3v2synchdata.cpp \
    taglib/mpeg/id3v2/id3v2tag.cpp \
    taglib/mpeg/mpegfile.cpp \
    taglib/mpeg/mpegheader.cpp \
    taglib/mpeg/mpegproperties.cpp \
    taglib/mpeg/xingheader.cpp \
    taglib/ogg/flac/oggflacfile.cpp \
    taglib/ogg/opus/opusfile.cpp \
    taglib/ogg/opus/opusproperties.cpp \
    taglib/ogg/speex/speexfile.cpp \
    taglib/ogg/speex/speexproperties.cpp \
    taglib/ogg/vorbis/vorbisfile.cpp \
    taglib/ogg/vorbis/vorbisproperties.cpp \
    taglib/ogg/oggfile.cpp \
    taglib/ogg/oggpage.cpp \
    taglib/ogg/oggpageheader.cpp \
    taglib/ogg/xiphcomment.cpp \
    taglib/riff/aiff/aifffile.cpp \
    taglib/riff/aiff/aiffproperties.cpp \
    taglib/riff/wav/infotag.cpp \
    taglib/riff/wav/wavfile.cpp \
    taglib/riff/wav/wavproperties.cpp \
    taglib/riff/rifffile.cpp \
    taglib/s3m/s3mfile.cpp \
    taglib/s3m/s3mproperties.cpp \
    taglib/toolkit/tbytevector.cpp \
    taglib/toolkit/tbytevectorlist.cpp \
    taglib/toolkit/tbytevectorstream.cpp \
    taglib/toolkit/tdebug.cpp \
    taglib/toolkit/tdebuglistener.cpp \
    taglib/toolkit/tfile.cpp \
    taglib/toolkit/tfilestream.cpp \
    taglib/toolkit/tiostream.cpp \
    taglib/toolkit/tpropertymap.cpp \
    taglib/toolkit/trefcounter.cpp \
    taglib/toolkit/tstring.cpp \
    taglib/toolkit/tstringlist.cpp \
    taglib/toolkit/tzlib.cpp \
    taglib/toolkit/unicode.cpp \
    taglib/trueaudio/trueaudiofile.cpp \
    taglib/trueaudio/trueaudioproperties.cpp \
    taglib/wavpack/wavpackfile.cpp \
    taglib/wavpack/wavpackproperties.cpp \
    taglib/xm/xmfile.cpp \
    taglib/xm/xmproperties.cpp \
    taglib/audioproperties.cpp \
    taglib/fileref.cpp \
    taglib/tag.cpp \
    taglib/tagunion.cpp \
    taglib/tagutils.cpp
}

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
    bmdbdialog.h \
    dlgcustompatterns.h \
    custompatternsmodel.h \
    audiorecorder.h \
    okjsongbookapi.h \
    dlgdbupdate.h \
    dlgbookcreator.h \
    dlgeq.h \
    audiofader.h \
    customlineedit.h \
    updatechecker.h \
    volslider.h \
    okjversion.h \
    dlgaddsinger.h \
    ticker.h \
    songshop.h \
    dlgsongshop.h \
    songshopmodel.h \
    shopsortfilterproxymodel.h \
    simplecrypt.h \
    dlgsongshoppurchase.h \
    dlgsetpassword.h \
    dlgpassword.h \
    dlgpurchaseprogress.h \
    karaokefileinfo.h \
    dlgeditsong.h \
    soundfxbutton.h \
    tableviewtooltipfilter.h \
    durationlazyupdater.h \
    idledetect.h \
    dlgdebugoutput.h

contains(DEFINES, STATIC_TAGLIB) {
    HEADERS += taglib/ape/apefile.h \
    taglib/ape/apefooter.h \
    taglib/ape/apeitem.h \
    taglib/ape/apeproperties.h \
    taglib/ape/apetag.h \
    taglib/asf/asfattribute.h \
    taglib/asf/asffile.h \
    taglib/asf/asfpicture.h \
    taglib/asf/asfproperties.h \
    taglib/asf/asftag.h \
    taglib/asf/asfutils.h \
    taglib/flac/flacfile.h \
    taglib/flac/flacmetadatablock.h \
    taglib/flac/flacpicture.h \
    taglib/flac/flacproperties.h \
    taglib/flac/flacunknownmetadatablock.h \
    taglib/it/itfile.h \
    taglib/it/itproperties.h \
    taglib/mod/modfile.h \
    taglib/mod/modfilebase.h \
    taglib/mod/modfileprivate.h \
    taglib/mod/modproperties.h \
    taglib/mod/modtag.h \
    taglib/mp4/mp4atom.h \
    taglib/mp4/mp4coverart.h \
    taglib/mp4/mp4file.h \
    taglib/mp4/mp4item.h \
    taglib/mp4/mp4properties.h \
    taglib/mp4/mp4tag.h \
    taglib/mpc/mpcfile.h \
    taglib/mpc/mpcproperties.h \
    taglib/mpeg/id3v1/id3v1genres.h \
    taglib/mpeg/id3v1/id3v1tag.h \
    taglib/mpeg/id3v2/frames/attachedpictureframe.h \
    taglib/mpeg/id3v2/frames/chapterframe.h \
    taglib/mpeg/id3v2/frames/commentsframe.h \
    taglib/mpeg/id3v2/frames/eventtimingcodesframe.h \
    taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.h \
    taglib/mpeg/id3v2/frames/ownershipframe.h \
    taglib/mpeg/id3v2/frames/podcastframe.h \
    taglib/mpeg/id3v2/frames/popularimeterframe.h \
    taglib/mpeg/id3v2/frames/privateframe.h \
    taglib/mpeg/id3v2/frames/relativevolumeframe.h \
    taglib/mpeg/id3v2/frames/synchronizedlyricsframe.h \
    taglib/mpeg/id3v2/frames/tableofcontentsframe.h \
    taglib/mpeg/id3v2/frames/textidentificationframe.h \
    taglib/mpeg/id3v2/frames/uniquefileidentifierframe.h \
    taglib/mpeg/id3v2/frames/unknownframe.h \
    taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h \
    taglib/mpeg/id3v2/frames/urllinkframe.h \
    taglib/mpeg/id3v2/id3v2extendedheader.h \
    taglib/mpeg/id3v2/id3v2footer.h \
    taglib/mpeg/id3v2/id3v2frame.h \
    taglib/mpeg/id3v2/id3v2framefactory.h \
    taglib/mpeg/id3v2/id3v2header.h \
    taglib/mpeg/id3v2/id3v2synchdata.h \
    taglib/mpeg/id3v2/id3v2tag.h \
    taglib/mpeg/mpegfile.h \
    taglib/mpeg/mpegheader.h \
    taglib/mpeg/mpegproperties.h \
    taglib/mpeg/mpegutils.h \
    taglib/mpeg/xingheader.h \
    taglib/ogg/flac/oggflacfile.h \
    taglib/ogg/opus/opusfile.h \
    taglib/ogg/opus/opusproperties.h \
    taglib/ogg/speex/speexfile.h \
    taglib/ogg/speex/speexproperties.h \
    taglib/ogg/vorbis/vorbisfile.h \
    taglib/ogg/vorbis/vorbisproperties.h \
    taglib/ogg/oggfile.h \
    taglib/ogg/oggpage.h \
    taglib/ogg/oggpageheader.h \
    taglib/ogg/xiphcomment.h \
    taglib/riff/aiff/aifffile.h \
    taglib/riff/aiff/aiffproperties.h \
    taglib/riff/wav/infotag.h \
    taglib/riff/wav/wavfile.h \
    taglib/riff/wav/wavproperties.h \
    taglib/riff/rifffile.h \
    taglib/riff/riffutils.h \
    taglib/s3m/s3mfile.h \
    taglib/s3m/s3mproperties.h \
    taglib/toolkit/taglib.h \
    taglib/toolkit/tbytevector.h \
    taglib/toolkit/tbytevectorlist.h \
    taglib/toolkit/tbytevectorstream.h \
    taglib/toolkit/tdebug.h \
    taglib/toolkit/tdebuglistener.h \
    taglib/toolkit/tfile.h \
    taglib/toolkit/tfilestream.h \
    taglib/toolkit/tiostream.h \
    taglib/toolkit/tlist.h \
    taglib/toolkit/tmap.h \
    taglib/toolkit/tpropertymap.h \
    taglib/toolkit/trefcounter.h \
    taglib/toolkit/tstring.h \
    taglib/toolkit/tstringlist.h \
    taglib/toolkit/tutils.h \
    taglib/toolkit/tzlib.h \
    taglib/toolkit/unicode.h \
    taglib/trueaudio/trueaudiofile.h \
    taglib/trueaudio/trueaudioproperties.h \
    taglib/wavpack/wavpackfile.h \
    taglib/wavpack/wavpackproperties.h \
    taglib/xm/xmfile.h \
    taglib/xm/xmproperties.h \
    taglib/audioproperties.h \
    taglib/fileref.h \
    taglib/tag.h \
    taglib/taglib_export.h \
    taglib/tagunion.h \
    taglib/tagutils.h
}

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
    bmdbdialog.ui \
    dlgcustompatterns.ui \
    dlgdbupdate.ui \
    dlgbookcreator.ui \
    dlgeq.ui \
    dlgaddsinger.ui \
    dlgsongshop.ui \
    dlgsongshoppurchase.ui \
    dlgsetpassword.ui \
    dlgpassword.ui \
    dlgpurchaseprogress.ui \
    dlgeditsong.ui \
    dlgdebugoutput.ui

RESOURCES += resources.qrc



DISTFILES +=

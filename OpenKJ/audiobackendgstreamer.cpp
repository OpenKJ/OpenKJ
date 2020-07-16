/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "audiobackendgstreamer.h"
#include <QApplication>
#include <QDebug>
#include <string.h>
#include <math.h>
#include <QFile>
#include <gst/audio/streamvolume.h>
#include <gst/gstdebugutils.h>
#include "settings.h"
#include <functional>
#include <gst/video/videooverlay.h>

extern Settings *settings;
//GstElement *playBinPub;


Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<GstMessage>);

MediaBackend::MediaBackend(bool pitchShift, QObject *parent, QString objectName) :
    AbstractAudioBackend(parent), objName(objectName), loadPitchShift(pitchShift)
{
#ifdef MAC_OVERRIDE_GST
    // This points GStreamer paths to the framework contained in the app bundle.  Not needed on brew installs.
    QString appDir = QCoreApplication::applicationDirPath();
    appDir.replace("MacOS/", "");
    qputenv("GST_PLUGIN_SYSTEM_PATH", QString(appDir + "/../Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0").toLocal8Bit());
    qputenv("GST_PLUGIN_SCANNER", QString(appDir + "/../Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0/gst-plugin-scanner").toLocal8Bit());
    qputenv("GTK_PATH", QString(appDir + "/../Frameworks/GStreamer.framework/Versions/Current/").toLocal8Bit());
    qputenv("GIO_EXTRA_MODULES", QString(appDir + "/../Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules").toLocal8Bit());
    qWarning() << "MacOS detected, changed GST env vars to point to the bundled framework";
    qInfo() << "Application dir: " << appDir;
    qWarning() << qgetenv("GST_PLUGIN_SYSTEM_PATH") << endl << qgetenv("GST_PLUGIN_SCANNER") << endl << qgetenv("GTK_PATH") << endl << qgetenv("GIO_EXTRA_MODULES") << endl;
#endif
    qInfo() << "Start constructing GStreamer backend";
    QMetaTypeId<std::shared_ptr<GstMessage>>::qt_metatype_id();



    if (!gst_is_initialized())
    {
        qInfo() << objName << " - gst not initialized - initializing";
        gst_init(nullptr,nullptr);
    }

    guint major, minor, micro, nano;
    gst_version (&major, &minor, &micro, &nano);
    qInfo() << "Using GStreamer version: " << major << "." << minor << "." << micro;
    GstElement *cdgdec = gst_element_factory_make("cdgdec", "CDGDecoder");
    if (!cdgdec)
    {
        m_canRenderCdg = false;
        qInfo() << objName << "CDG decoder from gst-plugins-rs not installed. Can't render CDG graphics via gstreamer";
    }
    else {
        m_canRenderCdg = true;
        qInfo() << objName << "CDG decoder from gst-plugins-rs is installed.  Can render CDG graphics via gstreamer";
        gst_object_unref(cdgdec);
    }

    faderVolumeElement = gst_element_factory_make("volume", "FaderVolumeElement");
    g_object_set(faderVolumeElement, "volume", 1.0, nullptr);
    fader = new AudioFader(this);
    fader->setObjName(objName + "Fader");
    fader->setVolumeElement(faderVolumeElement);
    gst_object_ref(faderVolumeElement);

    buildPipeline();

    auto monitor = gst_device_monitor_new ();
    auto moncaps = gst_caps_new_empty_simple ("audio/x-raw");
    gst_device_monitor_add_filter (monitor, "Audio/Sink", moncaps);
    gst_caps_unref (moncaps);
    gst_device_monitor_start (monitor);
    outputDeviceNames.clear();
    outputDeviceNames.append("0 - Default");
    GList *devices, *elem;
    devices = gst_device_monitor_get_devices(monitor);
    for(elem = devices; elem; elem = elem->next) {
        auto device = outputDevices.emplace_back(reinterpret_cast<GstDevice*>(elem->data));
        auto *deviceName = gst_device_get_display_name(device);
        outputDeviceNames.append(QString::number(outputDeviceNames.size()) + " - " + deviceName);
        g_free(deviceName);
    }
    g_object_set(playBin, "volume", 1.0, nullptr);
    gst_device_monitor_stop(monitor);
    g_object_unref(monitor);
    g_list_free(devices);

    connect(&slowTimer, &QTimer::timeout, this, &MediaBackend::slowTimer_timeout);
    connect(&fastTimer, &QTimer::timeout, this, &MediaBackend::fastTimer_timeout);
    connect(fader, &AudioFader::fadeStarted, this, &MediaBackend::faderStarted);
    connect(fader, &AudioFader::fadeComplete, this, &MediaBackend::faderFinished);
    connect(fader, &AudioFader::faderStateChanged, this, &MediaBackend::faderStateChanged);
    connect(settings, &Settings::mplxModeChanged, this, &MediaBackend::setMplxMode);

    qInfo() << "Done constructing GStreamer backend";
    initDone = true;
}

void MediaBackend::videoMute(const bool &mute)
{
    gint flags;
    if (mute)
    {
        g_object_get (playBin, "flags", &flags, nullptr);
        flags |= GST_PLAY_FLAG_AUDIO;
        flags &= ~GST_PLAY_FLAG_TEXT;
        flags &= ~GST_PLAY_FLAG_VIDEO;
        g_object_set (playBin, "flags", flags, nullptr);
    }
    else
    {
        gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(videoSink1), videoWinId);
        if (m_previewEnabledLastBuild)
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(videoSink2), videoWinId2);
        g_object_get (playBin, "flags", &flags, nullptr);
        flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO;
        flags &= ~GST_PLAY_FLAG_TEXT;
        g_object_set (playBin, "flags", flags, nullptr);
    }
    m_vidMuted = mute;
}



MediaBackend::~MediaBackend()
{
    slowTimer.stop();
    fastTimer.stop();
    if (state() == PlayingState)
        stop(true);
    destroyPipeline();
    gst_object_unref(faderVolumeElement);
    if (videoSink1)
        gst_object_unref(videoSink1);
    if (videoSink2)
        gst_object_unref(videoSink2);
}



qint64 MediaBackend::position()
{
    gint64 pos;
    if (gst_element_query_position (playBin, GST_FORMAT_TIME, &pos))
    {
        return pos / 1000000;
    }
    return 0;
}

qint64 MediaBackend::getCdgPosition()
{
    gint64 pos;
    if (gst_element_query_position (cdgPipeline, GST_FORMAT_TIME, &pos))
    {
        return pos / 1000000;
    }
    return 0;
}



qint64 MediaBackend::duration()
{
    gint64 duration;
    if (gst_element_query_duration (playBin, GST_FORMAT_TIME, &duration))
    {
        return duration / 1000000;
    }
    return 0;
}

AbstractAudioBackend::State MediaBackend::state()
{
    GstState state = GST_STATE_NULL;
    gst_element_get_state(playBin, &state, nullptr, GST_CLOCK_TIME_NONE);
    switch (state) {
    case GST_STATE_PLAYING:
        return PlayingState;
    case GST_STATE_PAUSED:
        return PausedState;
    case GST_STATE_NULL:
        return StoppedState;
    default:
        return StoppedState;
    }
}

AbstractAudioBackend::State MediaBackend::cdgState()
{
    GstState state = GST_STATE_NULL;
    gst_element_get_state(cdgPipeline, &state, nullptr, GST_CLOCK_TIME_NONE);
    switch (state) {
    case GST_STATE_PLAYING:
        return PlayingState;
    case GST_STATE_PAUSED:
        return PausedState;
    case GST_STATE_NULL:
        return StoppedState;
    default:
        return StoppedState;
    }
}





void MediaBackend::play()
{
    //gst_timed_value_control_source_unset_all(tv_csource);
    qInfo() << objName << " - play() called";
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(playBin), GST_STREAM_VOLUME_FORMAT_LINEAR, 1.0);
    if (state() == AbstractAudioBackend::PausedState)
    {
        qInfo() << objName << " - play - playback is currently paused, unpausing";
        gst_element_set_state(playBin, GST_STATE_PLAYING);
        if (m_fade)
            fadeIn();
        if (m_cdgMode)
            cdgPlay();
        return;
    }
    else
    {
        resetPipeline();
        if (!QFile::exists(m_filename))
        {
            qInfo() << "File doesn't exist, bailing out";
            emit stateChanged(PlayingState);
            QApplication::processEvents();
            emit stateChanged(EndOfMediaState);
            return;
        }
        auto uri = gst_filename_to_uri(m_filename.toLocal8Bit(), nullptr);
        g_object_set(playBin, "uri", uri, nullptr);
        g_free(uri);
        qInfo() << objName << " - playing file: " << m_filename;
        if (!m_cdgMode)
            gst_element_set_state(playBin, GST_STATE_PLAYING);
    }
    if (m_cdgMode)
    {
        if (!QFile::exists(m_cdgFilename))
        {
            qInfo() << "CDG file doesn't exist, bailing out";
            emit stateChanged(PlayingState);
            QApplication::processEvents();
            emit stateChanged(EndOfMediaState);
            return;
        }
        cdg.open(m_cdgFilename);
        cdg.process();
        qInfo() << objName << " - playing cdg:   " << m_cdgFilename;
        qInfo() << objName << " - playing audio: " << m_filename;
        gst_element_set_state(playBin, GST_STATE_PLAYING);
        gst_element_set_state(cdgPipeline, GST_STATE_PLAYING);
       // gst_element_set_state(playBinCdg, GST_STATE_PAUSED);
    }
    qInfo() << objName << " - play() completed";
}

void MediaBackend::cdgPlay()
{
    gst_element_set_state(cdgPipeline, GST_STATE_PLAYING);
}

void MediaBackend::pause()
{
    if (m_fade)
        fadeOut();
    gst_element_set_state(playBin, GST_STATE_PAUSED);
    if (m_cdgMode)
        cdgPause();
}

void MediaBackend::cdgPause()
{
    gst_element_set_state(cdgPipeline, GST_STATE_PAUSED);
}

void MediaBackend::setMedia(QString filename)
{
    m_filename = filename;
    m_cdgMode = false;
}

void MediaBackend::setMediaCdg(QString cdgFilename, QString audioFilename)
{
    m_cdgMode = true;
    m_filename = audioFilename;
    m_cdgFilename = cdgFilename;
}

void MediaBackend::setMuted(bool muted)
{

    gst_stream_volume_set_mute(GST_STREAM_VOLUME(volumeElement), muted);
    m_muted = muted;

}

void MediaBackend::setPosition(qint64 position)
{
    qInfo() << objName << " - Seeking to: " << position << "ms";
    if (!gst_element_seek_simple(playBin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position))
    {
      qWarning() << objName << " - Audio seek failed!";
    }
    if (m_cdgMode)
    {
        if (!gst_element_seek_simple(cdgPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position))
            qWarning() << objName << " - CDG seek failed!";
    }
    emit positionChanged(position);
}

void MediaBackend::cdgSetPosition(qint64 position)
{
    gst_element_seek_simple(cdgPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position);
}

void MediaBackend::setVolume(int volume)
{
    m_volume = volume;
    double floatVol = volume * .01;
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC, floatVol);
    emit volumeChanged(volume);
}

void MediaBackend::stop(bool skipFade)
{
    qInfo() << objName << " - AudioBackendGstreamer::stop(" << skipFade << ") called";
    if (state() == AbstractAudioBackend::StoppedState)
    {
        qInfo() << objName << " - AudioBackendGstreamer::stop -- Already stopped, skipping";
        emit stateChanged(AbstractAudioBackend::StoppedState);
        return;
    }
    if (state() == AbstractAudioBackend::PausedState)
    {
        qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping paused song";
        gst_element_set_state(playBin, GST_STATE_NULL);
        if (m_cdgMode)
            cdgStop();
        emit stateChanged(AbstractAudioBackend::StoppedState);
        qInfo() << objName << " - stop() completed";
        fader->immediateIn();
        return;
    }
    if ((m_fade) && (!skipFade) && (state() == AbstractAudioBackend::PlayingState))
    {
        if (fader->state() == AudioFader::FadedIn || fader->state() == AudioFader::FadingIn)
        {
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Fading enabled.  Fading out audio volume";
            fadeOut(true);
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Fading complete";
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping playback";
            gst_element_set_state(playBin, GST_STATE_NULL);
            if (m_cdgMode)
                cdgStop();
            emit stateChanged(AbstractAudioBackend::StoppedState);
            fader->immediateIn();
            return;
        }
    }
    qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping playback without fading";
    gst_element_set_state(playBin, GST_STATE_NULL);
    if (m_cdgMode)
        cdgStop();
    emit stateChanged(AbstractAudioBackend::StoppedState);
    qInfo() << objName << " - stop() completed";


}

void MediaBackend::cdgStop()
{
    gst_element_set_state(cdgPipeline, GST_STATE_NULL);
}

void MediaBackend::rawStop()
{
    qInfo() << objName << " - rawStop() called, just ending gstreamer playback";
    gst_element_set_state(playBin, GST_STATE_NULL);
}

void MediaBackend::fastTimer_timeout()
{
    static qint64 lastPos = 0;
    if (lastState != PlayingState)
    {
        if (lastPos == 0)
            return;
        lastPos = 0;
        emit positionChanged(0);
        return;
    }
    gint64 pos, mspos;
    GstFormat fmt = GST_FORMAT_TIME;
    if (gst_element_query_position (playBin, fmt, &pos))
        mspos = pos / 1000000;
    else
        mspos = 0;
    if (m_cdgMode && (getCdgPosition() > mspos + 10 || getCdgPosition() < mspos - 10))
    {
        qInfo() << "resyncing cdg";
        cdgSetPosition(mspos);
    }
    if (lastPos != mspos)
    {

        lastPos = mspos;
        emit positionChanged(mspos);
    }
}

void MediaBackend::slowTimer_timeout()
{
    auto curState = state();
    if (lastState != curState)
        emit stateChanged(curState);
    if (m_silenceDetect)
    {
        if ((curState == AbstractAudioBackend::PlayingState) && (isSilent()))
        {
            if (m_silenceDuration >= 2)
            {
                emit silenceDetected();
                m_silenceDuration++;
                return;
            }
            m_silenceDuration++;
        }

        else
            m_silenceDuration = 0;
    }
    auto cubicVolume = gst_stream_volume_get_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC);
    int intVol = cubicVolume * 100;
    if (m_volume != intVol)
    {
        qInfo() << objName << " - emitting volume changed: " << intVol;
        emit faderChangedVolume(intVol);
        m_volume = intVol;
    }
    // Detect hung playback
    static int lastpos = 0;
    if (curState == PlayingState)
    {
        if (lastpos == position() && lastpos > 10)
        {
            qWarning() << objName << " - Playback appears to be hung, emitting end of stream";
            emit stateChanged(EndOfMediaState);
        }
        lastpos = position();
    }
}

void MediaBackend::faderChangedVolume(int volume)
{
    m_volume = volume;
    emit volumeChanged(volume);
}

void MediaBackend::faderStarted()
{
    qInfo() << objName << " - Fader started";
}

void MediaBackend::faderFinished()
{
    qInfo() << objName << " - fader finished";
}









void MediaBackend::setPitchShift(int pitchShift)
{
    m_keyChange = pitchShift;
    if (m_keyChangerRubberBand)
        g_object_set(pitchShifterRubberBand, "semitones", pitchShift, nullptr);
    else if (m_keyChangerSoundtouch)
    {
        g_object_set(pitchShifterSoundtouch, "pitch", getPitchForSemitone(pitchShift), nullptr);
    }
    emit pitchChanged(pitchShift);
}

QStringList MediaBackend::GstGetPlugins()
{
    QStringList list;
    GList* plugins; /* The head of the plug-in list */
    GList* pnode;   /* The currently viewed node */

    /* Empty the list out here */
    list.clear();

    plugins = pnode = gst_registry_get_plugin_list(gst_registry_get());
    while(pnode) {
            /* plugin: the plug-in info object pointed to by pnode */
            GstPlugin* plugin = reinterpret_cast<GstPlugin*>(pnode->data);

            list << gst_plugin_get_name(plugin);
            qInfo() << gst_plugin_get_name(plugin);
            pnode = g_list_next(pnode);
    }

    /* Clean-up */
    gst_plugin_list_free (plugins);
    return list;
}

QStringList MediaBackend::GstGetElements(QString plugin)
{
    QStringList list;
    GList* features;        /* The list of plug-in features */
    GList* fnode;           /* The currently viewed node */

    /* Empty the list out here */
    list.clear();

    features = fnode = gst_registry_get_feature_list_by_plugin( gst_registry_get(), plugin.toUtf8().data());
    while(fnode) {
        if (fnode->data) {
            /* Currently pointed-to feature */
            GstPluginFeature* feature = GST_PLUGIN_FEATURE(fnode->data);

            if (GST_IS_ELEMENT_FACTORY (feature)) {
                GstElementFactory* factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(feature));
                list << QString(GST_OBJECT_NAME(factory));
                qInfo() << QString(GST_OBJECT_NAME(factory));
            }
        }
        fnode = g_list_next(fnode);
    }
    gst_plugin_feature_list_free(features);
    return list;
}

template <typename T> std::shared_ptr<T> takeGstObject(T *o)
{
  std::shared_ptr<T> ptr(o, [] (T *d) {
    gst_object_unref(reinterpret_cast<GstObject*>(d));
  });
  return ptr;
}

template <typename T> std::shared_ptr<T> takeGstMiniObject(T *o)
{

    std::shared_ptr<T> ptr(o, [] (T *d) {
        gst_mini_object_ref(reinterpret_cast<GstMiniObject*>(d));
    });
    return ptr;
}

void MediaBackend::busMessage(std::shared_ptr<GstMessage> message)
{
    switch (GST_MESSAGE_TYPE(message.get())) {
    case GST_MESSAGE_LATENCY:
        break;
    case GST_MESSAGE_ASYNC_DONE:
        break;
    case GST_MESSAGE_NEW_CLOCK:
        break;
    case GST_MESSAGE_ERROR:
        GError *err;
        gchar *debug;
        gst_message_parse_error(message.get(), &err, &debug);
        qInfo() << objName << " - Gst error: " << err->message;
        qInfo() << objName << " - Gst debug: " << debug;
        if (QString(err->message) == "Your GStreamer installation is missing a plug-in.")
        {
            QString player;
            if (objName == "KA")
                player = "karaoke";
            else
                player = "break music";
            qInfo() << objName << " - PLAYBACK ERROR - Missing Codec";
            emit audioError("Unable to play " + player + " file, missing gstreamer plugin");
            stop(true);
        }
        g_error_free(err);
        g_free(debug);
        break;
    case GST_MESSAGE_WARNING:
        GError *err2;
        gchar *debug2;
        gst_message_parse_warning(message.get(), &err2, &debug2);
        qInfo() << objName << " - Gst warning: " << err2->message;
        qInfo() << objName << " - Gst debug: " << debug2;
        g_error_free(err2);
        g_free(debug2);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        GstState state;
        gst_element_get_state(playBin, &state, nullptr, GST_CLOCK_TIME_NONE);
        if (state == GST_STATE_PLAYING && lastState != AbstractAudioBackend::PlayingState)
        {
            qInfo() << "GST notified of state change to PLAYING";
            if (m_cdgMode && cdgState() != PlayingState)
                cdgPlay();
            lastState = AbstractAudioBackend::PlayingState;
            emit stateChanged(AbstractAudioBackend::PlayingState);
        }
        else if (state == GST_STATE_PAUSED && lastState != AbstractAudioBackend::PausedState)
        {
            qInfo() << "GST notified of state change to PAUSED";
            if (m_cdgMode && cdgState() != PausedState)
                cdgPause();
            lastState = AbstractAudioBackend::PausedState;
            emit stateChanged(AbstractAudioBackend::PausedState);
        }
        else if (state == GST_STATE_NULL && lastState != AbstractAudioBackend::StoppedState)
        {
            qInfo() << "GST notified of state change to STOPPED";
            if (lastState != AbstractAudioBackend::StoppedState)
            {
                lastState = AbstractAudioBackend::StoppedState;
                if (m_cdgMode && cdgState() != StoppedState)
                    cdgStop();
                emit stateChanged(AbstractAudioBackend::StoppedState);
            }
        }
        else if (lastState != AbstractAudioBackend::UnknownState)
        {
            //qInfo() << "GST notified of state change to UNKNOWN";
            //lastState = AbstractAudioBackend::UnknownState;
            //emit stateChanged(AbstractAudioBackend::UnknownState);
        }
        break;
    case GST_MESSAGE_ELEMENT:
        if (QString(gst_structure_get_name (gst_message_get_structure(message.get()))) == "level")
        {
            auto array_val = gst_structure_get_value(gst_message_get_structure(message.get()), "rms");
            auto rms_arr = reinterpret_cast<GValueArray*>(g_value_get_boxed (array_val));
            double rmsValues = 0.0;
            for (unsigned int i{0}; i < rms_arr->n_values; ++i)
            {
                auto value = g_value_array_get_nth (rms_arr, i);
                auto rms_dB = g_value_get_double (value);
                auto rms = pow (10, rms_dB / 20);
                rmsValues = rmsValues + rms;
            }
            m_currentRmsLevel = rmsValues / rms_arr->n_values;
        }
        break;
    case GST_MESSAGE_STREAM_START:
        // workaround for gstreamer changing the volume unsolicited on playback start
        double cubicVolume;
        cubicVolume = gst_stream_volume_get_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC);
        int intVol;
        intVol = cubicVolume * 100;
        if (m_volume != intVol)
        {
            if ((!fader->isFading()) && ((intVol < m_volume - 1) || (intVol > m_volume + 1)))
            {
                qInfo() << objName << " - stream start - Unrequested volume change detected, squashing";
                setVolume(m_volume);
            }
        }
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        gint64 dur, msdur;
        qInfo() << objName << " - GST reports duration changed";
        if (gst_element_query_duration(playBin,GST_FORMAT_TIME,&dur))
            msdur = dur / 1000000;
        else
            msdur = 0;
        emit durationChanged(msdur);
        break;
    case GST_MESSAGE_EOS:
        qInfo() << objName << " - state change to EndOfMediaState emitted";
        emit stateChanged(EndOfMediaState);
        //emit stateChanged(StoppedState);
        break;
    case GST_MESSAGE_TAG:
        // do nothing
        break;
    case GST_MESSAGE_STREAM_STATUS:
        // do nothing
        break;
    case GST_MESSAGE_NEED_CONTEXT:
        qInfo() << objName  << " - context requested - " << message->src->name;
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink1), videoWinId);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink2), videoWinId2);
        break;
    default:
        //g_print("Msg type[%d], Msg type name[%s]\n", GST_MESSAGE_TYPE(message), GST_MESSAGE_TYPE_NAME(message));
        qInfo() << objName << " - Gst msg type: " << GST_MESSAGE_TYPE(message.get()) << " Gst msg name: " << GST_MESSAGE_TYPE_NAME(message.get()) << " Element: " << message->src->name;
        break;
    }
}

void MediaBackend::busMessageCdg(std::shared_ptr<GstMessage> message)
{
    switch (GST_MESSAGE_TYPE(message.get())) {
    case GST_MESSAGE_NEED_CONTEXT:
        qInfo() << objName  << " - CDG pipeline - context requested - " << message->src->name;
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink1), videoWinId);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(videoSink2), videoWinId2);
        break;
    default:
        //g_print("Msg type[%d], Msg type name[%s]\n", GST_MESSAGE_TYPE(message), GST_MESSAGE_TYPE_NAME(message));
        //qInfo() << objName << " - Gst msg type: " << GST_MESSAGE_TYPE(message.get()) << " Gst msg name: " << GST_MESSAGE_TYPE_NAME(message.get()) << " Element: " << message->src->name;
        break;
    }
}

void MediaBackend::gstPositionChanged(qint64 position)
{
    qInfo() << "gstPositionChanged(" << position << ") called";
    emit positionChanged(position);
}

void MediaBackend::gstDurationChanged(qint64 duration)
{
    emit durationChanged(duration);
}

void MediaBackend::gstFastTimerFired()
{
    if (lastState != PlayingState)
    {
        if (lastPosition == 0)
            return;
        lastPosition = 0;
        emit positionChanged(0);
        return;
    }
    gint64 pos;
    if (gst_element_query_position (playBin, GST_FORMAT_TIME, &pos))
    {
        auto mspos = pos / 1000000;
        if (lastPosition != mspos)
            emit positionChanged(mspos);
    }
    else
    {
        if (lastPosition != 0)
            emit positionChanged(0);
    }
}

void MediaBackend::faderChangedVolume(double volume)
{
    emit volumeChanged(volume * 100);
}

void MediaBackend::faderStateChanged(AudioFader::FaderState state)
{
    qInfo() << objName << " - Fader state changed to: " << fader->stateToStr(state);
}



GstBusSyncReply MediaBackend::busMessageDispatcher([[maybe_unused]]GstBus *bus, GstMessage *message, gpointer userData)
{
  auto myObj = static_cast<MediaBackend*>(userData);
  auto messagePtr = takeGstMiniObject(message);
  QMetaObject::invokeMethod(myObj, "busMessage", Qt::QueuedConnection, Q_ARG(std::shared_ptr<GstMessage>, messagePtr));
  return GST_BUS_DROP;
}


void MediaBackend::buildPipeline()
{
    qInfo() << objName << " - buildPipeline() called";
    if (!gst_is_initialized())
    {
        qInfo() << objName << " - gst not initialized - initializing";
        gst_init(nullptr,nullptr);
    }
#if defined(Q_OS_LINUX)
        switch (accelMode) {
        case OpenGL:
            videoSink1 = gst_element_factory_make("glimagesink", "videoSink1");
            videoSink2 = gst_element_factory_make("glimagesink", "videoSink2");
            break;
        case XVideo:
            videoSink1 = gst_element_factory_make("xvimagesink", "videoSink1");
            videoSink2 = gst_element_factory_make("xvimagesink", "videoSink2");
            break;
        }
#elif defined(Q_OS_WIN)
        videoSink1 = gst_element_factory_make ("d3dvideosink", "videoSink1");
        videoSink2 = gst_element_factory_make("d3dvideosink", "videoSink2");
#elif defined(Q_OS_MAC)
        videoSink1 = gst_element_factory_make("glimagesink", "videosink1");
        videoSink2 = gst_element_factory_make("glimagesink", "videosink2");
#else
        qWarning() << "Unknown platform, defaulting to OpenGL video output";
        videoSink1 = gst_element_factory_make ("glimagesink", "videoSink1");
        videoSink2 = gst_element_factory_make("glimagesink", "videoSink2");
#endif
    gst_object_ref(videoSink1);
    gst_object_ref(videoSink2);



    m_previewEnabledLastBuild = settings->previewEnabled();
    auto aConvInput = gst_element_factory_make("audioconvert", "aConvInput");
    auto aConvPrePitchShift = gst_element_factory_make("audioconvert", "aConvPrePitchShift");
    auto aConvPostPitchShift = gst_element_factory_make("audioconvert", "aConvPostPitchShift");
    audioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    auto rgVolume = gst_element_factory_make("rgvolume", "rgVolume");
    g_object_set(rgVolume, "pre-amp", 6.0, "headroom", 10.0, nullptr);
    auto rgLimiter = gst_element_factory_make("rglimiter", "rgLimiter");
    auto level = gst_element_factory_make("level", "level");
    pitchShifterSoundtouch = gst_element_factory_make("pitch", "pitch");
    pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
    equalizer = gst_element_factory_make("equalizer-10bands", "equalizer");
    playBin = gst_element_factory_make("playbin", "playBin");
    audioCapsStereo = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, nullptr);
    audioCapsMono = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 1, nullptr);
    audioBin = gst_bin_new("audioBin");
    auto aConvPostPanorama = gst_element_factory_make("audioconvert", "aConvPostPanorama");
    aConvEnd = gst_element_factory_make("audioconvert", "aConvEnd");
    fltrPostPanorama = gst_element_factory_make("capsfilter", "fltrPostPanorama");
    g_object_set(fltrPostPanorama, "caps", audioCapsStereo, nullptr);
    volumeElement = gst_element_factory_make("volume", "volumeElement");
    auto queueMainAudio = gst_element_factory_make("queue", "queueMainAudio");
    auto queueEndAudio = gst_element_factory_make("queue", "queueEndAudio");
    audioPanorama = gst_element_factory_make("audiopanorama", "audioPanorama");
    g_object_set(audioPanorama, "method", 1, nullptr);

    gst_bin_add_many(GST_BIN(audioBin),queueMainAudio, audioPanorama, level, aConvInput, rgVolume, rgLimiter, volumeElement, equalizer, aConvPostPanorama, fltrPostPanorama, gst_object_ref(faderVolumeElement), nullptr);
    gst_element_link(queueMainAudio, aConvInput);
    gst_element_link(aConvInput, rgVolume);
    gst_element_link(rgVolume, rgLimiter);
    gst_element_link(rgLimiter, level);
    gst_element_link(level, volumeElement);
    gst_element_link(volumeElement, equalizer);
    gst_element_link(equalizer, faderVolumeElement);

    gst_element_link(faderVolumeElement, audioPanorama);
    gst_element_link(audioPanorama, aConvPostPanorama);
    gst_element_link(aConvPostPanorama, fltrPostPanorama);

    if ((pitchShifterRubberBand) && (pitchShifterSoundtouch) && (loadPitchShift))
    {
        qInfo() << objName << " - Pitch shift RubberBand enabled";
        qInfo() << objName << " - Also loaded SoundTouch for tempo control";
        m_canChangeTempo = true;
        gst_bin_add_many(GST_BIN(audioBin), aConvPrePitchShift, pitchShifterRubberBand, aConvPostPitchShift, pitchShifterSoundtouch, aConvEnd, queueEndAudio, audioSink, nullptr);
        gst_element_link_many(fltrPostPanorama, aConvPrePitchShift, pitchShifterRubberBand, aConvPostPitchShift, pitchShifterSoundtouch, queueEndAudio, aConvEnd, audioSink, nullptr);
        m_canKeyChange = true;
        m_keyChangerRubberBand = true;
        g_object_set(pitchShifterRubberBand, "formant-preserving", true, nullptr);
        g_object_set(pitchShifterRubberBand, "crispness", 1, nullptr);
        g_object_set(pitchShifterRubberBand, "semitones", 0, nullptr);
    }
    else if ((pitchShifterSoundtouch) && (loadPitchShift))
    {
        m_canChangeTempo = true;
        qInfo() << objName << " - Pitch shifter SoundTouch enabled";
        gst_bin_add_many(GST_BIN(audioBin), aConvPrePitchShift, pitchShifterSoundtouch, aConvPostPitchShift, aConvEnd, queueEndAudio, audioSink, nullptr);
        gst_element_link_many(fltrPostPanorama, aConvPrePitchShift, pitchShifterSoundtouch, queueEndAudio, aConvEnd, audioSink, nullptr);
        m_canKeyChange = true;
        m_keyChangerSoundtouch = true;
        g_object_set(pitchShifterSoundtouch, "pitch", 1.0, "tempo", 1.0, nullptr);
    }
    else
    {
        gst_bin_add_many(GST_BIN(audioBin), aConvEnd, queueEndAudio, audioSink, nullptr);
        gst_element_link_many(fltrPostPanorama, queueEndAudio, aConvEnd, audioSink, nullptr);
    }

    GstPad *pad;
    pad = gst_element_get_static_pad(queueMainAudio, "sink");
    auto ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(audioBin, ghostPad);
    gst_object_unref(pad);
    g_object_set(playBin, "audio-sink", audioBin, nullptr);


    // CDG video pipeline
    if (m_cdgMode)
    {
        cdgPipeline = gst_pipeline_new("cdgPipeline");
        auto cdgAppSrc = gst_element_factory_make("appsrc", "cdgAppSrc");
        auto cdgVidConv = gst_element_factory_make("videoconvert", "cdgVideoConv");
        g_object_set(G_OBJECT(cdgAppSrc), "caps",
                     gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB16", "width", G_TYPE_INT, 288, "height",
                                         G_TYPE_INT, 192, "framerate", GST_TYPE_FRACTION, 1, 30, NULL),
                     NULL);
        if (settings->previewEnabled())
        {
            auto videoQueue1 = gst_element_factory_make("queue", "videoQueue1");
            auto videoQueue2 = gst_element_factory_make("queue", "videoQueue2");
            auto videoConv1 = gst_element_factory_make("videoconvert", "preOutVideoConvert1");
            auto videoConv2 = gst_element_factory_make("videoconvert", "preOutVideoConvert2");
            videoTee = gst_element_factory_make("tee", "videoTee");
            videoTeePad1 = gst_element_get_request_pad(videoTee, "src_%u");
            videoTeePad2 = gst_element_get_request_pad(videoTee, "src_%u");
            videoQueue1SrcPad = gst_element_get_static_pad(videoQueue1, "sink");
            videoQueue2SrcPad = gst_element_get_static_pad(videoQueue2, "sink");
            videoBin = gst_bin_new("videoBin");
            gst_bin_add_many(reinterpret_cast<GstBin *>(cdgPipeline), cdgAppSrc, cdgVidConv, videoConv1, videoConv2,videoTee,videoQueue1,videoQueue2,videoSink1,videoSink2,nullptr);
            gst_element_link(cdgAppSrc, cdgVidConv);
            gst_element_link(cdgVidConv, videoTee);
            gst_pad_link(videoTeePad1,videoQueue1SrcPad);
            gst_pad_link(videoTeePad2,videoQueue2SrcPad);
            gst_element_link(videoQueue1,videoConv1);
            gst_element_link(videoQueue2,videoConv2);
            gst_element_link(videoConv1, videoSink1);
            gst_element_link(videoConv2, videoSink2);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(videoSink1), videoWinId);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(videoSink2), videoWinId2);
        }
        else
        {
            gst_bin_add_many(reinterpret_cast<GstBin *>(cdgPipeline), cdgAppSrc, cdgVidConv, videoSink1, nullptr);
            gst_element_link_many(cdgAppSrc, cdgVidConv, videoSink1, nullptr);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(videoSink1), videoWinId);
        }
        g_object_set(G_OBJECT(cdgAppSrc), "stream-type", 1, "format", GST_FORMAT_TIME, NULL);
        g_signal_connect(cdgAppSrc, "need-data", G_CALLBACK(cb_need_data), this);
        g_signal_connect(cdgAppSrc, "seek-data", G_CALLBACK(cb_seek_data), this);
    }
    else
    {
        if (settings->previewEnabled())
        {
            auto queueMainVideo = gst_element_factory_make("queue", "queueMainVideo");
            auto videoQueue1 = gst_element_factory_make("queue", "videoQueue1");
            auto videoQueue2 = gst_element_factory_make("queue", "videoQueue2");
            videoTee = gst_element_factory_make("tee", "videoTee");
            videoTeePad1 = gst_element_get_request_pad(videoTee, "src_%u");
            videoTeePad2 = gst_element_get_request_pad(videoTee, "src_%u");
            videoQueue1SrcPad = gst_element_get_static_pad(videoQueue1, "sink");
            videoQueue2SrcPad = gst_element_get_static_pad(videoQueue2, "sink");
            videoBin = gst_bin_new("videoBin");
            gst_bin_add_many(GST_BIN(videoBin),queueMainVideo,videoTee,videoQueue1,videoQueue2,videoSink1,videoSink2,nullptr);
            gst_element_link(queueMainVideo, videoTee);
            gst_pad_link(videoTeePad1,videoQueue1SrcPad);
            gst_pad_link(videoTeePad2,videoQueue2SrcPad);
            gst_element_link(videoQueue1,videoSink1);
            gst_element_link(videoQueue2,videoSink2);
            auto ghostVideoPad = gst_ghost_pad_new("sink", gst_element_get_static_pad(queueMainVideo, "sink"));
            gst_pad_set_active(ghostVideoPad,true);
            gst_element_add_pad(videoBin, ghostVideoPad);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(videoSink1), videoWinId);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(videoSink2), videoWinId2);
            g_object_set(playBin, "video-sink", videoBin, nullptr);
        }
        else
        {
            qInfo() << "Main window preview disabled, building pipeline without video tee";
            gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(videoSink1), videoWinId);
            g_object_set(playBin, "video-sink", videoSink1, nullptr);
        }
    }
// End video output setup

    g_object_set(rgVolume, "album-mode", false, nullptr);
    g_object_set(level, "message", TRUE, nullptr);
    auto bus = takeGstObject(gst_element_get_bus(playBin));
    gst_bus_set_sync_handler(bus.get(), busMessageDispatcher, this, nullptr);
    if (m_cdgMode)
    {
        auto busCdg = takeGstObject(gst_element_get_bus(cdgPipeline));
        gst_bus_set_sync_handler(bus.get(), busMessageDispatcherCdg, this, nullptr);
    }
    auto csource = gst_interpolation_control_source_new ();
    if (!csource)
        qInfo() << objName << " - Error createing control source";
    GstControlBinding *cbind = gst_direct_control_binding_new (GST_OBJECT_CAST(faderVolumeElement), "volume", csource);
    if (!cbind)
        qInfo() << objName << " - Error creating control binding";
    if (!gst_object_add_control_binding (GST_OBJECT_CAST(faderVolumeElement), cbind))
        qInfo() << objName << " - Error adding control binding to volumeElement for fader control";
    g_object_set(csource, "mode", GST_INTERPOLATION_MODE_CUBIC, nullptr);
    setVolume(m_volume);
    slowTimer.start(1000);
    setOutputDevice(outputDeviceIdx);
    setEqBypass(bypass);
    setEqLevel1(eqLevels[0]);
    setEqLevel2(eqLevels[1]);
    setEqLevel3(eqLevels[2]);
    setEqLevel4(eqLevels[3]);
    setEqLevel5(eqLevels[4]);
    setEqLevel6(eqLevels[5]);
    setEqLevel7(eqLevels[6]);
    setEqLevel8(eqLevels[7]);
    setEqLevel9(eqLevels[8]);
    setEqLevel10(eqLevels[9]);
    setDownmix(downmix);
    setMuted(m_muted);
    videoMute(m_vidMuted);
    setVolume(m_volume);
    fastTimer.start(40);
    qInfo() << objName << " - buildPipeline() finished";
 //   GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)customBin, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
}

void MediaBackend::destroyPipeline()
{
    qInfo() << objName << " - destroyPipeline() called";
    slowTimer.stop();
    fastTimer.stop();
    gst_bin_remove(reinterpret_cast<GstBin *>(audioBin), faderVolumeElement);
    if (state() == PlayingState)
        stop(true);
    if (m_previewEnabledLastBuild)
    {
        gst_element_release_request_pad(videoTee, videoTeePad1);
        gst_element_release_request_pad(videoTee, videoTeePad2);
        gst_object_unref(videoTeePad1);
        gst_object_unref(videoTeePad2);
        gst_object_unref(videoQueue1SrcPad);
        gst_object_unref(videoQueue2SrcPad);
        gst_bin_remove(reinterpret_cast<GstBin *>(videoBin), videoSink1);
        gst_bin_remove(reinterpret_cast<GstBin *>(videoBin), videoSink2);
        gst_object_unref(videoBin);
    }
    gst_caps_unref(audioCapsMono);
    gst_caps_unref(audioCapsStereo);
    gst_object_unref(audioBin);
    //gst_object_unref(cdgPipeline);
    qInfo() << objName << " - destroyPipeline() finished";
}

void MediaBackend::resetPipeline()
{
    destroyPipeline();
    buildPipeline();
}

GstBusSyncReply MediaBackend::busMessageDispatcherCdg([[maybe_unused]]GstBus *bus, GstMessage *message, gpointer userData)
{
    auto myObj = static_cast<MediaBackend*>(userData);
    auto messagePtr = takeGstMiniObject(message);
    QMetaObject::invokeMethod(myObj, "busMessageCdg", Qt::QueuedConnection, Q_ARG(std::shared_ptr<GstMessage>, messagePtr));
    return GST_BUS_DROP;
}

void MediaBackend::cb_need_data(GstElement *appsrc, [[maybe_unused]]guint unused_size, gpointer user_data)
{
    auto backend = reinterpret_cast<MediaBackend *>(user_data);
    QImage vframe(QSize(288,192), QImage::Format_RGB16);
    vframe.fill(Qt::black);
    if (backend->curFrame < backend->cdg.getFrameCount())
        vframe = backend->cdg.videoImageByFrame(backend->curFrame);

#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    auto buffer = gst_buffer_new_allocate(NULL, vframe.sizeInBytes(), NULL);
#else
    auto buffer = gst_buffer_new_allocate(NULL, vframe.byteCount(), NULL);
#endif
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);

#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    memcpy(map.data, vframe.bits(), vframe.sizeInBytes());
#else
    memcpy(map.data, vframe.bits(), vframe.byteCount());
#endif
    gst_buffer_unmap(buffer, &map);
    GST_BUFFER_PTS(buffer) = backend->cdgPosition;
    GST_BUFFER_DURATION(buffer) = 40000000;
    backend->cdgPosition += GST_BUFFER_DURATION(buffer);
    backend->curFrame++;
    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
}

void MediaBackend::cb_seek_data([[maybe_unused]]GstElement *appsrc, guint64 position, gpointer user_data)
{
    qWarning() << "Got seek event: " << position << " ms: " << position / 40000000;
    auto backend = reinterpret_cast<MediaBackend *>(user_data);
    GST_DEBUG ("seek to offset %" G_GUINT64_FORMAT, position);
    if (position == 0)
    {
        backend->curFrame = 0;
        backend->cdgPosition = 0;
        return;
    }
    backend->curFrame = position / 40000000;
    backend->cdgPosition = position;
}



void MediaBackend::fadeOut(bool waitForFade)
{
    gdouble curVolume;
    g_object_get(G_OBJECT(volumeElement), "volume", &curVolume, nullptr);
    if (state() != PlayingState)
    {
        qInfo() << objName << " - fadeOut - State not playing, skipping fade and setting volume directly";
        fader->immediateOut();
        return;
    }
    fader->fadeOut(waitForFade);
}

void MediaBackend::fadeIn(bool waitForFade)
{
    if (state() != PlayingState)
    {
        qInfo() << objName << " - fadeIn - State not playing, skipping fade and setting volume";
        fader->immediateIn();
        return;
    }
    if (isSilent())
    {
        qInfo() << objName << "- fadeOut - Audio is currently slient, skipping fade and setting volume immediately";
        fader->immediateIn();
        return;
    }
    fader->fadeIn(waitForFade);
}

void MediaBackend::setUseFader(bool fade)
{
    m_fade = fade;
}




bool MediaBackend::isSilent()
{
    if ((m_currentRmsLevel <= 0.01) && (m_volume > 0) && (!fader->isFading()))
        return true;
    return false;
}

void MediaBackend::setUseSilenceDetection(bool enabled)
{
    m_silenceDetect = enabled;
}






void MediaBackend::setDownmix(bool enabled)
{
    downmix = enabled;
    if (enabled)
        g_object_set(fltrPostPanorama, "caps", audioCapsMono, nullptr);
    else
        g_object_set(fltrPostPanorama, "caps", audioCapsStereo, nullptr);
}

void MediaBackend::setTempo(int percent)
{
    float tempo = (float)percent / 100.0;
    m_tempo = percent;
    g_object_set(pitchShifterSoundtouch, "tempo", tempo, nullptr);
    setPosition(position());
    qInfo() << objName << " - Tempo changed to " << tempo;
}



void MediaBackend::setOutputDevice(int deviceIndex)
{
    outputDeviceIdx = deviceIndex;
    gst_element_unlink(aConvEnd, audioSink);
    gst_bin_remove(GST_BIN(audioBin), audioSink);
    if (deviceIndex == 0)
    {
        audioSink = gst_element_factory_make("autoaudiosink", "audioSink");;
    }
    else
    {
        audioSink = gst_device_create_element(outputDevices.at(deviceIndex - 1), NULL);
    }
    gst_bin_add(GST_BIN(audioBin), audioSink);
    gst_element_link(aConvEnd, audioSink);
}



void MediaBackend::setMplxMode(int mode)
{
 //   qInfo() << objName << " - setMplxMode(" << mode << ") called";
    if (mode == Multiplex_Normal)
    {
        g_object_set(audioPanorama, "panorama", 0.0, nullptr);
        setDownmix(settings->audioDownmix());
    }
    else if (mode == Multiplex_LeftChannel)
    {
        setDownmix(true);
        g_object_set(audioPanorama, "panorama", -1.0, nullptr);
    }
    else if (mode == Multiplex_RightChannel)
    {
        setDownmix(true);
        g_object_set(audioPanorama, "panorama", 1.0, nullptr);

    }
}


void MediaBackend::setEqBypass(bool bypass)
{
    if (bypass)
    {
        g_object_set(equalizer, "band0", 0.0, nullptr);
        g_object_set(equalizer, "band1", 0.0, nullptr);
        g_object_set(equalizer, "band2", 0.0, nullptr);
        g_object_set(equalizer, "band3", 0.0, nullptr);
        g_object_set(equalizer, "band4", 0.0, nullptr);
        g_object_set(equalizer, "band5", 0.0, nullptr);
        g_object_set(equalizer, "band6", 0.0, nullptr);
        g_object_set(equalizer, "band7", 0.0, nullptr);
        g_object_set(equalizer, "band8", 0.0, nullptr);
        g_object_set(equalizer, "band0", 0.0, nullptr);
    }
    else
    {
        g_object_set(equalizer, "band1", (double)eqLevels[0], nullptr);
        g_object_set(equalizer, "band2", (double)eqLevels[1], nullptr);
        g_object_set(equalizer, "band3", (double)eqLevels[2], nullptr);
        g_object_set(equalizer, "band4", (double)eqLevels[3], nullptr);
        g_object_set(equalizer, "band5", (double)eqLevels[4], nullptr);
        g_object_set(equalizer, "band6", (double)eqLevels[5], nullptr);
        g_object_set(equalizer, "band7", (double)eqLevels[6], nullptr);
        g_object_set(equalizer, "band8", (double)eqLevels[7], nullptr);
        g_object_set(equalizer, "band0", (double)eqLevels[8], nullptr);
        g_object_set(equalizer, "band9", (double)eqLevels[9], nullptr);

    }
    this->bypass = bypass;
}

void MediaBackend::setEqLevel1(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band0", (double)level, nullptr);
    eqLevels[0] = level;
}

void MediaBackend::setEqLevel2(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band1", (double)level, nullptr);
    eqLevels[1] = level;
}

void MediaBackend::setEqLevel3(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band2", (double)level, nullptr);
    eqLevels[2] = level;
}

void MediaBackend::setEqLevel4(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band3", (double)level, nullptr);
    eqLevels[3] = level;
}

void MediaBackend::setEqLevel5(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band4", (double)level, nullptr);
    eqLevels[4] = level;
}

void MediaBackend::setEqLevel6(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band5", (double)level, nullptr);
    eqLevels[5] = level;
}

void MediaBackend::setEqLevel7(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band6", (double)level, nullptr);
    eqLevels[6] = level;
}

void MediaBackend::setEqLevel8(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band7", (double)level, nullptr);
    eqLevels[7] = level;
}

void MediaBackend::setEqLevel9(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band8", (double)level, nullptr);
    eqLevels[8] = level;
}

void MediaBackend::setEqLevel10(int level)
{
    if (!bypass)
        g_object_set(equalizer, "band9", (double)level, nullptr);
    eqLevels[9] = level;
}


bool MediaBackend::hasVideo()
{
    if (m_cdgMode)
        return true;
    gint numVidStreams;
    g_object_get(playBin, "n-video", &numVidStreams, nullptr);
    if (numVidStreams > 0)
        return true;
    return false;
}

void MediaBackend::fadeInImmediate()
{
    fader->immediateIn();
}

void MediaBackend::fadeOutImmediate()
{
    fader->immediateOut();
}

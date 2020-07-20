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
#include <gst/app/gstappsrc.h>

extern Settings *settings;
//GstElement *playBinPub;


Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<GstMessage>);

MediaBackend::MediaBackend(bool pitchShift, QObject *parent, QString objectName) :
    QObject(parent), objName(objectName), m_loadPitchShift(pitchShift)
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
    faderVolumeElement = gst_element_factory_make("volume", "FaderVolumeElement");
    g_object_set(faderVolumeElement, "volume", 1.0, nullptr);
    m_fader = new AudioFader(this);
    m_fader->setObjName(objName + "Fader");
    m_fader->setVolumeElement(faderVolumeElement);
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
        auto device = m_outputDevices.emplace_back(reinterpret_cast<GstDevice*>(elem->data));
        auto *deviceName = gst_device_get_display_name(device);
        outputDeviceNames.append(QString::number(outputDeviceNames.size()) + " - " + deviceName);
        g_free(deviceName);
    }
    g_object_set(playBin, "volume", 1.0, nullptr);
    gst_device_monitor_stop(monitor);
    g_object_unref(monitor);
    g_list_free(devices);

    connect(settings, &Settings::enforceAspectRatioChanged, this, &MediaBackend::setEnforceAspectRatio);
    connect(&slowTimer, &QTimer::timeout, this, &MediaBackend::slowTimer_timeout);
    connect(&fastTimer, &QTimer::timeout, this, &MediaBackend::fastTimer_timeout);
    connect(m_fader, &AudioFader::fadeStarted, [&] () {
        qInfo() << objName << " - Fader started";
    });
    connect(m_fader, &AudioFader::fadeComplete, [&] () {
        qInfo() << objName << " - fader finished";
    });
    connect(m_fader, &AudioFader::faderStateChanged, [&] (auto state) {
        qInfo() << objName << " - Fader state changed to: " << m_fader->stateToStr(state);
    });
    connect(settings, &Settings::mplxModeChanged, this, &MediaBackend::setMplxMode);

    qInfo() << "Done constructing GStreamer backend";
    m_initDone = true;
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

float MediaBackend::getPitchForSemitone(int semitone)
{
    double pitch;
    if (semitone > 0)
    {
        pitch = pow(STUP,semitone);
    }
    else if (semitone < 0){
        pitch = 1 - ((100 - (pow(STDN,abs(semitone)) * 100)) / 100);
    }
    else
    {
        pitch = 1.0;
    }
    return pitch;
}

void MediaBackend::setEnforceAspectRatio(const bool &enforce)
{
    g_object_set(videoSink1, "force-aspect-ratio", enforce, nullptr);
    if (m_previewEnabledLastBuild)
        g_object_set(videoSink2, "force-aspect-ratio", enforce, nullptr);
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

MediaBackend::State MediaBackend::state()
{
    GstState state = GST_STATE_NULL;
    gst_element_get_state(playBin, &state, nullptr, GST_CLOCK_TIME_NONE);
    switch (state) {
    case GST_STATE_PLAYING:
        return PlayingState;
    case GST_STATE_PAUSED:
        return PausedState;
    case GST_STATE_NULL:
    default:
        return StoppedState;
    }
}

MediaBackend::State MediaBackend::cdgState()
{
    GstState state = GST_STATE_NULL;
    gst_element_get_state(cdgPipeline, &state, nullptr, GST_CLOCK_TIME_NONE);
    switch (state) {
    case GST_STATE_PLAYING:
        return PlayingState;
    case GST_STATE_PAUSED:
        return PausedState;
    case GST_STATE_NULL:
    default:
        return StoppedState;
    }
}

void MediaBackend::play()
{
    qInfo() << objName << " - play() called";
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(playBin), GST_STREAM_VOLUME_FORMAT_LINEAR, 1.0);
    if (state() == MediaBackend::PausedState)
    {
        qInfo() << objName << " - play - playback is currently paused, unpausing";
        gst_element_set_state(playBin, GST_STATE_PLAYING);
        if (m_fade)
            fadeIn();
        if (m_cdgMode)
            cdgPlay();
        return;
    }
    resetPipeline();
    if (!QFile::exists(m_filename))
    {
        qInfo() << " - play - File doesn't exist, bailing out";
        emit stateChanged(PlayingState);
        QApplication::processEvents();
        emit stateChanged(EndOfMediaState);
        return;
    }
    auto uri = gst_filename_to_uri(m_filename.toLocal8Bit(), nullptr);
    g_object_set(playBin, "uri", uri, nullptr);
    g_free(uri);
    if (!m_cdgMode)
    {
        qInfo() << objName << " - play - playing media: " << m_filename;
        gst_element_set_state(playBin, GST_STATE_PLAYING);
    }
    else
    {
        if (!QFile::exists(m_cdgFilename))
        {
            qInfo() << " - play - CDG file doesn't exist, bailing out";
            emit stateChanged(PlayingState);
            QApplication::processEvents();
            emit stateChanged(EndOfMediaState);
            return;
        }
        cdg.open(m_cdgFilename);
        cdg.process();
        qInfo() << objName << " - play - playing cdg:   " << m_cdgFilename;
        qInfo() << objName << " - play - playing audio: " << m_filename;
        gst_element_set_state(playBin, GST_STATE_PLAYING);
        gst_element_set_state(cdgPipeline, GST_STATE_PLAYING);
    }
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
    m_cdgMode = false;
    m_filename = filename;
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
    gst_element_seek_simple(playBin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position);
    if (m_cdgMode)
        cdgSetPosition(position);
    emit positionChanged(position);
}

void MediaBackend::cdgSetPosition(qint64 position)
{
    gst_element_seek_simple(cdgPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position);
}

void MediaBackend::setVolume(int volume)
{
    m_volume = volume;
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC, volume * .01);
    emit volumeChanged(volume);
}

void MediaBackend::stop(bool skipFade)
{
    qInfo() << objName << " - AudioBackendGstreamer::stop(" << skipFade << ") called";
    if (state() == MediaBackend::StoppedState)
    {
        qInfo() << objName << " - AudioBackendGstreamer::stop -- Already stopped, skipping";
        emit stateChanged(MediaBackend::StoppedState);
        return;
    }
    if (state() == MediaBackend::PausedState)
    {
        qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping paused song";
        gst_element_set_state(playBin, GST_STATE_NULL);
        if (m_cdgMode)
            cdgStop();
        emit stateChanged(MediaBackend::StoppedState);
        qInfo() << objName << " - stop() completed";
        m_fader->immediateIn();
        return;
    }
    if ((m_fade) && (!skipFade) && (state() == MediaBackend::PlayingState))
    {
        if (m_fader->state() == AudioFader::FadedIn || m_fader->state() == AudioFader::FadingIn)
        {
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Fading enabled.  Fading out audio volume";
            fadeOut(true);
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Fading complete";
            qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping playback";
            gst_element_set_state(playBin, GST_STATE_NULL);
            if (m_cdgMode)
                cdgStop();
            emit stateChanged(MediaBackend::StoppedState);
            m_fader->immediateIn();
            return;
        }
    }
    qInfo() << objName << " - AudioBackendGstreamer::stop -- Stoping playback without fading";
    gst_element_set_state(playBin, GST_STATE_NULL);
    if (m_cdgMode)
        cdgStop();
    emit stateChanged(MediaBackend::StoppedState);
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
    if (lastState != PlayingState)
    {
        if (m_lastPosition == 0)
            return;
        m_lastPosition = 0;
        emit positionChanged(0);
        return;
    }
    gint64 pos;
    if (!gst_element_query_position (playBin, GST_FORMAT_TIME, &pos))
    {
        m_lastPosition = 0;
        cdgSetPosition(0);
        emit positionChanged(0);
        return;
    }
    auto mspos = pos / 1000000;
    if (m_lastPosition != mspos)
    {
        m_lastPosition = mspos;
        emit positionChanged(mspos);
        if (m_cdgMode && (getCdgPosition() > mspos + 10 || getCdgPosition() < mspos - 10))
        {
            cdgSetPosition(mspos);
        }
    }
}

void MediaBackend::slowTimer_timeout()
{
    auto curState = state();
    if (lastState != curState)
        emit stateChanged(curState);
    if (m_silenceDetect)
    {
        if ((curState == MediaBackend::PlayingState) && (isSilent()))
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
    static int lastpos = 0;
    if (curState == PlayingState)
    {
        if (lastpos == position() && lastpos > 10)
        {
            qWarning() << objName << " - Playback appears to be hung, emitting end of stream";
            emit stateChanged(EndOfMediaState);
            if (m_cdgMode)
                cdgStop();
        }
        lastpos = position();
    }
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
    GList* plugins;
    GList* pnode;
    plugins = pnode = gst_registry_get_plugin_list(gst_registry_get());
    while(pnode) {
            list << gst_plugin_get_name(reinterpret_cast<GstPlugin*>(pnode->data));
            pnode = g_list_next(pnode);
    }
    gst_plugin_list_free (plugins);
    return list;
}

QStringList MediaBackend::GstGetElements(QString plugin)
{
    QStringList list;
    auto fnode = gst_registry_get_feature_list_by_plugin( gst_registry_get(), plugin.toUtf8().data());
    while(fnode) {
        if (fnode->data) {
            GstPluginFeature* feature = GST_PLUGIN_FEATURE(fnode->data);
            if (GST_IS_ELEMENT_FACTORY (feature)) {
                GstElementFactory* factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(feature));
                list << QString(GST_OBJECT_NAME(factory));
                qInfo() << QString(GST_OBJECT_NAME(factory));
            }
        }
        fnode = g_list_next(fnode);
    }
    gst_plugin_feature_list_free(fnode);
    return list;
}

int MediaBackend::gst_cb_bus_msg([[maybe_unused]]GstBus *bus, GstMessage *message, gpointer userData)
{
    auto backend = static_cast<MediaBackend*>(userData);
    auto objName = backend->objName;
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        GError *err;
        gchar *debug;
        gst_message_parse_error(message, &err, &debug);
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
            backend->audioError("Unable to play " + player + " file, missing gstreamer plugin");
            backend->stop(true);
        }
        g_error_free(err);
        g_free(debug);
        break;
    case GST_MESSAGE_WARNING:
        GError *err2;
        gchar *debug2;
        gst_message_parse_warning(message, &err2, &debug2);
        qInfo() << objName << " - Gst warning: " << err2->message;
        qInfo() << objName << " - Gst debug: " << debug2;
        g_error_free(err2);
        g_free(debug2);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        GstState state;
        gst_element_get_state(backend->playBin, &state, nullptr, GST_CLOCK_TIME_NONE);
        if (state == GST_STATE_PLAYING && backend->lastState != MediaBackend::PlayingState)
        {
            qInfo() << "GST notified of state change to PLAYING";
            if (backend->m_cdgMode && backend->cdgState() != PlayingState)
                backend->cdgPlay();
            backend->lastState = MediaBackend::PlayingState;
            backend->stateChanged(MediaBackend::PlayingState);
        }
        else if (state == GST_STATE_PAUSED && backend->lastState != MediaBackend::PausedState)
        {
            qInfo() << "GST notified of state change to PAUSED";
            if (backend->m_cdgMode && backend->cdgState() != PausedState)
                backend->cdgPause();
            backend->lastState = MediaBackend::PausedState;
            backend->stateChanged(MediaBackend::PausedState);
        }
        else if (state == GST_STATE_NULL && backend->lastState != MediaBackend::StoppedState)
        {
            qInfo() << "GST notified of state change to STOPPED";
            if (backend->lastState != MediaBackend::StoppedState)
            {
                backend->lastState = MediaBackend::StoppedState;
                if (backend->m_cdgMode && backend->cdgState() != StoppedState)
                    backend->cdgStop();
                backend->stateChanged(MediaBackend::StoppedState);
            }
        }
        else if (backend->lastState != MediaBackend::UnknownState)
        {
            //qInfo() << "GST notified of state change to UNKNOWN";
            //lastState = MediaBackend::UnknownState;
            //emit stateChanged(MediaBackend::UnknownState);
        }
        break;
    case GST_MESSAGE_ELEMENT:
        if (QString(gst_structure_get_name (gst_message_get_structure(message))) == "level")
        {
            auto array_val = gst_structure_get_value(gst_message_get_structure(message), "rms");
            auto rms_arr = reinterpret_cast<GValueArray*>(g_value_get_boxed (array_val));
            double rmsValues = 0.0;
            for (unsigned int i{0}; i < rms_arr->n_values; ++i)
            {
                auto value = g_value_array_get_nth (rms_arr, i);
                auto rms_dB = g_value_get_double (value);
                auto rms = pow (10, rms_dB / 20);
                rmsValues = rmsValues + rms;
            }
            backend->m_currentRmsLevel = rmsValues / rms_arr->n_values;
        }
        break;
    case GST_MESSAGE_STREAM_START:
        // workaround for gstreamer changing the volume unsolicited on playback start
        double cubicVolume;
        cubicVolume = gst_stream_volume_get_volume(GST_STREAM_VOLUME(backend->volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC);
        int intVol;
        intVol = cubicVolume * 100;
        if (backend->m_volume != intVol)
        {
            if ((!backend->fader->isFading()) && ((intVol < backend->m_volume - 1) || (intVol > backend->m_volume + 1)))
            {
                qInfo() << objName << " - stream start - Unrequested volume change detected, squashing";
                backend->setVolume(backend->m_volume);
            }
        }
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        gint64 dur, msdur;
        qInfo() << objName << " - GST reports duration changed";
        if (gst_element_query_duration(backend->playBin,GST_FORMAT_TIME,&dur))
            msdur = dur / 1000000;
        else
            msdur = 0;
        backend->durationChanged(msdur);
        break;
    case GST_MESSAGE_EOS:
        qInfo() << objName << " - state change to EndOfMediaState emitted";
        backend->stateChanged(EndOfMediaState);
        break;
    case GST_MESSAGE_NEED_CONTEXT:
        qInfo() << objName  << " - context requested - " << message->src->name;
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(backend->videoSink1), backend->videoWinId);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(backend->videoSink2), backend->videoWinId2);
        break;
    case GST_MESSAGE_TAG:
    case GST_MESSAGE_STREAM_STATUS:
    case GST_MESSAGE_LATENCY:
    case GST_MESSAGE_ASYNC_DONE:
    case GST_MESSAGE_NEW_CLOCK:
        break;
    default:
        qInfo() << objName << " - Gst msg type: " << GST_MESSAGE_TYPE(message) << " Gst msg name: " << GST_MESSAGE_TYPE_NAME(message) << " Element: " << message->src->name;
        break;
    }
  return true;
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
        videoSink1 = gst_element_factory_make("osxvideosink", "videosink1");
        videoSink2 = gst_element_factory_make("osxvideosink", "videosink2");
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
#ifdef Q_OS_LINUX
    pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
#endif
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
#ifdef Q_OS_LINUX
    if ((pitchShifterRubberBand) && (pitchShifterSoundtouch) && (m_loadPitchShift))
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
    else if ((pitchShifterSoundtouch) && (m_loadPitchShift))
#else
    if ((pitchShifterSoundtouch) && (loadPitchShift))
#endif
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
    auto pad = gst_element_get_static_pad(queueMainAudio, "sink");
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
    //auto bus = gst_element_get_bus(playBin);
    //gst_bus_set_sync_handler(bus.get(), busMessageDispatcher, this, nullptr);
    gst_bus_add_watch(gst_element_get_bus(playBin), (GstBusFunc)gst_cb_bus_msg, this);
    //g_signal_connect(bus, "message", G_CALLBACK(busMessageDispatcher), this);
    if (m_cdgMode)
    {
        gst_bus_add_watch(gst_element_get_bus(cdgPipeline), (GstBusFunc)gst_cb_bus_msg_cdg, this);
        //auto busCdg = takeGstObject(gst_element_get_bus(cdgPipeline));
        //gst_bus_set_sync_handler(bus.get(), busMessageDispatcherCdg, this, nullptr);
    }
    auto csource = gst_interpolation_control_source_new ();
    if (!csource)
        qInfo() << objName << " - Error createing control source";
    auto *cbind = gst_direct_control_binding_new(GST_OBJECT_CAST(faderVolumeElement), "volume", csource);
    if (!cbind)
        qInfo() << objName << " - Error creating control binding";
    if (!gst_object_add_control_binding (GST_OBJECT_CAST(faderVolumeElement), cbind))
        qInfo() << objName << " - Error adding control binding to volumeElement for fader control";
    g_object_set(csource, "mode", GST_INTERPOLATION_MODE_CUBIC, nullptr);
    setVolume(m_volume);
    slowTimer.start(1000);
    setOutputDevice(m_outputDeviceIdx);
    setEqBypass(m_bypass);
    setEqLevel1(m_eqLevels[0]);
    setEqLevel2(m_eqLevels[1]);
    setEqLevel3(m_eqLevels[2]);
    setEqLevel4(m_eqLevels[3]);
    setEqLevel5(m_eqLevels[4]);
    setEqLevel6(m_eqLevels[5]);
    setEqLevel7(m_eqLevels[6]);
    setEqLevel8(m_eqLevels[7]);
    setEqLevel9(m_eqLevels[8]);
    setEqLevel10(m_eqLevels[9]);
    setDownmix(m_downmix);
    setMuted(m_muted);
    videoMute(m_vidMuted);
    setVolume(m_volume);
    fastTimer.start(40);
    qInfo() << objName << " - buildPipeline() finished";
    setEnforceAspectRatio(settings->enforceAspectRatio());
}

void MediaBackend::destroyPipeline()
{
    qInfo() << objName << " - destroyPipeline() called";
    //gst_bus_remove_signal_watch(gst_element_get_bus(playBin));
//    if (cdgPipeline)
//        gst_bus_remove_signal_watch(gst_element_get_bus(cdgPipeline));
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

int MediaBackend::gst_cb_bus_msg_cdg([[maybe_unused]]GstBus *bus, GstMessage *message, gpointer userData)
{
    auto myObj = static_cast<MediaBackend*>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_NEED_CONTEXT:
        qInfo() << myObj->objName  << " - CDG pipeline - context requested - " << message->src->name;
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(myObj->videoSink1), myObj->videoWinId);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(myObj->videoSink2), myObj->videoWinId2);
        break;
    default:
        break;
    }

    return true;

 //   auto messagePtr = takeGstMiniObject(message);
 //   QMetaObject::invokeMethod(myObj, "busMessageCdg", Qt::QueuedConnection, Q_ARG(std::shared_ptr<GstMessage>, messagePtr));
 //   return true;
}

void MediaBackend::cb_need_data(GstElement *appsrc, [[maybe_unused]]guint unused_size, gpointer user_data)
{
    auto backend = reinterpret_cast<MediaBackend *>(user_data);
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    auto bufferSize = backend->cdg.videoFrameByIndex(backend->curFrame).sizeInBytes();
#else
    auto bufferSize = backend->cdg.videoFrameByIndex(backend->curFrame).byteCount();
#endif
    auto buffer = gst_buffer_new_and_alloc(bufferSize);
    gst_buffer_fill(buffer,
                    0,
                    backend->cdg.videoFrameByIndex(backend->curFrame).constBits(),
                    bufferSize
                    );
    GST_BUFFER_PTS(buffer) = backend->cdgPosition;
    GST_BUFFER_DURATION(buffer) = 40000000; // 40ms
    backend->cdgPosition += GST_BUFFER_DURATION(buffer);
    backend->curFrame++;
    gst_app_src_push_buffer(reinterpret_cast<GstAppSrc *>(appsrc), buffer);
}

gboolean MediaBackend::cb_seek_data([[maybe_unused]]GstElement *appsrc, guint64 position, gpointer user_data)
{
    auto backend = reinterpret_cast<MediaBackend *>(user_data);
    if (position == 0)
    {
        backend->curFrame = 0;
        backend->cdgPosition = 0;
        return true;
    }
    backend->curFrame = position / 40000000;
    backend->cdgPosition = position;
    return true;
}



void MediaBackend::fadeOut(bool waitForFade)
{
    gdouble curVolume;
    g_object_get(G_OBJECT(volumeElement), "volume", &curVolume, nullptr);
    if (state() != PlayingState)
    {
        qInfo() << objName << " - fadeOut - State not playing, skipping fade and setting volume directly";
        m_fader->immediateOut();
        return;
    }
    m_fader->fadeOut(waitForFade);
}

void MediaBackend::fadeIn(bool waitForFade)
{
    if (state() != PlayingState)
    {
        qInfo() << objName << " - fadeIn - State not playing, skipping fade and setting volume";
        m_fader->immediateIn();
        return;
    }
    if (isSilent())
    {
        qInfo() << objName << "- fadeOut - Audio is currently slient, skipping fade and setting volume immediately";
        m_fader->immediateIn();
        return;
    }
    m_fader->fadeIn(waitForFade);
}

bool MediaBackend::isSilent()
{
    if ((m_currentRmsLevel <= 0.01) && (m_volume > 0) && (!m_fader->isFading()))
        return true;
    return false;
}

void MediaBackend::setDownmix(bool enabled)
{
    m_downmix = enabled;
    if (enabled)
        g_object_set(fltrPostPanorama, "caps", audioCapsMono, nullptr);
    else
        g_object_set(fltrPostPanorama, "caps", audioCapsStereo, nullptr);
}

void MediaBackend::setTempo(int percent)
{
    m_tempo = percent;
    g_object_set(pitchShifterSoundtouch, "tempo", (float)percent / 100.0, nullptr);
    setPosition(position());
}

void MediaBackend::setOutputDevice(int deviceIndex)
{
    m_outputDeviceIdx = deviceIndex;
    gst_element_unlink(aConvEnd, audioSink);
    gst_bin_remove(GST_BIN(audioBin), audioSink);
    if (deviceIndex == 0)
    {
        audioSink = gst_element_factory_make("autoaudiosink", "audioSink");;
    }
    else
    {
        audioSink = gst_device_create_element(m_outputDevices.at(deviceIndex - 1), NULL);
    }
    gst_bin_add(GST_BIN(audioBin), audioSink);
    gst_element_link(aConvEnd, audioSink);
}



void MediaBackend::setMplxMode(int mode)
{
    switch (mode) {
    case Multiplex_Normal:
        g_object_set(audioPanorama, "panorama", 0.0, nullptr);
        setDownmix(settings->audioDownmix());
        break;
    case Multiplex_LeftChannel:
        setDownmix(true);
        g_object_set(audioPanorama, "panorama", -1.0, nullptr);
        break;
    case Multiplex_RightChannel:
        setDownmix(true);
        g_object_set(audioPanorama, "panorama", 1.0, nullptr);
        break;
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
        g_object_set(equalizer, "band1", (double)m_eqLevels[0], nullptr);
        g_object_set(equalizer, "band2", (double)m_eqLevels[1], nullptr);
        g_object_set(equalizer, "band3", (double)m_eqLevels[2], nullptr);
        g_object_set(equalizer, "band4", (double)m_eqLevels[3], nullptr);
        g_object_set(equalizer, "band5", (double)m_eqLevels[4], nullptr);
        g_object_set(equalizer, "band6", (double)m_eqLevels[5], nullptr);
        g_object_set(equalizer, "band7", (double)m_eqLevels[6], nullptr);
        g_object_set(equalizer, "band8", (double)m_eqLevels[7], nullptr);
        g_object_set(equalizer, "band0", (double)m_eqLevels[8], nullptr);
        g_object_set(equalizer, "band9", (double)m_eqLevels[9], nullptr);

    }
    this->m_bypass = bypass;
}

void MediaBackend::setEqLevel1(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band0", (double)level, nullptr);
    m_eqLevels[0] = level;
}

void MediaBackend::setEqLevel2(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band1", (double)level, nullptr);
    m_eqLevels[1] = level;
}

void MediaBackend::setEqLevel3(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band2", (double)level, nullptr);
    m_eqLevels[2] = level;
}

void MediaBackend::setEqLevel4(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band3", (double)level, nullptr);
    m_eqLevels[3] = level;
}

void MediaBackend::setEqLevel5(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band4", (double)level, nullptr);
    m_eqLevels[4] = level;
}

void MediaBackend::setEqLevel6(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band5", (double)level, nullptr);
    m_eqLevels[5] = level;
}

void MediaBackend::setEqLevel7(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band6", (double)level, nullptr);
    m_eqLevels[6] = level;
}

void MediaBackend::setEqLevel8(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band7", (double)level, nullptr);
    m_eqLevels[7] = level;
}

void MediaBackend::setEqLevel9(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band8", (double)level, nullptr);
    m_eqLevels[8] = level;
}

void MediaBackend::setEqLevel10(int level)
{
    if (!m_bypass)
        g_object_set(equalizer, "band9", (double)level, nullptr);
    m_eqLevels[9] = level;
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
    m_fader->immediateIn();
}

void MediaBackend::fadeOutImmediate()
{
    m_fader->immediateOut();
}

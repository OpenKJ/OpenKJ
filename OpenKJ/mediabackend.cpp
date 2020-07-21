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

#include "mediabackend.h"
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

//GstElement *playBinPub;


Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<GstMessage>);

MediaBackend::MediaBackend(bool pitchShift, QObject *parent, QString objectName) :
    QObject(parent), m_objName(objectName), m_loadPitchShift(pitchShift)
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
    if (!gst_is_initialized())
    {
        qInfo() << m_objName << " - gst not initialized - initializing";
        gst_init(nullptr,nullptr);
    }
    guint major, minor, micro, nano;
    gst_version (&major, &minor, &micro, &nano);
    qInfo() << "Using GStreamer version: " << major << "." << minor << "." << micro;
    m_faderVolumeElement = gst_element_factory_make("volume", "FaderVolumeElement");
    g_object_set(m_faderVolumeElement, "volume", 1.0, nullptr);
    m_fader = new AudioFader(this);
    m_fader->setObjName(m_objName + "Fader");
    m_fader->setVolumeElement(m_faderVolumeElement);
    gst_object_ref(m_faderVolumeElement);

    buildPipeline();

    auto monitor = gst_device_monitor_new ();
    auto moncaps = gst_caps_new_empty_simple ("audio/x-raw");
    gst_device_monitor_add_filter (monitor, "Audio/Sink", moncaps);
    gst_caps_unref (moncaps);
    gst_device_monitor_start (monitor);
    m_outputDeviceNames.clear();
    m_outputDeviceNames.append("0 - Default");
    GList *devices, *elem;
    devices = gst_device_monitor_get_devices(monitor);
    for(elem = devices; elem; elem = elem->next) {
        auto device = m_outputDevices.emplace_back(reinterpret_cast<GstDevice*>(elem->data));
        auto *deviceName = gst_device_get_display_name(device);
        m_outputDeviceNames.append(QString::number(m_outputDeviceNames.size()) + " - " + deviceName);
        g_free(deviceName);
    }
    g_object_set(m_playBin, "volume", 1.0, nullptr);
    gst_device_monitor_stop(monitor);
    g_object_unref(monitor);
    g_list_free(devices);
    connect(&m_timerSlow, &QTimer::timeout, this, &MediaBackend::timerSlow_timeout);
    connect(&m_timerFast, &QTimer::timeout, this, &MediaBackend::timerFast_timeout);
    connect(m_fader, &AudioFader::fadeStarted, [&] () {
        qInfo() << m_objName << " - Fader started";
    });
    connect(m_fader, &AudioFader::fadeComplete, [&] () {
        qInfo() << m_objName << " - fader finished";
    });
    connect(m_fader, &AudioFader::faderStateChanged, [&] (auto state) {
        qInfo() << m_objName << " - Fader state changed to: " << m_fader->stateToStr(state);
    });
    qInfo() << "Done constructing GStreamer backend";
}

void MediaBackend::videoMute(const bool &mute)
{
    gint flags;
    if (mute)
    {
        g_object_get (m_playBin, "flags", &flags, nullptr);
        flags |= GST_PLAY_FLAG_AUDIO;
        flags &= ~GST_PLAY_FLAG_TEXT;
        flags &= ~GST_PLAY_FLAG_VIDEO;
        g_object_set (m_playBin, "flags", flags, nullptr);
    }
    else
    {
        gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink1), m_videoWinId);
        if (m_previewEnabledLastBuild)
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink2), m_videoWinId2);
        g_object_get (m_playBin, "flags", &flags, nullptr);
        flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO;
        flags &= ~GST_PLAY_FLAG_TEXT;
        g_object_set (m_playBin, "flags", flags, nullptr);
    }
    m_vidMuted = mute;
}

double MediaBackend::getPitchForSemitone(const int &semitone)
{
    if (semitone > 0)
        return pow(STUP,semitone);
    else if (semitone < 0)
        return 1 - ((100 - (pow(STDN,abs(semitone)) * 100)) / 100);
    return 1.0;
}

void MediaBackend::setEnforceAspectRatio(const bool &enforce)
{
    g_object_set(m_videoSink1, "force-aspect-ratio", enforce, nullptr);
    if (m_previewEnabledLastBuild)
        g_object_set(m_videoSink2, "force-aspect-ratio", enforce, nullptr);
}

MediaBackend::~MediaBackend()
{
    m_timerSlow.stop();
    m_timerFast.stop();
    if (state() == PlayingState)
        stop(true);
    destroyPipeline();
    gst_object_unref(m_faderVolumeElement);
    if (m_videoSink1)
        gst_object_unref(m_videoSink1);
    if (m_videoSink2)
        gst_object_unref(m_videoSink2);
}

qint64 MediaBackend::position()
{
    gint64 pos;
    if (gst_element_query_position(m_playBin, GST_FORMAT_TIME, &pos))
        return pos / 1000000;
    return 0;
}

qint64 MediaBackend::getCdgPosition()
{
    gint64 pos;
    if (gst_element_query_position(m_cdgPipeline, GST_FORMAT_TIME, &pos))
        return pos / 1000000;
    return 0;
}

qint64 MediaBackend::duration()
{
    gint64 duration;
    if (gst_element_query_duration(m_playBin, GST_FORMAT_TIME, &duration))
        return duration / 1000000;
    return 0;
}

MediaBackend::State MediaBackend::state()
{
    GstState state = GST_STATE_NULL;
    gst_element_get_state(m_playBin, &state, nullptr, GST_CLOCK_TIME_NONE);
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
    gst_element_get_state(m_cdgPipeline, &state, nullptr, GST_CLOCK_TIME_NONE);
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
    qInfo() << m_objName << " - play() called";
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_playBin), GST_STREAM_VOLUME_FORMAT_LINEAR, 1.0);
    if (state() == MediaBackend::PausedState)
    {
        qInfo() << m_objName << " - play - playback is currently paused, unpausing";
        gst_element_set_state(m_playBin, GST_STATE_PLAYING);
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
    g_object_set(m_playBin, "uri", uri, nullptr);
    g_free(uri);
    if (!m_cdgMode)
    {
        qInfo() << m_objName << " - play - playing media: " << m_filename;
        gst_element_set_state(m_playBin, GST_STATE_PLAYING);
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
        m_cdg.open(m_cdgFilename);
        m_cdg.process();
        qInfo() << m_objName << " - play - playing cdg:   " << m_cdgFilename;
        qInfo() << m_objName << " - play - playing audio: " << m_filename;
        gst_element_set_state(m_playBin, GST_STATE_PLAYING);
        gst_element_set_state(m_cdgPipeline, GST_STATE_PLAYING);
    }
}

void MediaBackend::cdgPlay()
{
    gst_element_set_state(m_cdgPipeline, GST_STATE_PLAYING);
}

void MediaBackend::pause()
{
    if (m_fade)
        fadeOut();
    gst_element_set_state(m_playBin, GST_STATE_PAUSED);
    if (m_cdgMode)
        cdgPause();
}

void MediaBackend::cdgPause()
{
    gst_element_set_state(m_cdgPipeline, GST_STATE_PAUSED);
}

void MediaBackend::setMedia(const QString &filename)
{
    m_cdgMode = false;
    m_filename = filename;
}

void MediaBackend::setMediaCdg(const QString &cdgFilename, const QString &audioFilename)
{
    m_cdgMode = true;
    m_filename = audioFilename;
    m_cdgFilename = cdgFilename;
}

void MediaBackend::setMuted(const bool &muted)
{
    gst_stream_volume_set_mute(GST_STREAM_VOLUME(m_volumeElement), muted);
}

void MediaBackend::setPosition(const qint64 &position)
{
    gst_element_seek_simple(m_playBin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position);
    if (m_cdgMode)
        cdgSetPosition(position);
    emit positionChanged(position);
}

void MediaBackend::cdgSetPosition(const qint64 &position)
{
    gst_element_seek_simple(m_cdgPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position);
}

void MediaBackend::setVolume(const int &volume)
{
    m_volume = volume;
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_volumeElement), GST_STREAM_VOLUME_FORMAT_CUBIC, volume * .01);
    emit volumeChanged(volume);
}

void MediaBackend::stop(const bool &skipFade)
{
    qInfo() << m_objName << " - AudioBackendGstreamer::stop(" << skipFade << ") called";
    if (state() == MediaBackend::StoppedState)
    {
        qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Already stopped, skipping";
        emit stateChanged(MediaBackend::StoppedState);
        return;
    }
    if (state() == MediaBackend::PausedState)
    {
        qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Stopping paused song";
        gst_element_set_state(m_playBin, GST_STATE_NULL);
        if (m_cdgMode)
            cdgStop();
        emit stateChanged(MediaBackend::StoppedState);
        qInfo() << m_objName << " - stop() completed";
        m_fader->immediateIn();
        return;
    }
    if ((m_fade) && (!skipFade) && (state() == MediaBackend::PlayingState))
    {
        if (m_fader->state() == AudioFader::FadedIn || m_fader->state() == AudioFader::FadingIn)
        {
            qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Fading enabled.  Fading out audio volume";
            fadeOut(true);
            qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Fading complete";
            qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Stoping playback";
            gst_element_set_state(m_playBin, GST_STATE_NULL);
            if (m_cdgMode)
                cdgStop();
            emit stateChanged(MediaBackend::StoppedState);
            m_fader->immediateIn();
            return;
        }
    }
    qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Stoping playback without fading";
    gst_element_set_state(m_playBin, GST_STATE_NULL);
    if (m_cdgMode)
        cdgStop();
    emit stateChanged(MediaBackend::StoppedState);
    qInfo() << m_objName << " - stop() completed";
}

void MediaBackend::cdgStop()
{
    gst_element_set_state(m_cdgPipeline, GST_STATE_NULL);
}

void MediaBackend::rawStop()
{
    qInfo() << m_objName << " - rawStop() called, just ending gstreamer playback";
    gst_element_set_state(m_playBin, GST_STATE_NULL);
    if (m_cdgMode)
        cdgStop();
}

void MediaBackend::timerFast_timeout()
{
    if (m_lastState != PlayingState)
    {
        if (m_lastPosition == 0)
            return;
        m_lastPosition = 0;
        emit positionChanged(0);
        return;
    }
    gint64 pos;
    if (!gst_element_query_position (m_playBin, GST_FORMAT_TIME, &pos))
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

void MediaBackend::timerSlow_timeout()
{
    if (m_silenceDetect)
    {
        if ((state() == MediaBackend::PlayingState) && (isSilent()))
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
    if (state() == PlayingState)
    {
        if (lastpos == position() && lastpos > 10)
        {
            qWarning() << m_objName << " - Playback appears to be hung, emitting end of stream";
            emit stateChanged(EndOfMediaState);
            if (m_cdgMode)
                cdgStop();
        }
        lastpos = position();
    }
}

void MediaBackend::setPitchShift(const int &pitchShift)
{
    if (m_keyChangerRubberBand)
        g_object_set(m_pitchShifterRubberBand, "semitones", pitchShift, nullptr);
    else if (m_keyChangerSoundtouch)
    {
        g_object_set(m_pitchShifterSoundtouch, "pitch", getPitchForSemitone(pitchShift), nullptr);
    }
    emit pitchChanged(pitchShift);
}

int MediaBackend::gst_cb_bus_msg([[maybe_unused]]GstBus *bus, GstMessage *message, gpointer userData)
{
    auto backend = static_cast<MediaBackend*>(userData);
    auto objName = backend->m_objName;
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        GError *err;
        gchar *debug;
        gst_message_parse_error(message, &err, &debug);
        qInfo() << objName << " - Gst error: " << err->message;
        qInfo() << objName << " - Gst debug: " << debug;
        if (QString(err->message) == "Your GStreamer installation is missing a plug-in.")
        {
            QString player = (objName == "KAR") ? "karaoke" : "break music";
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
        gst_element_get_state(backend->m_playBin, &state, nullptr, GST_CLOCK_TIME_NONE);
        if (state == GST_STATE_PLAYING && backend->m_lastState != MediaBackend::PlayingState)
        {
            qInfo() << "GST notified of state change to PLAYING";
            if (backend->m_cdgMode && backend->cdgState() != PlayingState)
                backend->cdgPlay();
            backend->m_lastState = MediaBackend::PlayingState;
            backend->stateChanged(MediaBackend::PlayingState);
        }
        else if (state == GST_STATE_PAUSED && backend->m_lastState != MediaBackend::PausedState)
        {
            qInfo() << "GST notified of state change to PAUSED";
            if (backend->m_cdgMode && backend->cdgState() != PausedState)
                backend->cdgPause();
            backend->m_lastState = MediaBackend::PausedState;
            backend->stateChanged(MediaBackend::PausedState);
        }
        else if (state == GST_STATE_NULL && backend->m_lastState != MediaBackend::StoppedState)
        {
            qInfo() << "GST notified of state change to STOPPED";
            if (backend->m_lastState != MediaBackend::StoppedState)
            {
                backend->m_lastState = MediaBackend::StoppedState;
                if (backend->m_cdgMode && backend->cdgState() != StoppedState)
                    backend->cdgStop();
                backend->stateChanged(MediaBackend::StoppedState);
            }
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
                rmsValues += rms;
            }
            backend->m_currentRmsLevel = rmsValues / rms_arr->n_values;
        }
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        gint64 dur, msdur;
        qInfo() << objName << " - GST reports duration changed";
        if (gst_element_query_duration(backend->m_playBin,GST_FORMAT_TIME,&dur))
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
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(backend->m_videoSink1), backend->m_videoWinId);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(backend->m_videoSink2), backend->m_videoWinId2);
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
    qInfo() << m_objName << " - buildPipeline() called";
    if (!gst_is_initialized())
    {
        qInfo() << m_objName << " - gst not initialized - initializing";
        gst_init(nullptr,nullptr);
    }
#if defined(Q_OS_LINUX)
        switch (m_accelMode) {
        case OpenGL:
            m_videoSink1 = gst_element_factory_make("glimagesink", "videoSink1");
            m_videoSink2 = gst_element_factory_make("glimagesink", "videoSink2");
            break;
        case XVideo:
            m_videoSink1 = gst_element_factory_make("xvimagesink", "videoSink1");
            m_videoSink2 = gst_element_factory_make("xvimagesink", "videoSink2");
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
    gst_object_ref(m_videoSink1);
    gst_object_ref(m_videoSink2);
    m_previewEnabledLastBuild = m_settings.previewEnabled();
    auto aConvInput = gst_element_factory_make("audioconvert", "aConvInput");
    auto aConvPrePitchShift = gst_element_factory_make("audioconvert", "aConvPrePitchShift");
    auto aConvPostPitchShift = gst_element_factory_make("audioconvert", "aConvPostPitchShift");
    m_audioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    auto rgVolume = gst_element_factory_make("rgvolume", "rgVolume");
    g_object_set(rgVolume, "pre-amp", 6.0, "headroom", 10.0, nullptr);
    auto rgLimiter = gst_element_factory_make("rglimiter", "rgLimiter");
    auto level = gst_element_factory_make("level", "level");
    m_pitchShifterSoundtouch = gst_element_factory_make("pitch", "pitch");
#ifdef Q_OS_LINUX
    m_pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
#endif
    m_equalizer = gst_element_factory_make("equalizer-10bands", "equalizer");
    m_playBin = gst_element_factory_make("playbin", "playBin");
    m_audioCapsStereo = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, nullptr);
    m_audioCapsMono = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 1, nullptr);
    m_audioBin = gst_bin_new("audioBin");
    auto aConvPostPanorama = gst_element_factory_make("audioconvert", "aConvPostPanorama");
    m_aConvEnd = gst_element_factory_make("audioconvert", "aConvEnd");
    m_fltrPostPanorama = gst_element_factory_make("capsfilter", "fltrPostPanorama");
    g_object_set(m_fltrPostPanorama, "caps", m_audioCapsStereo, nullptr);
    m_volumeElement = gst_element_factory_make("volume", "volumeElement");
    auto queueMainAudio = gst_element_factory_make("queue", "queueMainAudio");
    auto queueEndAudio = gst_element_factory_make("queue", "queueEndAudio");
    m_audioPanorama = gst_element_factory_make("audiopanorama", "audioPanorama");
    g_object_set(m_audioPanorama, "method", 1, nullptr);
    gst_bin_add_many(GST_BIN(m_audioBin),queueMainAudio, m_audioPanorama, level, aConvInput, rgVolume, rgLimiter, m_volumeElement, m_equalizer, aConvPostPanorama, m_fltrPostPanorama, gst_object_ref(m_faderVolumeElement), nullptr);
    gst_element_link(queueMainAudio, aConvInput);
    gst_element_link(aConvInput, rgVolume);
    gst_element_link(rgVolume, rgLimiter);
    gst_element_link(rgLimiter, level);
    gst_element_link(level, m_volumeElement);
    gst_element_link(m_volumeElement, m_equalizer);
    gst_element_link(m_equalizer, m_faderVolumeElement);
    gst_element_link(m_faderVolumeElement, m_audioPanorama);
    gst_element_link(m_audioPanorama, aConvPostPanorama);
    gst_element_link(aConvPostPanorama, m_fltrPostPanorama);
#ifdef Q_OS_LINUX
    if ((m_pitchShifterRubberBand) && (m_pitchShifterSoundtouch) && (m_loadPitchShift))
    {
        qInfo() << m_objName << " - Pitch shift RubberBand enabled";
        qInfo() << m_objName << " - Also loaded SoundTouch for tempo control";
        m_canChangeTempo = true;
        gst_bin_add_many(GST_BIN(m_audioBin), aConvPrePitchShift, m_pitchShifterRubberBand, aConvPostPitchShift, m_pitchShifterSoundtouch, m_aConvEnd, queueEndAudio, m_audioSink, nullptr);
        gst_element_link_many(m_fltrPostPanorama, aConvPrePitchShift, m_pitchShifterRubberBand, aConvPostPitchShift, m_pitchShifterSoundtouch, queueEndAudio, m_aConvEnd, m_audioSink, nullptr);
        m_canKeyChange = true;
        m_keyChangerRubberBand = true;
        g_object_set(m_pitchShifterRubberBand, "formant-preserving", true, nullptr);
        g_object_set(m_pitchShifterRubberBand, "crispness", 1, nullptr);
        g_object_set(m_pitchShifterRubberBand, "semitones", 0, nullptr);
    }
    else if ((m_pitchShifterSoundtouch) && (m_loadPitchShift))
#else
    if ((pitchShifterSoundtouch) && (loadPitchShift))
#endif
    {
        m_canChangeTempo = true;
        qInfo() << m_objName << " - Pitch shifter SoundTouch enabled";
        gst_bin_add_many(GST_BIN(m_audioBin), aConvPrePitchShift, m_pitchShifterSoundtouch, aConvPostPitchShift, m_aConvEnd, queueEndAudio, m_audioSink, nullptr);
        gst_element_link_many(m_fltrPostPanorama, aConvPrePitchShift, m_pitchShifterSoundtouch, queueEndAudio, m_aConvEnd, m_audioSink, nullptr);
        m_canKeyChange = true;
        m_keyChangerSoundtouch = true;
        g_object_set(m_pitchShifterSoundtouch, "pitch", 1.0, "tempo", 1.0, nullptr);
    }
    else
    {
        gst_bin_add_many(GST_BIN(m_audioBin), m_aConvEnd, queueEndAudio, m_audioSink, nullptr);
        gst_element_link_many(m_fltrPostPanorama, queueEndAudio, m_aConvEnd, m_audioSink, nullptr);
    }
    auto pad = gst_element_get_static_pad(queueMainAudio, "sink");
    auto ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(m_audioBin, ghostPad);
    gst_object_unref(pad);
    g_object_set(m_playBin, "audio-sink", m_audioBin, nullptr);


    // CDG video pipeline
    if (m_cdgMode)
    {
        m_cdgPipeline = gst_pipeline_new("cdgPipeline");
        auto cdgAppSrc = gst_element_factory_make("appsrc", "cdgAppSrc");
        auto cdgVidConv = gst_element_factory_make("videoconvert", "cdgVideoConv");
        g_object_set(G_OBJECT(cdgAppSrc), "caps",
                     gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB16", "width", G_TYPE_INT, 288, "height",
                                         G_TYPE_INT, 192, "framerate", GST_TYPE_FRACTION, 1, 30, NULL),
                     NULL);
        if (m_settings.previewEnabled())
        {
            auto videoQueue1 = gst_element_factory_make("queue", "videoQueue1");
            auto videoQueue2 = gst_element_factory_make("queue", "videoQueue2");
            auto videoConv1 = gst_element_factory_make("videoconvert", "preOutVideoConvert1");
            auto videoConv2 = gst_element_factory_make("videoconvert", "preOutVideoConvert2");
            m_videoTee = gst_element_factory_make("tee", "videoTee");
            m_videoTeePad1 = gst_element_get_request_pad(m_videoTee, "src_%u");
            m_videoTeePad2 = gst_element_get_request_pad(m_videoTee, "src_%u");
            m_videoQueue1SrcPad = gst_element_get_static_pad(videoQueue1, "sink");
            m_videoQueue2SrcPad = gst_element_get_static_pad(videoQueue2, "sink");
            m_videoBin = gst_bin_new("videoBin");
            gst_bin_add_many(reinterpret_cast<GstBin *>(m_cdgPipeline), cdgAppSrc, cdgVidConv, videoConv1, videoConv2, m_videoTee,videoQueue1,videoQueue2,m_videoSink1,m_videoSink2,nullptr);
            gst_element_link(cdgAppSrc, cdgVidConv);
            gst_element_link(cdgVidConv, m_videoTee);
            gst_pad_link(m_videoTeePad1,m_videoQueue1SrcPad);
            gst_pad_link(m_videoTeePad2,m_videoQueue2SrcPad);
            gst_element_link(videoQueue1,videoConv1);
            gst_element_link(videoQueue2,videoConv2);
            gst_element_link(videoConv1, m_videoSink1);
            gst_element_link(videoConv2, m_videoSink2);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink1), m_videoWinId);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink2), m_videoWinId2);
        }
        else
        {
            gst_bin_add_many(reinterpret_cast<GstBin *>(m_cdgPipeline), cdgAppSrc, cdgVidConv, m_videoSink1, nullptr);
            gst_element_link_many(cdgAppSrc, cdgVidConv, m_videoSink1, nullptr);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink1), m_videoWinId);
        }
        g_object_set(G_OBJECT(cdgAppSrc), "stream-type", 1, "format", GST_FORMAT_TIME, NULL);
        g_signal_connect(cdgAppSrc, "need-data", G_CALLBACK(cb_need_data), this);
        g_signal_connect(cdgAppSrc, "seek-data", G_CALLBACK(cb_seek_data), this);
    }
    else
    {
        if (m_settings.previewEnabled())
        {
            auto queueMainVideo = gst_element_factory_make("queue", "queueMainVideo");
            auto videoQueue1 = gst_element_factory_make("queue", "videoQueue1");
            auto videoQueue2 = gst_element_factory_make("queue", "videoQueue2");
            m_videoTee = gst_element_factory_make("tee", "videoTee");
            m_videoTeePad1 = gst_element_get_request_pad(m_videoTee, "src_%u");
            m_videoTeePad2 = gst_element_get_request_pad(m_videoTee, "src_%u");
            m_videoQueue1SrcPad = gst_element_get_static_pad(videoQueue1, "sink");
            m_videoQueue2SrcPad = gst_element_get_static_pad(videoQueue2, "sink");
            m_videoBin = gst_bin_new("videoBin");
            gst_bin_add_many(GST_BIN(m_videoBin),queueMainVideo,m_videoTee,videoQueue1,videoQueue2,m_videoSink1,m_videoSink2,nullptr);
            gst_element_link(queueMainVideo, m_videoTee);
            gst_pad_link(m_videoTeePad1,m_videoQueue1SrcPad);
            gst_pad_link(m_videoTeePad2,m_videoQueue2SrcPad);
            gst_element_link(videoQueue1,m_videoSink1);
            gst_element_link(videoQueue2,m_videoSink2);
            auto ghostVideoPad = gst_ghost_pad_new("sink", gst_element_get_static_pad(queueMainVideo, "sink"));
            gst_pad_set_active(ghostVideoPad,true);
            gst_element_add_pad(m_videoBin, ghostVideoPad);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink1), m_videoWinId);
            gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink2), m_videoWinId2);
            g_object_set(m_playBin, "video-sink", m_videoBin, nullptr);
        }
        else
        {
            qInfo() << "Main window preview disabled, building pipeline without video tee";
            gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(m_videoSink1), m_videoWinId);
            g_object_set(m_playBin, "video-sink", m_videoSink1, nullptr);
        }
    }
// End video output setup

    g_object_set(rgVolume, "album-mode", false, nullptr);
    g_object_set(level, "message", TRUE, nullptr);
    gst_bus_add_watch(gst_element_get_bus(m_playBin), (GstBusFunc)gst_cb_bus_msg, this);
    if (m_cdgMode)
        gst_bus_add_watch(gst_element_get_bus(m_cdgPipeline), (GstBusFunc)gst_cb_bus_msg_cdg, this);
    auto csource = gst_interpolation_control_source_new ();
    if (!csource)
        qInfo() << m_objName << " - Error createing control source";
    auto *cbind = gst_direct_control_binding_new(GST_OBJECT_CAST(m_faderVolumeElement), "volume", csource);
    if (!cbind)
        qInfo() << m_objName << " - Error creating control binding";
    if (!gst_object_add_control_binding (GST_OBJECT_CAST(m_faderVolumeElement), cbind))
        qInfo() << m_objName << " - Error adding control binding to volumeElement for fader control";
    g_object_set(csource, "mode", GST_INTERPOLATION_MODE_CUBIC, nullptr);
    setVolume(m_volume);
    m_timerSlow.start(1000);
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
    videoMute(m_vidMuted);
    setVolume(m_volume);
    m_timerFast.start(40);
    qInfo() << m_objName << " - buildPipeline() finished";
    setEnforceAspectRatio(m_settings.enforceAspectRatio());
    m_cdgModeLastBuild = m_cdgMode;
}

void MediaBackend::destroyPipeline()
{
    qInfo() << m_objName << " - destroyPipeline() called";
    m_timerSlow.stop();
    m_timerFast.stop();
    gst_bin_remove(reinterpret_cast<GstBin *>(m_audioBin), m_faderVolumeElement);
    if (state() == PlayingState)
        stop(true);
    if (m_previewEnabledLastBuild)
    {
        gst_element_release_request_pad(m_videoTee, m_videoTeePad1);
        gst_element_release_request_pad(m_videoTee, m_videoTeePad2);
        gst_object_unref(m_videoTeePad1);
        gst_object_unref(m_videoTeePad2);
        gst_object_unref(m_videoQueue1SrcPad);
        gst_object_unref(m_videoQueue2SrcPad);
        if (!m_cdgModeLastBuild)
        {
            gst_bin_remove(reinterpret_cast<GstBin *>(m_videoBin), m_videoSink1);
            gst_bin_remove(reinterpret_cast<GstBin *>(m_videoBin), m_videoSink2);
        }
        else
        {
            gst_bin_remove(reinterpret_cast<GstBin *>(m_cdgPipeline), m_videoSink1);
            gst_bin_remove(reinterpret_cast<GstBin *>(m_cdgPipeline), m_videoSink2);
        }
        gst_object_unref(m_videoBin);
    }
    gst_caps_unref(m_audioCapsMono);
    gst_caps_unref(m_audioCapsStereo);
    gst_object_unref(m_audioBin);
    if (m_cdgModeLastBuild)
        gst_object_unref(m_cdgPipeline);
    qInfo() << m_objName << " - destroyPipeline() finished";
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
        qInfo() << myObj->m_objName  << " - CDG pipeline - context requested - " << message->src->name;
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(myObj->m_videoSink1), myObj->m_videoWinId);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(myObj->m_videoSink2), myObj->m_videoWinId2);
        break;
    default:
        break;
    }

    return true;
}

void MediaBackend::cb_need_data(GstElement *appsrc, [[maybe_unused]]guint unused_size, gpointer user_data)
{
    auto backend = reinterpret_cast<MediaBackend *>(user_data);
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    auto bufferSize = backend->m_cdg.videoFrameByIndex(backend->curFrame).sizeInBytes();
#else
    auto bufferSize = backend->m_cdg.videoFrameByIndex(backend->curFrame).byteCount();
#endif
    auto buffer = gst_buffer_new_and_alloc(bufferSize);
    gst_buffer_fill(buffer,
                    0,
                    backend->m_cdg.videoFrameByIndex(backend->curFrame).constBits(),
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



void MediaBackend::fadeOut(const bool &waitForFade)
{
    gdouble curVolume;
    g_object_get(G_OBJECT(m_volumeElement), "volume", &curVolume, nullptr);
    if (state() != PlayingState)
    {
        qInfo() << m_objName << " - fadeOut - State not playing, skipping fade and setting volume directly";
        m_fader->immediateOut();
        return;
    }
    m_fader->fadeOut(waitForFade);
}

void MediaBackend::fadeIn(const bool &waitForFade)
{
    if (state() != PlayingState)
    {
        qInfo() << m_objName << " - fadeIn - State not playing, skipping fade and setting volume";
        m_fader->immediateIn();
        return;
    }
    if (isSilent())
    {
        qInfo() << m_objName << "- fadeOut - Audio is currently slient, skipping fade and setting volume immediately";
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

void MediaBackend::setDownmix(const bool &enabled)
{
    m_downmix = enabled;
    g_object_set(m_fltrPostPanorama, "caps", (enabled) ? m_audioCapsMono : m_audioCapsStereo, nullptr);
}

void MediaBackend::setTempo(const int &percent)
{
    m_tempo = percent;
    g_object_set(m_pitchShifterSoundtouch, "tempo", (float)percent / 100.0, nullptr);
    setPosition(position());
}

void MediaBackend::setOutputDevice(int deviceIndex)
{
    m_outputDeviceIdx = deviceIndex;
    gst_element_unlink(m_aConvEnd, m_audioSink);
    gst_bin_remove(GST_BIN(m_audioBin), m_audioSink);
    if (deviceIndex == 0)
    {
        m_audioSink = gst_element_factory_make("autoaudiosink", "audioSink");;
    }
    else
    {
        m_audioSink = gst_device_create_element(m_outputDevices.at(deviceIndex - 1), NULL);
    }
    gst_bin_add(GST_BIN(m_audioBin), m_audioSink);
    gst_element_link(m_aConvEnd, m_audioSink);
}



void MediaBackend::setMplxMode(const int &mode)
{
    switch (mode) {
    case Multiplex_Normal:
        g_object_set(m_audioPanorama, "panorama", 0.0, nullptr);
        setDownmix(m_settings.audioDownmix());
        break;
    case Multiplex_LeftChannel:
        setDownmix(true);
        g_object_set(m_audioPanorama, "panorama", -1.0, nullptr);
        break;
    case Multiplex_RightChannel:
        setDownmix(true);
        g_object_set(m_audioPanorama, "panorama", 1.0, nullptr);
        break;
    }
}


void MediaBackend::setEqBypass(const bool &bypass)
{
    if (bypass)
    {
        g_object_set(m_equalizer, "band0", 0.0, nullptr);
        g_object_set(m_equalizer, "band1", 0.0, nullptr);
        g_object_set(m_equalizer, "band2", 0.0, nullptr);
        g_object_set(m_equalizer, "band3", 0.0, nullptr);
        g_object_set(m_equalizer, "band4", 0.0, nullptr);
        g_object_set(m_equalizer, "band5", 0.0, nullptr);
        g_object_set(m_equalizer, "band6", 0.0, nullptr);
        g_object_set(m_equalizer, "band7", 0.0, nullptr);
        g_object_set(m_equalizer, "band8", 0.0, nullptr);
        g_object_set(m_equalizer, "band0", 0.0, nullptr);
    }
    else
    {
        g_object_set(m_equalizer, "band1", (double)m_eqLevels[0], nullptr);
        g_object_set(m_equalizer, "band2", (double)m_eqLevels[1], nullptr);
        g_object_set(m_equalizer, "band3", (double)m_eqLevels[2], nullptr);
        g_object_set(m_equalizer, "band4", (double)m_eqLevels[3], nullptr);
        g_object_set(m_equalizer, "band5", (double)m_eqLevels[4], nullptr);
        g_object_set(m_equalizer, "band6", (double)m_eqLevels[5], nullptr);
        g_object_set(m_equalizer, "band7", (double)m_eqLevels[6], nullptr);
        g_object_set(m_equalizer, "band8", (double)m_eqLevels[7], nullptr);
        g_object_set(m_equalizer, "band0", (double)m_eqLevels[8], nullptr);
        g_object_set(m_equalizer, "band9", (double)m_eqLevels[9], nullptr);

    }
    this->m_bypass = bypass;
}

void MediaBackend::setEqLevel1(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band0", (double)level, nullptr);
    m_eqLevels[0] = level;
}

void MediaBackend::setEqLevel2(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band1", (double)level, nullptr);
    m_eqLevels[1] = level;
}

void MediaBackend::setEqLevel3(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band2", (double)level, nullptr);
    m_eqLevels[2] = level;
}

void MediaBackend::setEqLevel4(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band3", (double)level, nullptr);
    m_eqLevels[3] = level;
}

void MediaBackend::setEqLevel5(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band4", (double)level, nullptr);
    m_eqLevels[4] = level;
}

void MediaBackend::setEqLevel6(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band5", (double)level, nullptr);
    m_eqLevels[5] = level;
}

void MediaBackend::setEqLevel7(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band6", (double)level, nullptr);
    m_eqLevels[6] = level;
}

void MediaBackend::setEqLevel8(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band7", (double)level, nullptr);
    m_eqLevels[7] = level;
}

void MediaBackend::setEqLevel9(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band8", (double)level, nullptr);
    m_eqLevels[8] = level;
}

void MediaBackend::setEqLevel10(const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, "band9", (double)level, nullptr);
    m_eqLevels[9] = level;
}


bool MediaBackend::hasVideo()
{
    if (m_cdgMode)
        return true;
    gint numVidStreams;
    g_object_get(m_playBin, "n-video", &numVidStreams, nullptr);
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

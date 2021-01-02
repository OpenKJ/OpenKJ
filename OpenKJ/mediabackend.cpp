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


Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<GstMessage>);

std::atomic<size_t> g_appSrcCurFrame{0};
std::atomic<uint64_t> g_appSrcCurPosition{0};
std::atomic<bool> g_appSrcNeedData{false};

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
    Settings settings;
    if (!settings.hardwareAccelEnabled())
    {
        qInfo() << "Hardware accelerated video rendering disabled";
        m_videoAccelEnabled = false;
    }
    else
    {
        qInfo() << "Hardware accelerated video rendering enabled";
        m_videoAccelEnabled = true;
    }
    QMetaTypeId<std::shared_ptr<GstMessage>>::qt_metatype_id();
    buildPipeline();
    //buildCdgBin();
    getGstDevices();
    qInfo() << "Done constructing GStreamer backend";
    connect(&m_timerSlow, &QTimer::timeout, this, &MediaBackend::timerSlow_timeout);
    connect(&m_timerFast, &QTimer::timeout, this, &MediaBackend::timerFast_timeout);
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
        gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink1), m_videoWinId1);
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
    m_enforceAspectRatio = enforce;
    g_object_set(m_videoSink1, "force-aspect-ratio", enforce, nullptr);
    g_object_set(m_videoSink2, "force-aspect-ratio", enforce, nullptr);
    g_object_set(m_videoSink1Cdg, "force-aspect-ratio", enforce, nullptr);
    g_object_set(m_videoSink2Cdg, "force-aspect-ratio", enforce, nullptr);
}

MediaBackend::~MediaBackend()
{
    m_timerSlow.stop();
    m_timerFast.stop();
    if (state() == PlayingState)
        stop(true);
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
    //gint64 pos;
   // if (gst_element_query_position(m_cdgPipeline, GST_FORMAT_TIME, &pos))
   //     return (pos / 1000000);
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

void MediaBackend::testCdgDecode()
{
    m_cdg.open(m_cdgFilename);
    m_cdg.process();
}

void MediaBackend::newFrame()
{
    GstSample* sample = gst_app_sink_pull_sample((GstAppSink*)m_videoAppSink);

    if (sample) {
        if (m_vidMuted)
        {
            gst_sample_unref(sample);
            return;
        }
        m_noaccelHasVideo = true;
        GstBuffer *buffer;
        GstCaps *caps;
        GstStructure *s;
        int width, height;
        caps = gst_sample_get_caps (sample);
        s = gst_caps_get_structure (caps, 0);
        gst_structure_get_int (s, "width", &width);
        gst_structure_get_int (s, "height", &height);
        buffer = gst_sample_get_buffer (sample);
        GstMapInfo bufferInfo;
        gst_buffer_map(buffer,&bufferInfo,GST_MAP_READ);
        guint8 *rawFrame = bufferInfo.data;
        QImage frame = QImage(rawFrame,width,height,QImage::Format_RGB16);
        emit newVideoFrame(frame, m_objName);
        gst_buffer_unmap(buffer, &bufferInfo);
        gst_sample_unref(sample);
    }
}

void MediaBackend::newFrameCdg()
{
    GstSample* sample = gst_app_sink_pull_sample((GstAppSink*)m_videoAppSinkCdg);

    if (sample) {
        if (m_vidMuted)
        {
            gst_sample_unref(sample);
            return;
        }
        m_noaccelHasVideo = true;
        GstBuffer *buffer;
        GstCaps *caps;
        GstStructure *s;
        int width, height;
        caps = gst_sample_get_caps (sample);
        s = gst_caps_get_structure (caps, 0);
        gst_structure_get_int (s, "width", &width);
        gst_structure_get_int (s, "height", &height);
        buffer = gst_sample_get_buffer (sample);
        GstMapInfo bufferInfo;
        gst_buffer_map(buffer,&bufferInfo,GST_MAP_READ);
        guint8 *rawFrame = bufferInfo.data;
        QImage frame = QImage(rawFrame,width,height,QImage::Format_RGB16);
        emit newVideoFrame(frame, m_objName);
        gst_buffer_unmap(buffer, &bufferInfo);
        gst_sample_unref(sample);
    }
}

MediaBackend::State MediaBackend::cdgState()
{
    return MediaBackend::PlayingState;
    GstState state = GST_STATE_NULL;
   // gst_element_get_state(m_cdgPipeline, &state, nullptr, GST_CLOCK_TIME_NONE);
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

void MediaBackend::EndOfStreamCallback(GstAppSink *appsink, gpointer user_data)
{
    Q_UNUSED(appsink)
    Q_UNUSED(user_data)
}

GstFlowReturn MediaBackend::NewPrerollCallback(GstAppSink *appsink, gpointer user_data)
{
    Q_UNUSED(user_data)
    Q_UNUSED(appsink)
    return GST_FLOW_OK;
}

GstFlowReturn MediaBackend::NewSampleCallback(GstAppSink *appsink, gpointer user_data)
{
    Q_UNUSED(appsink)
    MediaBackend *myObject = (MediaBackend*) user_data;
    myObject->newFrame();
    //qInfo() << "got new frame";
    return GST_FLOW_OK;
}

GstFlowReturn MediaBackend::NewSampleCallbackCdg(GstAppSink *appsink, gpointer user_data)
{
    Q_UNUSED(appsink)
    MediaBackend *myObject = (MediaBackend*) user_data;
    myObject->newFrameCdg();
    //qInfo() << "got new frame";
    return GST_FLOW_OK;
}

GstFlowReturn MediaBackend::NewAudioSampleCallback(GstAppSink *appsink, gpointer user_data)
{
    Q_UNUSED(appsink);
    Q_UNUSED(user_data);
    return GST_FLOW_OK;
}

void MediaBackend::DestroyCallback(gpointer user_data)
{
    Q_UNUSED(user_data);
}



void MediaBackend::play()
{
    qInfo() << m_objName << " - play() called";
    m_videoOffsetMs = m_settings.videoOffsetMs();
    if (!m_cdgMode)
    {
        auto gstOffset = (m_videoOffsetMs * GST_MSECOND) * -1;
        g_object_set(m_playBin, "av-offset", gstOffset, nullptr);
    }
    else
    {
        g_object_set(m_playBin, "av-offset", 0, nullptr);
    }
    gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_playBin), GST_STREAM_VOLUME_FORMAT_LINEAR, 0.85);
    if (m_currentlyFadedOut)
    {
        g_object_set(m_faderVolumeElement, "volume", 0.0, nullptr);
    }
    if (state() == MediaBackend::PausedState)
    {
        qInfo() << m_objName << " - play - playback is currently paused, unpausing";
        gst_element_set_state(m_playBin, GST_STATE_PLAYING);
        if (m_fade)
            fadeIn();
        return;
    }
    if (!QFile::exists(m_filename))
    {
        qInfo() << " - play - File doesn't exist, bailing out";
        emit stateChanged(PlayingState);
        QApplication::processEvents();
        emit stateChanged(EndOfMediaState);
        return;
    }

    resetVideoSinks();

    auto uri = gst_filename_to_uri(m_filename.toLocal8Bit(), nullptr);
    g_object_set(m_playBin, "uri", uri, nullptr);
    g_free(uri);
    if (!m_cdgMode)
    {
        gst_bin_remove(reinterpret_cast<GstBin*>(m_audioBin), m_cdgBin);
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
        auto parentElement = gst_element_get_parent(m_cdgBin);
        if (parentElement == NULL)
            gst_bin_add(reinterpret_cast<GstBin*>(m_audioBin), m_cdgBin);
        else
            gst_object_unref(parentElement);
        m_cdg.open(m_cdgFilename);
        m_cdg.process();
        g_appSrcCurFrame = 0;
        g_appSrcCurPosition = 0;
        g_appSrcNeedData = false;
        gst_app_src_set_max_bytes(reinterpret_cast<GstAppSrc*>(m_cdgAppSrc), cdg::CDG_IMAGE_SIZE * 500);
        gst_app_src_set_size(reinterpret_cast<GstAppSrc*>(m_cdgAppSrc), m_cdg.getFrameCount() * cdg::CDG_IMAGE_SIZE);
        gst_app_src_set_duration(reinterpret_cast<GstAppSrc*>(m_cdgAppSrc), (m_cdg.getFrameCount() * 40) * GST_MSECOND);
        qInfo() << m_objName << " - play - playing cdg:   " << m_cdgFilename;
        qInfo() << m_objName << " - play - playing audio: " << m_filename;
        gst_element_set_state(m_playBin, GST_STATE_PLAYING);
    }
}

void MediaBackend::pause()
{
    if (m_fade)
        fadeOut();
    gst_element_set_state(m_playBin, GST_STATE_PAUSED);
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
    if (position > 1000 && position > duration() - 1000)
    {
        stop(true);
        return;
    }
    gst_element_seek_simple(m_playBin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_MSECOND * position);
    emit positionChanged(position);
}

void MediaBackend::setVolume(const int &volume)
{
    qInfo() << m_objName << " - fadeOut called";
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
            emit stateChanged(MediaBackend::StoppedState);
            m_fader->immediateIn();
            return;
        }
    }
    qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Stoping playback without fading";
    gst_element_set_state(m_playBin, GST_STATE_NULL);
    emit stateChanged(MediaBackend::StoppedState);
    qInfo() << m_objName << " - stop() completed";
}

void MediaBackend::rawStop()
{
    qInfo() << m_objName << " - rawStop() called, just ending gstreamer playback";
    gst_element_set_state(m_playBin, GST_STATE_NULL);
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
    if (!gst_element_query_position (m_audioBin, GST_FORMAT_TIME, &pos))
    {
        m_lastPosition = 0;
        emit positionChanged(0);
        return;
    }
    auto mspos = pos / GST_MSECOND;
    if (m_lastPosition != mspos)
    {
        m_lastPosition = mspos;
        emit positionChanged(mspos);
    }
}

void MediaBackend::timerSlow_timeout()
{
    if (m_silenceDetect)
    {
        if (isSilent() && state() == MediaBackend::PlayingState)
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
        }
        lastpos = position();
    }
}

void MediaBackend::setVideoOffset(const int offsetMs) {
    m_videoOffsetMs = offsetMs;
    if (state() == PlayingState)
    {
        setPosition(position());
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

GstBusSyncReply MediaBackend::busMessageDispatcher([[maybe_unused]]GstBus *bus, GstMessage *message, gpointer userData)
{  
    auto backend = static_cast<MediaBackend*>(userData);
    switch(GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING:
    case GST_MESSAGE_INFO:
    case GST_MESSAGE_STATE_CHANGED:
    case GST_MESSAGE_ELEMENT:
    case GST_MESSAGE_DURATION_CHANGED:
    case GST_MESSAGE_EOS:
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
        QMetaObject::invokeMethod(backend, [backend, message] () { backend->gstBusMsg(takeGstMiniObject(message)); }, Qt::QueuedConnection);
#else
        QMetaObject::invokeMethod(static_cast<MediaBackend*>(userData), "gstBusMsg", Qt::QueuedConnection, Q_ARG(std::shared_ptr<GstMessage>, takeGstMiniObject(message)));
#endif
        return GST_BUS_DROP;
    default:
        return GST_BUS_PASS;
    }
}

void MediaBackend::gstBusMsg(std::shared_ptr<GstMessage> message)
{
    switch (GST_MESSAGE_TYPE(message.get())) {
    case GST_MESSAGE_ERROR:
        GError *err;
        gchar *debug;
        gst_message_parse_error(message.get(), &err, &debug);
        qInfo() << m_objName << " - Gst error: " << err->message;
        qInfo() << m_objName << " - Gst debug: " << debug;
        if (QString(err->message) == "Your GStreamer installation is missing a plug-in.")
        {
            QString player = (m_objName == "KAR") ? "karaoke" : "break music";
            qInfo() << m_objName << " - PLAYBACK ERROR - Missing Codec";
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
        qInfo() << m_objName << " - Gst warning: " << err2->message;
        qInfo() << m_objName << " - Gst debug: " << debug2;
        g_error_free(err2);
        g_free(debug2);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        GstState state;
        gst_element_get_state(m_playBin, &state, nullptr, GST_CLOCK_TIME_NONE);
        if (m_currentlyFadedOut)
            g_object_set(m_faderVolumeElement, "volume", 0.0, nullptr);
        if (state == GST_STATE_PLAYING && m_lastState != MediaBackend::PlayingState)
        {
            qInfo() << "GST notified of state change to PLAYING";
            m_lastState = MediaBackend::PlayingState;
            emit stateChanged(MediaBackend::PlayingState);
            if (m_currentlyFadedOut)
                m_fader->immediateOut();
        }
        else if (state == GST_STATE_PAUSED && m_lastState != MediaBackend::PausedState)
        {
            qInfo() << "GST notified of state change to PAUSED";
            m_lastState = MediaBackend::PausedState;
            emit stateChanged(MediaBackend::PausedState);
        }
        else if (state == GST_STATE_NULL && m_lastState != MediaBackend::StoppedState)
        {
            qInfo() << "GST notified of state change to STOPPED";
            if (m_lastState != MediaBackend::StoppedState)
            {
                m_lastState = MediaBackend::StoppedState;
                emit stateChanged(MediaBackend::StoppedState);
            }
        }
        break;
    case GST_MESSAGE_ELEMENT:
    {
        auto msgStructure = gst_message_get_structure(message.get());
        if (std::string(gst_structure_get_name(msgStructure)) == "level")
        {
            auto array_val = gst_structure_get_value(msgStructure, "rms");
            auto rms_arr = reinterpret_cast<GValueArray*>(g_value_get_boxed (array_val));
            double rmsValues = 0.0;
            for (unsigned int i{0}; i < rms_arr->n_values; ++i)
            {
                auto value = g_value_array_get_nth (rms_arr, i);
                auto rms_dB = g_value_get_double (value);
                auto rms = pow (10, rms_dB / 20);
                rmsValues += rms;
            }
            m_currentRmsLevel = rmsValues / rms_arr->n_values;
        }
        break;
    }
    case GST_MESSAGE_DURATION_CHANGED:
        gint64 dur, msdur;
        qInfo() << m_objName << " - GST reports duration changed";
        if (gst_element_query_duration(m_playBin,GST_FORMAT_TIME,&dur))
            msdur = dur / 1000000;
        else
            msdur = 0;
        emit durationChanged(msdur);
        break;
    case GST_MESSAGE_EOS:
        qInfo() << m_objName << " - state change to EndOfMediaState emitted";
        emit stateChanged(EndOfMediaState);
        break;
    case GST_MESSAGE_NEED_CONTEXT:
    case GST_MESSAGE_TAG:
    case GST_MESSAGE_STREAM_STATUS:
    case GST_MESSAGE_LATENCY:
    case GST_MESSAGE_ASYNC_DONE:
    case GST_MESSAGE_NEW_CLOCK:
        break;
    default:
        qInfo() << m_objName << " - Gst msg type: " << GST_MESSAGE_TYPE(message.get()) << " Gst msg name: " << GST_MESSAGE_TYPE_NAME(message.get()) << " Element: " << message.get()->src->name;
        break;
    }

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
        //m_accelMode = OpenGL;
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
        m_videoSink1 = gst_element_factory_make ("d3dvideosink", "videoSink1");
        m_videoSink2 = gst_element_factory_make("d3dvideosink", "videoSink2");
#else
        m_videoSink1 = gst_element_factory_make ("glimagesink", "videoSink1");
        m_videoSink2 = gst_element_factory_make("glimagesink", "videoSink2");
#endif
    m_faderVolumeElement = gst_element_factory_make("volume", "FaderVolumeElement");
    g_object_set(m_faderVolumeElement, "volume", 1.0, nullptr);
    m_fader = new AudioFader(this);
    m_fader->setObjName(m_objName + "Fader");
    m_fader->setVolumeElement(m_faderVolumeElement);
    auto aConvInput = gst_element_factory_make("audioconvert", "aConvInput");
    auto aConvPrePitchShift = gst_element_factory_make("audioconvert", "aConvPrePitchShift");
    auto aConvPostPitchShift = gst_element_factory_make("audioconvert", "aConvPostPitchShift");
    m_audioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    auto rgVolume = gst_element_factory_make("rgvolume", "rgVolume");
    //g_object_set(rgVolume, "pre-amp", 6.0, "headroom", 10.0, nullptr);
    //auto rgLimiter = gst_element_factory_make("rglimiter", "rgLimiter");
    auto level = gst_element_factory_make("level", "level");
    m_pitchShifterSoundtouch = gst_element_factory_make("pitch", "pitch");
#ifdef Q_OS_LINUX
    m_pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo");
#endif
    m_equalizer = gst_element_factory_make("equalizer-10bands", "equalizer");
    m_playBin = gst_element_factory_make("playbin", "playBin");
    auto bus = gst_element_get_bus(m_playBin);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)busMessageDispatcher, this, NULL);
    gst_object_unref(bus);
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
    m_scaleTempo = gst_element_factory_make("scaletempo", "scaleTempo");
    m_audioPanorama = gst_element_factory_make("audiopanorama", "audioPanorama");
    g_object_set(m_audioPanorama, "method", 1, nullptr);
    buildCdgBin();
    gst_bin_add_many(GST_BIN(m_audioBin),m_cdgBin, queueMainAudio, m_audioPanorama, level, m_scaleTempo, aConvInput, rgVolume, /*rgLimiter,*/ m_volumeElement, m_equalizer, aConvPostPanorama, m_fltrPostPanorama, m_faderVolumeElement, nullptr);
    gst_element_link_many(queueMainAudio, aConvInput, rgVolume, /*rgLimiter,*/ m_scaleTempo, level, m_volumeElement, m_equalizer, m_faderVolumeElement, m_audioPanorama, aConvPostPanorama, m_fltrPostPanorama, nullptr);
    //gst_element_link_many(m_cdgAppSrc, videoConvert, buffer, videoConvert2, autoVideoSink, nullptr);
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
    if ((m_pitchShifterSoundtouch) && (m_loadPitchShift))
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


    auto csource = gst_interpolation_control_source_new ();
    if (!csource)
        qInfo() << m_objName << " - Error createing control source";
    GstControlBinding *cbind = gst_direct_control_binding_new (GST_OBJECT_CAST(m_faderVolumeElement), "volume", csource);
    if (!cbind)
        qInfo() << m_objName << " - Error creating control binding";
    if (!gst_object_add_control_binding (GST_OBJECT_CAST(m_faderVolumeElement), cbind))
        qInfo() << m_objName << " - Error adding control binding to volumeElement for fader control";
    g_object_set(csource, "mode", GST_INTERPOLATION_MODE_CUBIC, nullptr);


    // Video output setup
    if (m_videoAccelEnabled)
    {
    m_videoBin = gst_bin_new("videoBin");
    auto queueMainVideo = gst_element_factory_make("queue", "queueMainVideo");
    auto videoQueue1 = gst_element_factory_make("queue", "videoQueue1");
    auto videoQueue2 = gst_element_factory_make("queue", "videoQueue2");
    auto videoConv1 = gst_element_factory_make("videoconvert", "preOutVideoConvert1");
    auto videoConv2 = gst_element_factory_make("videoconvert", "preOutVideoConvert2");
    m_videoScale1 = gst_element_factory_make("videoscale", "videoScale1");
    m_videoScale2 = gst_element_factory_make("videoscale", "videoScale2");
    auto videoTee = gst_element_factory_make("tee", "videoTee");
    auto videoTeePad1 = gst_element_get_request_pad(videoTee, "src_%u");
    auto videoTeePad2 = gst_element_get_request_pad(videoTee, "src_%u");
    auto videoQueue1SrcPad = gst_element_get_static_pad(videoQueue1, "sink");
    auto videoQueue2SrcPad = gst_element_get_static_pad(videoQueue2, "sink");
    gst_bin_add_many(GST_BIN(m_videoBin), queueMainVideo, videoTee, videoQueue1, videoQueue2, videoConv1,
                     videoConv2, m_videoScale1, m_videoScale2, m_videoSink1, m_videoSink2,nullptr);
    gst_element_link(queueMainVideo, videoTee);
    gst_pad_link(videoTeePad1,videoQueue1SrcPad);
    gst_pad_link(videoTeePad2,videoQueue2SrcPad);
    gst_element_link_many(videoQueue1, videoConv1, m_videoScale1, m_videoSink1, nullptr);
    gst_element_link_many(videoQueue2, videoConv2, m_videoScale2, m_videoSink2, nullptr);
    auto ghostVideoPad = gst_ghost_pad_new("sink", gst_element_get_static_pad(queueMainVideo, "sink"));
    gst_pad_set_active(ghostVideoPad,true);
    gst_element_add_pad(m_videoBin, ghostVideoPad);
    g_object_set(m_playBin, "video-sink", m_videoBin, nullptr);
    }
    else
    {
        GstAppSinkCallbacks appsinkCallbacks;
        appsinkCallbacks.new_preroll	= &MediaBackend::NewPrerollCallback;
        appsinkCallbacks.new_sample		= &MediaBackend::NewSampleCallback;
        appsinkCallbacks.eos			= &MediaBackend::EndOfStreamCallback;
        m_videoAppSink = gst_element_factory_make("appsink", "videoAppSink");
        auto videoCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB16", NULL);
        g_object_set(m_videoAppSink, "caps", videoCaps, NULL);
        g_object_set(G_OBJECT(m_playBin), "video-sink", m_videoAppSink, NULL);
        gst_app_sink_set_callbacks(GST_APP_SINK(m_videoAppSink), &appsinkCallbacks, this, (GDestroyNotify)MediaBackend::DestroyCallback);
    }
    // End video output setup

    g_object_set(rgVolume, "album-mode", false, nullptr);
    g_object_set(level, "message", TRUE, nullptr);
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
    m_timerFast.start(250);
    qInfo() << m_objName << " - buildPipeline() finished";
    //setEnforceAspectRatio(m_settings.enforceAspectRatio());
    connect(m_fader, &AudioFader::fadeStarted, [&] () {
        qInfo() << m_objName << " - Fader started";
    });
    connect(m_fader, &AudioFader::fadeComplete, [&] () {
        qInfo() << m_objName << " - fader finished";
    });
    connect(m_fader, &AudioFader::faderStateChanged, [&] (auto state) {
        qInfo() << m_objName << " - Fader state changed to: " << m_fader->stateToStr(state);
    });
    g_object_set(G_OBJECT(m_cdgAppSrc), "stream-type", 1, "format", GST_FORMAT_TIME, NULL);
    g_signal_connect(m_cdgAppSrc, "need-data", G_CALLBACK(cb_need_data), this);
    g_signal_connect(m_cdgAppSrc, "seek-data", G_CALLBACK(cb_seek_data), this);
    g_signal_connect(m_cdgAppSrc, "enough-data", G_CALLBACK(cb_enough_data), this);
}

void MediaBackend::resetVideoSinks()
{
    if (!m_videoAccelEnabled)
        return;

    gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink1), m_videoWinId1);
    gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink2), m_videoWinId2);
    gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink1Cdg), m_videoWinId1);
    gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(m_videoSink2Cdg), m_videoWinId2);
}

void MediaBackend::buildCdgBin()
{
#if defined(Q_OS_LINUX)
    //m_accelMode = OpenGL;
    switch (m_accelMode) {
    case OpenGL:
        m_videoSink1Cdg = gst_element_factory_make("glimagesink", "cdgvideoSink1");
        m_videoSink2Cdg = gst_element_factory_make("glimagesink", "cdgvideoSink2");
        break;
    case XVideo:
        m_videoSink1Cdg = gst_element_factory_make("xvimagesink", "cdgvideoSink1");
        m_videoSink2Cdg = gst_element_factory_make("xvimagesink", "cdgvideoSink2");
        break;
    }
#elif defined(Q_OS_WIN)
    m_videoSink1Cdg = gst_element_factory_make ("d3dvideosink", "cdgvideoSink1");
    m_videoSink2Cdg = gst_element_factory_make("d3dvideosink", "cdgvideoSink2");
#else
    m_videoSink1Cdg = gst_element_factory_make ("glimagesink", "cdgvideoSink1");
    m_videoSink2Cdg = gst_element_factory_make("glimagesink", "cdgvideoSink2");
#endif
    m_cdgBin = gst_bin_new("cdgBin");
    g_object_ref(m_cdgBin);
    m_cdgAppSrc = gst_element_factory_make("appsrc", "cdgAppSrc");
    auto cdgVidConv = gst_element_factory_make("videoconvert", "cdgVideoConv");
    g_object_set(G_OBJECT(m_cdgAppSrc), "caps",
                 gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB16", "width", G_TYPE_INT, 288, "height",
                                     G_TYPE_INT, 192, NULL),
                 NULL);

    if (m_videoAccelEnabled)
    {
        auto videoQueue1 = gst_element_factory_make("queue", "videoQueue1");
        auto videoQueue2 = gst_element_factory_make("queue", "videoQueue2");
        auto videoConv1 = gst_element_factory_make("videoconvert", "preOutVideoConvert1");
        auto videoConv2 = gst_element_factory_make("videoconvert", "preOutVideoConvert2");
        m_videoScale1Cdg = gst_element_factory_make("videoscale", "videoScale1");
        m_videoScale2Cdg = gst_element_factory_make("videoscale", "videoScale2");
        auto videoTee = gst_element_factory_make("tee", "videoTee");
        auto videoTeePad1 = gst_element_get_request_pad(videoTee, "src_%u");
        auto videoTeePad2 = gst_element_get_request_pad(videoTee, "src_%u");
        auto videoQueue1SrcPad = gst_element_get_static_pad(videoQueue1, "sink");
        auto videoQueue2SrcPad = gst_element_get_static_pad(videoQueue2, "sink");
        gst_bin_add_many(reinterpret_cast<GstBin *>(m_cdgBin), m_cdgAppSrc, cdgVidConv, videoConv1,
                         videoConv2, videoTee, videoQueue1, videoQueue2, m_videoScale1Cdg, m_videoScale2Cdg,
                         m_videoSink1Cdg, m_videoSink2Cdg, nullptr);
        gst_element_link(m_cdgAppSrc, cdgVidConv);
        gst_element_link(cdgVidConv, videoTee);
        gst_pad_link(videoTeePad1, videoQueue1SrcPad);
        gst_pad_link(videoTeePad2, videoQueue2SrcPad);
        gst_element_link_many(videoQueue1, videoConv1, m_videoScale1Cdg, m_videoSink1Cdg, nullptr);
        gst_element_link_many(videoQueue2, videoConv2, m_videoScale2Cdg, m_videoSink2Cdg, nullptr);
    }
    else
    {
        GstAppSinkCallbacks appsinkCallbacks;
        appsinkCallbacks.new_preroll	= &MediaBackend::NewPrerollCallback;
        appsinkCallbacks.new_sample		= &MediaBackend::NewSampleCallbackCdg;
        appsinkCallbacks.eos			= &MediaBackend::EndOfStreamCallback;
        m_videoAppSinkCdg = gst_element_factory_make("appsink", "videoAppSinkCdg");
        auto videoCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB16", NULL);
        g_object_set(m_videoAppSinkCdg, "caps", videoCaps, NULL);
        gst_app_sink_set_callbacks(GST_APP_SINK(m_videoAppSinkCdg), &appsinkCallbacks, this, (GDestroyNotify)MediaBackend::DestroyCallback);
        auto videoQueue = gst_element_factory_make("queue", "cdgVideoQueue");
        auto videoScale = gst_element_factory_make("videoscale", "cdgVideoScale");
        auto videoConvert = gst_element_factory_make("videoconvert", "cdgVideoConvert");
        gst_bin_add_many(reinterpret_cast<GstBin *>(m_cdgBin), m_cdgAppSrc, videoQueue, videoConvert, videoScale, m_videoAppSinkCdg, nullptr);
        gst_element_link_many(m_cdgAppSrc, videoQueue, videoConvert, videoScale, m_videoAppSinkCdg, nullptr);
    }
}

void MediaBackend::getGstDevices()
{
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
}
void MediaBackend::cb_enough_data([[maybe_unused]]GstElement *appsrc, [[maybe_unused]]gpointer user_data)
{
    g_appSrcNeedData = false;
    //qInfo() << "cdg buffer full";
}


void MediaBackend::cb_need_data(GstElement *appsrc, [[maybe_unused]]guint unused_size, gpointer user_data)
{
    auto backend = reinterpret_cast<MediaBackend *>(user_data);
    g_appSrcNeedData = true;
    //qInfo() << "cdg buffering - free space: " << unused_size;
    while (g_appSrcNeedData && g_appSrcCurFrame < backend->m_cdg.getFrameCount())
    {
        auto buffer = gst_buffer_new_and_alloc(cdg::CDG_IMAGE_SIZE);
//        gst_buffer_fill(buffer,
//                        0,
//                        backend->m_cdg.videoFrameByIndex(adjustedFrame).constBits(),
//                        cdg::CDG_IMAGE_SIZE
//                        );
        gst_buffer_fill(buffer,
                        0,
                        backend->m_cdg.videoFrameDataByTime((g_appSrcCurPosition / GST_MSECOND) + backend->m_videoOffsetMs).data(),
                        cdg::CDG_IMAGE_SIZE
                        );
        GST_BUFFER_TIMESTAMP(buffer) = g_appSrcCurPosition;
        //GST_BUFFER_PTS(buffer) = g_appSrcCurPosition;
        //GST_BUFFER_DTS(buffer) = g_appSrcCurPosition - 10;
        GST_BUFFER_DURATION(buffer) = 40 * GST_MSECOND;
        GST_BUFFER_OFFSET(buffer) = g_appSrcCurFrame;
        auto rc = gst_app_src_push_buffer(reinterpret_cast<GstAppSrc *>(appsrc), buffer);
        if (rc != GST_FLOW_OK)
        {
            qWarning() << "push buffer returned non-OK status: " << rc;
            break;
        }
        if (g_appSrcCurFrame == backend->m_cdg.getFrameCount() - 1)
        {
            //qInfo() << "Reached EOS in decoding";
            gst_app_src_end_of_stream(reinterpret_cast<GstAppSrc*>(appsrc));
            break;
        }
        g_appSrcCurPosition += 40 * GST_MSECOND;
        g_appSrcCurFrame++;
        QApplication::processEvents();
    }

    //qInfo() << "curFrame: " << g_appSrcCurFrame << " total frames: " << backend->m_cdg.getFrameCount();
    //qInfo() << "cdg done buffering";
}

gboolean MediaBackend::cb_seek_data([[maybe_unused]]GstElement *appsrc, guint64 position, [[maybe_unused]]gpointer user_data)
{
    //qInfo() << "Got seek request to position " << position;
    if (position == 0)
    {
        g_appSrcCurFrame = 0;
        g_appSrcCurPosition = 0;
        return true;
    }
    g_appSrcCurFrame = position / (40 * GST_MSECOND);
    g_appSrcCurPosition = position;
    return true;
}



void MediaBackend::fadeOut(const bool &waitForFade)
{
    qInfo() << m_objName << " - fadeOut called";
    m_currentlyFadedOut = true;
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
    qInfo() << m_objName << " - fadeIn called";
    m_currentlyFadedOut = false;
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

void MediaBackend::setUseSilenceDetection(const bool &enabled) {
    QString state = enabled ? "on" : "off";
    qInfo() << m_objName << " - Setting silence detection to " << state;
    m_silenceDetect = enabled;
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
    if (!m_cdgMode)
    {
        gint64 curpos;
        gst_element_query_position(m_audioBin, GST_FORMAT_TIME, &curpos);
        gst_element_send_event(m_playBin, gst_event_new_seek((double)m_tempo / 100.0, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), GST_SEEK_TYPE_SET, curpos, GST_SEEK_TYPE_END, 0));
        return;
    }
    g_object_set(m_pitchShifterSoundtouch, "tempo", (double)percent / 100.0, nullptr);
    m_cdg.setTempo(percent);
    gint64 curpos;
    gst_element_query_position(m_playBin, GST_FORMAT_TIME, &curpos);
    setPosition(curpos / GST_MSECOND);

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
    qInfo() << m_objName << " - fadeInImmediate called";
    m_currentlyFadedOut = false;
    m_fader->immediateIn();
}

void MediaBackend::fadeOutImmediate()
{
    qInfo() << m_objName << " - fadeOutImmediate called";
    m_currentlyFadedOut = true;
    m_fader->immediateOut();
}

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
#include "softwarerendervideosink.h"
#include <QDir>
#include <QProcess>
#include <functional>
#include <gst/video/videooverlay.h>
#include <gst/gstsegment.h>
#include "gstreamer/gstreamerhelper.h"


Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<GstMessage>);

std::atomic<bool> g_appSrcNeedData{false};

MediaBackend::MediaBackend(QObject *parent, QString objectName, MediaType type) :
    QObject(parent), m_objName(objectName), m_type(type), m_loadPitchShift(type == Karaoke)
{
    Settings settings;

    qInfo() << "Start constructing GStreamer backend";
    m_videoAccelEnabled = settings.hardwareAccelEnabled();
    qInfo() << "Hardware accelerated video rendering" << (m_videoAccelEnabled ? "enabled" : "disabled");

    QMetaTypeId<std::shared_ptr<GstMessage>>::qt_metatype_id();
    buildPipeline();
    getAudioOutputDevices();
    qInfo() << "Done constructing GStreamer backend";

    connect(&m_timerSlow, &QTimer::timeout, this, &MediaBackend::timerSlow_timeout);
    connect(&m_timerFast, &QTimer::timeout, this, &MediaBackend::timerFast_timeout);
}

void MediaBackend::setVideoEnabled(const bool &enabled)
{
    if(m_videoEnabled != enabled)
    {
        m_videoEnabled = enabled;
        patchPipelineSinks();
    }
}

bool MediaBackend::hasActiveVideo()
{
    if (m_videoEnabled && m_hasVideo)
    {
        auto st = state();
        return st == PlayingState || st == PausedState;
    }
    return false;
}

void MediaBackend::writePipelineGraphToFile(GstBin *bin, QString filePath, QString fileName)
{
    fileName = QString("%1/%2 - %3").arg(QDir::cleanPath(filePath + QDir::separator()), m_objName, fileName);
    auto filenameDot = fileName + ".dot";
    auto filenamePng = fileName + ".png";

    auto data = gst_debug_bin_to_dot_data(bin, GST_DEBUG_GRAPH_SHOW_ALL);

    QFile f {filenameDot};

    if (f.open(QIODevice::WriteOnly))
    {
        QTextStream out{&f};
        out << QString(data);
    }
    g_free(data);

    QStringList dotArguments { "-Tpng", "-o" + filenamePng, filenameDot };
    QProcess process;
    process.start("dot", dotArguments);
    process.waitForFinished();
    f.remove();
}

void MediaBackend::writePipelinesGraphToFile(const QString filePath)
{
    writePipelineGraphToFile(reinterpret_cast<GstBin*>(m_videoBin), filePath, "GS graph video");
    writePipelineGraphToFile(reinterpret_cast<GstBin*>(m_audioBin), filePath, "GS graph audio");
    writePipelineGraphToFile(reinterpret_cast<GstBin*>(m_pipeline), filePath, "GS graph Pipeline");
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

    for (auto &vs : m_videoSinks)
    {
        if (vs.softwareRenderVideoSink)
        {
            g_object_set(vs.videoScale, "add-borders", enforce, nullptr);
        }
        else
        {
            // natively support the property force-aspect-ratio
            g_object_set(vs.videoSink, "force-aspect-ratio", enforce, nullptr);
        }
    }
}

MediaBackend::~MediaBackend()
{
    qInfo() << "MediaBackend destructor called";
    resetPipeline();
    m_timerSlow.stop();
    m_timerFast.stop();
    m_gstMsgBusHandlerTimer.stop();
    gst_object_unref(m_bus);
    gst_caps_unref(m_audioCapsMono);
    gst_caps_unref(m_audioCapsStereo);
    g_object_unref(m_pipeline);
    g_object_unref(m_decoder);
    g_object_unref(m_audioBin);
    g_object_unref(m_videoBin);
    delete m_cdgSrc;
    for (auto &device : m_audioOutputDevices)
    {
       g_object_unref(device);
    }

    for (auto &vs : m_videoSinks)
    {
        delete vs.softwareRenderVideoSink;
    }

    // Uncomment the following when running valgrind to eliminate noise
//    if (gst_is_initialized())
//        gst_deinit();
}

qint64 MediaBackend::position()
{
    gint64 pos;
    if (gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos))
        return pos / GST_MSECOND;
    return 0;
}

qint64 MediaBackend::duration()
{
    gint64 duration;
    if (gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &duration))
        return duration / GST_MSECOND;
    return 0;
}

MediaBackend::State MediaBackend::state()
{
    // TODO: this is called very often! Instead store the state from the state-changed bus message and return that!
    GstState state = GST_STATE_NULL;
    gst_element_get_state(m_pipeline, &state, nullptr, GST_CLOCK_TIME_NONE);

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
    m_videoOffsetMs = m_settings.videoOffsetMs();

    // todo - athom: leftover from playbin. Is what nessecary - what does it do?
    //gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_pipeline), GST_STREAM_VOLUME_FORMAT_LINEAR, 0.85);

    if (m_currentlyFadedOut)
    {
        g_object_set(m_faderVolumeElement, "volume", 0.0, nullptr);
    }
    if (state() == MediaBackend::PausedState)
    {
        qInfo() << m_objName << " - play - playback is currently paused, unpausing";
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        if (m_fade)
            fadeIn();
        return;
    }

    resetPipeline();

    bool allowMissingAudio = false;

    if (m_cdgMode)
    {
        // Check if cdg file exists
        if (!QFile::exists(m_cdgFilename))
        {
            qInfo() << " - play - CDG file doesn't exist, bailing out";
            emit stateChanged(PlayingState);
            QApplication::processEvents();
            emit stateChanged(EndOfMediaState);
            return;
        }

        // Use m_cdgAppSrc as source for video. m_decoder will still be used for audio file
        gst_bin_add(reinterpret_cast<GstBin*>(m_pipeline), m_cdgSrc->getSrcElement());
        m_videoSrcPad = new PadInfo { m_cdgSrc->getSrcElement(), "src" };
        patchPipelineSinks();

        allowMissingAudio = m_type == VideoPreview;

        m_cdgSrc->load(m_cdgFilename);

        qInfo() << m_objName << " - play - playing cdg:   " << m_cdgFilename;
    }

    if (!QFile::exists(m_filename))
    {
        if (!allowMissingAudio)
        {
            qInfo() << " - play - File doesn't exist, bailing out";
            emit stateChanged(PlayingState);
            QApplication::processEvents();
            emit stateChanged(EndOfMediaState);
            return;
        }
    }
    else
    {
        gst_bin_add(reinterpret_cast<GstBin*>(m_pipeline), m_decoder);
        qInfo() << m_objName << " - play - playing media: " << m_filename;
        auto uri = gst_filename_to_uri(m_filename.toLocal8Bit(), nullptr);
        g_object_set(m_decoder, "uri", uri, nullptr);
        g_free(uri);
    }

    resetVideoSinks();

    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

void MediaBackend::resetPipeline()
{
    // Stop pipeline
    gst_element_set_state(m_pipeline, GST_STATE_NULL);

    m_hasVideo = false;
    gst_element_unlink(m_decoder, m_audioBin);
    gst_element_unlink(m_decoder, m_videoBin);
    gst_element_unlink(m_cdgSrc->getSrcElement(), m_videoBin);

    gsthlp_bin_try_remove(m_pipelineAsBin, {m_cdgSrc->getSrcElement(), m_decoder, m_audioBin, m_videoBin});

    m_cdgSrc->reset();

    delete m_audioSrcPad; delete m_videoSrcPad; m_audioSrcPad = m_videoSrcPad = nullptr;
}

void MediaBackend::patchPipelineSinks()
{
    // Audio
    if(m_audioBin)
    {
        bool isLinked = gsthlp_is_sink_linked(m_audioBin);
        if(!isLinked && m_audioSrcPad)
        {
            gst_bin_add(m_pipelineAsBin, m_audioBin);
            gst_element_link_pads(m_audioSrcPad->element, m_audioSrcPad->pad.c_str(), m_audioBin, "sink");
            gst_element_sync_state_with_parent(m_audioBin);
        }
        else
        if(isLinked && !m_audioSrcPad)
        {
            // not used but for completeness sake
            auto currentSrc = gsthlp_get_peer_element(m_audioBin, "sink");
            if (currentSrc)
            {
                gst_element_unlink(currentSrc, m_audioBin);
                gst_bin_remove(m_pipelineAsBin, m_audioBin);
                gst_element_set_state(m_audioBin, GST_STATE_NULL);
            }
        }
    }

    // Video
    if(m_videoBin)
    {
        bool isLinked = gsthlp_is_sink_linked(m_videoBin);
        if(!isLinked && m_videoSrcPad && m_videoEnabled)
        {
            m_hasVideo = true;
            gst_bin_add(m_pipelineAsBin, m_videoBin);
            gst_element_link_pads(m_videoSrcPad->element, m_videoSrcPad->pad.c_str(), m_videoBin, "sink");
            gst_element_sync_state_with_parent(m_videoBin);
            emit hasActiveVideoChanged(true);
        }
        else
        if(isLinked && !(m_videoSrcPad && m_videoEnabled))
        {
            auto currentSrc = gsthlp_get_peer_element(m_videoBin, "sink");
            if (currentSrc)
            {
                m_hasVideo = false;
                gst_element_unlink(currentSrc, m_videoBin);
                gst_bin_remove(m_pipelineAsBin, m_videoBin);
                gst_element_set_state(m_videoBin, GST_STATE_NULL);
                emit hasActiveVideoChanged(false);
            }
        }
    }

    setVideoOffset(m_videoOffsetMs);
}

void MediaBackend::pause()
{
    if (m_fade)
        fadeOut();
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
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

bool MediaBackend::isMuted()
{
    return gst_stream_volume_get_mute(GST_STREAM_VOLUME(m_volumeElement));
}

void MediaBackend::setPosition(const qint64 &position)
{
    if (position > 1000 && position > duration() - 1000)
    {
        stop(true);
        return;
    }
    gst_element_send_event(m_pipeline, gst_event_new_seek(m_playbackRate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, position * GST_MSECOND, GST_SEEK_TYPE_NONE, 0));
    emit positionChanged(position);
}

void MediaBackend::setVolume(const int &volume)
{
    qInfo() << m_objName << " - setVolume called";
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
        stopPipeline();
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
            stopPipeline();
            m_fader->immediateIn();
            return;
        }
    }
    qInfo() << m_objName << " - AudioBackendGstreamer::stop -- Stoping playback without fading";
    stopPipeline();
    qInfo() << m_objName << " - stop() completed";
}

void MediaBackend::rawStop()
{
    qInfo() << m_objName << " - rawStop() called, just ending gstreamer playback";
    stopPipeline();
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
    auto currPos = position(); // local copy

    // Detect silence (if enabled)
    if (m_silenceDetect)
    {
        if (isSilent() && state() == MediaBackend::PlayingState)
        {
            if (m_silenceDuration++ >= 2)
            {
                // silence detected for 2+ seconds

                bool doEmit = true;

                if(m_type == Karaoke)
                {
                    doEmit = false;
                    if (m_cdgMode)
                    {
                        // In CDG-karaoke mode, only cut of the song if there are no more image frames to be shown
                        int finalFramePos = m_cdgSrc->positionOfFinalFrameMS();
                        doEmit = finalFramePos > 0 && finalFramePos <= currPos;
                    }
                    else
                    {
                        // Karaoke video-file.
                        // We can't interrupt this one as the detected silence might just be a musical break.
                    }
                }

                if (doEmit)
                {
                    emit silenceDetected();
                }
                return; // todo: reason to return here?
            }
        }
        else
            m_silenceDuration = 0;
    }

    // Check if playback is hung (playing but no movement since 1 second ago) for some reason
    if (state() == PlayingState)
    {
        if (m_positionWatchdogLastPos == currPos && m_positionWatchdogLastPos > 10)
        {
            qWarning() << m_objName << " - Playback appears to be hung, emitting end of stream";
            emit stateChanged(EndOfMediaState);
        }
        m_positionWatchdogLastPos = currPos;
    }
}

void MediaBackend::setVideoOffset(const int offsetMs) {
    m_videoOffsetMs = offsetMs;

    gint64 offset = GST_MSECOND * offsetMs;

    set_sink_ts_offset(reinterpret_cast<GstBin *>(m_audioBin), MAX(G_GINT64_CONSTANT(0), offset));
    set_sink_ts_offset(reinterpret_cast<GstBin *>(m_videoBin), MAX(G_GINT64_CONSTANT(0), -offset));
}


void MediaBackend::setPitchShift(const int &pitchShift)
{
    if (m_pitchShifterRubberBand)
    {
        g_object_set(m_pitchShifterRubberBand, "semitones", pitchShift, nullptr);
    }
    else if (m_pitchShifterSoundtouch)
    {
        g_object_set(m_pitchShifterSoundtouch, "pitch", getPitchForSemitone(pitchShift), nullptr);
    }
    else
    {
        qWarning() << "No pitch shifting plugin loaded!";
        return;
    }
    emit pitchChanged(pitchShift);
}

gboolean MediaBackend::gstBusFunc([[maybe_unused]]GstBus *bus, GstMessage *message, gpointer user_data)
{
        auto mb = reinterpret_cast<MediaBackend *>(user_data);

        switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            GError *err;
            gchar *debug;
            gst_message_parse_error(message, &err, &debug);
            qInfo() << mb->m_objName << " - Gst error: " << err->message;
            qInfo() << mb->m_objName << " - Gst debug: " << debug;
            if (QString(err->message) == "Your GStreamer installation is missing a plug-in.")
            {
                QString player = (mb->m_objName == "KAR") ? "karaoke" : "break music";
                qInfo() << mb->m_objName << " - PLAYBACK ERROR - Missing Codec";
                emit mb->audioError("Unable to play " + player + " file, missing gstreamer plugin");
                mb->stop(true);
            }
            g_error_free(err);
            g_free(debug);
            break;
        case GST_MESSAGE_WARNING:
            GError *err2;
            gchar *debug2;
            gst_message_parse_warning(message, &err2, &debug2);
            qInfo() << mb->m_objName << " - Gst warning: " << err2->message;
            qInfo() << mb->m_objName << " - Gst debug: " << debug2;
            g_error_free(err2);
            g_free(debug2);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            // todo: use gst_message_parse_state_changed instead:
            // GstState oldState, state, pending;
            // gst_message_parse_state_changed(message, &oldState, &state, &pending);
            // mb->m_currentState = state;*/

            GstState state;
            gst_element_get_state(mb->m_pipeline, &state, nullptr, GST_CLOCK_TIME_NONE);
            if (mb->m_currentlyFadedOut)
                g_object_set(mb->m_faderVolumeElement, "volume", 0.0, nullptr);
            if (state == GST_STATE_PLAYING && mb->m_lastState != MediaBackend::PlayingState)
            {
                qInfo() << "GST notified of state change to PLAYING";
                mb->m_lastState = MediaBackend::PlayingState;
                emit mb->stateChanged(MediaBackend::PlayingState);
                if (mb->m_currentlyFadedOut)
                    mb->m_fader->immediateOut();
            }
            else if (state == GST_STATE_PAUSED && mb->m_lastState != MediaBackend::PausedState)
            {
                qInfo() << "GST notified of state change to PAUSED";
                mb->m_lastState = MediaBackend::PausedState;
                emit mb->stateChanged(MediaBackend::PausedState);
            }
            else if (state == GST_STATE_NULL && mb->m_lastState != MediaBackend::StoppedState)
            {
                // this code is probably never reached as state changes to NULL are not reported...
                qInfo() << "GST notified of state change to STOPPED";
                if (mb->m_lastState != MediaBackend::StoppedState)
                {
                    mb->m_lastState = MediaBackend::StoppedState;
                    emit mb->stateChanged(MediaBackend::StoppedState);
                }
            }
            break;
        case GST_MESSAGE_ELEMENT:
        {
            auto msgStructure = gst_message_get_structure(message);
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
                mb->m_currentRmsLevel = rmsValues / rms_arr->n_values;
            }
            break;
        }
        case GST_MESSAGE_DURATION_CHANGED:
            gint64 dur, msdur;
            qInfo() << mb->m_objName << " - GST reports duration changed";
            if (gst_element_query_duration(mb->m_pipeline,GST_FORMAT_TIME,&dur))
                msdur = dur / 1000000;
            else
                msdur = 0;
            emit mb->durationChanged(msdur);
            break;
        case GST_MESSAGE_EOS:
            qInfo() << mb->m_objName << " - state change to EndOfMediaState emitted";
            emit mb->stateChanged(EndOfMediaState);
            break;
        case GST_MESSAGE_NEED_CONTEXT:
        case GST_MESSAGE_TAG:
        case GST_MESSAGE_STREAM_STATUS:
        case GST_MESSAGE_LATENCY:
        case GST_MESSAGE_ASYNC_DONE:
        case GST_MESSAGE_NEW_CLOCK:
            break;
        default:
            qInfo() << mb->m_objName << " - Gst msg type: " << GST_MESSAGE_TYPE(message) << " Gst msg name: " << GST_MESSAGE_TYPE_NAME(message) << " Element: " << message->src->name;
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

    m_pipeline = gst_pipeline_new("pipeline");
    m_pipelineAsBin = reinterpret_cast<GstBin *>(m_pipeline);

    auto bus = gst_element_get_bus(m_pipeline);
    gst_bus_add_watch(bus, (GstBusFunc)gstBusFunc, this);
    gst_object_unref(bus);

    m_decoder = gst_element_factory_make("uridecodebin", "uridecodebin");
    g_signal_connect(m_decoder, "pad-added", G_CALLBACK(padAddedToDecoder_cb), this);
    g_object_ref(m_decoder);

    m_cdgSrc = new CdgAppSrc();

    buildVideoSinkBin();
    buildAudioSinkBin();

    qInfo() << m_objName << " - buildPipeline() finished";
    //setEnforceAspectRatio(m_settings.enforceAspectRatio());
}

void MediaBackend::buildVideoSinkBin()
{
    m_videoBin = gst_bin_new("videoBin");
    g_object_ref(m_videoBin);

    auto queueMainVideo = gst_element_factory_make("queue", "queueMainVideo");
    gst_bin_add(reinterpret_cast<GstBin *>(m_videoBin), queueMainVideo);

    auto queuePad = gst_element_get_static_pad(queueMainVideo, "sink");
    auto ghostVideoPad = gst_ghost_pad_new("sink", queuePad);
    gst_pad_set_active(ghostVideoPad, true);
    gst_element_add_pad(m_videoBin, ghostVideoPad);
    gst_object_unref(queuePad);

    m_videoTee = gst_element_factory_make("tee", "videoTee");
    gst_bin_add(reinterpret_cast<GstBin *>(m_videoBin), m_videoTee);
    gst_element_link(queueMainVideo, m_videoTee);
}

void MediaBackend::buildAudioSinkBin()
{
    m_audioBin = gst_bin_new("audioBin");
    g_object_ref(m_audioBin);

    m_faderVolumeElement = gst_element_factory_make("volume", "FaderVolumeElement");
    g_object_set(m_faderVolumeElement, "volume", 1.0, nullptr);
    m_fader = new AudioFader(this);
    m_fader->setObjName(m_objName + "Fader");
    m_fader->setVolumeElement(m_faderVolumeElement);
    auto aConvInput = gst_element_factory_make("audioconvert", "aConvInput");
    m_audioSink = gst_element_factory_make("autoaudiosink", "autoAudioSink");
    auto rgVolume = gst_element_factory_make("rgvolume", "rgVolume");
    //g_object_set(rgVolume, "pre-amp", 6.0, "headroom", 10.0, nullptr);
    //auto rgLimiter = gst_element_factory_make("rglimiter", "rgLimiter");
    auto level = gst_element_factory_make("level", "level");
    m_equalizer = gst_element_factory_make("equalizer-10bands", "equalizer");

    m_bus = gst_element_get_bus(m_pipeline);
    //gst_bus_set_sync_handler(bus, (GstBusSyncHandler)busMessageDispatcher, this, NULL);
#if defined(Q_OS_LINUX)
    gst_bus_add_watch(m_bus, (GstBusFunc)gstBusFunc, this);
#else
    // We need to pop messages off the gst bus ourselves on non-linux platforms since
    // there is no GMainLoop running
    m_gstMsgBusHandlerTimer.start(40);
    connect(&m_gstMsgBusHandlerTimer, &QTimer::timeout, [&] () {
        while (gst_bus_have_pending(m_bus))
        {
            auto msg = gst_bus_pop(m_bus);
            if (!msg)
                continue;
            gstBusFunc(m_bus, msg, this);
            gst_message_unref(msg);
        }
    });
#endif

    m_audioCapsStereo = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, nullptr);
    m_audioCapsMono = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 1, nullptr);

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

    GstElement *audioBinLastElement;

    gst_bin_add_many(GST_BIN(m_audioBin), queueMainAudio, m_audioPanorama, level, m_scaleTempo, aConvInput, rgVolume, /*rgLimiter,*/ m_volumeElement, m_equalizer, aConvPostPanorama, m_fltrPostPanorama, m_faderVolumeElement, nullptr);
    // todo: athom: link m_volumeElement (and perhaps m_faderVolumeElement) after queueEndAudio to avoid delay when changing volume
    gst_element_link_many(queueMainAudio, aConvInput, rgVolume, /*rgLimiter,*/ m_scaleTempo, level, m_volumeElement, m_equalizer, m_faderVolumeElement, m_audioPanorama, aConvPostPanorama, audioBinLastElement = m_fltrPostPanorama, nullptr);

    if (m_loadPitchShift)
    {
#ifdef Q_OS_LINUX
        // try to initialize Rubber Band
        if ((m_pitchShifterRubberBand = gst_element_factory_make("ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo", "ladspa-ladspa-rubberband-so-rubberband-pitchshifter-stereo")))
        {
            qInfo() << m_objName << " - Pitch shift RubberBand enabled";

            auto aConvPrePitchShift = gst_element_factory_make("audioconvert", "aConvPrePitchShift");
            auto aConvPostPitchShift = gst_element_factory_make("audioconvert", "aConvPostPitchShift");

            gst_bin_add_many(GST_BIN(m_audioBin), aConvPrePitchShift, m_pitchShifterRubberBand, aConvPostPitchShift, nullptr);
            gst_element_link_many(audioBinLastElement, aConvPrePitchShift, m_pitchShifterRubberBand, aConvPostPitchShift, nullptr);
            audioBinLastElement = aConvPostPitchShift;
            g_object_set(m_pitchShifterRubberBand, "formant-preserving", true, nullptr);
            g_object_set(m_pitchShifterRubberBand, "crispness", 1, nullptr);
            g_object_set(m_pitchShifterRubberBand, "semitones", 0, nullptr);
        }
#endif
        // fail back to "pitch" plugin
        if (!m_pitchShifterRubberBand && (m_pitchShifterSoundtouch = gst_element_factory_make("pitch", "pitch")))
        {
            qInfo() << m_objName << " - Pitch shifter SoundTouch enabled";

            auto aConvPrePitchShift = gst_element_factory_make("audioconvert", "aConvPrePitchShift");

            gst_bin_add_many(GST_BIN(m_audioBin), aConvPrePitchShift, m_pitchShifterSoundtouch, nullptr);
            gst_element_link_many(audioBinLastElement, aConvPrePitchShift, m_pitchShifterSoundtouch, nullptr);
            audioBinLastElement = m_pitchShifterSoundtouch;
            g_object_set(m_pitchShifterSoundtouch, "pitch", 1.0, "tempo", 1.0, nullptr);
        }
    }

    gst_bin_add_many(GST_BIN(m_audioBin), m_aConvEnd, queueEndAudio, m_audioSink, nullptr);
    gst_element_link_many(audioBinLastElement, queueEndAudio, m_aConvEnd, m_audioSink, nullptr);

    auto csource = gst_interpolation_control_source_new ();
    if (!csource)
        qInfo() << m_objName << " - Error createing control source";
    GstControlBinding *cbind = gst_direct_control_binding_new (GST_OBJECT_CAST(m_faderVolumeElement), "volume", csource);
    if (!cbind)
        qInfo() << m_objName << " - Error creating control binding";
    if (!gst_object_add_control_binding (GST_OBJECT_CAST(m_faderVolumeElement), cbind))
        qInfo() << m_objName << " - Error adding control binding to volumeElement for fader control";
    g_object_set(csource, "mode", GST_INTERPOLATION_MODE_CUBIC, nullptr);
    g_object_unref(csource);

    auto pad = gst_element_get_static_pad(queueMainAudio, "sink");
    auto ghostPad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghostPad, true);
    gst_element_add_pad(m_audioBin, ghostPad);
    gst_object_unref(pad);

    g_object_set(rgVolume, "album-mode", false, nullptr);
    g_object_set(level, "message", TRUE, nullptr);
    setVolume(m_volume);
    m_timerSlow.start(1000);
    setAudioOutputDevice(m_outputDeviceIdx);
    setEqBypass(m_bypass);
    setDownmix(m_downmix);
    setVolume(m_volume);
    m_timerFast.start(250);

    connect(m_fader, &AudioFader::fadeStarted, [&] () {
        qInfo() << m_objName << " - Fader started";
    });
    connect(m_fader, &AudioFader::fadeComplete, [&] () {
        qInfo() << m_objName << " - fader finished";
    });
    connect(m_fader, &AudioFader::faderStateChanged, [&] (auto state) {
        qInfo() << m_objName << " - Fader state changed to: " << m_fader->stateToStr(state);
    });
}


void MediaBackend::padAddedToDecoder_cb(GstElement *element,  GstPad *pad, gpointer caller)
{
    MediaBackend *backend = (MediaBackend*)caller;

    auto new_pad_caps = gst_pad_get_current_caps (pad);
    auto new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    auto new_pad_type = gst_structure_get_name (new_pad_struct);

    bool doPatch = false;

    if (!backend->m_audioSrcPad && g_str_has_prefix (new_pad_type, "audio/x-raw"))
    {
        backend->m_audioSrcPad = new PadInfo(getPadInfo(element, pad));
        doPatch = true;
    }

    else if (!backend->m_videoSrcPad && g_str_has_prefix (new_pad_type, "video/x-raw"))
    {
        backend->m_videoSrcPad = new PadInfo(getPadInfo(element, pad));
        doPatch = true;
    }
    gst_caps_unref (new_pad_caps);

    if (doPatch)
    {
        backend->patchPipelineSinks();
    }
}

void MediaBackend::stopPipeline()
{
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    m_lastState = MediaBackend::StoppedState;
    m_hasVideo = false;
    emit stateChanged(MediaBackend::StoppedState);
    emit hasActiveVideoChanged(false);
}

void MediaBackend::resetVideoSinks()
{
    if (!m_videoAccelEnabled)
        return;

    for(auto &vs : m_videoSinks)
    {
        gst_video_overlay_set_window_handle(reinterpret_cast<GstVideoOverlay*>(vs.videoSink), vs.surface->winId());
    }
}

void MediaBackend::getAudioOutputDevices()
{
    auto monitor = gst_device_monitor_new ();
    auto moncaps = gst_caps_new_empty_simple ("audio/x-raw");
    auto monId = gst_device_monitor_add_filter (monitor, "Audio/Sink", moncaps);
    m_outputDeviceNames.clear();
    m_outputDeviceNames.append("0 - Default");
    GList *devices, *elem;
    devices = gst_device_monitor_get_devices(monitor);
    for(elem = devices; elem; elem = elem->next) {
        auto device = m_audioOutputDevices.emplace_back(reinterpret_cast<GstDevice*>(elem->data));
        auto *deviceName = gst_device_get_display_name(device);
        m_outputDeviceNames.append(QString::number(m_outputDeviceNames.size()) + " - " + deviceName);
        g_free(deviceName);
    }
    gst_device_monitor_remove_filter(monitor, monId);
    gst_caps_unref (moncaps);
    g_object_unref(monitor);
    g_list_free(devices);
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
    m_playbackRate = percent / 100.0;
    optimize_scaleTempo_for_rate(m_scaleTempo, m_playbackRate);

#if GST_CHECK_VERSION(1,8,0)
    // With gstreamer 1.18 we can change rate without seeking. Only works with videos and not appsrc it seems. Perhaps fixable with "handle-segment-change"...
    if (!m_cdgMode)
    {
        gst_element_send_event(m_pipeline, gst_event_new_seek(m_playbackRate, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_INSTANT_RATE_CHANGE), GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE));
        return;
    }
#endif

    // Calling setTempo repeatedly can cause the position watchdog to trigger, due to excessive seeking, so restart that before doing anything else
    if (m_timerSlow.isActive())
    {
        m_timerSlow.start();
    }

    // Change rate by doing a flushing seek to ~current position
    gint64 curpos;
    gst_element_query_position (m_pipeline, GST_FORMAT_TIME, &curpos);
    gst_element_send_event(m_pipeline, gst_event_new_seek(m_playbackRate, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), GST_SEEK_TYPE_SET, curpos, GST_SEEK_TYPE_NONE, 0));
}

void MediaBackend::setAudioOutputDevice(int deviceIndex)
{
    m_outputDeviceIdx = deviceIndex;
    gst_element_unlink(m_aConvEnd, m_audioSink);
    gst_bin_remove(GST_BIN(m_audioBin), m_audioSink);
    if (deviceIndex == 0)
    {
        m_audioSink = gst_element_factory_make("autoaudiosink", "audioSink");
    }
    else
    {
        m_audioSink = gst_device_create_element(m_audioOutputDevices.at(deviceIndex - 1), NULL);
    }

    gst_bin_add(GST_BIN(m_audioBin), m_audioSink);
    gst_element_link(m_aConvEnd, m_audioSink);
}

void MediaBackend::setVideoOutputWidgets(std::vector<QWidget*> surfaces)
{
    if (m_videoSinks.size() > 0)
    {
        throw std::runtime_error(("Video output widget(s) already set."));
    }

    int i = 0;

    for (auto &surface : surfaces)
    {
        i++;
        VideoSinkData vd;

        vd.surface = surface;

        if (m_videoAccelEnabled)
        {
            auto sinkElementName = getVideoSinkElementNameForFactory();
            vd.videoSink = gst_element_factory_make(sinkElementName, QString("videoSink%1").arg(i).toLocal8Bit());
        }
        else
        {
            vd.softwareRenderVideoSink = new SoftwareRenderVideoSink(surface);
            vd.videoSink = GST_ELEMENT(vd.softwareRenderVideoSink->getSink());
        }

        auto videoQueue = gst_element_factory_make("queue", QString("videoqueue%1").arg(i).toLocal8Bit());
        auto videoConv = gst_element_factory_make("videoconvert", QString("preOutVideoConvert%1").arg(i).toLocal8Bit());
        vd.videoScale = gst_element_factory_make("videoscale", QString("videoScale%1").arg(i).toLocal8Bit());

        gst_bin_add_many(GST_BIN(m_videoBin), videoQueue, videoConv, vd.videoScale, vd.videoSink, nullptr);
        gst_element_link_many(m_videoTee, videoQueue, videoConv, vd.videoScale, vd.videoSink, nullptr);

        m_videoSinks.push_back(vd);
    }
    resetVideoSinks();
}

const char* MediaBackend::getVideoSinkElementNameForFactory()
{
#if defined(Q_OS_LINUX)
    switch (m_accelMode)
    {
        case OpenGL:
            return "glimagesink";
        case XVideo:
            return "xvimagesink";
    }
#elif defined(Q_OS_WIN)
    return "d3dvideosink";
#else
    return "glimagesink";
#endif
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
    for (int band=0; band<10; band++)
    {
        g_object_set(m_equalizer, QString("band%1").arg(band).toLocal8Bit(), bypass ? 0.0 : (double)m_eqLevels[band], nullptr);
    }
    this->m_bypass = bypass;
}

void MediaBackend::setEqLevel(const int &band, const int &level)
{
    if (!m_bypass)
        g_object_set(m_equalizer, QString("band%1").arg(band).toLocal8Bit(), (double)level, nullptr);
    m_eqLevels[band] = level;
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

#include "mediabackend.h"
#include "mzarchive.h"
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <math.h>

// MediaBackend constructor & methods
MediaBackend::MediaBackend(QObject *parent, QString objectName, MediaType type)
    : QObject(parent), m_objName(objectName), m_type(type)
{
    m_loggingPrefix = "[" + objectName.toStdString() + "]";
    m_logger = spdlog::get("logger");
    if (!m_logger) {
        m_logger = spdlog::default_logger();
    }

    m_logger->info("{} Initializing GStreamer media backend", m_loggingPrefix);

    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }

    setupPipeline();

    // Set up AudioFader
    m_fader = new AudioFader(this);
    m_fader->setObjName(objectName);
    connect(m_fader, &AudioFader::volumeChanged, this, [this](double volMult) {
        if (m_volumeElement) {
            int vol = static_cast<int>(m_volume * volMult);
            if (m_muted) vol = 0;
            g_object_set(G_OBJECT(m_volumeElement), "volume", (double)vol / 100.0, nullptr);
        }
    });
    connect(m_fader, &AudioFader::fadeComplete, this, [this]() {
        if (m_fader->state() == AudioFader::FadedOut) {
            rawStop();
        }
    });

    // Apply saved settings on startup
    if (m_type == Karaoke) {
        setAudioOutputDevice(m_settings.audioOutputDevice());
    } else if (m_type == BackgroundMusic) {
        setAudioOutputDevice(m_settings.audioOutputDeviceBm());
    } else {
        setAudioOutputDevice("Default");
    }

    connect(&m_eventTimer, &QTimer::timeout, this, &MediaBackend::eventTimer_timeout);
    m_eventTimer.start(100);
}

MediaBackend::~MediaBackend() {
    m_eventTimer.stop();
    rawStop();
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }
    m_tempDir.reset();
}

void MediaBackend::setupPipeline() {
    m_pipeline = gst_element_factory_make("playbin", nullptr);
    if (!m_pipeline) {
        m_logger->error("{} Failed to create playbin", m_loggingPrefix);
        return;
    }

    // Audio bin
    m_audioBin = gst_bin_new("audio_sink_bin");
    
    m_pitch = gst_element_factory_make("pitch", "pitch");
    m_eq = gst_element_factory_make("equalizer-10bands", "eq");
    m_panorama = gst_element_factory_make("audiopanorama", "panorama");
    m_volumeElement = gst_element_factory_make("volume", "vol");
    m_audioSink = gst_element_factory_make("autoaudiosink", "audio_sink");

    if (m_audioBin && m_pitch && m_eq && m_panorama && m_volumeElement && m_audioSink) {
        gst_bin_add_many(GST_BIN(m_audioBin), m_pitch, m_eq, m_panorama, m_volumeElement, m_audioSink, nullptr);
        gst_element_link_many(m_pitch, m_eq, m_panorama, m_volumeElement, m_audioSink, nullptr);
        
        GstPad *pad = gst_element_get_static_pad(m_pitch, "sink");
        gst_element_add_pad(m_audioBin, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);
        
        g_object_set(G_OBJECT(m_pipeline), "audio-sink", m_audioBin, nullptr);
    } else {
        m_logger->error("{} Failed to create audio elements", m_loggingPrefix);
    }

    // Bus
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_add_watch(bus, busCall, this);
    gst_object_unref(bus);
}

gboolean MediaBackend::busCall(GstBus *bus, GstMessage *msg, gpointer data) {
    MediaBackend *backend = static_cast<MediaBackend*>(data);
    backend->handleBusMessage(msg);
    return TRUE;
}

void MediaBackend::handleBusMessage(GstMessage *msg) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            m_logger->info("{} End of stream", m_loggingPrefix);
            m_state = EndOfMediaState;
            emit stateChanged(m_state);
            break;
        case GST_MESSAGE_ERROR: {
            GError *err = nullptr;
            gchar *debug_info = nullptr;
            gst_message_parse_error(msg, &err, &debug_info);
            m_logger->error("{} Error received from element {}: {}", m_loggingPrefix, GST_OBJECT_NAME(msg->src), err->message);
            m_logger->error("{} Debug info: {}", m_loggingPrefix, debug_info ? debug_info : "none");
            emit audioError(QString::fromUtf8(err->message));
            g_clear_error(&err);
            g_free(debug_info);
            m_state = StoppedState;
            emit stateChanged(m_state);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(m_pipeline)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                if (new_state == GST_STATE_PLAYING) m_state = PlayingState;
                else if (new_state == GST_STATE_PAUSED) m_state = PausedState;
                else if (new_state == GST_STATE_NULL) m_state = StoppedState;
                emit stateChanged(m_state);
            }
            break;
        }
        default:
            break;
    }
}

bool MediaBackend::isSilent() {
    return false;
}

void MediaBackend::setAudioOutputDevice(const AudioOutputDevice &device) {
    setAudioOutputDevice(device.name);
}

void MediaBackend::setAudioOutputDevice(const QString &deviceName) {
    // Left as default
}

GstFlowReturn MediaBackend::onNewVideoSample(
    GstAppSink *sink,
    gpointer userData)
{
    auto *backend = static_cast<MediaBackend*>(userData);

    GstSample *sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_EOS;

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);

    GstVideoInfo info;

    if (!buffer || !caps ||
        !gst_video_info_from_caps(&info, caps)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstVideoFrame videoFrame;

    if (!gst_video_frame_map(
            &videoFrame,
            &info,
            buffer,
            GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    QImage image(
        static_cast<const uchar*>(
            GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 0)
        ),
        GST_VIDEO_INFO_WIDTH(&info),
        GST_VIDEO_INFO_HEIGHT(&info),
        GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 0),
        QImage::Format_RGB888
    );

    // Take ownership of the pixels before releasing
    // the GStreamer buffer.
    QImage ownedImage = image.copy();

    gst_video_frame_unmap(&videoFrame);
    gst_sample_unref(sample);

    if (!ownedImage.isNull())
        emit backend->frameReady(ownedImage);

    return GST_FLOW_OK;
}

void MediaBackend::setVideoOutputWidgets(
    const std::vector<QWidget*>& surfaces)
{
    // Remove previous frame-delivery connections.
    for (const auto &connection : m_videoConnections) {
        QObject::disconnect(connection);
    }

    m_videoConnections.clear();
    m_videoSinks.clear();

    // Register every video display, including external monitors.
    for (auto *surface : surfaces) {
        if (auto *display = qobject_cast<VideoDisplay*>(surface)) {
            m_videoSinks.push_back(display);

            m_videoConnections.push_back(
                connect(
                    this,
                    &MediaBackend::frameReady,
                    display,
                    &VideoDisplay::updateFrame,
                    Qt::QueuedConnection
                )
            );
        }
    }

    if (!m_pipeline || m_videoSinks.empty())
        return;

    // Build a GStreamer sink that converts decoded video
    // into RGB frames suitable for QImage.
    GError *error = nullptr;

    GstElement *videoBin = gst_parse_bin_from_description(
        "videoconvert ! "
        "video/x-raw,format=RGB ! "
        "appsink name=openkj_video_sink "
        "sync=true max-buffers=2 drop=true",
        TRUE,
        &error
    );

    if (!videoBin) {
        m_logger->error(
            "{} Failed to create video bin: {}",
            m_loggingPrefix,
            error ? error->message : "Unknown error"
        );

        if (error)
            g_error_free(error);

        return;
    }

    if (error)
        g_error_free(error);

    GstElement *appSink = gst_bin_get_by_name(
        GST_BIN(videoBin),
        "openkj_video_sink"
    );

    if (!appSink) {
        m_logger->error(
            "{} Failed to find appsink",
            m_loggingPrefix
        );

        gst_object_unref(videoBin);
        return;
    }

    GstAppSinkCallbacks callbacks{};
    callbacks.new_sample = &MediaBackend::onNewVideoSample;

    gst_app_sink_set_callbacks(
        GST_APP_SINK(appSink),
        &callbacks,
        this,
        nullptr
    );

    gst_object_unref(appSink);

    // Give playbin our new video-output bin.
    g_object_set(
        G_OBJECT(m_pipeline),
        "video-sink",
        videoBin,
        nullptr
    );


    m_logger->info(
        "{} Configured appsink video output for {} displays",
        m_loggingPrefix,
        m_videoSinks.size()
    );
}

void MediaBackend::setVideoEnabled(const bool &enabled) {
    m_videoEnabled = enabled;
}

bool MediaBackend::hasActiveVideo() {
    return m_hasVideo;
}

qint64 MediaBackend::position() {
    if (m_pipeline) {
        gint64 pos = 0;
        if (gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos)) {
            return pos / GST_MSECOND;
        }
    }
    return 0;
}

qint64 MediaBackend::duration() {
    if (m_pipeline) {
        gint64 dur = 0;
        if (gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &dur)) {
            return dur / GST_MSECOND;
        }
    }
    return 0;
}

MediaBackend::State MediaBackend::state() {
    return m_state;
}

QStringList MediaBackend::getOutputDevices() {
    QStringList list;
    list.append("Default");
    return list;
}

void MediaBackend::eventTimer_timeout() {
    if (!m_pipeline) return;
    
    static qint64 lastDur = 0;
    qint64 dur = duration();
    if (dur != lastDur) {
        lastDur = dur;
        emit durationChanged(dur);
    }

    if (m_state == PlayingState) {
        emit positionChanged(position());

        if (m_silenceDetect && !m_silenceDetectedEmitted) {
            qint64 pos = position();
            if (dur > 10000 && pos > 10000) {
                qint64 remaining = dur - pos;
                if (m_cdgLastDrawPosMs > 0) {
                    if (pos >= m_cdgLastDrawPosMs + 2000 && remaining <= 6000) {
                        m_silenceDetectedEmitted = true;
                        emit silenceDetected();
                    }
                } else {
                    if (remaining <= 4000) {
                        m_silenceDetectedEmitted = true;
                        emit silenceDetected();
                    }
                }
            }
        }
    }
}

void MediaBackend::play() {
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        if (m_fade) {
            m_fader->fadeIn(false);
        } else {
            m_fader->immediateIn();
        }
    }
}

void MediaBackend::pause() {
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    }
}

void MediaBackend::setMedia(const QString &filename) {
    m_silenceDetectedEmitted = false;
    rawStop();
    m_filename = filename;
    m_cdgFilename.clear();
    m_tempDir.reset();

    QString playPath = filename;

    if (filename.endsWith(".zip", Qt::CaseInsensitive)) {
        m_tempDir = std::make_unique<QTemporaryDir>();
        if (m_tempDir->isValid()) {
            MzArchive archive(filename);
            if (archive.isValidKaraokeFile()) {
                QString audioExt = archive.audioExtension();
                QString tempAudio = "temp_audio" + audioExt;
                QString tempCdg = "temp_audio.cdg";
                if (archive.extractAudio(m_tempDir->path(), tempAudio) &&
                    archive.extractCdg(m_tempDir->path(), tempCdg)) {
                    playPath = m_tempDir->path() + QDir::separator() + tempAudio;
                    m_cdgFilename = m_tempDir->path() + QDir::separator() + tempCdg;
                } else {
                    emit audioError("Failed to extract zip archive");
                    return;
                }
            } else {
                emit audioError("Invalid zip archive: " + archive.getLastError());
                return;
            }
        } else {
            emit audioError("Failed to create temporary directory");
            return;
        }
    }

    if (!m_pipeline) return;

    QString uri = "file://" + playPath;
    g_object_set(G_OBJECT(m_pipeline), "uri", uri.toUtf8().constData(), nullptr);
    
    if (!m_cdgFilename.isEmpty()) {
        QString subUri = "file://" + m_cdgFilename;
        g_object_set(G_OBJECT(m_pipeline), "suburi", subUri.toUtf8().constData(), nullptr);
        m_cdgLastDrawPosMs = getCdgLastDrawPositionMs(m_cdgFilename);
    } else {
        m_cdgLastDrawPosMs = 0;
    }

    m_hasVideo = false;
    emit hasActiveVideoChanged(false);

    updateVolume();
    updateAudioFilters();
}

void MediaBackend::setMediaCdg(const QString &cdgFilename, const QString &audioFilename) {
    m_silenceDetectedEmitted = false;
    rawStop();
    m_filename = audioFilename;
    m_cdgFilename = cdgFilename;
    m_tempDir.reset();

    m_tempDir = std::make_unique<QTemporaryDir>();
    if (m_tempDir->isValid()) {
        QFileInfo audioInfo(audioFilename);
        QString targetAudio = m_tempDir->path() + QDir::separator() + "song." + audioInfo.suffix();
        QString targetCdg = m_tempDir->path() + QDir::separator() + "song.cdg";

        if (QFile::copy(audioFilename, targetAudio) && QFile::copy(cdgFilename, targetCdg)) {
            QString uri = "file://" + targetAudio;
            g_object_set(G_OBJECT(m_pipeline), "uri", uri.toUtf8().constData(), nullptr);
            QString subUri = "file://" + targetCdg;
            g_object_set(G_OBJECT(m_pipeline), "suburi", subUri.toUtf8().constData(), nullptr);

            m_hasVideo = false;
            emit hasActiveVideoChanged(false);
            
            if (!m_cdgFilename.isEmpty()) {
                m_cdgLastDrawPosMs = getCdgLastDrawPositionMs(m_cdgFilename);
            } else {
                m_cdgLastDrawPosMs = 0;
            }

            updateVolume();
            updateAudioFilters();
        } else {
            emit audioError("Failed to copy media files to temp dir");
        }
    } else {
        emit audioError("Failed to create temporary directory");
    }
}

void MediaBackend::setMuted(const bool &muted) {
    m_muted = muted;
    updateVolume();
    emit mutedChanged(muted);
}

bool MediaBackend::isMuted() {
    return m_muted;
}

void MediaBackend::setPosition(const qint64 &position) {
    if (m_pipeline) {
        gst_element_seek_simple(m_pipeline, GST_FORMAT_TIME, 
            (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), position * GST_MSECOND);
    }
}

void MediaBackend::setVolume(const int &volume) {
    m_volume = volume;
    updateVolume();
}

void MediaBackend::stop(const bool &skipFade) {
    if (m_fade && !skipFade && m_state == PlayingState) {
        m_fader->fadeOut(false);
    } else {
        m_fader->immediateOut();
        rawStop();
    }
}

void MediaBackend::rawStop() {
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
    }
    m_state = StoppedState;
    emit stateChanged(m_state);
    for (auto* display : m_videoSinks) {
        if (display) {
            display->clearFrame();
        }
    }
}

void MediaBackend::setPitchShift(const int &pitchShift) {
    m_pitchShift = pitchShift;
    emit pitchChanged(m_pitchShift);
    if (m_pitch) {
        double ratio = pow(1.0594630943592953, pitchShift);
        g_object_set(G_OBJECT(m_pitch), "pitch", ratio, nullptr);
    }
}

void MediaBackend::fadeOut(const bool &waitForFade) {
    m_fader->fadeOut(waitForFade);
}

void MediaBackend::fadeIn(const bool &waitForFade) {
    m_fader->fadeIn(waitForFade);
}

void MediaBackend::setTempo(const int &percent) {
    m_tempoPercent = percent;
    if (m_pitch) {
        double tempo = (double)percent / 100.0;
        g_object_set(G_OBJECT(m_pitch), "tempo", tempo, nullptr);
    }
}

void MediaBackend::setMplxMode(const int &mode) {
    m_mplxMode = mode;
    updateAudioFilters();
}

void MediaBackend::setEqBypass(const bool &bypass) {
    m_eqBypass = bypass;
    updateAudioFilters();
}

void MediaBackend::setEqLevel(const int &band, const int &level) {
    if (band >= 0 && band < 10) {
        m_eqLevels[band] = level;
        updateAudioFilters();
    }
}

void MediaBackend::updateVolume() {
    if (!m_volumeElement) return;
    double volMult = m_fader ? m_fader->volume() : 1.0;
    int vol = static_cast<int>(m_volume * volMult);
    if (m_muted) vol = 0;
    g_object_set(G_OBJECT(m_volumeElement), "volume", (double)vol / 100.0, nullptr);
    emit volumeChanged(m_volume);
}

void MediaBackend::updateAudioFilters() {
    if (m_panorama) {
        if (m_mplxMode == Multiplex_LeftChannel) {
            g_object_set(G_OBJECT(m_panorama), "panorama", -1.0, nullptr);
        } else if (m_mplxMode == Multiplex_RightChannel) {
            g_object_set(G_OBJECT(m_panorama), "panorama", 1.0, nullptr);
        } else {
            g_object_set(G_OBJECT(m_panorama), "panorama", 0.0, nullptr);
        }
    }
    
    if (m_eq) {
        if (m_eqBypass) {
            for (int i=0; i<10; i++) {
                gchar name[16];
                g_snprintf(name, sizeof(name), "band%d", i);
                g_object_set(G_OBJECT(m_eq), name, 0.0, nullptr);
            }
        } else {
            for (int i=0; i<10; i++) {
                gchar name[16];
                g_snprintf(name, sizeof(name), "band%d", i);
                g_object_set(G_OBJECT(m_eq), name, (double)m_eqLevels[i], nullptr);
            }
        }
    }
}

void MediaBackend::setDownmix(const bool &enabled) {
    // left empty
}

qint64 MediaBackend::getCdgLastDrawPositionMs(const QString &cdgPath) {
    QFile file(cdgPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return 0;
    }
    qint64 size = file.size();
    qint64 numPackets = size / 24;
    if (numPackets == 0) return 0;

    uchar *data = file.map(0, size);
    if (data) {
        for (qint64 i = numPackets - 1; i >= 0; --i) {
            uchar command = data[i * 24] & 0x3F;
            uchar instruction = data[i * 24 + 1] & 0x3F;
            if (command == 0x09) {
                if (instruction == 6 || instruction == 38 || instruction == 20 || instruction == 24 || instruction == 1) {
                    file.unmap(data);
                    return (i * 10) / 3;
                }
            }
        }
        file.unmap(data);
    } else {
        const int chunkSize = 4096;
        QByteArray buffer;
        qint64 offset = size;
        while (offset > 0) {
            qint64 readSize = std::min(static_cast<qint64>(chunkSize), offset);
            offset -= readSize;
            if (!file.seek(offset)) break;
            buffer = file.read(readSize);
            qint64 localPackets = buffer.size() / 24;
            for (qint64 j = localPackets - 1; j >= 0; --j) {
                qint64 globalIndex = (offset / 24) + j;
                uchar command = buffer[j * 24] & 0x3F;
                uchar instruction = buffer[j * 24 + 1] & 0x3F;
                if (command == 0x09) {
                    if (instruction == 6 || instruction == 38 || instruction == 20 || instruction == 24 || instruction == 1) {
                        return (globalIndex * 10) / 3;
                    }
                }
            }
        }
    }
    return 0;
}

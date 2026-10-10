#include "mediabackend.h"
#include "mzarchive.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <math.h>
#include <cmath>

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

    m_cdgTempoTimer.setSingleShot(true);
    m_cdgTempoTimer.setInterval(250);

    connect(&m_cdgTempoTimer, &QTimer::timeout, this, [this]() {
        if (!m_cdgPipeline)
            return;

        gint64 cdgPosition = GST_CLOCK_TIME_NONE;

        if (gst_element_query_position(
                m_cdgPipeline,
                GST_FORMAT_TIME,
                &cdgPosition) &&
            GST_CLOCK_TIME_IS_VALID(cdgPosition)) {

            seekCdg(cdgPosition / GST_MSECOND);
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

    destroyCdgPipeline();

    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }

    if (m_sharedClock) {
        gst_object_unref(m_sharedClock);
        m_sharedClock = nullptr;
    }   
    m_tempDir.reset();
}

void MediaBackend::setupPipeline() {
    m_pipeline = gst_element_factory_make("playbin", nullptr);
    // Use one clock for the audio and CDG pipelines.
    m_sharedClock = gst_system_clock_obtain();

    gst_pipeline_use_clock(
        GST_PIPELINE(m_pipeline),
        m_sharedClock
    );

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
}

bool MediaBackend::setupCdgPipeline(const QString &cdgPath)
{
    destroyCdgPipeline();

    if (cdgPath.isEmpty())
        return false;

    GError *error = nullptr;

    GstElement *pipeline = gst_parse_launch(
        "filesrc name=cdg_source ! "
        "cdgparse ! cdgdec ! videoconvert ! "
        "video/x-raw,format=RGB ! "
        "appsink name=cdg_sink "
        "sync=true max-buffers=2 drop=true",
        &error
    );

    if (error) {
        m_logger->error(
            "{} CDG pipeline error: {}",
            m_loggingPrefix,
            error->message
        );
        g_error_free(error);
        error = nullptr;
    }

    if (!pipeline)
        return false;

    GstElement *source = gst_bin_get_by_name(
        GST_BIN(pipeline), "cdg_source"
    );

    GstElement *sink = gst_bin_get_by_name(
        GST_BIN(pipeline), "cdg_sink"
    );

    if (!source || !sink) {
        m_logger->error(
            "{} Missing CDG source or appsink",
            m_loggingPrefix
        );

        if (source) gst_object_unref(source);
        if (sink) gst_object_unref(sink);

        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return false;
    }

    QByteArray pathBytes = QFile::encodeName(cdgPath);

    g_object_set(
        G_OBJECT(source),
        "location",
        pathBytes.constData(),
        nullptr
    );

    GstAppSinkCallbacks callbacks{};
    callbacks.new_sample = &MediaBackend::onNewVideoSample;

    gst_app_sink_set_callbacks(
        GST_APP_SINK(sink),
        &callbacks,
        this,
        nullptr
    );

    gst_object_unref(source);
    gst_object_unref(sink);
    
    // Use the same clock for the CDG pipeline as the main audio pipeline.
    if (m_sharedClock) {
        gst_pipeline_use_clock(
            GST_PIPELINE(pipeline),
            m_sharedClock
        );
    }
    // Store the CDG pipeline for later use.
    m_cdgPipeline = pipeline;

    m_logger->info(
        "{} Created separate CDG pipeline: {}",
        m_loggingPrefix,
        cdgPath.toStdString()
    );

    return true;
}

bool MediaBackend::seekCdg(qint64 positionMs)
{
    if (!m_cdgPipeline || positionMs < 0)
        return false;

    const double rate = m_tempoPercent / 100.0;

    if (rate <= 0.0)
        return false;

    const gboolean success = gst_element_seek(
        m_cdgPipeline,
        rate,
        GST_FORMAT_TIME,
        static_cast<GstSeekFlags>(
            GST_SEEK_FLAG_FLUSH |
            GST_SEEK_FLAG_ACCURATE
        ),
        GST_SEEK_TYPE_SET,
        static_cast<gint64>(positionMs) * GST_MSECOND,
        GST_SEEK_TYPE_NONE,
        GST_CLOCK_TIME_NONE
    );

    if (!success) {
        m_logger->warn(
            "{} CDG seek/rate change failed: {} ms at {}%",
            m_loggingPrefix,
            positionMs,
            m_tempoPercent
        );
    }

    return success;
}

void MediaBackend::destroyCdgPipeline()
{
    if (!m_cdgPipeline)
        return;

    gst_element_set_state(m_cdgPipeline, GST_STATE_NULL);
    gst_object_unref(m_cdgPipeline);
    m_cdgPipeline = nullptr;
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
        case GST_MESSAGE_ASYNC_DONE:
            m_logger->info(
            "{} GStreamer async operation completed; position={} ms",
            m_loggingPrefix,
            position()
        );
        break;

        case GST_MESSAGE_SEGMENT_DONE:
            m_logger->info(
            "{} GStreamer segment completed; position={} ms",
            m_loggingPrefix,
            position()
        );
    break;
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
     if (!m_pipeline)
        return;

    // Process pending GStreamer messages using Qt's event loop.
    GstBus *bus = gst_element_get_bus(m_pipeline);

    if (bus) {
        // Limit work per tick so message bursts can't freeze the UI.
        for (int i = 0; i < 100; ++i) {
            GstMessage *msg = gst_bus_pop(bus);

            if (!msg)
                break;

            handleBusMessage(msg);
            gst_message_unref(msg);
        }

        gst_object_unref(bus);
    }

    // Handle coordinated seek if needed.
    processCoordinatedSeek();

    qint64 dur = duration();

    if (dur != m_lastDuration) {
        m_lastDuration = dur;
        emit durationChanged(dur);
    }

// Keep CDG graphics synchronized with audio playback.
// Diagnostic: monitor CDG/audio positions without correcting.
if (m_cdgPipeline &&
    m_pipeline &&
    m_state == PlayingState) {

    if (!m_cdgSyncTimer.isValid())
        m_cdgSyncTimer.start();

    const qint64 now = m_cdgSyncTimer.elapsed();

    if (m_lastCdgSyncCheckMs < 0 ||
        now - m_lastCdgSyncCheckMs >= 1000) {

        m_lastCdgSyncCheckMs = now;

        gint64 audioPos = GST_CLOCK_TIME_NONE;
        gint64 cdgPos = GST_CLOCK_TIME_NONE;

        const gboolean audioOk = gst_element_query_position(
            m_pipeline,
            GST_FORMAT_TIME,
            &audioPos
        );

        const gboolean cdgOk = gst_element_query_position(
            m_cdgPipeline,
            GST_FORMAT_TIME,
            &cdgPos
        );

        if (audioOk && cdgOk &&
            GST_CLOCK_TIME_IS_VALID(audioPos) &&
            GST_CLOCK_TIME_IS_VALID(cdgPos)) {

            const qint64 audioMs = audioPos / GST_MSECOND;
            const qint64 cdgMs = cdgPos / GST_MSECOND;

            m_logger->info(
                "{} SYNC tempo={} audio={}ms cdg={}ms diff={}ms",
                m_loggingPrefix,
                m_tempoPercent,
                audioMs,
                cdgMs,
                audioMs - cdgMs
            );
        }
    }
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
    
    if (m_cdgPipeline) {
        gst_element_set_state(m_cdgPipeline, GST_STATE_PLAYING);
    }
}

void MediaBackend::pause() {
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    }

    if (m_cdgPipeline) {
        gst_element_set_state(m_cdgPipeline, GST_STATE_PAUSED);
    }
}

void MediaBackend::setMedia(const QString &filename) {
    m_silenceDetectedEmitted = false;
    rawStop();
    destroyCdgPipeline();
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

    // Clear any CDG stream left over from the previous song.
    g_object_set(G_OBJECT(m_pipeline), "suburi", nullptr, nullptr);

    QString uri = "file://" + playPath;
    g_object_set(G_OBJECT(m_pipeline), "uri", uri.toUtf8().constData(), nullptr);
    
    if (!m_cdgFilename.isEmpty()) {
        setupCdgPipeline(m_cdgFilename);
        m_cdgLastDrawPosMs =
            getCdgLastDrawPositionMs(m_cdgFilename);
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
    destroyCdgPipeline();
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
            g_object_set(G_OBJECT(m_pipeline), "suburi", nullptr, nullptr);

            // Use the copied CDG file in the temporary directory.
            m_cdgFilename = targetCdg;
            setupCdgPipeline(m_cdgFilename);

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

void MediaBackend::setPosition(const qint64 &position)
{
    if (!m_pipeline || position < 0)
        return;

    // MP4 and other single-pipeline media keep their
    // existing direct seeking behavior.
    if (!m_cdgPipeline) {
        const gboolean success = gst_element_seek_simple(
            m_pipeline,
            GST_FORMAT_TIME,
            static_cast<GstSeekFlags>(
                GST_SEEK_FLAG_FLUSH |
                GST_SEEK_FLAG_ACCURATE
            ),
            static_cast<gint64>(position) * GST_MSECOND
        );

        if (!success) {
            m_logger->warn(
                "{} Seek failed at {} ms",
                m_loggingPrefix,
                position
            );
        }

        return;
    }

    // MP3+G: coordinate both pipelines.
    // Preserve the intended playback state, even if another
    // seek arrives while a coordinated seek is underway.
    if (m_seekPhase == SeekPhase::Idle) {
        m_resumeAfterSeek = (m_state == PlayingState);
    }

    m_seekTargetMs = position;
    m_seekPhase = SeekPhase::WaitingForPause;
    m_seekTimer.start();

    // Don't allow a pending tempo adjustment to interrupt seeking.
    m_cdgTempoTimer.stop();

    m_logger->info(
        "{} Coordinated seek requested: {} ms",
        m_loggingPrefix,
        position
    );

    // Pause both pipelines before issuing either seek.
    const GstStateChangeReturn audioResult =
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);

    const GstStateChangeReturn cdgResult =
        gst_element_set_state(m_cdgPipeline, GST_STATE_PAUSED);

    if (audioResult == GST_STATE_CHANGE_FAILURE ||
        cdgResult == GST_STATE_CHANGE_FAILURE) {

        finishCoordinatedSeek(false);
    }
}

void MediaBackend::processCoordinatedSeek()
{
    if (m_seekPhase == SeekPhase::Idle)
        return;

    if (!m_pipeline || !m_cdgPipeline) {
        m_seekPhase = SeekPhase::Idle;
        return;
    }

    // Never wait indefinitely for an element that fails to preroll.
    constexpr qint64 seekTimeoutMs = 5000;

    if (m_seekTimer.elapsed() > seekTimeoutMs) {
        m_logger->error(
            "{} Coordinated seek timed out after {} ms",
            m_loggingPrefix,
            m_seekTimer.elapsed()
        );

        finishCoordinatedSeek(false);
        return;
    }

    GstState audioState = GST_STATE_VOID_PENDING;
    GstState cdgState = GST_STATE_VOID_PENDING;

    const GstStateChangeReturn audioResult =
        gst_element_get_state(
            m_pipeline,
            &audioState,
            nullptr,
            0
        );

    const GstStateChangeReturn cdgResult =
        gst_element_get_state(
            m_cdgPipeline,
            &cdgState,
            nullptr,
            0
        );

    if (audioResult == GST_STATE_CHANGE_FAILURE ||
        cdgResult == GST_STATE_CHANGE_FAILURE) {

        m_logger->error(
            "{} Pipeline state failure during coordinated seek",
            m_loggingPrefix
        );

        finishCoordinatedSeek(false);
        return;
    }

    // Both pipelines must complete their pending operations.
    if (audioResult == GST_STATE_CHANGE_ASYNC ||
        cdgResult == GST_STATE_CHANGE_ASYNC ||
        audioState != GST_STATE_PAUSED ||
        cdgState != GST_STATE_PAUSED) {
        return;
    }

    if (m_seekPhase == SeekPhase::WaitingForPause) {

        m_logger->info(
            "{} Both pipelines paused; seeking to {} ms",
            m_loggingPrefix,
            m_seekTargetMs
        );

        m_seekPhase = SeekPhase::WaitingForSeek;

        const gboolean audioSeekAccepted =
            gst_element_seek_simple(
                m_pipeline,
                GST_FORMAT_TIME,
                static_cast<GstSeekFlags>(
                    GST_SEEK_FLAG_FLUSH |
                    GST_SEEK_FLAG_ACCURATE
                ),
                static_cast<gint64>(m_seekTargetMs) *
                    GST_MSECOND
            );

        // Diagnostic: map the audio seek timeline to the CDG timeline.
        const double tempoRatio =
        static_cast<double>(m_tempoPercent) / 100.0;

        const qint64 cdgTargetMs = static_cast<qint64>(
            std::llround(m_seekTargetMs * tempoRatio)
        );

        const gboolean cdgSeekAccepted =
            seekCdg(cdgTargetMs);

        m_logger->info(
            "{} Tempo-aware seek: audio={}ms cdg={}ms tempo={}%",
            m_loggingPrefix,
            m_seekTargetMs,
            cdgTargetMs,
            m_tempoPercent
        );

        if (!audioSeekAccepted || !cdgSeekAccepted) {
            m_logger->error(
                "{} Coordinated seek rejected: audio={} cdg={}",
                m_loggingPrefix,
                audioSeekAccepted ? "yes" : "no",
                cdgSeekAccepted ? "yes" : "no"
            );

            finishCoordinatedSeek(false);
        }

        return;
    }

    if (m_seekPhase == SeekPhase::WaitingForSeek) {

        // Both pipelines have finished seeking and prerolling.
        // They are now ready to resume.

        gint64 audioPos = GST_CLOCK_TIME_NONE;
        gint64 cdgPos = GST_CLOCK_TIME_NONE;

        const gboolean audioOk = gst_element_query_position(
            m_pipeline, GST_FORMAT_TIME, &audioPos
        );

        const gboolean cdgOk = gst_element_query_position(
            m_cdgPipeline, GST_FORMAT_TIME, &cdgPos
        );

        if (audioOk && cdgOk &&
            GST_CLOCK_TIME_IS_VALID(audioPos) &&
            GST_CLOCK_TIME_IS_VALID(cdgPos)) {

            m_logger->info(
                "{} Seek ready: audio={}ms cdg={}ms diff={}ms",
                m_loggingPrefix,
                audioPos / GST_MSECOND,
                cdgPos / GST_MSECOND,
                (audioPos - cdgPos) / GST_MSECOND
            );
        }

        finishCoordinatedSeek(true);
    }
}

void MediaBackend::finishCoordinatedSeek(bool success)
{
    if (m_seekPhase == SeekPhase::Idle)
        return;

    const bool shouldResume = m_resumeAfterSeek;

    m_seekPhase = SeekPhase::Idle;
    m_resumeAfterSeek = false;

    if (!m_pipeline || !m_cdgPipeline)
        return;

    if (success) {
        m_logger->info(
            "{} Coordinated seek completed in {} ms",
            m_loggingPrefix,
            m_seekTimer.elapsed()
        );
    } else {
        m_logger->warn(
            "{} Coordinated seek did not complete successfully",
            m_loggingPrefix
        );
    }

    // Restore the previous playback state.
    if (shouldResume) {
        gst_element_set_state(
            m_cdgPipeline,
            GST_STATE_PLAYING
        );

        gst_element_set_state(
            m_pipeline,
            GST_STATE_PLAYING
        );
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
    // Prevent outstanding seek from restarting an old song.
    m_seekPhase = SeekPhase::Idle;
    m_resumeAfterSeek = false;
    m_cdgTempoTimer.stop();

    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
    }

    if (m_cdgPipeline) {
        gst_element_set_state(m_cdgPipeline, GST_STATE_NULL);
    }
    m_state = StoppedState;
    emit stateChanged(m_state);
    for (auto* display : m_videoSinks) {
        if (display) {
            display->clearFrame();
        }
    }

    m_cdgTempoTimer.stop();
    m_cdgSyncTimer.invalidate();
    m_lastCdgSyncCheckMs = -1;
    m_lastCdgCorrectionMs = -1;
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

void MediaBackend::setTempo(const int &percent)
{
    if (percent <= 0)
        return;

    // Capture CDG's actual position before changing its rate.
    gint64 cdgPosition = GST_CLOCK_TIME_NONE;
    bool haveCdgPosition = false;

    if (m_cdgPipeline &&
        (m_state == PlayingState ||
         m_state == PausedState)) {
        m_cdgTempoTimer.start();
    }

    m_tempoPercent = percent;

    // Update audio tempo through SoundTouch.
    if (m_pitch) {
        const double tempo = percent / 100.0;

        g_object_set(
            G_OBJECT(m_pitch),
            "tempo",
            tempo,
            nullptr
        );
    }

    // Change CDG playback rate at its current position,
    // not at the audio pipeline's reported position.
    if (haveCdgPosition &&
        GST_CLOCK_TIME_IS_VALID(cdgPosition)) {

        const gboolean success = gst_element_seek(
            m_cdgPipeline,
            percent / 100.0,
            GST_FORMAT_TIME,
            static_cast<GstSeekFlags>(
                GST_SEEK_FLAG_FLUSH |
                GST_SEEK_FLAG_ACCURATE
            ),
            GST_SEEK_TYPE_SET,
            cdgPosition,
            GST_SEEK_TYPE_NONE,
            GST_CLOCK_TIME_NONE
        );

        m_logger->info(
            "{} CDG tempo changed to {}% at {} ms (success: {})",
            m_loggingPrefix,
            percent,
            cdgPosition / GST_MSECOND,
            success ? "yes" : "no"
        );
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

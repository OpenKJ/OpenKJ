#include "mediabackend.h"
#include "mzarchive.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <math.h>
#include <cmath>
#include <QtConcurrent/QtConcurrentRun>

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
        if (m_stopAfterFade && m_fader->state() == AudioFader::FadedOut) {
            rawStop();
        }
    });

    connect(&m_cdgStateWatcher, &QFutureWatcher<CdgStateResult>::finished, this, [this] {
        m_cdgStateBusy = false;
        const auto result = m_cdgStateWatcher.result();
        m_logger->info("{} CDG state request {}: {}ms, cancelled={}", m_loggingPrefix,
                       gst_element_state_get_name(result.target), result.elapsedMs, result.cancelled);
        const bool cancelled = result.cancelled || result.revision != m_cdgStateRevision->load();
        if (cancelled && !result.cancelled) {
            // Stop may arrive after the worker returned but before this slot.
            // Finish native cleanup asynchronously before replaying controls.
            requestCdgState(GST_STATE_NULL);
            return;
        }
        if (!cancelled) {
            if (result.audio == GST_STATE_CHANGE_FAILURE || result.cdg == GST_STATE_CHANGE_FAILURE) {
                rawStop();
                emit audioError("Unable to change synchronized CDG playback state");
            } else if (result.target == GST_STATE_PLAYING) {
                m_cdgStartPending = false;
                m_logger->info("{} Coordinated CDG start: preroll={}ms, runningTime={}ms, shared base time",
                               m_loggingPrefix, m_cdgStartTimer.elapsed(), m_cdgRunningTime / GST_MSECOND);
            }
        }
        auto controls = std::move(m_deferredControls);
        m_deferredControls.clear();
        for (auto &control : controls)
            control();
    });

    // Set up CDG tempo timer for synchronizing CDG graphics with audio playback
    m_cdgTempoTimer.setSingleShot(true);
    m_cdgTempoTimer.setInterval(250);

    connect(&m_cdgTempoTimer, &QTimer::timeout, this, [this]() {
        if (m_cdgStateBusy) {
            m_cdgTempoTimer.start();
            return;
        }
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

    // Set up video tempo timer for synchronizing video playback with audio playback
    m_videoTempoTimer.setSingleShot(true);
    m_videoTempoTimer.setInterval(250);

    connect(
        &m_videoTempoTimer,
        &QTimer::timeout,
        this,
        [this]() {
            if (!m_rateControlledVideo || !m_pipeline)
                return;

            if (m_state != PlayingState &&
                m_state != PausedState)
                return;

            const double requestedRate =
                static_cast<double>(m_tempoPercent) / 100.0;

            // Do not issue another flushing seek if the
            // requested rate is already applied.
            if (std::abs(requestedRate - m_appliedVideoRate) < 0.0001)
                return;

            seekVideoRate(position());
        }
    );

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
    ++*m_cdgStateRevision;
    // Streaming callbacks refer to this backend until the worker puts both
    // pipelines in NULL. Destruction is the only place that joins the task.
    if (m_cdgStateBusy) {
        m_cdgStateWatcher.waitForFinished();
        m_cdgStateBusy = false;
    }
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
    // Create a scaletempo element for MP4 file tempo adjustment
    m_scaleTempo = gst_element_factory_make(
        "scaletempo", "scale_tempo"
    );
    m_eq = gst_element_factory_make("equalizer-10bands", "eq");
    m_panorama = gst_element_factory_make("audiopanorama", "panorama");
    m_volumeElement = gst_element_factory_make("volume", "vol");
    m_audioSink = gst_element_factory_make("autoaudiosink", "audio_sink");

    if (m_audioBin && m_pitch && m_eq && m_panorama && m_volumeElement && m_audioSink) {
        gst_bin_add_many(GST_BIN(m_audioBin), m_pitch, m_scaleTempo, m_eq, m_panorama, m_volumeElement, m_audioSink, nullptr);
        gst_element_link_many(m_pitch, m_scaleTempo, m_eq, m_panorama, m_volumeElement, m_audioSink, nullptr);
        
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
                else if (new_state == GST_STATE_PAUSED) {
                    if (m_cdgStartPending)
                        break; // Preroll is internal, not a user pause.
                    m_state = PausedState;
                }
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
    const auto generation = backend->m_frameGeneration->load();

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

    if (!ownedImage.isNull() && (generation & 1))
        emit backend->frameReady(ownedImage, generation);

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
                    [frameGeneration = m_frameGeneration, display](const QImage &image, quint64 generation) {
                        // A queued sample can outlive Stop or a track change.
                        if ((generation & 1) && generation == frameGeneration->load())
                            display->updateFrame(image);
                    },
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
    if (m_cdgStateBusy)
        return m_stateChangePosition;
    if (m_pipeline) {
        gint64 pos = 0;
        if (gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos)) {
            return pos / GST_MSECOND;
        }
    }
    return 0;
}

qint64 MediaBackend::duration() {
    if (m_cdgStateBusy)
        return m_stateChangeDuration;
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
    // Even zero-timeout queries can contend with an element's state-change lock.
    if (m_cdgStateBusy)
        return;
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

    if (m_cdgStateBusy)
        return;

    // Handle coordinated seek if needed.
    processCoordinatedSeek();
    processCdgStartup();
    if (m_cdgStateBusy)
        return;

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

        QElapsedTimer queryTimer;
        queryTimer.start();
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
                "{} SYNC tempo={} audio={}ms cdg={}ms diff={}ms querySpan={}us baseDiff={}us",
                m_loggingPrefix,
                m_tempoPercent,
                audioMs,
                cdgMs,
                audioMs - cdgMs,
                queryTimer.nsecsElapsed() / 1000,
                (static_cast<gint64>(gst_element_get_base_time(m_pipeline)) -
                 static_cast<gint64>(gst_element_get_base_time(m_cdgPipeline))) / 1000
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

void MediaBackend::play(bool startSilent) {
    if (deferControl([this, startSilent] { play(startSilent); }))
        return;
    if (!(m_frameGeneration->load() & 1))
        ++*m_frameGeneration;
    m_stopAfterFade = false;
    // Set gain before starting the pipeline, including silent playlist advances.
    if (startSilent)
        m_fader->immediateOut();
    else if (m_fade)
        m_fader->fadeIn(false);
    else
        m_fader->immediateIn();
    if (m_cdgPipeline && m_pipeline) {
        if (m_seekPhase != SeekPhase::Idle) {
            m_resumeAfterSeek = true;
            return;
        }
        prepareCdgPlayback();
        return;
    }
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        // Schedule initial tempo adjustment without tying it to every GStreamer state transition.
        if (m_rateControlledVideo &&
            m_tempoPercent != 100 &&
            std::abs(
                static_cast<double>(m_tempoPercent) / 100.0 -
                m_appliedVideoRate
            ) > 0.0001) {

            m_videoTempoTimer.start();
        }
    }
    
    if (m_cdgPipeline) {
        gst_element_set_state(m_cdgPipeline, GST_STATE_PLAYING);
    }
}

void MediaBackend::prepareCdgPlayback() {
    if (m_cdgStartPending || m_state == PlayingState)
        return;
    m_cdgStartPending = true;
    m_cdgStartTimer.start();
    requestCdgState(GST_STATE_PAUSED);
}

void MediaBackend::processCdgStartup() {
    if (m_cdgStateBusy)
        return;
    // Once both PLAYING transitions finish, let GStreamer manage future pause
    // and flushing-seek running times again. The common base time stays intact.
    if (!m_cdgStartPending) {
        if (m_cdgPipeline && gst_element_get_start_time(m_pipeline) == GST_CLOCK_TIME_NONE) {
            GstState audioState, cdgState;
            const auto audio = gst_element_get_state(m_pipeline, &audioState, nullptr, 0);
            const auto cdg = gst_element_get_state(m_cdgPipeline, &cdgState, nullptr, 0);
            if (audio != GST_STATE_CHANGE_ASYNC && cdg != GST_STATE_CHANGE_ASYNC &&
                audioState == GST_STATE_PLAYING && cdgState == GST_STATE_PLAYING) {
                gst_element_set_start_time(m_pipeline, 0);
                gst_element_set_start_time(m_cdgPipeline, 0);
            }
        }
        return;
    }
    GstState audioState, cdgState;
    const auto audio = gst_element_get_state(m_pipeline, &audioState, nullptr, 0);
    const auto cdg = gst_element_get_state(m_cdgPipeline, &cdgState, nullptr, 0);
    if (audio == GST_STATE_CHANGE_FAILURE || cdg == GST_STATE_CHANGE_FAILURE ||
        m_cdgStartTimer.elapsed() > 5000) {
        rawStop();
        emit audioError("Synchronized CDG playback failed to preroll");
        return;
    }
    if (audio == GST_STATE_CHANGE_ASYNC || cdg == GST_STATE_CHANGE_ASYNC ||
        audioState != GST_STATE_PAUSED || cdgState != GST_STATE_PAUSED)
        return;

    const auto runningTime = gst_element_get_start_time(m_pipeline);
    if (GST_CLOCK_TIME_IS_VALID(runningTime))
        m_cdgRunningTime = runningTime;
    requestCdgState(GST_STATE_PLAYING);
}

bool MediaBackend::deferControl(std::function<void()> control) {
    if (!m_cdgStateBusy)
        return false;
    m_deferredControls.push_back(std::move(control));
    return true;
}

void MediaBackend::requestCdgState(GstState target) {
    if (target != GST_STATE_NULL) {
        m_stateChangePosition = position();
        m_stateChangeDuration = duration();
    }
    m_cdgStateBusy = true;
    auto audio = GST_ELEMENT(gst_object_ref(m_pipeline));
    auto cdg = GST_ELEMENT(gst_object_ref(m_cdgPipeline));
    auto clock = GST_CLOCK(gst_object_ref(m_sharedClock));
    const auto revision = m_cdgStateRevision->load();
    m_cdgStateWatcher.setFuture(QtConcurrent::run(
        [audio, cdg, clock, target, revision, cancellation = m_cdgStateRevision,
         runningTime = m_cdgRunningTime] {
            QElapsedTimer timer;
            timer.start();
            CdgStateResult result{target, revision, GST_STATE_CHANGE_SUCCESS, GST_STATE_CHANGE_SUCCESS, false, 0};
            if (cancellation->load() == revision) {
                if (target == GST_STATE_PLAYING) {
                    const auto base = gst_clock_get_time(clock) + 20 * GST_MSECOND - runningTime;
                    for (auto pipeline : {audio, cdg}) {
                        gst_element_set_start_time(pipeline, GST_CLOCK_TIME_NONE);
                        gst_element_set_base_time(pipeline, base);
                    }
                }
                result.audio = gst_element_set_state(audio, target);
                if (cancellation->load() == revision)
                    result.cdg = gst_element_set_state(cdg, target);
            }
            result.cancelled = cancellation->load() != revision;
            if (result.cancelled || result.audio == GST_STATE_CHANGE_FAILURE || result.cdg == GST_STATE_CHANGE_FAILURE) {
                for (auto pipeline : {audio, cdg}) {
                    gst_element_set_state(pipeline, GST_STATE_NULL);
                    gst_element_set_start_time(pipeline, 0);
                }
            }
            if (target == GST_STATE_NULL) {
                gst_element_set_start_time(audio, 0);
                gst_element_set_start_time(cdg, 0);
            }
            result.elapsedMs = timer.elapsed();
            gst_object_unref(audio);
            gst_object_unref(cdg);
            gst_object_unref(clock);
            return result;
        }));
}

void MediaBackend::pause() {
    if (m_cdgStateBusy) {
        m_cdgStartPending = false;
        m_resumeAfterSeek = false;
        deferControl([this] { pause(); });
        return;
    }
    if (m_cdgPipeline && m_sharedClock && !m_cdgStartPending && m_state == PlayingState) {
        const auto now = gst_clock_get_time(m_sharedClock);
        const auto base = gst_element_get_base_time(m_pipeline);
        m_cdgRunningTime = now > base ? now - base : 0;
    }
    m_cdgStartPending = false;
    m_resumeAfterSeek = false;
    if (m_cdgPipeline && gst_element_get_start_time(m_pipeline) == GST_CLOCK_TIME_NONE) {
        gst_element_set_start_time(m_pipeline, m_cdgRunningTime);
        gst_element_set_start_time(m_cdgPipeline, m_cdgRunningTime);
    }
    if (m_cdgPipeline && m_pipeline) {
        requestCdgState(GST_STATE_PAUSED);
    } else if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    }
}

void MediaBackend::setMedia(const QString &filename) {
    if (m_cdgStateBusy) {
        rawStop();
        deferControl([this, filename] { setMedia(filename); });
        return;
    }
    m_silenceDetectedEmitted = false;
    rawStop();
    m_appliedVideoRate = 1.0;
    destroyCdgPipeline();
    m_filename = filename;
    m_cdgFilename.clear();
    m_tempDir.reset();

    // Determine the actual file path to play, which may be extracted from a zip archive.
    QString playPath = filename;

    // Determine if the media file is a type that requires rate-controlled video playback.
    const QString extension =
    QFileInfo(playPath).suffix().toLower();

    m_rateControlledVideo =
        extension == "mp4" ||
        extension == "m4v" ||
        extension == "mov" ||
        extension == "mkv" ||
        extension == "webm" ||
        extension == "avi";

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

    // Reset SoundTouch when loading another media file, so that tempo and pitch adjustments start fresh.
    // This prevents switching from an MP3+G track at 120% to an MP4 from leaving SoundTouch at 120%, which would cause double speed adjustment.
    if (m_pitch) {
        const double audioTempo =
            m_rateControlledVideo
                ? 1.0
                : static_cast<double>(m_tempoPercent) / 100.0;

        g_object_set(
            G_OBJECT(m_pitch),
            "tempo",
            audioTempo,
            nullptr
        );
    }

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
    if (m_cdgStateBusy) {
        rawStop();
        deferControl([this, cdgFilename, audioFilename] { setMediaCdg(cdgFilename, audioFilename); });
        return;
    }
    QElapsedTimer preparationStage;
    preparationStage.start();
    const auto logStage = [this, &preparationStage](const char *stage) {
        const auto elapsed = preparationStage.restart();
        if (elapsed >= 50)
            m_logger->warn("{} CDG preparation on UI thread: {} took {}ms", m_loggingPrefix, stage, elapsed);
    };
    m_silenceDetectedEmitted = false;
    rawStop();
    m_appliedVideoRate = 1.0;
    destroyCdgPipeline();
    m_filename = audioFilename;
    m_cdgFilename = cdgFilename;
    m_tempDir.reset();
    m_rateControlledVideo = false;
    logStage("pipeline and temporary file cleanup");

    m_tempDir = std::make_unique<QTemporaryDir>();
    if (m_tempDir->isValid()) {
        QFileInfo audioInfo(audioFilename);
        QString targetAudio = m_tempDir->path() + QDir::separator() + "song." + audioInfo.suffix();
        QString targetCdg = m_tempDir->path() + QDir::separator() + "song.cdg";

        if (QFile::copy(audioFilename, targetAudio) && QFile::copy(cdgFilename, targetCdg)) {
            logStage("copying audio and CDG files");
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
            logStage("pipeline setup and CDG analysis");
        } else {
            emit audioError("Failed to copy media files to temp dir");
        }
    } else {
        emit audioError("Failed to create temporary directory");
    }
}

void MediaBackend::setMuted(const bool &muted) {
    m_muted = muted;
    // Also mute playbin itself, including its default sink if the custom
    // audio bin could not be attached (for example, a missing plugin).
    if (m_pipeline)
        g_object_set(G_OBJECT(m_pipeline), "mute", muted, nullptr);
    updateVolume();
    emit mutedChanged(muted);
}

bool MediaBackend::isMuted() {
    return m_muted;
}

void MediaBackend::setPosition(const qint64 &position)
{
    if (deferControl([this, position] { setPosition(position); }))
        return;
    if (!m_pipeline || position < 0)
        return;

    // MP4 and other single-pipeline media keep their
    // existing direct seeking behavior.
    if (!m_cdgPipeline) {
        if (m_rateControlledVideo) {
            m_videoTempoTimer.stop();
            seekVideoRate(position);
        } else {
            gst_element_seek_simple(
                m_pipeline,
                GST_FORMAT_TIME,
                static_cast<GstSeekFlags>(
                    GST_SEEK_FLAG_FLUSH |
                    GST_SEEK_FLAG_ACCURATE
                ),
                static_cast<gint64>(position) * GST_MSECOND
            );
        }

        return;
    }

    // MP3+G: coordinate both pipelines.
    // Preserve the intended playback state, even if another
    // seek arrives while a coordinated seek is underway.
    if (m_seekPhase == SeekPhase::Idle) {
        m_resumeAfterSeek = (m_state == PlayingState || m_cdgStartPending);
    }

    m_cdgStartPending = false;
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

    // Flushing seeks restart the running-time segment at zero.
    if (success)
        m_cdgRunningTime = 0;
    if (shouldResume) {
        prepareCdgPlayback();
    }
}

bool MediaBackend::seekVideoRate(qint64 positionMs)
{
    if (!m_pipeline || positionMs < 0)
        return false;

    const double rate =
        static_cast<double>(m_tempoPercent) / 100.0;

    if (rate <= 0.0)
        return false;

    const gboolean success = gst_element_seek(
        m_pipeline,
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

    m_logger->info(
        "{} Video playback rate={}x position={}ms accepted={}",
        m_loggingPrefix,
        rate,
        positionMs,
        success ? "yes" : "no"
    );

    if (success) {
        m_appliedVideoRate = rate;
    }

    return success;
}

void MediaBackend::setVolume(const int &volume) {
    m_volume = volume;
    updateVolume();
}

void MediaBackend::stop(const bool &skipFade) {
    if (m_fade && !skipFade && m_state == PlayingState) {
        m_stopAfterFade = true;
        m_fader->fadeOut(false);
    } else {
        m_fader->immediateOut();
        rawStop();
    }
}

void MediaBackend::rawStop() {
    QElapsedTimer stopTimer;
    stopTimer.start();
    ++*m_cdgStateRevision;
    m_deferredControls.clear();
    m_stateChangePosition = 0;
    m_stateChangeDuration = 0;
    m_cdgStartPending = false;
    m_cdgRunningTime = 0;
    // Invalidate queued frames before waiting for the streaming threads to stop.
    if (m_frameGeneration->load() & 1)
        ++*m_frameGeneration;
    m_stopAfterFade = false;
    m_fader->immediateOut();
    // Prevent outstanding seek from restarting an old song.
    m_seekPhase = SeekPhase::Idle;
    m_resumeAfterSeek = false;
    m_cdgTempoTimer.stop();
    m_videoTempoTimer.stop();

    if (m_pipeline && !m_cdgStateBusy) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
    }

    if (m_cdgPipeline && !m_cdgStateBusy) {
        gst_element_set_state(m_cdgPipeline, GST_STATE_NULL);
    }
    // Return ownership of timing to GStreamer for subsequent single-pipeline media.
    if (m_pipeline && !m_cdgStateBusy)
        gst_element_set_start_time(m_pipeline, 0);
    if (m_cdgPipeline && !m_cdgStateBusy)
        gst_element_set_start_time(m_cdgPipeline, 0);
    m_state = StoppedState;
    for (auto* display : m_videoSinks) {
        if (display) {
            display->clearFrame();
        }
    }

    m_cdgTempoTimer.stop();
    m_cdgSyncTimer.invalidate();
    m_lastCdgSyncCheckMs = -1;
    m_lastCdgCorrectionMs = -1;
    if (stopTimer.elapsed() >= 50)
        m_logger->warn("{} Playback cleanup on UI thread took {}ms", m_loggingPrefix, stopTimer.elapsed());
    // Consumers may start another track synchronously. Complete cleanup first.
    emit stateChanged(m_state);
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
    m_stopAfterFade = false;
    if (m_fade)
        m_fader->fadeOut(waitForFade);
    else
        m_fader->immediateOut();
}

void MediaBackend::fadeIn(const bool &waitForFade) {
    m_stopAfterFade = false;
    if (m_fade)
        m_fader->fadeIn(waitForFade);
    else
        m_fader->immediateIn();
}

void MediaBackend::fadeInImmediate() {
    m_stopAfterFade = false;
    m_fader->immediateIn();
}

void MediaBackend::fadeOutImmediate() {
    m_stopAfterFade = false;
    m_fader->immediateOut();
}

void MediaBackend::setTempo(const int &percent)
{
    if (deferControl([this, percent] { setTempo(percent); }))
        return;
    if (percent <= 0)
        return;

    // Capture CDG's actual position before changing its rate.
    gint64 cdgPosition = GST_CLOCK_TIME_NONE;
    bool haveCdgPosition = false;

    m_tempoPercent = percent;

    // Update audio tempo through SoundTouch.
    if (m_pitch) {
        const double audioTempo =
            m_rateControlledVideo
                ? 1.0
                : static_cast<double>(percent) / 100.0;

        g_object_set(
            G_OBJECT(m_pitch),
            "tempo",
            audioTempo,
            nullptr
        );
    }

    // Update video playback rate through GStreamer.
    if (m_rateControlledVideo) {
        // MP4: change audio and video together.
        m_cdgTempoTimer.stop();

        if (m_state == PlayingState ||
            m_state == PausedState) {
            m_videoTempoTimer.start();
        }
    } else {
        // MP3+G: preserve the existing debounced CDG behavior.
        m_videoTempoTimer.stop();

        if (m_cdgPipeline &&
            (m_state == PlayingState ||
             m_state == PausedState)) {
            m_cdgTempoTimer.start();
        }
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

    // Diagnostic: log the actual EQ band 5 gain to verify that the GStreamer element is receiving the correct values.
    if (m_eq) {
        double actualGain = 0.0;

        g_object_get(
            G_OBJECT(m_eq),
            "band5",
            &actualGain,
            nullptr
        );

        m_logger->info(
            "{} EQ diagnostic: bypass={} requestedBand5={} actualBand5={}",
            m_loggingPrefix,
            m_eqBypass,
            m_eqLevels[5],
            actualGain
        );
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

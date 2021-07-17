#include "audiorecorder.h"
#include <QDir>
#include <QDateTime>
#include <spdlog/spdlog.h>

void AudioRecorder::generateDeviceList() {
    logger->debug("{} Getting input devices", m_loggingPrefix);
    GstDeviceMonitor *monitor;
    monitor = gst_device_monitor_new();
    GstCaps *moncaps;
    moncaps = gst_caps_new_empty_simple("audio/x-raw");
    gst_device_monitor_add_filter(monitor, "Audio/Source", moncaps);
    gst_caps_unref(moncaps);
    gst_device_monitor_start(monitor);
    m_inputDeviceNames.clear();
    GList *devices, *elem;
    devices = gst_device_monitor_get_devices(monitor);
    for (elem = devices; elem; elem = elem->next) {
        auto device = (GstDevice *) elem->data;
        gchar *deviceName = gst_device_get_display_name(device);
        logger->debug("{} Found audio input device: {}", m_loggingPrefix, deviceName);
        m_inputDeviceNames.append(deviceName);
        g_free(deviceName);
        m_inputDevices.append(device);
    }
    logger->debug("{} Found {} input devices", m_loggingPrefix, m_inputDeviceNames.size());
    gst_device_monitor_stop(monitor);
    g_list_free(devices);
    g_object_unref(monitor);
}

void AudioRecorder::initGStreamer() {
    logger->debug("{} initGStreamer() called", m_loggingPrefix);
#ifndef Q_OS_WIN
    generateDeviceList();
#else
    // Give Windows users something to look at in the dropdown
    m_inputDeviceNames.append("System default");
#endif
    logger->debug("{} Initializing gstreamer", m_loggingPrefix);
    gst_init(nullptr, nullptr);
    logger->debug("{} Creating elements", m_loggingPrefix);
#ifdef Q_OS_WIN
    m_autoAudioSrc    = gst_element_factory_make("autoaudiosrc", NULL);
    if (!m_autoAudioSrc)
        logger->error("{} Failed to create autoAudioSrc", m_loggingPrefix);
#endif
    m_audioConvert = gst_element_factory_make("audioconvert", nullptr);
    if (!m_audioConvert)
        logger->error("{} Failed to create audioConvert", m_loggingPrefix);
    m_fileSink = gst_element_factory_make("filesink", nullptr);
    if (!m_fileSink)
        logger->error("{} Failed to create fileSink", m_loggingPrefix);
    m_audioRate = gst_element_factory_make("audiorate", nullptr);
    if (!m_audioRate)
        logger->error("{} Failed to create audioRate", m_loggingPrefix);
    m_oggMux = gst_element_factory_make("oggmux", nullptr);
    if (!m_oggMux)
        logger->error("{} Failed to create oggMux", m_loggingPrefix);
    m_vorbisEnc = gst_element_factory_make("vorbisenc", nullptr);
    if (!m_vorbisEnc)
        logger->error("{} Failed to create vorbisEnc", m_loggingPrefix);
    m_lameMp3Enc = gst_element_factory_make("lamemp3enc", nullptr);
    if (!m_lameMp3Enc)
        logger->error("{} Failed to create lameMp3Enc", m_loggingPrefix);
    m_wavEnc = gst_element_factory_make("wavenc", nullptr);
    if (!m_wavEnc)
        logger->error("{} Failed to create wavEnc", m_loggingPrefix);
#ifndef Q_OS_WIN
    if (m_inputDevices.isEmpty())
        m_audioSrc = gst_element_factory_make("fakesrc", "fakesrc");
    else
        m_audioSrc = gst_device_create_element(m_inputDevices.at(0), nullptr);
#endif
    m_pipeline = gst_pipeline_new("pipeline");
    if (!m_pipeline)
        logger->error("{} Failed to create pipeline", m_loggingPrefix);
    m_bus = gst_element_get_bus(m_pipeline);
    if (!m_bus)
        logger->error("{} Failed to create bus", m_loggingPrefix);
    logger->debug("{} Elements created, adding to pipeline and linking", m_loggingPrefix);
    g_object_set(m_vorbisEnc, "quality", 0.9, nullptr);
#ifdef Q_OS_WIN
    gst_bin_add_many(GST_BIN (m_pipeline), m_autoAudioSrc, m_audioRate, m_audioConvert, m_lameMp3Enc, m_wavEnc, m_vorbisEnc, m_oggMux, m_fileSink, nullptr);
    bool result = gst_element_link_many(m_autoAudioSrc, m_audioRate, m_audioConvert, m_vorbisEnc, m_oggMux, m_fileSink, nullptr);
#else
    gst_bin_add_many(GST_BIN (m_pipeline), m_audioSrc, m_audioRate, m_audioConvert, m_lameMp3Enc, m_wavEnc, m_vorbisEnc, m_oggMux,
                     m_fileSink, nullptr);
    bool result = gst_element_link_many(m_audioSrc, m_audioRate, m_audioConvert, m_vorbisEnc, m_oggMux, m_fileSink, nullptr);
#endif
    if (!result)
        logger->info("{} [gstreamer] Error linking elements", m_loggingPrefix);
    getRecordingSettings();
    logger->debug("{} initGStreamer() completed", m_loggingPrefix);
}


void AudioRecorder::processGstMessage() {
    bool done{false};
    while (!done) {
        if (auto message = gst_bus_pop(m_bus); message) {
            switch (message->type) {
                case GST_MESSAGE_STATE_CHANGED:
                    break;
                case GST_MESSAGE_WARNING:
                case GST_MESSAGE_ERROR:
                    GError *err;
                    gchar *debug;
                    if (message->type == GST_MESSAGE_WARNING) {
                        gst_message_parse_warning(message, &err, &debug);
                        logger->warn("{} [gstreamer] {}", m_loggingPrefix, err->message);
                    } else {
                        gst_message_parse_error(message, &err, &debug);
                        logger->error("{} [gstreamer] {}", m_loggingPrefix, err->message);
                    }
                    logger->debug("{} [gstreamer] {}", m_loggingPrefix, debug);
                    gst_message_unref(message);
                    g_error_free(err);
                    g_free(debug);
                    break;
                default:
                    logger->debug("{} [gstreamer] Unhandled GStreamer msg received - Element: {} - Type: {} - Name: {}",
                                  m_loggingPrefix,
                                  message->src->name,
                                  GST_MESSAGE_TYPE(message),
                                  GST_MESSAGE_TYPE_NAME(message)
                    );
            }
            gst_message_unref(message);
        } else {
            done = true;
        }
    }
}


void AudioRecorder::getRecordingSettings() {
    QString captureDevice = m_settings.recordingInput();
    m_currentDevice = m_inputDeviceNames.indexOf(captureDevice);
    if ((m_currentDevice == -1) || (m_currentDevice >= m_inputDevices.size()))
        m_currentDevice = 0;
    int codec = m_codecs.indexOf(m_settings.recordingCodec());
    if (codec == -1)
        codec = 1;
    setCurrentCodec(codec);
}

void AudioRecorder::record(const QString &filename) {
    getRecordingSettings();
    setInputDevice(m_currentDevice);
    logger->info("{} Recording to file: {}", m_loggingPrefix, filename.toStdString());
    setOutputFile(filename);
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

void AudioRecorder::stop() {
    logger->info("{} Stopping recording", m_loggingPrefix);
    gst_element_set_state(m_pipeline, GST_STATE_NULL);

}

void AudioRecorder::pause() {
    logger->info("{} Pausing recording", m_loggingPrefix);
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);

}

void AudioRecorder::unpause() {
    logger->info("{} Resuming recording", m_loggingPrefix);
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);

}

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent) {
    logger = spdlog::get("logger");
    logger->info("{} Initializing AudioRecorder instance", m_loggingPrefix);
    m_startDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm");
    initGStreamer();
    getRecordingSettings();
    m_timer.start(100);
    connect(&m_timer, &QTimer::timeout, [&]() {
        processGstMessage();
    });
}

AudioRecorder::~AudioRecorder() {
    logger->debug("{} AudioRecorder destructor called", m_loggingPrefix);
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    g_object_unref(m_pipeline);
}

QStringList AudioRecorder::getDeviceList() {
    return m_inputDeviceNames;
}

QStringList AudioRecorder::getCodecs() {
    return m_codecs;
}

void AudioRecorder::setOutputFile(const QString &filename) {
    QString outputDir = m_settings.recordingOutputDir() + QDir::separator() + "Karaoke Recordings" + QDir::separator() +
                        "Show Beginning " + m_startDateTime;
    QDir dir;
    std::string outputFilePath;
    dir.mkpath(outputDir);
#ifdef Q_OS_WIN
    outputFilePath = outputDir.toStdString() + "/" + filename.toStdString() + "." + m_currentFileExt.toStdString();
#else
    outputFilePath = outputDir.toStdString() + "/" + filename.toStdString() + m_currentFileExt.toStdString();
#endif
    logger->info("{} AudioRecorder - Capturing to: {}", m_loggingPrefix, outputFilePath);
    g_object_set(GST_OBJECT(m_fileSink), "location", outputFilePath.c_str(), nullptr);
}

void AudioRecorder::setInputDevice(const int inputDeviceId) {
#ifndef Q_OS_WIN
    logger->debug("{} setInputDevice({}) called", m_loggingPrefix, inputDeviceId);
    gst_element_unlink(m_audioSrc, m_audioRate);
    gst_bin_remove(GST_BIN(m_pipeline), m_audioSrc);
    m_audioSrc = gst_device_create_element(m_inputDevices.at(inputDeviceId), nullptr);
    if (!m_audioSrc)
        logger->error("{} Error creating audio source element", m_loggingPrefix);
    gst_bin_add(GST_BIN(m_pipeline), m_audioSrc);
    gst_element_link(m_audioSrc, m_audioRate);
#endif
}

void AudioRecorder::setCurrentCodec(const int value) {
    static int lastCodec = 1;
    if (value != lastCodec) {
        // Unlink previous encoder in pipeline
        if (lastCodec == 0) {
            // mp3
            gst_element_unlink(m_audioConvert, m_lameMp3Enc);
            gst_element_unlink(m_lameMp3Enc, m_fileSink);
        } else if (lastCodec == 1) {
            //ogg
            gst_element_unlink(m_audioConvert, m_vorbisEnc);
            gst_element_unlink(m_vorbisEnc, m_oggMux);
            gst_element_unlink(m_oggMux, m_fileSink);
        } else if (lastCodec == 2) {
            //wav
            gst_element_unlink(m_audioConvert, m_wavEnc);
            gst_element_unlink(m_wavEnc, m_fileSink);
        }

        // Link new encoder in pipeline
        if (value == 0) {
            // mp3
            gst_element_link(m_audioConvert, m_lameMp3Enc);
            gst_element_link(m_lameMp3Enc, m_fileSink);
        } else if (value == 1) {
            //ogg
            gst_element_link(m_audioConvert, m_vorbisEnc);
            gst_element_link(m_vorbisEnc, m_oggMux);
            gst_element_link(m_oggMux, m_fileSink);
        } else if (value == 2) {
            //wav
            gst_element_link(m_audioConvert, m_wavEnc);
            gst_element_link(m_wavEnc, m_fileSink);
        }

        logger->debug("{} AudioRecorder::setCurrentCodec({})", m_loggingPrefix, value);
        lastCodec = value;
        m_currentFileExt = m_fileExtensions.at(value);
    }
}

#include "audiorecorder.h"
#include <QDir>
#include <QDateTime>
#include <spdlog/spdlog.h>

void AudioRecorder::generateDeviceList()
{
    logger->debug("{} Getting input devices", m_loggingPrefix);
    GstDeviceMonitor *monitor;
    monitor = gst_device_monitor_new ();
    GstCaps *moncaps;
    moncaps = gst_caps_new_empty_simple ("audio/x-raw");
    gst_device_monitor_add_filter (monitor, "Audio/Source", moncaps);
    gst_caps_unref (moncaps);
    gst_device_monitor_start (monitor);
    inputDeviceNames.clear();
    GList *devices, *elem;
    devices = gst_device_monitor_get_devices(monitor);
    for(elem = devices; elem; elem = elem->next) {
        GstDevice *device = (GstDevice*)elem->data;
        gchar *deviceName = gst_device_get_display_name(device);
        logger->debug("{} Found audio input device: {}", m_loggingPrefix, deviceName);
        inputDeviceNames.append(deviceName);
        g_free(deviceName);
        inputDevices.append(device);
    }
    logger->debug("{} Found {} input devices", m_loggingPrefix, inputDeviceNames.size());
    gst_device_monitor_stop(monitor);
    g_list_free(devices);
    g_object_unref(monitor);
}

void AudioRecorder::initGStreamer()
{
    logger->debug("{} initGStreamer() called", m_loggingPrefix);
#ifndef Q_OS_WIN
    generateDeviceList();
#else
    // Give Windows users something to look at in the dropdown
    inputDeviceNames.append("System default");
#endif
    logger->debug("{} Initializing gstreamer", m_loggingPrefix);
    gst_init(NULL,NULL);
    logger->debug("{} Creating elements", m_loggingPrefix);
#ifdef Q_OS_WIN
    autoAudioSrc    = gst_element_factory_make("autoaudiosrc", NULL);
    if (!autoAudioSrc)
        logger->error("{} Failed to create autoAudioSrc", m_loggingPrefix);
#endif
    audioConvert    = gst_element_factory_make("audioconvert", NULL);
    if (!audioConvert)
        logger->error("{} Failed to create audioConvert", m_loggingPrefix);
    fileSink        = gst_element_factory_make("filesink", NULL);
    if (!fileSink)
        logger->error("{} Failed to create fileSink", m_loggingPrefix);
    audioRate       = gst_element_factory_make("audiorate", NULL);
    if (!audioRate)
        logger->error("{} Failed to create audioRate", m_loggingPrefix);
    oggMux          = gst_element_factory_make("oggmux", NULL);
    if (!oggMux)
        logger->error("{} Failed to create oggMux", m_loggingPrefix);
    vorbisEnc       = gst_element_factory_make("vorbisenc", NULL);
    if (!vorbisEnc)
        logger->error("{} Failed to create vorbisEnc", m_loggingPrefix);
    lameMp3Enc      = gst_element_factory_make("lamemp3enc", NULL);
    if (!lameMp3Enc)
        logger->error("{} Failed to create lameMp3Enc", m_loggingPrefix);
    else
    {
        //g_object_set(lameMp3Enc, "quality", 2, NULL);
        //g_object_set(lameMp3Enc, "mono", false, NULL);
    }
    wavEnc          = gst_element_factory_make("wavenc", NULL);
    if (!wavEnc)
        logger->error("{} Failed to create wavEnc", m_loggingPrefix);
#ifndef Q_OS_WIN
    if (inputDevices.isEmpty())
        audioSrc = gst_element_factory_make("fakesrc", "fakesrc");
    else
        audioSrc        = gst_device_create_element(inputDevices.at(0), NULL);
#endif
    pipeline        = gst_pipeline_new("pipeline");
    if (!pipeline)
        logger->error("{} Failed to create pipeline", m_loggingPrefix);
    bus             = gst_element_get_bus (pipeline);
    if (!bus)
        logger->error("{} Failed to create bus", m_loggingPrefix);
    logger->debug("{} Elements created, adding to pipeline and linking", m_loggingPrefix);
    g_object_set(vorbisEnc, "quality", 0.9, nullptr);
#ifdef Q_OS_WIN
    gst_bin_add_many(GST_BIN (pipeline), autoAudioSrc, audioRate, audioConvert, lameMp3Enc, wavEnc, vorbisEnc, oggMux, fileSink, nullptr);
    bool result = gst_element_link_many(autoAudioSrc, audioRate, audioConvert, vorbisEnc, oggMux, fileSink, nullptr);
#else
    gst_bin_add_many(GST_BIN (pipeline), audioSrc, audioRate, audioConvert, lameMp3Enc, wavEnc, vorbisEnc, oggMux, fileSink, nullptr);
    bool result = gst_element_link_many(audioSrc, audioRate, audioConvert, vorbisEnc, oggMux, fileSink, nullptr);
#endif
    if (result == false)
        logger->info("{} [gstreamer] Error linking elements", m_loggingPrefix);
    getRecordingSettings();
    logger->debug("{} initGStreamer() completed", m_loggingPrefix);
}


void AudioRecorder::processGstMessage()
{
    GstMessage *message = NULL;
    bool done = false;
    while (!done)
    {
        message = gst_bus_pop(bus);
        if (message != NULL)
        {
            if (message->type == GST_MESSAGE_STATE_CHANGED)
            {
//                GstState state;
//                gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
//                if (state == GST_STATE_PLAYING)
//                    logger->info("{} AudioRecorder - Gst - State changed to PLAYING";
//                else if (state == GST_STATE_PAUSED)
//                    logger->info("{} AudioRecorder - Gst - State changed to PAUSED";
//                else if (state == GST_STATE_NULL)
//                    logger->info("{} AudioRecorder - Gst - State changed to NULL";
//                else
//                    logger->info("{} AudioRecorder - Gst - State changed to undefined state";
            }
            else if (message->type == GST_MESSAGE_WARNING || message->type == GST_MESSAGE_ERROR)
            {
                logger->info("{} [gstreamer] - Error or warning generated", m_loggingPrefix);
//              GError *gerror;
//              gchar *debug;

//              gst_message_parse_error (message, &gerror, &debug);
//              gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);

              GError *err;
              gchar *debug;
              if (message->type == GST_MESSAGE_WARNING)
              {
                gst_message_parse_warning(message, &err, &debug);
                logger->warn("{} [gstreamer] {}", m_loggingPrefix, err->message);
              }
              else
              {
                gst_message_parse_error(message, &err, &debug);
                logger->error("{} [gstreamer] {}", m_loggingPrefix, err->message);
              }
              logger->debug("{} [gstreamer] {}", m_loggingPrefix, debug);
              gst_message_unref (message);
              g_error_free (err);
              g_free (debug);
            }
            else
            {
                logger->debug("{} [gstreamer] Unhandled GStreamer msg received - Element: {} - Type: {} - Name: {}",
                              m_loggingPrefix,
                              message->src->name,
                              GST_MESSAGE_TYPE(message),
                              GST_MESSAGE_TYPE_NAME(message)
                              );
            }
            gst_message_unref(message);
        }
        else
        {
            done = true;
        }
    }
}

int AudioRecorder::getCurrentCodec() const
{
    logger->trace("{} AudioRecorder::getCurrentCodec() called.  Return: {}", m_loggingPrefix, currentCodec);
    return currentCodec;
}



void AudioRecorder::timerFired()
{
    processGstMessage();
}

void AudioRecorder::getRecordingSettings()
{
    QString captureDevice = settings.recordingInput();
    currentDevice = inputDeviceNames.indexOf(captureDevice);
    if ((currentDevice == -1) || (currentDevice >= inputDevices.size()))
        currentDevice = 0;
    int codec = codecs.indexOf(settings.recordingCodec());
    if (codec == -1)
        codec = 1;
    setCurrentCodec(codec);
}

void AudioRecorder::record(QString filename)
{
    getRecordingSettings();
    setInputDevice(currentDevice);

    logger->info("{} Recording to file: {}", m_loggingPrefix, filename.toStdString());
    setOutputFile(filename);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void AudioRecorder::stop()
{
    logger->info("{} Stopping recording", m_loggingPrefix);
    gst_element_set_state(pipeline, GST_STATE_NULL);

}

void AudioRecorder::pause()
{
    logger->info("{} Pausing recording", m_loggingPrefix);
    gst_element_set_state(pipeline, GST_STATE_PAUSED);

}

void AudioRecorder::unpause()
{
    logger->info("{} Resuming recording", m_loggingPrefix);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

}

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent)
{
    logger = spdlog::get("logger");
    logger->info("{} Initializing AudioRecorder instance", m_loggingPrefix);
    currentFileExt = ".ogg";
    currentCodec = 1;
    codecs << "MPEG 2 Layer 3 (mp3)";
    fileExtensions << ".mp3";
    codecs << "OGG Vorbis";
    fileExtensions << ".ogg";
    codecs << "WAV/PCM";
    fileExtensions << ".wav";
    startDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm");
    initGStreamer();
    getRecordingSettings();
    timer = new QTimer(this);
    timer->start(100);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerFired()));
}

AudioRecorder::~AudioRecorder()
{
    logger->debug("{} AudioRecorder destructor called", m_loggingPrefix);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_object_unref(pipeline);

}

void AudioRecorder::refreshDeviceList()
{
    generateDeviceList();
}

QStringList AudioRecorder::getDeviceList()
{
    return inputDeviceNames;
}

QStringList AudioRecorder::getCodecs()
{
    return codecs;
}

QString AudioRecorder::getCodecFileExtension(int CodecId)
{
    return fileExtensions.at(CodecId);
}

void AudioRecorder::setOutputFile(QString filename)
{
    QString outputDir = settings.recordingOutputDir() + QDir::separator() + "Karaoke Recordings" + QDir::separator() + "Show Beginning " + startDateTime;
    QDir dir;
    std::string outputFilePath;
    dir.mkpath(outputDir);
  //  m_filename = filename;
#ifdef Q_OS_WIN
    outputFilePath = outputDir.toStdString() + "/" + filename.toStdString() + "." + currentFileExt.toStdString();
#else
    outputFilePath = outputDir.toStdString() + "/" + filename.toStdString() + currentFileExt.toStdString();
#endif
    logger->info("{} AudioRecorder - Capturing to: {}", m_loggingPrefix, outputFilePath);
    g_object_set(GST_OBJECT(fileSink), "location", outputFilePath.c_str(), nullptr);
}

void AudioRecorder::setInputDevice(int inputDeviceId)
{
#ifndef Q_OS_WIN
    logger->debug("{} setInputDevice({}) called", m_loggingPrefix, inputDeviceId);
    gst_element_unlink(audioSrc, audioRate);
    gst_bin_remove(GST_BIN(pipeline), audioSrc);
    audioSrc = gst_device_create_element(inputDevices.at(inputDeviceId), NULL);
    if (!audioSrc)
        logger->error("{} Error creating audio source element", m_loggingPrefix);
    gst_bin_add(GST_BIN(pipeline), audioSrc);
    gst_element_link(audioSrc, audioRate);
#endif
}

void AudioRecorder::setCurrentCodec(int value)
{
    static int lastCodec = 1;
    if (value != lastCodec)
    {
        // Unlink previous encoder in pipeline
        if (lastCodec == 0)
        {
            // mp3
            gst_element_unlink(audioConvert,lameMp3Enc);
            gst_element_unlink(lameMp3Enc,fileSink);
        }
        else if (lastCodec == 1)
        {
            //ogg
            gst_element_unlink(audioConvert,vorbisEnc);
            gst_element_unlink(vorbisEnc,oggMux);
            gst_element_unlink(oggMux,fileSink);
        }
        else if (lastCodec == 2)
        {
            //wav
            gst_element_unlink(audioConvert,wavEnc);
            gst_element_unlink(wavEnc,fileSink);
        }

        // Link new encoder in pipeline
        if (value == 0)
        {
            // mp3
            gst_element_link(audioConvert,lameMp3Enc);
            gst_element_link(lameMp3Enc,fileSink);
        }
        else if (value == 1)
        {
            //ogg
            gst_element_link(audioConvert,vorbisEnc);
            gst_element_link(vorbisEnc,oggMux);
            gst_element_link(oggMux,fileSink);
        }
        else if (value == 2)
        {
            //wav
            gst_element_link(audioConvert,wavEnc);
            gst_element_link(wavEnc,fileSink);
        }

        logger->debug("{} AudioRecorder::setCurrentCodec({})", m_loggingPrefix, value);
        currentCodec = value;
        lastCodec = value;
        currentFileExt = fileExtensions.at(value);
    }
}

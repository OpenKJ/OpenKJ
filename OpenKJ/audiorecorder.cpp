#include "audiorecorder.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include "settings.h"

extern Settings *settings;






void AudioRecorder::generateDeviceList()
{
    qWarning() << "AudioRecorder::generateDeviceList() called";
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
        inputDeviceNames.append(gst_device_get_display_name(device));
        inputDevices.append(device);
    }
}

void AudioRecorder::initGStreamer()
{
    qWarning() << "AudioRecorder::initGStreamer() called";
#ifndef Q_OS_WIN
    generateDeviceList();
#else
    // Give Windows users something to look at in the dropdown
    inputDeviceNames.append("System default");
#endif
    qWarning() << "AudioRecorder - Initializing gstreamer";
    gst_init(NULL,NULL);
    qWarning() << "AudioRecorder - Creating elements";
    autoAudioSrc    = gst_element_factory_make("autoaudiosrc", NULL);
    if (!autoAudioSrc)
        qCritical() << "Failed to create autoAudioSrc";
    audioConvert    = gst_element_factory_make("audioconvert", NULL);
    if (!audioConvert)
        qCritical() << "Failed to create audioConvert";
    fileSink        = gst_element_factory_make("filesink", NULL);
    if (!fileSink)
        qCritical() << "Failed to create fileSink";
    audioRate       = gst_element_factory_make("audiorate", NULL);
    if (!audioRate)
        qCritical() << "Failed to create audioRate";
    oggMux          = gst_element_factory_make("oggmux", NULL);
    if (!oggMux)
        qCritical() << "Failed to create oggMux";
    vorbisEnc       = gst_element_factory_make("vorbisenc", NULL);
    if (!vorbisEnc)
        qCritical() << "Failed to create vorbisEnc";
    lameMp3Enc      = gst_element_factory_make("lamemp3enc", NULL);
    if (!lameMp3Enc)
        qCritical() << "Failed to create lameMp3Enc";
    else
    {
        //g_object_set(lameMp3Enc, "quality", 2, NULL);
        //g_object_set(lameMp3Enc, "mono", false, NULL);
    }
    wavEnc          = gst_element_factory_make("wavenc", NULL);
    if (!wavEnc)
        qCritical() << "Failed to create wavEnc";
#ifndef Q_OS_WIN
    audioSrc        = gst_device_create_element(inputDevices.at(0), NULL);
#endif
    pipeline        = gst_pipeline_new("pipeline");
    if (!pipeline)
        qCritical() << "Failed to create pipeline";
    bus             = gst_element_get_bus (pipeline);
    if (!bus)
        qCritical() << "Failed to create bus";
    qWarning() << "Elements created, adding to pipeline and linking";
    g_object_set(vorbisEnc, "quality", 0.9, NULL);
#ifdef Q_OS_WIN
    gst_bin_add_many(GST_BIN (pipeline), autoAudioSrc, audioRate, audioConvert, lameMp3Enc, wavEnc, vorbisEnc, oggMux, fileSink, NULL);
    bool result = gst_element_link_many(autoAudioSrc, audioRate, audioConvert, vorbisEnc, oggMux, fileSink, NULL);
#else
    gst_bin_add_many(GST_BIN (pipeline), audioSrc, audioRate, audioConvert, lameMp3Enc, wavEnc, vorbisEnc, oggMux, fileSink, NULL);
    bool result = gst_element_link_many(audioSrc, audioRate, audioConvert, vorbisEnc, oggMux, fileSink, NULL);
#endif
    if (result == false)
        qWarning() << "Gst - Error linking elements";
    getRecordingSettings();
    qWarning() << "AudioRecorder::initGStreamer() completed";
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
//                    qWarning() << "AudioRecorder - Gst - State changed to PLAYING";
//                else if (state == GST_STATE_PAUSED)
//                    qWarning() << "AudioRecorder - Gst - State changed to PAUSED";
//                else if (state == GST_STATE_NULL)
//                    qWarning() << "AudioRecorder - Gst - State changed to NULL";
//                else
//                    qWarning() << "AudioRecorder - Gst - State changed to undefined state";
            }
            else if (message->type == GST_MESSAGE_WARNING || message->type == GST_MESSAGE_ERROR)
            {
                qWarning() << "AudioRecorder - Gst - Error or warning generated";
              GError *gerror;
              gchar *debug;

              gst_message_parse_error (message, &gerror, &debug);
              gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
              gst_message_unref (message);
              g_error_free (gerror);
              g_free (debug);
            }
            else
            {
                g_print("Msg type[%d], Msg type name[%s]\n", GST_MESSAGE_TYPE(message), GST_MESSAGE_TYPE_NAME(message));
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
    qWarning() << "AudioRecorder::getCurrentCodec() called.  Return: " << currentCodec;
    return currentCodec;
}



void AudioRecorder::timerFired()
{
    processGstMessage();
}

void AudioRecorder::getRecordingSettings()
{
    QString captureDevice = settings->recordingInput();
    currentDevice = inputDeviceNames.indexOf(captureDevice);
    if ((currentDevice == -1) || (currentDevice >= inputDevices.size()))
        currentDevice = 0;
    int codec = codecs.indexOf(settings->recordingCodec());
    if (codec == -1)
        codec = 1;
    setCurrentCodec(codec);
}

void AudioRecorder::record(QString filename)
{
    getRecordingSettings();
    setInputDevice(currentDevice);

    qWarning() << "AudioRecorder::record(" << filename << ") called";
    setOutputFile(filename);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void AudioRecorder::stop()
{
    qWarning() << "AudioRecorder::stop() called";
    gst_element_set_state(pipeline, GST_STATE_NULL);

}

void AudioRecorder::pause()
{
    qWarning() << "AudioRecorder::pause() called";
    gst_element_set_state(pipeline, GST_STATE_PAUSED);

}

void AudioRecorder::unpause()
{
    qWarning() << "AudioRecorder::unpause() called";
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

}

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent)
{
    qWarning() << "AudioRecorder constructor called";
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
    QString outputDir = settings->recordingOutputDir() + QDir::separator() + "Karaoke Recordings" + QDir::separator() + "Show Beginning " + startDateTime;
    QDir dir;
    std::string outputFilePath;
    dir.mkpath(outputDir);
  //  m_filename = filename;
#ifdef Q_OS_WIN
    outputFilePath = outputDir.toStdString() + "/" + filename.toStdString() + "." + currentFileExt.toStdString();
#else
    outputFilePath = outputDir.toStdString() + "/" + filename.toStdString() + currentFileExt.toStdString();
#endif
    qWarning() << "AudioRecorder - Capturing to: " << QString(outputFilePath.c_str());
    g_object_set(GST_OBJECT(fileSink), "location", outputFilePath.c_str(), NULL);
}

void AudioRecorder::setInputDevice(int inputDeviceId)
{
#ifndef Q_OS_WIN
    qWarning() << "AudioRecorder::setInputDevice(" << inputDeviceId << ") called";
    gst_element_unlink(audioSrc, audioRate);
    gst_bin_remove(GST_BIN(pipeline), audioSrc);
    audioSrc = gst_device_create_element(inputDevices.at(inputDeviceId), NULL);
    if (!audioSrc)
        qWarning() << "Error creating audio source element";
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

        qWarning() << "AudioRecorder::setCurrentCodec(" << value << ")";
        currentCodec = value;
        lastCodec = value;
        currentFileExt = fileExtensions.at(value);
    }
}

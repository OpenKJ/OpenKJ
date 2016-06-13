#include "khaudiobackendqtmultimedia.h"
#include <QDebug>
#include <QApplication>
#include <QAudioDeviceInfo>
#include <numeric>


KhAudioBackendQtMultimedia::KhAudioBackendQtMultimedia(QObject *parent) : KhAbstractAudioBackend(parent)
{
    m_muted = false;
    m_silent = false;
    m_detectSilence = false;
    m_fade = false;
    m_changingDevice = false;
    m_downmix = false;
    silenceDetectTimer = new QTimer(this);
    silenceDetectTimer->start(1000);
    m_array = new QByteArray();
    QAudioFormat format = QAudioDeviceInfo::defaultOutputDevice().preferredFormat();
    if (m_downmix)
        format.setChannelCount(1);
    buffer = new QBuffer(m_array, this);
    buffer->open(QIODevice::ReadWrite);
    stProxy = new StProxyIODevice(buffer, this);
    stProxy->open(QIODevice::ReadOnly);
    audioDecoder = new QAudioDecoder(this);
    audioOutput = new QAudioOutput(format,this);
    audioDecoder->setAudioFormat(format);
    audioOutput->setNotifyInterval(20);

    fader = new FaderQtMultimedia(audioOutput, this);

    connect(audioDecoder, SIGNAL(bufferReady()), this, SLOT(processAudio()));
    connect(audioDecoder, SIGNAL(durationChanged(qint64)), this, SLOT(durationReceived(qint64)));
    connect(audioOutput, SIGNAL(notify()), this, SLOT(audioOutputNotify()));
    connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioOutputStateChanged(QAudio::State)));
    connect(silenceDetectTimer, SIGNAL(timeout()), this, SLOT(silenceDetectTimerTimeout()));
    connect(fader, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
}


int KhAudioBackendQtMultimedia::volume()
{
    return audioOutput->volume() * 100;
}

qint64 KhAudioBackendQtMultimedia::position()
{
    int curBufferPos = buffer->pos();
    int usPos = audioOutput->format().durationForBytes(curBufferPos);
    qint64 msPos = usPos * 0.001;
    return msPos;
}

bool KhAudioBackendQtMultimedia::isMuted()
{
    return m_muted;
}

qint64 KhAudioBackendQtMultimedia::duration()
{
    return audioDecoder->duration();
}

KhAbstractAudioBackend::State KhAudioBackendQtMultimedia::state()
{
    QAudio::State state = audioOutput->state();
    switch (state) {
    case QAudio::StoppedState:
        // Stopped for other reasons
        return KhAbstractAudioBackend::StoppedState;
        break;
    case QAudio::ActiveState:
        return KhAbstractAudioBackend::PlayingState;
        break;
    case QAudio::SuspendedState:
        return KhAbstractAudioBackend::PausedState;
        break;
    default:
        break;
    }
    return KhAbstractAudioBackend::StoppedState;
}

bool KhAudioBackendQtMultimedia::canPitchShift()
{
    return false;
}

int KhAudioBackendQtMultimedia::pitchShift()
{
    return 0;
}

bool KhAudioBackendQtMultimedia::canFade()
{
    return true;
}

QString KhAudioBackendQtMultimedia::backendName()
{
    return QString("QtMultimedia");
}

bool KhAudioBackendQtMultimedia::canDetectSilence()
{
    return true;
}

bool KhAudioBackendQtMultimedia::isSilent()
{
    return m_silent;
}

bool KhAudioBackendQtMultimedia::canDownmix()
{
    return true;
}

bool KhAudioBackendQtMultimedia::downmixChangeRequiresRestart()
{
    return false;
}

void KhAudioBackendQtMultimedia::setOutputDevice(int deviceIndex)
{
    if (this->deviceIndex != deviceIndex)
    {
        m_changingDevice = true;
        this->deviceIndex = deviceIndex;
        int curVolume = audioOutput->volume();
        int curPosition = position();
        QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
        qCritical() << "User selected device: " << devices.at(deviceIndex).deviceName();
        QAudio::State currentState = audioOutput->state();
        stop(true);
        delete(audioOutput);
        audioOutput = new QAudioOutput(audioDecoder->audioFormat(),this);
        audioOutput->setVolume(curVolume);
        if (m_downmix)

        if (currentState == QAudio::ActiveState || currentState == QAudio::SuspendedState)
        {
            play();
            setPosition(curPosition);
        }
        if (currentState == QAudio::SuspendedState)
            pause();
        connect(audioOutput, SIGNAL(notify()), this, SLOT(audioOutputNotify()));
        connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioOutputStateChanged(QAudio::State)));
        audioOutput->setNotifyInterval(20);
        m_changingDevice = false;
    }
}

bool KhAudioBackendQtMultimedia::stopping()
{
    return false;
}

void KhAudioBackendQtMultimedia::play()
{
    if (audioOutput->state() == QAudio::SuspendedState)
        audioOutput->resume();
    else
        audioDecoder->start();
}

void KhAudioBackendQtMultimedia::pause()
{
    audioOutput->suspend();
}

void KhAudioBackendQtMultimedia::setMedia(QString filename)
{
    audioDecoder->setSourceFilename(filename);
}

void KhAudioBackendQtMultimedia::setMuted(bool mute)
{
    static float preMuteVol;
    if (mute != m_muted)
    {
        if (mute)
        {
            preMuteVol = audioOutput->volume();
            audioOutput->setVolume(0.0);
            m_muted = true;
        }
        else
        {
            audioOutput->setVolume(preMuteVol);
            m_muted = false;
        }
    }
}

void KhAudioBackendQtMultimedia::setPosition(qint64 position)
{
    qint64 bytePos = audioOutput->format().bytesForDuration(position * 1000);
    buffer->seek(bytePos);
}

void KhAudioBackendQtMultimedia::setVolume(int volume)
{
    audioOutput->setVolume(volume * .01);
}

void KhAudioBackendQtMultimedia::stop(bool skipFade)
{
    if ((m_fade) && (!skipFade))
        fadeOut();
    audioDecoder->stop();
    audioOutput->stop();
    buffer->buffer().clear();
    buffer->seek(0);
    if (m_fade && !skipFade)
        fader->restoreVolume();
}

void KhAudioBackendQtMultimedia::setPitchShift(int pitchShift)
{
}

void KhAudioBackendQtMultimedia::fadeOut()
{
    fader->fadeOut();
}

void KhAudioBackendQtMultimedia::fadeIn()
{
    fader->fadeIn();
}

void KhAudioBackendQtMultimedia::setUseFader(bool fade)
{
    m_fade = fade;
}

void KhAudioBackendQtMultimedia::setUseSilenceDetection(bool enabled)
{
    m_detectSilence = enabled;
}

void KhAudioBackendQtMultimedia::setDownmix(bool enabled)
{
    if (m_downmix != enabled)
    {
        m_downmix = enabled;
        QAudioFormat format = QAudioDeviceInfo::defaultOutputDevice().preferredFormat();
        if (m_downmix)
            format.setChannelCount(1);
        audioDecoder->setAudioFormat(format);
        m_changingDevice = true;
        this->deviceIndex = deviceIndex;
        int curVolume = audioOutput->volume();
        int curPosition = position();
        QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
        qCritical() << "User selected device: " << devices.at(deviceIndex).deviceName();
        QAudio::State currentState = audioOutput->state();
        stop(true);
        delete(audioOutput);
        audioOutput = new QAudioOutput(audioDecoder->audioFormat(),this);
        audioOutput->setVolume(curVolume);
        if (currentState == QAudio::ActiveState || currentState == QAudio::SuspendedState)
        {
            play();
            setPosition(curPosition);
        }
        if (currentState == QAudio::SuspendedState)
            pause();
        connect(audioOutput, SIGNAL(notify()), this, SLOT(audioOutputNotify()));
        connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioOutputStateChanged(QAudio::State)));
        audioOutput->setNotifyInterval(20);
        m_changingDevice = false;
    }
}

void KhAudioBackendQtMultimedia::processAudio()
{
    static bool processing = false;
    while (processing)
        QApplication::processEvents();
    if (audioDecoder->bufferAvailable())
    {
        QAudioBuffer newBuffer = audioDecoder->read();
        processing = true;
        QByteArray data = QByteArray::fromRawData((char*)newBuffer.data(), newBuffer.byteCount());
        m_array->append(data);
        processing = false;
        if (!m_changingDevice)
        {
            if (audioOutput->state() != QAudio::ActiveState)
            {
                audioOutput->start(stProxy);
            }
        }
    }
}

void KhAudioBackendQtMultimedia::durationReceived(qint64 duration)
{
    emit durationChanged(duration);
}

void KhAudioBackendQtMultimedia::audioOutputNotify()
{
    emit positionChanged(position());
}

void KhAudioBackendQtMultimedia::audioOutputStateChanged(QAudio::State state)
{
    if (!m_changingDevice)
    {
        switch (state) {
        case QAudio::IdleState:
            // Finished playing (no more data)
            stop(true);
            break;
        case QAudio::StoppedState:
            // Stopped for other reasons
            emit stateChanged(KhAbstractAudioBackend::StoppedState);
            break;
        case QAudio::ActiveState:
            emit stateChanged(KhAbstractAudioBackend::PlayingState);
            break;
        case QAudio::SuspendedState:
            emit stateChanged(KhAbstractAudioBackend::PausedState);
            break;
        default:
            break;
        }
    }
}

void KhAudioBackendQtMultimedia::silenceDetectTimerTimeout()
{
    if (m_detectSilence && !m_changingDevice)
    {
        static int seconds = 0;
        int bytesPerSecond = audioOutput->format().bytesForDuration(1000000);
        if (buffer->size() > buffer->pos() + bytesPerSecond)
        {
            QByteArray data = buffer->peek(bytesPerSecond);
            int sum = std::accumulate(data.begin(),data.end(),0);
            if (sum > -12000)
            {
                m_silent = true;
                qCritical() << "Detected silence";
                seconds++;
                if (seconds >= 2)
                    emit silenceDetected();
            }
            else
            {
                m_silent = false;
                seconds = 0;
            }
        }
    }
}

FaderQtMultimedia::FaderQtMultimedia(QAudioOutput *audioOutput, QObject *parent) : QThread(parent)
{
    this->audioOutput = audioOutput;
    m_targetVolume = 0;
    m_preOutVolume = audioOutput->volume();
    fading = false;
}

void FaderQtMultimedia::run()
{
    fading = true;
    qDebug() << "Fading - Current Volume: " << audioOutput->volume() << " Target Volume: " << m_targetVolume;
    while (audioOutput->volume() != m_targetVolume)
    {
        if (audioOutput->volume() > m_targetVolume)
        {
            audioOutput->setVolume(audioOutput->volume() - 0.01);
            if (audioOutput->volume() < .03)
                audioOutput->setVolume(0.0);
        }
        else if (audioOutput->volume() < m_targetVolume)
            audioOutput->setVolume(audioOutput->volume() + 0.01);
        QThread::msleep(30);
        emit volumeChanged(audioOutput->volume() * 100);
    }
    fading = false;
}

void FaderQtMultimedia::fadeIn()
{
    m_targetVolume = m_preOutVolume;
    if (!fading)
    {
        start();
    }
    while(fading)
        QApplication::processEvents();
}


void FaderQtMultimedia::fadeOut()
{
    if (audioOutput->state() == QAudio::ActiveState)
    {
    m_targetVolume = 0.0;
    if (!fading)
    {
        fading = true;
        m_preOutVolume = audioOutput->volume();
        qCritical() << "m_preOutVolume set to " << m_preOutVolume;
        start();
    }
    while(fading)
        QApplication::processEvents();
    }
}

bool FaderQtMultimedia::isFading()
{
    return fading;
}

void FaderQtMultimedia::restoreVolume()
{
    qCritical() << "Setting volume back to " << m_preOutVolume;
    audioOutput->setVolume(m_preOutVolume);
    emit volumeChanged(audioOutput->volume() * 100);
}

void FaderQtMultimedia::setBaseVolume(int volume)
{
    m_preOutVolume = (float)volume / 100.0;
}


QStringList KhAudioBackendQtMultimedia::getOutputDevices()
{
    QStringList devNames;
    QAudioDeviceInfo adInfo;
    QList<QAudioDeviceInfo> devices = adInfo.availableDevices(QAudio::AudioOutput);
    for (int i=0; i < devices.size(); i++)
    {
        devNames << devices.at(i).deviceName();
    }
    return devNames;
}

#include "audioprocproxyiodevice.h"
#include "smbpitchshift.cpp"
#include <QtDebug>
#include <QApplication>
#include <QMutexLocker>


AudioProcProxyIODevice::AudioProcProxyIODevice(QBuffer *audioBuffer, QObject *parent) : QIODevice(parent)
{
    this->audioBuffer = audioBuffer;
    buffering = false;
}

void AudioProcProxyIODevice::setFormat(QAudioFormat format)
{
    m_format = format;
}


bool AudioProcProxyIODevice::isSequential() const
{
    return audioBuffer->isSequential();
}

qint64 AudioProcProxyIODevice::pos() const
{
    return audioBuffer->pos();
}

qint64 AudioProcProxyIODevice::size() const
{
    return audioBuffer->size();
}

bool AudioProcProxyIODevice::seek(qint64 pos)
{
    return audioBuffer->seek(pos);
}

bool AudioProcProxyIODevice::atEnd() const
{
    return audioBuffer->atEnd();
}

bool AudioProcProxyIODevice::reset()
{
    return audioBuffer->reset();
}

qint64 AudioProcProxyIODevice::bytesAvailable() const
{
    return audioBuffer->bytesAvailable();
}

qint64 AudioProcProxyIODevice::bytesToWrite() const
{
    return audioBuffer->bytesToWrite();
}

bool AudioProcProxyIODevice::canReadLine() const
{
    return audioBuffer->canReadLine();
}

bool AudioProcProxyIODevice::waitForReadyRead(int msecs)
{
    return audioBuffer->waitForBytesWritten(msecs);
//    qCritical() << "waitForReadyRead() called";
//    while (buffering)
//        QApplication::processEvents();
//    return true;
}

bool AudioProcProxyIODevice::waitForBytesWritten(int msecs)
{
    return audioBuffer->waitForBytesWritten(msecs);
}

qint64 AudioProcProxyIODevice::readData(char *data, qint64 maxlen)
{
    int requestSize = maxlen;
    if (maxlen > audioBuffer->bytesAvailable())
        requestSize = audioBuffer->bytesAvailable();
    qint64 returnSize = audioBuffer->read(data, requestSize);
    if (m_keyChange != 1.0)
    {
//        pitchShift thread(m_keyChange, m_format.framesForBytes(returnSize) * m_format.channelCount(),m_format.sampleRate(),(float*)data, (float*)data);
//        thread.start(QThread::HighPriority);
//        while (thread.getRunning())
//            QApplication::processEvents();
        smbPitchShift(m_keyChange, m_format.framesForBytes(returnSize) * m_format.channelCount(), 2048, 4, m_format.sampleRate(), (float*)data, (float*)data);
    }
    return returnSize;
}

qint64 AudioProcProxyIODevice::readLineData(char *data, qint64 maxlen)
{
    return audioBuffer->readLine(data, maxlen);
}


float AudioProcProxyIODevice::keyChange() const
{
    return m_keyChange;
}

void AudioProcProxyIODevice::setKeyChange(float keyChange)
{
    m_keyChange = keyChange;
}


qint64 AudioProcProxyIODevice::writeData(const char *data, qint64 len)
{
    // We don't write through this proxy, only read
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
}

bool pitchShift::getRunning() const
{
    return running;
}

pitchShift::pitchShift(float keyChange, int samples, int sampleRate, float *dataIn, float *dataOut, QObject *parent)
{
    Q_UNUSED(parent)
    m_keyChange = keyChange;
    m_samples = samples;
    m_sampleRate = sampleRate;
    m_dataIn = dataIn;
    m_dataOut = dataOut;
    mutex = new QMutex();
}

void pitchShift::run()
{
    running = true;
    QMutexLocker locker(mutex);
    smbPitchShift(m_keyChange, m_samples, 2048, 4, m_sampleRate, m_dataIn, m_dataOut);
    running = false;
    locker.unlock();
}

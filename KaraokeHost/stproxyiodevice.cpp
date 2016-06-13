#include "stproxyiodevice.h"

StProxyIODevice::StProxyIODevice(QBuffer *audioBuffer, QObject *parent) : QIODevice(parent)
{
    this->audioBuffer = audioBuffer;
    st = new SoundTouch();
    st->setChannels(2);
    st->setSampleRate(44100);
    st->setTempoChange(0.0f);
    st->setPitchSemiTones(-8);
}

void StProxyIODevice::setFormat(QAudioFormat format)
{
    m_format = format;
}


bool StProxyIODevice::isSequential() const
{
    return audioBuffer->isSequential();
}

qint64 StProxyIODevice::pos() const
{
    return audioBuffer->pos();
}

qint64 StProxyIODevice::size() const
{
    return audioBuffer->size();
}

bool StProxyIODevice::seek(qint64 pos)
{
    return audioBuffer->seek(pos);
}

bool StProxyIODevice::atEnd() const
{
    return audioBuffer->atEnd();
}

bool StProxyIODevice::reset()
{
    return audioBuffer->reset();
}

qint64 StProxyIODevice::bytesAvailable() const
{
    return audioBuffer->bytesAvailable();
}

qint64 StProxyIODevice::bytesToWrite() const
{
    return audioBuffer->bytesToWrite();
}

bool StProxyIODevice::canReadLine() const
{
    return audioBuffer->canReadLine();
}

bool StProxyIODevice::waitForReadyRead(int msecs)
{
    return audioBuffer->waitForReadyRead(msecs);
}

bool StProxyIODevice::waitForBytesWritten(int msecs)
{
    return audioBuffer->waitForBytesWritten(msecs);
}

qint64 StProxyIODevice::readData(char *data, qint64 maxlen)
{
    return audioBuffer->read(data, maxlen);
//    SAMPLETYPE *datain = new SAMPLETYPE[maxlen];
//    int len = audioBuffer->read((char*)datain, maxlen);
//    int frames = m_format.framesForBytes(len);
//    st->putSamples(datain, frames);
//    st->flush();
//    QByteArray buffer;
//    while (!st->isEmpty())
//    {
//        SAMPLETYPE *dataout = new SAMPLETYPE[maxlen];
//        int availSamples = st->numSamples();
//        st->receiveSamples((SAMPLETYPE*)dataout, availSamples);
//        buffer.append(dataout);
//    }
//    delete [] data;
//    data = new char[sizeof(buffer.size())];
//    strcpy(data, buffer.data());
//    return sizeof(data);
}

qint64 StProxyIODevice::readLineData(char *data, qint64 maxlen)
{
    return audioBuffer->readLine(data, maxlen);
}

qint64 StProxyIODevice::writeData(const char *data, qint64 len)
{
    return 0;
}

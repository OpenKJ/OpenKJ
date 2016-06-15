#ifndef AUDIOPROCPROXYIODEVICE_H
#define AUDIOPROCPROXYIODEVICE_H

#include <QIODevice>
#include <QBuffer>
#include <QAudioFormat>
#include <QThread>
#include <QMutex>
#include "smbPitchShift/smbPitchShift.fftw3.h"

//using namespace soundtouch;

class AudioProcProxyIODevice : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioProcProxyIODevice(QBuffer *audioBuffer, QAudioFormat format, QObject *parent);

    // QIODevice interface
public:
    void setFormat(QAudioFormat format);
    bool isSequential() const;
    qint64 pos() const;
    qint64 size() const;
    bool seek(qint64 pos);
    bool atEnd() const;
    bool reset();
    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;
    bool canReadLine() const;
    bool waitForReadyRead(int msecs);
    bool waitForBytesWritten(int msecs);
    float keyChange() const;
    void setKeyChange(float keyChange);

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 readLineData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    QBuffer *audioBuffer;
//    SoundTouch *st;
    QAudioFormat m_format;
    bool buffering;
    float m_keyChange;
    PitchShifter *pitchShifter;
};

#endif // AUDIOPROCPROXYIODEVICE_H

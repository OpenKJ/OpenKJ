#ifndef STPROXYIODEVICE_H
#define STPROXYIODEVICE_H

#include <QIODevice>
#include <QBuffer>
#include <SoundTouch.h>
#include <QAudioFormat>

using namespace soundtouch;

class StProxyIODevice : public QIODevice
{
    Q_OBJECT
public:
    explicit StProxyIODevice(QBuffer *audioBuffer, QObject *parent);

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

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 readLineData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    QBuffer *audioBuffer;
    SoundTouch *st;
    QAudioFormat m_format;
};

#endif // STPROXYIODEVICE_H

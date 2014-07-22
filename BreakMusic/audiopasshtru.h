#ifndef AUDIOPASSHTRU_H
#define AUDIOPASSHTRU_H

#include <QObject>
#include <QStringList>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

class audioPasshtru : public QObject
{
    Q_OBJECT
private:
    QAudioDeviceInfo m_inputDevice;
    QAudioFormat m_audioFormat;
    QAudioInput *m_audioInput;
    QAudioOutput *m_audioOutput;

public:
    explicit audioPasshtru(QObject *parent = 0);
    QStringList inputDevices();
    QAudioDeviceInfo getDeviceInfoByName(QString name);
    void setInputDevice(QString name);
    void enable();
    void disable();

signals:
    void volumeChanged(int volume);

public slots:
    void setVolume(int volume);

};

#endif // AUDIOPASSHTRU_H

#include "audiopasshtru.h"

audioPasshtru::audioPasshtru(QObject *parent) :
    QObject(parent)
{
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setSampleRate(44100);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);

    m_audioOutput = new QAudioOutput(m_audioFormat,this);
    m_audioInput = new QAudioInput(m_audioFormat,this);
    m_audioInput->setVolume(0.5);

}

QStringList audioPasshtru::inputDevices()
{
    QStringList names;
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (int i=0; i < devices.size(); i++)
        names << devices.at(i).deviceName();
    return names;
}

QAudioDeviceInfo audioPasshtru::getDeviceInfoByName(QString name)
{
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (int i=0; i < devices.size(); i++)
    {
        if (devices.at(i).deviceName() == name)
            return devices.at(i);
    }
    return QAudioDeviceInfo::defaultInputDevice();
}

void audioPasshtru::setInputDevice(QString name)
{
    delete m_audioInput;
    m_audioInput = new QAudioInput(getDeviceInfoByName(name),m_audioFormat,this);
    m_audioInput->setVolume(0.05);
}

void audioPasshtru::enable()
{
    m_audioOutput->start(m_audioInput->start());
}

void audioPasshtru::disable()
{
    m_audioOutput->stop();
    m_audioInput->stop();
}

void audioPasshtru::setVolume(int volume)
{

}

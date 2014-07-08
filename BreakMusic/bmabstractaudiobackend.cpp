#include "bmabstractaudiobackend.h"
#include <QStringList>
BmAbstractAudioBackend::BmAbstractAudioBackend(QObject *parent) :
    QObject(parent)
{
}

QString BmAbstractAudioBackend::msToMMSS(qint64 msec)
{
    QString sec;
    QString min;
    int seconds = (int) (msec / 1000) % 60 ;
    int minutes = (int) ((msec / (1000*60)) % 60);
    if (seconds < 10)
        sec = "0" + QString::number(seconds);
    else
        sec = QString::number(seconds);   
    min = QString::number(minutes);
    return QString(min + ":" + sec);
}

QStringList BmAbstractAudioBackend::getOutputDevices()
{
    QStringList devices;
    devices << "System Default Output";
    return devices;
}

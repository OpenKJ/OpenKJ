#ifndef AUDIOFADER_H
#define AUDIOFADER_H

#include <QObject>
#include <QTimer>
#include <gst/gst.h>

class AudioFader : public QObject
{
    Q_OBJECT
private:
    GstElement *volumeElement;
    QTimer *timer;
    bool fading;
    double preFadeVol;
    double targetVol;
    double volume();
    bool paused;
    QString objName;

public:
    explicit AudioFader(QObject *parent = 0);
    void setVolumeElement(GstElement *volumeElement);
    void setPaused(bool paused);
    void setObjName(QString name);
    bool isFading() { return fading; }
    void setVolume(double volume);


signals:
    void volumeChanged(double volume);
    void volumeChanged(int volume);
    void fadeStarted();
    void fadeComplete();

public slots:
    void fadeOut(bool block = false);
    void fadeIn(bool block = false);


private slots:
    void timerTimeout();
};


#endif // AUDIOFADER_H

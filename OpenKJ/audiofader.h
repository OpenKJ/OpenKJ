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
    bool fadingIn;
    bool fadingOut;
    double preFadeVol;
    double targetVol;
    void setVolume(double volume);
    double volume();
    bool stoppingEvents;

public:
    explicit AudioFader(GstElement *volElement, QObject *parent = 0);
    void setPreFadeVol(double preFadeVol);
    double getPreFadeVol();
    void stopEvents();
    bool isFadingIn() { return fadingIn; }
    bool isFadingOut() { return fadingOut; }
    bool isFading() { return fading; }


signals:
    void volumeChanged(double volume);
    void volumeChanged(int volume);
    void fadeComplete();

public slots:
    void fadeOut(bool block = false);
    void fadeIn(bool block = false);
    void fadeInToTargetVolume(double volume, bool block = false);


private slots:
    void timerTimeout();
};

#endif // AUDIOFADER_H

#ifndef AUDIOFADER_H
#define AUDIOFADER_H

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <gst/gst.h>

class AudioFader : public QObject
{
    Q_OBJECT
private:
    GstElement *volumeElement;
    QTimer *timer;
    double targetVol;
    double volume();
    QString objName;

public:
    explicit AudioFader(QObject *parent = 0);
    enum FaderState{FadedIn=0,FadingIn,FadedOut,FadingOut};
    QString stateToStr(FaderState state);
    void setVolumeElement(GstElement *volumeElement);
    void setObjName(QString name);
    bool isFading();
    void setVolume(double volume);
    void immediateIn();
    void immediateOut();
    FaderState state();

private:
    FaderState curState;

signals:
    void volumeChanged(double volume);
    void volumeChanged(int volume);
    void fadeStarted();
    void fadeComplete();
    void faderStateChanged(AudioFader::FaderState);

public slots:
    void fadeOut(bool block = false);
    void fadeIn(bool block = false);


private slots:
    void timerTimeout();
};


#endif // AUDIOFADER_H

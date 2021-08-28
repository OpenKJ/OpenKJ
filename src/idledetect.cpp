#include "idledetect.h"

IdleDetect::IdleDetect(QObject *parent) : QObject(parent)
{
    idleMins = 0;
    idle = false;
    idleIncrement = new QTimer(this);
    int timerGranularity = 60000;
    idleIncrement->start(timerGranularity);
    connect(idleIncrement, &QTimer::timeout, this, &IdleDetect::idleIncrementTimeout);
}

bool IdleDetect::eventFilter(QObject *obj, QEvent *ev)
{
    if(ev->type() == QEvent::KeyPress || ev->type() == QEvent::MouseMove)
    {
        idleMins = 0;
        if (idle)
        {
            idle = false;
            emit idleStateChanged(false);
        }
    }
    return QObject::eventFilter(obj, ev);
}

void IdleDetect::idleIncrementTimeout()
{
    idleMins++;
    if (idleMins > 60 && !idle)
    {
        idle = true;
        emit idleStateChanged(true);
    }
}

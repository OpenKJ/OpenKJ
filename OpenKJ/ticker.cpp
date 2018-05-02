#include <QtGui>

#include "ticker.h"

Ticker::Ticker(QWidget *parent)
    : QWidget(parent)
{
    offset = 0;
    myTimerId = 0;
    msec = 20;
}

void Ticker::setText(const QString &newText)
{
    myText = newText;
    update();
    updateGeometry();
}

QSize Ticker::sizeHint() const
{
    return fontMetrics().size(0, text());
}

void Ticker::setSpeed(int speed)
{
    if (speed > 50)
        speed = 50;
    changeTimerInterval(51 - speed);
}

void Ticker::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    int textWidth = fontMetrics().width(text());
    if (textWidth < 1)
        return;
    int x = -offset;
    while (x < width()) {
        painter.drawText(x, 0, textWidth, height(),
                         Qt::AlignLeft | Qt::AlignVCenter, text());
        x += textWidth;
    }
}

void Ticker::showEvent(QShowEvent * /* event */)
{
    myTimerId = startTimer(msec, Qt::PreciseTimer);
}

void Ticker::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == myTimerId) {
        ++offset;
        ++offset;
        ++offset;
        if (offset >= fontMetrics().width(text()))
            offset = 0;
        scroll(-1, 0);
    } else {
        QWidget::timerEvent(event);
    }
}

void Ticker::hideEvent(QHideEvent * /* event */)
{
    killTimer(myTimerId);
    myTimerId = 0;
}

void Ticker::changeTimerInterval(int msec)
{
    killTimer(myTimerId);
    myTimerId = startTimer(msec, Qt::PreciseTimer);
    this->msec = msec;
}

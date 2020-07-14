#include "videodisplay.h"
#include <QPainter>
#include <QPaintEvent>

bool VideoDisplay::videoIsPlaying()
{
    if (kmb->state() != MediaBackend::PlayingState && kmb->state() != MediaBackend::PausedState &&
            (!bmb->hasVideo() || (bmb->state() != MediaBackend::PlayingState && bmb->state() != MediaBackend::PausedState)))
        return false;
    return true;
}

VideoDisplay::VideoDisplay(QWidget *parent) : QWidget(parent)
{
    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);
    setMouseTracking(true);
}

void VideoDisplay::setBackground(const QPixmap &pixmap)
{
    m_currentBg = pixmap;
    if (!videoIsPlaying())
        update();
}

void VideoDisplay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if(videoIsPlaying())
    {
        painter.fillRect(event->rect(), Qt::black);
        return;
    }
    painter.drawPixmap(rect(), m_currentBg, m_currentBg.rect());
}


VideoDisplayAR::VideoDisplayAR(QWidget *parent) :
    QWidget(parent)
{
    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    this->setPalette(palette);
    m_videoDisplay = new VideoDisplay(this);
    layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    layout->addItem(new QSpacerItem(0, 0));
    layout->addWidget(m_videoDisplay);
    layout->addItem(new QSpacerItem(0, 0));
}

void VideoDisplayAR::resizeEvent(QResizeEvent *event)
{
    qInfo() << "resize event fired";
    float thisAspectRatio = (float)event->size().width() / event->size().height();
    int widgetStretch, outerStretch;

    if (thisAspectRatio > (arWidth/arHeight)) // too wide
    {
        layout->setDirection(QBoxLayout::LeftToRight);
        widgetStretch = height() * (arWidth/arHeight); // i.e., my width
        outerStretch = (width() - widgetStretch) / 2 + 0.5;
    }
    else // too tall
    {
        layout->setDirection(QBoxLayout::TopToBottom);
        widgetStretch = width() * (arHeight/arWidth); // i.e., my height
        outerStretch = (height() - widgetStretch) / 2 + 0.5;
    }

    layout->setStretch(0, outerStretch);
    layout->setStretch(1, widgetStretch);
    layout->setStretch(2, outerStretch);
}

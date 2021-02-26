#include "videodisplay.h"
#include <QPainter>
#include <QPaintEvent>
#include <QSvgRenderer>

VideoDisplay::VideoDisplay(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);
    setMouseTracking(true);
}


void VideoDisplay::setBackground(const QPixmap &pixmap)
{
    m_useDefaultBg = false;
    m_currentBg = pixmap;
    update();
}

void VideoDisplay::useDefaultBackground()
{
    m_useDefaultBg = true;
    update();
}

void VideoDisplay::setHasActiveVideo(const bool &value)
{
    if (m_hasActiveVideo != value)
    {
        if (value)
        {
            // Switching to having video.
            // Make sure the background is painted black initially to prevent
            // any old buffer from "bleeding though" when in HW mode.
            m_repaintBackgroundOnce = true;
        }
        m_hasActiveVideo = value;
        update();
    }
}

void VideoDisplay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (m_hasActiveVideo)
    {
        // playing - fix black bars in hw mode (event is not called in sw mode when playing).
        // (perhaps simplify all this by always using fillOnPaint. Performance should be measured though.)
        if (m_fillOnPaint || m_repaintBackgroundOnce)
        {
            painter.fillRect(event->rect(), Qt::black);
            m_repaintBackgroundOnce = false;
        }
    }
    else
    {
        // stopped - draw background image
        painter.fillRect(event->rect(), Qt::black);
        if (m_useDefaultBg)
        {
            QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
#if (QT_VERSION >= QT_VERSION_CHECK(5,15,0))
            renderer.setAspectRatioMode(Qt::KeepAspectRatio);
#endif
            renderer.render(&painter);
        }
        else
        {
            painter.drawPixmap(rect(), m_currentBg, m_currentBg.rect());
        }
    }
}

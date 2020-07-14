#include "cdgdisplay.h"
#include <QPainter>
#include <QPaintEvent>

bool CdgDisplay::videoIsPlaying()
{
    if (kmb->state() != MediaBackend::PlayingState && kmb->state() != MediaBackend::PausedState &&
            (!bmb->hasVideo() || (bmb->state() != MediaBackend::PlayingState && bmb->state() != MediaBackend::PausedState)))
        return false;
    return true;
}

CdgDisplay::CdgDisplay(QWidget *parent) : QWidget(parent)
{
    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);
    setMouseTracking(true);
}

void CdgDisplay::setBackground(const QPixmap &pixmap)
{
    m_currentBg = pixmap;
    if (!videoIsPlaying())
        update();
}

void CdgDisplay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if(videoIsPlaying())
    {
        painter.fillRect(event->rect(), Qt::black);
        return;
    }
    painter.drawPixmap(rect(), m_currentBg, m_currentBg.rect());
}

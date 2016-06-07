#include "cdgvideowidget.h"
#include <QVideoSurfaceFormat>
#include <QPainter>
#include <QPaintEvent>
#include <QRegion>
#include <QDebug>

/*
 * This was a test to see if QAbstractVideoSurface use would be faster/more efficient than drawing to a glcanvas
 * directly.  Turns out this isn't the case at all.  This file is no longer in use and is only here for possible
 * later use.
 *
*/

CdgVideoWidget::CdgVideoWidget(QWidget *parent) : QWidget(parent) , surface(0)
{
    setUpdatesEnabled(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
//   setAttribute(Qt::WA_PaintOnScreen, true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    surface = new CdgVideoSurface(this);
    surface->start();
}

CdgVideoWidget::~CdgVideoWidget()
{
         delete surface;
}

QSize CdgVideoWidget::sizeHint() const
{
         return surface->surfaceFormat().sizeHint();
}


void CdgVideoWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    surface->updateVideoRect();
}

void CdgVideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (surface->isActive()) {
        const QRect videoRect = surface->videoRect();
        if (!videoRect.contains(event->rect())) {
            QRegion region = event->region();
            region.subtracted(videoRect);
            QBrush brush = palette().background();
            foreach (const QRect &rect, region.rects())
                painter.fillRect(rect, brush);
        }

        surface->paint(&painter);
    } else {
        qCritical() << "CdgVideoWidget::paintEvent fired - surface not active";
        painter.fillRect(event->rect(), palette().background());
    }
}

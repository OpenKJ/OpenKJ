#include "cdgvideowidget.h"
#include <QVideoSurfaceFormat>
#include <QPainter>
#include <QPaintEvent>
#include <QRegion>
#include <QDebug>

extern KhSettings *settings;

#ifdef USE_GL
CdgVideoWidget::CdgVideoWidget(QWidget *parent) : QGLWidget(parent) , surface(0)
#else
CdgVideoWidget::CdgVideoWidget(QWidget *parent) : QWidget(parent) , surface(0)
#endif
{
    setUpdatesEnabled(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
//   setAttribute(Qt::WA_PaintOnScreen, true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    useBgImage = false;
    surface = new CdgVideoSurface(this);
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

bool CdgVideoWidget::getUseBgImage() const
{
    return useBgImage;
}

void CdgVideoWidget::setUseBgImage(bool value)
{
    useBgImage = value;
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
        painter.fillRect(event->rect(), palette().background());
    }
}

void CdgVideoWidget::presentBgImage()
{
    QImage cdgBg;
    if ((settings->cdgDisplayBackgroundImage() != "") && (useBgImage))
    {
        if (!cdgBg.load(settings->cdgDisplayBackgroundImage()))
        {
            surface->present(QImage(":/icons/Icons/openkjlogo1.png").scaled(size(), Qt::KeepAspectRatio));
        }
        else
            surface->present(cdgBg);
    }
    else
    {
        surface->present(QImage(":/icons/Icons/openkjlogo1.png"));
    }
}

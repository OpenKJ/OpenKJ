#include "cdgvideowidget.h"
#include <QVideoSurfaceFormat>
#include <QPainter>
#include <QPaintEvent>
#include <QRegion>
#include <QDebug>

extern Settings *settings;

#ifdef USE_GL
CdgVideoWidget::CdgVideoWidget(QWidget *parent) : QGLWidget(parent) , surface(0)
#else
CdgVideoWidget::CdgVideoWidget(QWidget *parent) : QWidget(parent) , surface(0)
#endif
{
    keepAspect = false;
    setUpdatesEnabled(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);
   // setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    surface = new CdgVideoSurface(this);
}

CdgVideoWidget::~CdgVideoWidget()
{
         delete surface;
}

QSize CdgVideoWidget::sizeHint() const
{
    return QSize(300,216);
}

void CdgVideoWidget::clear()
{
    surface->blankImage();
}

void CdgVideoWidget::setKeepAspect(bool keep)
{
    keepAspect = keep;
}

void CdgVideoWidget::arResize(int w)
{
    int h = w * .5625;
    this->setMinimumSize(QSize(w, h));
    this->adjustSize();
}


void CdgVideoWidget::resizeEvent(QResizeEvent *event)
{

//    if (keepAspect)
//    {
//        event->accept();
//        int width = event->size().width();
//        int newHeight = width * 0.5625;
//        if (event->size() == QSize(width, newHeight))
//            return;
//        QWidget::resize(width, newHeight);
//        //    if(event->size().width() > event->size().height()){
//        //        QWidget::resize(event->size().height(),event->size().height());
//        //    }else{
//        //        QWidget::resize(event->size().width(),event->size().width());
//        //    }
//        //    QWidget::resizeEvent(event);
//        qInfo() << "Width: " << width << " target height: " << newHeight;
//        emit resized(QSize(event->size().width(), event->size().height() * 0.5625));
//        surface->updateVideoRect();
//        emit resizeEvent(new QResizeEvent(QSize(width, newHeight), event->size()));
//    }
//    else
//    {
        QWidget::resizeEvent(event);
        surface->updateVideoRect();
        emit resized(event->size());
//    }
//    //    if (keepAspect)
// //        resize(width, newHeight);

}

void CdgVideoWidget::paintEvent(QPaintEvent *event)
{
    return;
    QPainter painter(this);
    if (surface->isActive()) {
        const QRect videoRect = surface->videoRect();
        if (!videoRect.contains(event->rect())) {
            QRegion region = event->region();
         //   region.subtracted(videoRect);
            QBrush brush = palette().background();
            foreach (const QRect &rect, region.rects())
                painter.fillRect(rect, brush);
        }

        surface->paint(&painter);
    } else {
        painter.fillRect(event->rect(), palette().background());
    }
}


QSize CdgVideoWidget::minimumSizeHint() const
{
    return QSize(300,216);
}

int CdgVideoWidget::heightForWidth(int width) const
{
    return width * .72;
}

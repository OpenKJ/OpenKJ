#include "videodisplay.h"
#include <QPainter>
#include <QPaintEvent>
#include <QSvgRenderer>

bool VideoDisplay::videoIsPlaying()
{
    if (kmb->state() != MediaBackend::PlayingState && kmb->state() != MediaBackend::PausedState &&
            (!bmb->hasVideo() || (bmb->state() != MediaBackend::PlayingState && bmb->state() != MediaBackend::PausedState)))
        return false;
    return true;
}

VideoDisplay::VideoDisplay(QWidget *parent) : QWidget(parent)
{

    setAttribute(Qt::WA_NoSystemBackground, true);
    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);
    setMouseTracking(true);
}

void VideoDisplay::renderFrame(QImage frame)
{
    m_curFrame = QPixmap::fromImage(frame);
    update();
}

void VideoDisplay::setBackground(const QPixmap &pixmap)
{
    m_useDefaultBg = false;
    m_currentBg = pixmap;
    if (!videoIsPlaying())
        update();
}

void VideoDisplay::useDefaultBackground()
{
    m_useDefaultBg = true;
    if (!videoIsPlaying())
        update();
}

void VideoDisplay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (!m_softwareRenderMode)
    {
        if(videoIsPlaying())
        {
            painter.fillRect(event->rect(), Qt::black);
            return;
        }
        if (m_useDefaultBg)
        {
            QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
            renderer.render(&painter);
            return;
        }
        painter.fillRect(event->rect(), Qt::black);
        painter.drawPixmap(rect(), m_currentBg, m_currentBg.rect());
    }
    else
    {
        if (m_keepAspectRatio)
        {
            double widgetWidth = this->width();
            double widgetHeight = this->height();
            QRectF target(0, 0, widgetWidth, widgetHeight);

            QPixmap tempQImage = m_curFrame.scaled(rect().size(), Qt::KeepAspectRatio, Qt::FastTransformation);

            double imageSizeWidth = static_cast<double>(tempQImage.width());
            double imageSizeHeight = static_cast<double>(tempQImage.height());
            QRectF source(0.0, 0.0, imageSizeWidth, imageSizeHeight);

            int deltaX = 0;
            int deltaY = 0;
            if(source.width() < target.width())
                deltaX = target.width() - source.width();
            else
                deltaX = source.width() - target.width();

            if(source.height() < target.height())
                deltaY = target.height() - source.height();
            else
                deltaY = source.height() - target.height();

            QPainter painter(this);
            painter.setBackground(Qt::black);
            painter.fillRect(this->rect(), Qt::black);
            painter.translate(deltaX / 2, deltaY / 2);
            painter.drawPixmap(source, tempQImage, tempQImage.rect());
//            auto scaled = m_curFrame.scaled(width(),height(),Qt::KeepAspectRatio, Qt::SmoothTransformation);
//            QRect rect = QRect((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled.width(), scaled.height());
//            painter.drawPixmap(this->rect(), scaled, rect);
        }
        else
        {
            auto scaled = m_curFrame.scaled(width(),height(),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter.drawPixmap(rect(), scaled, scaled.rect());
        }
    }
}

void VideoDisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
//    QPainter painter(this);
//    if(videoIsPlaying())
//    {
//        painter.fillRect(rect(), Qt::black);
//        return;
//    }
//    if (m_useDefaultBg)
//    {
//        QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
//#if (QT_VERSION >= QT_VERSION_CHECK(5,15,0))
//        renderer.setAspectRatioMode(Qt::KeepAspectRatio);
//#endif
//        renderer.render(&painter);
//        return;
//    }
//    painter.fillRect(rect(), Qt::black);
//    painter.drawPixmap(rect(), m_currentBg, m_currentBg.rect());
}





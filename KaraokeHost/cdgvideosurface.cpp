#include "cdgvideosurface.h"
#include <QVideoFrame>
#include <QVideoSurfaceFormat>
#include <QWidget>
#include <QPainter>
#include <QTransform>
#include <QDebug>

CdgVideoSurface::CdgVideoSurface(QWidget *widget, QObject *parent)
{
    Q_UNUSED(parent);
    CdgVideoSurface::widget = widget;
}

QList<QVideoFrame::PixelFormat> CdgVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle) {
        return QList<QVideoFrame::PixelFormat>()
                << QVideoFrame::Format_RGB32
                << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_ARGB32_Premultiplied
                << QVideoFrame::Format_RGB565
                << QVideoFrame::Format_RGB555;
    } else {
        return QList<QVideoFrame::PixelFormat>();
    }
}

bool CdgVideoSurface::start(const QVideoSurfaceFormat &format)
{
    const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    const QSize size = format.frameSize();
    if (imageFormat != QImage::Format_Invalid && !size.isEmpty()) {
        this->imageFormat = imageFormat;
        imageSize = size;
        sourceRect = format.viewport();
        QAbstractVideoSurface::start(format);
        widget->updateGeometry();
        updateVideoRect();
        return true;
    } else {
        return false;
    }
}

bool CdgVideoSurface::start()
{
    QVideoSurfaceFormat format(QSize(300,216),QVideoFrame::Format_RGB24);
    const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    const QSize size = format.frameSize();
    if (imageFormat != QImage::Format_Invalid && !size.isEmpty()) {
        this->imageFormat = imageFormat;
        imageSize = size;
        sourceRect = format.viewport();
        QAbstractVideoSurface::start(format);
        widget->updateGeometry();
        updateVideoRect();
        return true;
    } else {
        qCritical() << "Error in CdgVideoSurface::start()";
        return false;
    }
}

void CdgVideoSurface::stop()
{
    currentFrame = QVideoFrame();
    targetRect = QRect();
    QAbstractVideoSurface::stop();
    widget->repaint();
}

bool CdgVideoSurface::present(const QVideoFrame &frame)
{
    if (surfaceFormat().pixelFormat() != frame.pixelFormat() || surfaceFormat().frameSize() != frame.size()) {
        stop();
        start(QVideoSurfaceFormat(frame.size(), frame.pixelFormat()));
        currentFrame = frame;
        widget->repaint(targetRect);
        return true;
    } else {
        currentFrame = frame;
        widget->repaint(targetRect);
        return true;
    }
}

void CdgVideoSurface::updateVideoRect()
{
    QSize size = surfaceFormat().sizeHint();
    size.scale(widget->size(), Qt::IgnoreAspectRatio);
    targetRect = QRect(QPoint(0, 0), size);
    targetRect.moveCenter(widget->rect().center());
}

void CdgVideoSurface::paint(QPainter *painter)
{
    if (currentFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        const QTransform oldTransform = painter->transform();
        painter->setRenderHint(QPainter::Antialiasing);
        if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop) {
           painter->scale(1, -1);
           painter->translate(0, -widget->height());
        }
        QImage image(currentFrame.bits(), currentFrame.width(), currentFrame.height(), currentFrame.bytesPerLine(), imageFormat);
        painter->drawImage(targetRect, image, sourceRect);
        painter->setTransform(oldTransform);
        currentFrame.unmap();
    }
}

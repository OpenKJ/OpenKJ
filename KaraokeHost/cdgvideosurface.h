#ifndef CDGVIDEOSURFACE_H
#define CDGVIDEOSURFACE_H
#include <QAbstractVideoSurface>

class CdgVideoSurface : public QAbstractVideoSurface
{
public:
    CdgVideoSurface();

    // QAbstractVideoSurface interface
public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const;
    bool start(const QVideoSurfaceFormat &format);
    void stop();
    bool present(const QVideoFrame &frame);
    void updateVideoRect();

private:
    QWidget *widget;
    QImage::Format imageFormat;
    QRect targetRect;
    QSize imageSize;
    QRect sourceRect;
    QVideoFrame currentFrame;
};

#endif // CDGVIDEOSURFACE_H

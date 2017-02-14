#ifndef CDGVIDEOSURFACE_H
#define CDGVIDEOSURFACE_H
#include <QAbstractVideoSurface>


class CdgVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    CdgVideoSurface(QWidget *widget, QObject *parent = 0);

    // QAbstractVideoSurface interface
public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const;
    bool start(const QVideoSurfaceFormat &format);
    bool start();
    void stop();
    bool present(const QVideoFrame &frame);
    void updateVideoRect();
    QRect videoRect() const { return targetRect; }
    void paint(QPainter *painter);

private:
    QWidget *widget;
    QImage::Format imageFormat;
    QRect targetRect;
    QSize imageSize;
    QRect sourceRect;
    QVideoFrame currentFrame;
};

#endif // CDGVIDEOSURFACE_H

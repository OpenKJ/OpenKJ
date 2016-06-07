#ifndef CDGVIDEOSURFACE_H
#define CDGVIDEOSURFACE_H
#include <QAbstractVideoSurface>

/*
 * This was a test to see if QAbstractVideoSurface use would be faster/more efficient than drawing to a glcanvas
 * directly.  Turns out this isn't the case at all.  This file is no longer in use and is only here for possible
 * later use.
 *
*/

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

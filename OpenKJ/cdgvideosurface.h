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
    void blankImage();
    void setPlaying(const bool &playing) { m_currentlyPlaying = playing; }
    bool getPlaying() { return m_currentlyPlaying; }

private:
    QWidget *widget;
    QImage::Format imageFormat;
    QRect targetRect;
    QSize imageSize;
    QRect sourceRect;
    QVideoFrame currentFrame;
    bool m_currentlyPlaying{false};
};

#endif // CDGVIDEOSURFACE_H

#ifndef CDGDISPLAY_H
#define CDGDISPLAY_H

#include <QWidget>
#include <QBoxLayout>
#include <QResizeEvent>
#include "mediabackend.h"



class VideoDisplay : public QWidget
{
    Q_OBJECT
private:
    QPixmap m_currentBg;
    MediaBackend *kmb;
    MediaBackend *bmb;
    bool videoIsPlaying();

public:
    explicit VideoDisplay(QWidget *parent = nullptr);
    void setMediaBackends(MediaBackend *k, MediaBackend *b) { kmb = k; bmb = b; }

signals:
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    void setBackground(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event) override;
};


class VideoDisplayAR : public QWidget
{
public:
    VideoDisplayAR(QWidget *parent = 0);
    void resizeEvent(QResizeEvent *event);
    VideoDisplay* videoDisplay() { return m_videoDisplay; }
    void setAspectRatio(float w, float h) { arWidth = w; arHeight = h;}
private:
    QBoxLayout *layout;
    float arWidth{16.0f}; // aspect ratio width
    float arHeight{9.0f}; // aspect ratio height
    VideoDisplay *m_videoDisplay;
};

#endif // CDGDISPLAY_H

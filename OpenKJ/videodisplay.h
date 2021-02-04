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
    QPixmap m_curFrame;
    bool m_useDefaultBg{true};
    MediaBackend *kmb{nullptr};
    MediaBackend *bmb{nullptr};
    bool videoIsPlaying();
    bool m_softwareRenderMode{false};
    bool m_keepAspectRatio{true};

public:
    explicit VideoDisplay(QWidget *parent = nullptr);
    void setMediaBackends(MediaBackend *k, MediaBackend *b) { kmb = k; bmb = b; }
    void renderFrame(QImage frame);
    void setSoftwareRenderMode(bool enabled) { m_softwareRenderMode = enabled; if (!enabled) update(); }
    void setKeepAspectRatio(const bool enabled) { m_keepAspectRatio = enabled; }

signals:
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    void setBackground(const QPixmap &pixmap);
    void useDefaultBackground();
protected:
    void paintEvent(QPaintEvent *event) override;

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event) override;
};


#endif // CDGDISPLAY_H

#ifndef CDGDISPLAY_H
#define CDGDISPLAY_H

#include <QWidget>
#include "audiobackendgstreamer.h"

class CdgDisplay : public QWidget
{
    Q_OBJECT
private:
    QPixmap m_currentBg;
    MediaBackend *kmb;
    MediaBackend *bmb;
    bool videoIsPlaying();

public:
    explicit CdgDisplay(QWidget *parent = nullptr);
    void setMediaBackends(MediaBackend *k, MediaBackend *b) { kmb = k; bmb = b; }

signals:
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    void setBackground(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // CDGDISPLAY_H

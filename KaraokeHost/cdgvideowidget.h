#ifndef CDGVIDEOWIDGET_H
#define CDGVIDEOWIDGET_H

#include <QWidget>
#include "cdgvideosurface.h"

class CdgVideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CdgVideoWidget(QWidget *parent = 0);
    ~CdgVideoWidget();
    CdgVideoSurface *videoSurface() const { return surface; }
    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    CdgVideoSurface *surface;
};

#endif // CDGVIDEOWIDGET_H

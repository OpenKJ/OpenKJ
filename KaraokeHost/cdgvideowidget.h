#ifndef CDGVIDEOWIDGET_H
#define CDGVIDEOWIDGET_H

#include "cdgvideosurface.h"
#ifdef USE_GL
    #include <QGLWidget>
#else
    #include <QWidget>
#endif

#ifdef USE_GL
class CdgVideoWidget : public QGLWidget
#else
class CdgVideoWidget : public QWidget
#endif
{
    Q_OBJECT
public:
    explicit CdgVideoWidget(QWidget *parent = 0);
    ~CdgVideoWidget();
    CdgVideoSurface *videoSurface() const { return surface; }
    QSize sizeHint() const;

protected:
    void resizeEvent(QResizeEvent *event);

private:
    CdgVideoSurface *surface;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event);
};

#endif // CDGVIDEOWIDGET_H

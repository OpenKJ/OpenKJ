#ifndef CDGVIDEOWIDGET_H
#define CDGVIDEOWIDGET_H

#include "cdgvideosurface.h"
#ifdef USE_GL
    #include <QGLWidget>
#else
    #include <QWidget>
#endif
#include "khsettings.h"


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

    bool getUseBgImage() const;
    void setUseBgImage(bool value);

protected:
    void resizeEvent(QResizeEvent *event);

private:
    CdgVideoSurface *surface;
    bool useBgImage;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event);

public slots:
    void presentBgImage();
};

#endif // CDGVIDEOWIDGET_H

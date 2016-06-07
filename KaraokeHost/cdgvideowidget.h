#ifndef CDGVIDEOWIDGET_H
#define CDGVIDEOWIDGET_H

#include <QWidget>
#include "cdgvideosurface.h"

/*
 * This was a test to see if QAbstractVideoSurface use would be faster/more efficient than drawing to a glcanvas
 * directly.  Turns out this isn't the case at all.  This file is no longer in use and is only here for possible
 * later use.
 *
*/

class CdgVideoWidget : public QWidget
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

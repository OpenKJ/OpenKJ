#ifndef ASPECTRATIOWIDGET_H
#define ASPECTRATIOWIDGET_H

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPaintEvent>
#include "cdgvideowidget.h"

class AspectRatioWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AspectRatioWidget(QWidget *parent = nullptr);
//    AspectRatioWidget(QWidget *widget, float width, float height, QWidget *parent = 0);
    void resizeEvent(QResizeEvent *event);
    CdgVideoWidget* getCdgWidget() {return vidWidget;}

private:
    QBoxLayout *layout;
    float arWidth; // aspect ratio width
    float arHeight; // aspect ratio height
    CdgVideoWidget *vidWidget;

signals:

public slots:
};

#endif // ASPECTRATIOWIDGET_H

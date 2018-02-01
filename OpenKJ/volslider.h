#ifndef VOLSLIDER_H
#define VOLSLIDER_H

#include <QWidget>
#include <QSlider>

class VolSlider : public QSlider
{
    Q_OBJECT
public:
    explicit VolSlider(QWidget *parent = nullptr);

signals:

public slots:

    // QWidget interface
protected:
    void wheelEvent(QWheelEvent *event);
};

#endif // VOLSLIDER_H

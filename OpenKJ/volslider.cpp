#include "volslider.h"

VolSlider::VolSlider(QWidget *parent) : QSlider(parent)
{

}


void VolSlider::wheelEvent(QWheelEvent *event)
{
    QAbstractSlider::wheelEvent(event);
    emit sliderMoved(this->value());
}

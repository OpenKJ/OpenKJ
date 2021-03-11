#include "soundfxbutton.h"
#include <QMouseEvent>
#include <QDebug>

SoundFxButton::SoundFxButton()
{

}

void SoundFxButton::setButtonData(QVariant data)
{
    this->data = data;
}



void SoundFxButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        qInfo() << "Right mouse button clicked";
        emit customContextMenuRequested(event->pos());
        return;
    }
    QAbstractButton::mouseReleaseEvent(event);
}

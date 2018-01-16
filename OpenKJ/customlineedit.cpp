#include "customlineedit.h"
#include <QKeyEvent>

CustomLineEdit::CustomLineEdit(QWidget *parent) : QLineEdit(parent)
{
}


void CustomLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit escapePressed();
    }
    else
    {
        QLineEdit::keyPressEvent(event);
    }
}

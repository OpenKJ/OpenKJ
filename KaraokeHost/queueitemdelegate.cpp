#include "queueitemdelegate.h"

QueueItemDelegate::QueueItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}


void QueueItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int topPad = (option.rect.height() - 16) / 2;
    int leftPad = (option.rect.width() - 16) / 2;
    if (index.sibling(index.row(), 8).data().toBool())
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, QColor("gray"));
        else
            painter->fillRect(option.rect, QColor("darkGray"));
    }
    if (index.column() == 7)
    {
        QString displayText = index.data().toString();
        if (index.data().toInt() > 0)
            displayText.prepend("+");
        if (index.data().toInt() == 0)
            displayText = "";
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, " " + displayText);
        return;
    }
    if (index.column() == 8)
    {
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/edit-delete.png"));
        return;
    }
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
}

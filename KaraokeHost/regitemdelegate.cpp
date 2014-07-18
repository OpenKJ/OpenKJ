#include "regitemdelegate.h"

RegItemDelegate::RegItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}


void RegItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int topPad = (option.rect.height() - 16) / 2;
    int leftPad = (option.rect.width() - 16) / 2;
    if (index.column() == 2)
    {
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/list-add-user-small.png"));
        return;
    }
    if (index.column() == 3)
    {
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/edit-delete.png"));
        return;
    }
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
}

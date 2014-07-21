#include "regitemdelegate.h"

#include <QSqlQuery>

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
        QSqlQuery query;
        query.exec("SELECT COUNT(*) FROM regularsongs WHERE regsingerid == " + index.sibling(index.row(), 0).data().toString());
        if (query.first())
            painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, query.value(0).toString());
        return;
    }
    if (index.column() == 3)
    {
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/list-add-user-small.png"));
        return;
    }
    if (index.column() == 4)
    {
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/edit-delete.png"));
        return;
    }
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
}

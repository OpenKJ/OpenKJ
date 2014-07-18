#include "dbitemdelegate.h"
#include <QTime>

DbItemDelegate::DbItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}


void DbItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == 4)
    {
        if (index.data().toInt() <= 0)
            return;
        QString duration = QTime(0,0,0,0).addMSecs(index.data().toInt()).toString("m:ss");
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, duration);
        return;
    }
    QItemDelegate::paint(painter, option, index);
}

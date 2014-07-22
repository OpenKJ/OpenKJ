#include "dbitemdelegate.h"
#include <QTime>

DbItemDelegate::DbItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}


void DbItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    if (index.column() == 4)
    {
        if (index.data().toInt() <= 0)
            return;
        QString duration = QTime(0,0,0,0).addMSecs(index.data().toInt()).toString("m:ss");
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, duration);
        painter->restore();
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}

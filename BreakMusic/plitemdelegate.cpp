#include "plitemdelegate.h"
#include <QPainter>


int PlItemDelegate::currentSong() const
{
    return m_currentSong;
}

void PlItemDelegate::setCurrentSong(int value)
{
    m_currentSong = value;
}
PlItemDelegate::PlItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    m_currentSong = -1;
}


void PlItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.row() == m_currentSong)
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, QColor("green"));
        else
            painter->fillRect(option.rect, QColor("yellow"));
    }
    else
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
    }

    if (index.column() == 2)
    {
        if (index.row() != m_currentSong)
            return;
        painter->drawImage(QRect(option.rect.x(),option.rect.y(), 16, 16), QImage(":/icons/play-small.png"));
        return;
    }
    if (index.column() == 7)
    {
        painter->drawImage(QRect(option.rect.x(), option.rect.y(), 16, 16), QImage(":/icons/edit-delete.png"));
        return;
    }
    if (index.column() == 6)
    {
        painter->drawText(option.rect, Qt::AlignCenter, index.data().toString());
        return;
    }
    painter->drawText(option.rect, index.data().toString());
}


QSize PlItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ((index.column() == 2) || (index.column() == 7))
    {
        return QSize(16,16);
    }
    else return QItemDelegate::sizeHint(option, index);
}

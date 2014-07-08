#include "playlistitemdelegate.h"
#include <QPainter>


int PlaylistItemDelegate::currentSong() const
{
    return m_currentSong;
}

void PlaylistItemDelegate::setCurrentSong(int value)
{
    m_currentSong = value;
}
PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    m_currentSong = -1;
}


void PlaylistItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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
    painter->drawText(option.rect, index.data().toString());
    painter->setRenderHint(QPainter::Antialiasing, true);
}


QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ((index.column() == 2) || (index.column() == 6))
    {
        return QSize(16,16);
    }
    else return QItemDelegate::sizeHint(option, index);
}

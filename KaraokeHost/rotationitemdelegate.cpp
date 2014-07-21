#include "rotationitemdelegate.h"
#include <QDebug>
#include <QSqlQuery>

RotationItemDelegate::RotationItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    m_currentSingerId = -1;
}

int RotationItemDelegate::currentSinger()
{
    return m_currentSingerId;
}

void RotationItemDelegate::setCurrentSinger(int currentSingerId)
{
    m_currentSingerId = currentSingerId;
}


void RotationItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int topPad = (option.rect.height() - 16) / 2;
    int leftPad = (option.rect.width() - 16) / 2;
    if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, QColor("yellow"));
    }
    if (index.column() == 3)
    {
        if (index.data().toBool())
            painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/emblem-favorite-16x16.png"));
        else
            painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/emblem-favorite-disabled-16x16"));
        return;
    }
    if (index.column() == 2)
    {
        QString nextSong;
        QString sql = "select dbsongs.artist,dbsongs.title from dbsongs,queuesongs WHERE queuesongs.singer = " + index.sibling(index.row(), 0).data().toString() + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1";
        QSqlQuery query(sql);
        if (query.first())
            nextSong = " " + query.value(0).toString() + " - " + query.value(1).toString();
        else
            nextSong = "  -- Empty -- ";
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, nextSong);
        painter->restore();
        return;
    }
    if (index.column() == 0)
    {
        if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
            painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/microphone"));
        return;
    }
    if (index.column() == 4)
    {
        painter->drawImage(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, 16, 16), QImage(":/icons/Icons/edit-delete.png"));
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}

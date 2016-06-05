/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

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

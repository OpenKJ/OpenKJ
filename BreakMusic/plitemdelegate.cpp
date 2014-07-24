/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#include "plitemdelegate.h"
#include <QPainter>
#include <QTime>
#include <QDebug>

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
    int topPad = (option.rect.height() - 16) / 2;
    int leftPad = (option.rect.width() - 16) / 2;

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    if (index.row() == m_currentSong)
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, QColor("yellow"));
    }
    if (index.column() == 2)
    {
        if (index.row() != m_currentSong)
            return;
        painter->drawImage(QRect(option.rect.x(),option.rect.y(), 16, 16), QImage(":/icons/play-small.png"));
        return;
    }
    if (index.column() == 6)
    {
        int sec = index.data().toInt();
        if (sec <= 0)
            return;
        QString duration = QTime(0,0,0,0).addSecs(sec).toString("m:ss");
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, duration);
        painter->restore();
        return;
    }
    if (index.column() == 7)
    {
        painter->drawImage(QRect(option.rect.x() + leftPad, option.rect.y() + topPad, 16, 16), QImage(":/icons/edit-delete.png"));
        return;
    }
    if (index.column() == 6)
    {
        painter->drawText(option.rect, Qt::AlignCenter, index.data().toString());
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}


QSize PlItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ((index.column() == 2) || (index.column() == 7))
    {
        return QSize(16,16);
    }
    else return QItemDelegate::sizeHint(option, index);
}

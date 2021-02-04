/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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

#include "bmplitemdelegate.h"
#include <QPainter>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include "settings.h"

extern Settings settings;

int BmPlItemDelegate::currentSong() const
{
    return m_currentSong;
}

void BmPlItemDelegate::setCurrentSong(int value)
{
    m_currentSong = value;
}
BmPlItemDelegate::BmPlItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    m_currentSong = -1;
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_iconDelete = QIcon(thm + "actions/22/edit-delete.svg");
    m_iconPlaying = QIcon(thm + "actions/22/media-playback-start.svg");
}

void BmPlItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sbSize(QFontMetrics(settings.applicationFont()).height(), QFontMetrics(settings.applicationFont()).height());
    int topPad = (option.rect.height() - sbSize.height()) / 2;
    int leftPad = (option.rect.width() - sbSize.width()) / 2;

    if (option.state & QStyle::State_Selected)
    {
        if (index.column() > 2 && index.column() < 7)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (index.row() % 2) ? option.palette.alternateBase() : option.palette.base());
        //painter->fillRect(option.rect, option.palette.highlight());
    }

    if (index.row() == m_currentSong && index.column() > 2 && index.column() < 7)
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (settings.theme() == 1) ? QColor(180,180,0) : QColor("yellow"));
    }
    if (index.column() == 2)
    {
        if (index.row() == m_currentSong)
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), m_iconPlaying.pixmap(sbSize));
        return;
    }
    if (index.column() == 5)
    {
        QFileInfo fi(index.data().toString());
        QString fn = fi.fileName();
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        else if (index.row() == m_currentSong)
            painter->setPen(QColor("black"));
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + fn);
        painter->restore();
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
        else if (index.row() == m_currentSong)
            painter->setPen(QColor("black"));
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, duration);
        painter->restore();
        return;
    }
    if (index.column() == 7)
    {
        painter->drawPixmap(QRect(option.rect.x() + leftPad, option.rect.y() + topPad, sbSize.width(), sbSize.height()), m_iconDelete.pixmap(sbSize));
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
    if (index.row() == m_currentSong)
        painter->setPen(QColor("black"));
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}


QSize BmPlItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if ((index.column() == 2) || (index.column() == 7))
    {
        return QSize(16,16);
    }
    else return QItemDelegate::sizeHint(option, index);
}

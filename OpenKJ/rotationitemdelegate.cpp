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

#include "rotationitemdelegate.h"
#include <QDebug>
#include <QSqlQuery>
#include "settings.h"

extern Settings settings;

RotationItemDelegate::RotationItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    m_currentSingerId = -1;
    singerCount = 0;
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    favorite16Off = QIcon(thm + "actions/16/im-user.svg");
    favorite22Off = QIcon(thm + "actions/22/im-user.svg");
    favorite16On = QIcon(thm + "actions/16/im-user-online.svg");
    favorite22On = QIcon(thm + "actions/22/im-user-online.svg");
    mic16 = QIcon(thm + "status/16/mic-on");
    mic22 = QIcon(thm + "status/22/mic-on");
    delete16 = QIcon(thm + "actions/16/edit-delete.svg");
    delete22 = QIcon(thm + "actions/22/edit-delete.svg");

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
    QSize sbSize(QFontMetrics(settings.applicationFont()).height(), QFontMetrics(settings.applicationFont()).height());
    int topPad = (option.rect.height() - sbSize.height()) / 2;
    int leftPad = (option.rect.width() - sbSize.width()) / 2;

    QString nextSong;
    bool hasSong{false};
    QString sql = "select dbsongs.artist,dbsongs.title from dbsongs,queuesongs WHERE queuesongs.singer = " + index.sibling(index.row(), 0).data().toString() + " AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1";
    QSqlQuery query(sql);
    if (query.first())
    {
        nextSong = " " + query.value(0).toString() + " - " + query.value(1).toString();
        hasSong = true;
    }
    else
        nextSong = "  -- Empty -- ";

    if (option.state & QStyle::State_Selected)
    {
        if (index.column() == 1)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (index.row() % 2) ? option.palette.alternateBase() : option.palette.base());
    }
    if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId && index.column() < 2 && index.column() > 0)
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (settings.theme() == 1) ? QColor(180,180,0) : QColor("yellow"));
    }
    if (index.column() == 3)
    {
            if (sbSize.height() > 18)
            {
                if (index.data().toBool())
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite22On.pixmap(sbSize));
                else
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite22Off.pixmap(sbSize));
            }
            else
            {
                if (index.data().toBool())
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite16On.pixmap(sbSize));
                else
                    painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), favorite16Off.pixmap(sbSize));
            }
        return;
    }
    if (index.column() == 2)
    {
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        else if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
        {
            painter->setPen(QColor("black"));
        }
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, nextSong);
        painter->restore();
        return;
    }
    if (index.column() == 0)
    {
        if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
        {
            if (sbSize.height() > 18)
                painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), mic22.pixmap(sbSize));
            else
                painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), mic16.pixmap(sbSize));

        }
        else if (settings.rotationDisplayPosition())
        {
            int curSingerPos = 0;
            int drawSingerPos = index.row();
            QSqlQuery query;
            int wait = 0;
            query.exec("SELECT position FROM rotationsingers WHERE singerId == " + QString::number(m_currentSingerId) + " LIMIT 1");
            if (query.first())
            {
                curSingerPos = query.value("position").toInt();
            }
            if (curSingerPos < drawSingerPos)
                wait = drawSingerPos - curSingerPos;
            else if (curSingerPos > drawSingerPos)
                wait = drawSingerPos + (singerCount - curSingerPos);
            if (wait > 0)
            {
                painter->save();
                if (option.state & QStyle::State_Selected)
                    painter->setPen(option.palette.highlightedText().color());
                else if (index.sibling(index.row(), 0).data().toInt() == m_currentSingerId)
                {
                    painter->setPen(QColor("black"));
                }
                painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignHCenter, QString::number(wait));
                painter->restore();
            }
            return;
        }
        return;
    }
    if (index.column() == 4)
    {
        if (sbSize.height() > 18)
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete22.pixmap(sbSize));
        else
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete16.pixmap(sbSize));
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    else if ((index.sibling(index.row(), 0).data().toInt() == m_currentSingerId) && (settings.theme() == 1))
    {
        painter->setPen(QColor("black"));
    }
    if (hasSong && index.column() == 1)
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString() + " ∙");
    else
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}

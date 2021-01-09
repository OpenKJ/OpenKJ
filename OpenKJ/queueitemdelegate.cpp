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

#include "queueitemdelegate.h"
#include "settings.h"

extern Settings settings;

QueueItemDelegate::QueueItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    delete16 = QIcon(thm + "actions/16/edit-delete.svg");
    delete22 = QIcon(thm + "actions/22/edit-delete.svg");
}


void QueueItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sbSize(QFontMetrics(settings.applicationFont()).height(), QFontMetrics(settings.applicationFont()).height());
    int topPad = (option.rect.height() - sbSize.height()) / 2;
    int leftPad = (option.rect.width() - sbSize.width()) / 2;
    if ((index.sibling(index.row(), 8).data().toBool()) && (index.column() != 7) && (index.column() != 8))
    {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        else
        {
            if (settings.theme() != 1)
                painter->fillRect(option.rect, QColor("darkGrey"));
        }
    }
    if (option.state & QStyle::State_Selected)
    {
        if (index.column() < 8)
            painter->fillRect(option.rect, option.palette.highlight());
        else
            painter->fillRect(option.rect, (index.row() % 2) ? option.palette.alternateBase() : option.palette.base());
        //painter->fillRect(option.rect, option.palette.highlight());
    }
    if (index.column() == 7)
    {
        QString displayText = index.data().toString();
        if (index.data().toInt() > 0)
            displayText.prepend("+");
        if (index.data().toInt() == 0)
            displayText = "";
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, displayText);
        painter->restore();
        return;
    }
    if (index.column() == 8)
    {
        if (sbSize.height() > 18)
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete22.pixmap(sbSize));
        else
            painter->drawPixmap(QRect(option.rect.x() + leftPad,option.rect.y() + topPad, sbSize.width(), sbSize.height()), delete16.pixmap(sbSize));
        return;
    }
    if ((index.column() == 5) && (index.data().toString() == "!!DROPPED!!"))
    {
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    if (index.sibling(index.row(), 8).data().toBool() && settings.theme() == 1)
    {
        painter->setPen("darkGrey");
        QFont font = painter->font();
        font.setStrikeOut(true);
        painter->setFont(font);
    }
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}

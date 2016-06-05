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

#ifndef ROTATIONITEMDELEGATE_H
#define ROTATIONITEMDELEGATE_H

#include <QItemDelegate>
#include <QPainter>

class RotationItemDelegate : public QItemDelegate
{
    Q_OBJECT

private:
    int m_currentSingerId;

public:
    explicit RotationItemDelegate(QObject *parent = 0);
    int currentSinger();
    void setCurrentSinger(int currentSingerId);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

#endif // ROTATIONITEMDELEGATE_H

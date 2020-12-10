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

#ifndef REGITEMDELEGATE_H
#define REGITEMDELEGATE_H

#include <QItemDelegate>
#include <QPainter>
#include <QIcon>

class RegItemDelegate : public QItemDelegate
{
    Q_OBJECT
private:
    QIcon m_iconDelete16;
    QIcon m_iconDelete22;
    QIcon m_iconLoadReg16;
    QIcon m_iconLoadReg22;
public:
    explicit RegItemDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

#endif // REGITEMDELEGATE_H

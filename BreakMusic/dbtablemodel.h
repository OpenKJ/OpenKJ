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

#ifndef SONGSTABLEMODEL_H
#define SONGSTABLEMODEL_H

#include <QSqlTableModel>

class DbTableModel : public QSqlTableModel
{
    Q_OBJECT
private:
    int sortColumn;
    QString artistOrder;
    QString titleOrder;
    QString filenameOrder;
public:
    explicit DbTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    void search(QString searchString);
    enum {SORT_ARTIST=1,SORT_TITLE=2,SORT_FILENAME=4,SORT_DURATION=5};

    // QAbstractItemModel interface
public:
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    void sort(int column, Qt::SortOrder order);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // QSqlTableModel interface
protected:
    QString orderByClause() const;

};

#endif // SONGSTABLEMODEL_H

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

#ifndef DBTABLEMODEL_H
#define DBTABLEMODEL_H

#include <QSqlTableModel>
#include "settings.h"

class DbTableModel : public QSqlTableModel
{
    Q_OBJECT

private:
    int sortColumn;
    QString artistOrder;
    QString titleOrder;
    QString songIdOrder;
    QString durationOrder;
    QString lastSearch;
    QSqlDatabase db;
    Settings *settings;
    QIcon m_cdgIcon;
    QIcon m_zipIcon;
    QIcon m_vidIcon;

public:
    explicit DbTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());
    enum {SORT_ARTIST=1,SORT_TITLE=2,SORT_SONGID=3,SORT_DURATION=4};
    enum SearchType{SEARCH_TYPE_ALL=1, SEARCH_TYPE_ARTIST, SEARCH_TYPE_TITLE};
    void search(QString searchString);
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void sort(int column, Qt::SortOrder order);
    void refreshCache();
    void setSearchType(SearchType type);

private:
    SearchType m_searchType{SearchType::SEARCH_TYPE_ALL};

protected:
    QString orderByClause() const;


    // QAbstractItemModel interface
public:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // DBTABLEMODEL_H

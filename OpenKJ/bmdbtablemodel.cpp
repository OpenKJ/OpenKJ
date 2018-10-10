/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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

#include "bmdbtablemodel.h"
#include <QMimeData>
#include <QSqlQuery>
#include <QStringList>
#include <QDebug>
#include <QDataStream>

BmDbTableModel::BmDbTableModel(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
    sortColumn = SORT_ARTIST;
    artistOrder = "ASC";
    titleOrder = "ASC";
    filenameOrder = "ASC";
    this->db = db;
    QSqlQuery query;
    query.exec("ATTACH DATABASE ':memory:' AS mem");
    query.exec("CREATE TABLE mem.bmsongs AS SELECT * FROM main.bmsongs");
    query.exec("CREATE UNIQUE INDEX mem.idx_mem_path ON dbsongs(path)");
    query.exec("CREATE UNIQUE INDEX mem.idx_mem_songid ON dbsongs(songid)");
}

Qt::ItemFlags BmDbTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

void BmDbTableModel::search(QString searchString)
{
    QStringList terms;
    terms = searchString.split(" ",QString::SkipEmptyParts);
    if (terms.size() < 1)
    {
        setFilter("");
        return;
    }
    QString whereClause = "searchstring LIKE \"%" + terms.at(0) + "%\"";
    for (int i=1; i < terms.size(); i++)
    {
        whereClause = whereClause + " AND searchstring LIKE \"%" + terms.at(i) + "%\"";
    }
    setFilter(whereClause);
    lastSearch = searchString;
}

void BmDbTableModel::refreshCache()
{
    qWarning() << "Refreshing bmsongs cache";
    QSqlQuery query(db);
    query.exec("DELETE FROM mem.bmsongs");
    query.exec("VACUUM mem");
    query.exec("INSERT INTO mem.bmsongs SELECT * FROM main.bmsongs");
    setTable("mem.bmsongs");
    select();
    search(lastSearch);
}


QMimeData *BmDbTableModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    QList<int> songids;
    foreach (const QModelIndex &index, indexes) {
        if (index.isValid()) {
            if(index.column() == 5)
                songids.append(index.sibling(index.row(),0).data().toInt());
        }
    }
    stream << songids;
    mimeData->setData("application/vnd.bmsongid.list", encodedData);
    qWarning() << songids;
    return mimeData;
}

QString BmDbTableModel::orderByClause() const
{
    QString sql = " ORDER BY ";
    switch (sortColumn) {
    case SORT_ARTIST:
        sql.append("artist " + artistOrder + ", title " + titleOrder + ", filename " + filenameOrder);
        break;
    case SORT_TITLE:
        sql.append("title " + titleOrder + ", artist " + artistOrder + ", filename " + filenameOrder);
        break;
    case SORT_FILENAME:
        sql.append("filename " + filenameOrder + ", artist " + artistOrder + ", title " + titleOrder);
        break;
    }
    return sql;
}


void BmDbTableModel::sort(int column, Qt::SortOrder order)
{
    QString orderString = "ASC";
    if (order == Qt::DescendingOrder)
        orderString = "DESC";
    sortColumn = column;
    switch (sortColumn) {
    case SORT_ARTIST:
        artistOrder = orderString;
        break;
    case SORT_TITLE:
        titleOrder = orderString;
        break;
    case SORT_FILENAME:
        filenameOrder = orderString;
        break;
    }
    select();
}


QVariant BmDbTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 1 && role == Qt::DisplayRole)
        return tr("Artist");
    if (section == 2 && role == Qt::DisplayRole)
        return tr("Title");
    if (section == 4 && role == Qt::DisplayRole)
        return tr("Filename");
    if (section == 5 && role == Qt::DisplayRole)
        return tr("Duration");
    return QSqlTableModel::headerData(section, orientation, role);
}

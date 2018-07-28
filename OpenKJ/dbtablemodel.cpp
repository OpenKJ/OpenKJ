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

#include "dbtablemodel.h"
#include <QMimeData>
#include <QByteArray>
#include <QStringList>
#include <QSqlQuery>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>



DbTableModel::DbTableModel(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
    settings = new Settings(this);
    this->db = db;
    QSqlQuery query(db);
    query.exec("ATTACH DATABASE ':memory:' AS mem");
    query.exec("CREATE TABLE mem.dbsongs AS SELECT * FROM main.dbsongs");
    query.exec("CREATE UNIQUE INDEX mem.idx_mem_path ON dbsongs(path)");
    query.exec("CREATE UNIQUE INDEX mem.idx_mem_songid ON dbsongs(songid)");
    //refreshCache();
    setTable("mem.dbsongs");
    sortColumn = SORT_ARTIST;
    artistOrder = "ASC";
    titleOrder = "ASC";
    songIdOrder = "ASC";
    durationOrder = "ASC";
    select();
    search("");
}


QMimeData *DbTableModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/songid", data(indexes.at(0).sibling(indexes.at(0).row(), 0)).toByteArray().data());
    return mimeData;
}

Qt::ItemFlags DbTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

void DbTableModel::search(QString searchString)
{
    lastSearch = searchString;
    if (settings->ignoreAposInSearch())
        searchString.remove("'");
    QStringList terms;
    terms = searchString.split(" ",QString::SkipEmptyParts);
    if (terms.size() < 1)
    {
        setFilter("discid != \"!!BAD!!\" AND discid != \"!!DROPPED!!\"");
        return;
    }
    QString whereClause;
    if (settings->ignoreAposInSearch())
        whereClause = "discid != \"!!BAD!!\" AND discid != \"!!DROPPED!!\" AND replace(searchstring, \"'\", \"\") LIKE \"%" + terms.at(0) + "%\"";
    else
        whereClause = "discid != \"!!BAD!!\" AND discid != \"!!DROPPED!!\" AND searchstring LIKE \"%" + terms.at(0) + "%\"";
    for (int i=1; i < terms.size(); i++)
    {
        if (settings->ignoreAposInSearch())
            whereClause = whereClause + " AND replace(searchstring, \"'\", \"\") LIKE \"%" + terms.at(i) + "%\"";
        else
            whereClause = whereClause + " AND searchstring LIKE \"%" + terms.at(i) + "%\"";
    }
    setFilter(whereClause);
}

QString DbTableModel::orderByClause() const
{
    QString sql = " ORDER BY ";
    switch (sortColumn) {
    case SORT_ARTIST:
        sql.append("artist " + artistOrder + ", title " + titleOrder + ", discid " + songIdOrder + ", duration " + durationOrder);
        break;
    case SORT_TITLE:
        sql.append("title " + titleOrder + ", artist " + artistOrder + ", discid " + songIdOrder + ", duration " + durationOrder);
        break;
    case SORT_SONGID:
        sql.append("discid " + songIdOrder + ", artist " + artistOrder + ", title " + titleOrder + ", duration " + durationOrder);
        break;
    case SORT_DURATION:
        sql.append("duration " + durationOrder + ", title " + titleOrder + ", artist " + artistOrder + ", discid " + songIdOrder);
        break;
    default:
        sql.append("artist " + artistOrder + ", title " + titleOrder + ", discid " + songIdOrder + ", duration " + durationOrder);
        break;
    }
    return sql;
}

QVariant DbTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 3 && role == Qt::DisplayRole)
        return "SongID";
    return QSqlTableModel::headerData(section, orientation, role);
}

void DbTableModel::sort(int column, Qt::SortOrder order)
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
    case SORT_SONGID:
        songIdOrder = orderString;
        break;
    case SORT_DURATION:
        durationOrder = orderString;
        break;
    }
    select();
}

void DbTableModel::refreshCache()
{
    qWarning() << "Refreshing dbsongs cache";
    QSqlQuery query(db);
    query.exec("DELETE FROM mem.dbsongs");
    query.exec("VACUUM mem");
    query.exec("INSERT INTO mem.dbsongs SELECT * FROM main.dbsongs");
    setTable("mem.dbsongs");
    select();

}

void DbTableModel::updateSong(int song_id,QString artist, QString title, QString discID)
{
    QSqlQuery query;

    query.prepare("SELECT filename FROM main.dbsongs WHERE songid = :song_id");
    query.bindValue(":song_id", QVariant(song_id));
    query.exec();
    if (query.first())
    {
        static const QString queries[2] = {
            "UPDATE main.dbsongs "
            "SET artist =:artistName, title = :title, searchstring = :searchstring, DiscId = :discId "
            "WHERE songid = :song_id",
            "UPDATE mem.dbsongs "
            "SET artist =:artistName, title = :title, searchstring = :searchstring, DiscId = :discId "
            "WHERE songid = :song_id"
        };

        QString filename = query.value(0).toString();
        for (int idx = 0; idx < 2; ++idx )
        {
            query.prepare( queries[idx]);
            query.bindValue(":song_id", QVariant(song_id));
            query.bindValue(":artistName", artist);
            query.bindValue(":title", title);
            query.bindValue(":discId", discID);
            query.bindValue(":searchstring", QString(filename + " " + artist + " " + title + " " + discID));
            query.exec();
          }
        select();
        search(lastSearch);
    }
}

bool DbTableModel::fetchSongDetail(int song_id, QString &artist, QString &title, QString &discID, QString &filename)
{
    QSqlQuery query;
    query.prepare("SELECT artist, title, discId, filename FROM main.dbsongs WHERE songid = :song_id");
    query.bindValue(":song_id", QVariant(song_id));
    query.exec();
    if (!query.first())
      return false;
    artist = query.value(0).toString();
    title = query.value(1).toString();
    discID = query.value(2).toString();
    filename = query.value(3).toString();
    return true;
}

//QVariant DbTableModel::headerData(int section, Qt::Orientation orientation, int role) const
//{
//    QSize sbSize(QFontMetrics(settings->applicationFont()).height(), QFontMetrics(settings->applicationFont()).height());
//    if (section == 1 && role == Qt:: SizeHintRole)
//        return 500;
//    if (section == 2 && role == Qt::SizeHintRole)
//        return 500;
//    if (section == 4 && role == Qt::SizeHintRole)
//        return QFontMetrics(settings->applicationFont()).width(" Duration ");
//    if (section == 3 && role == Qt::SizeHintRole)
//        return QFontMetrics(settings->applicationFont()).width(" AA0000000-00 ");

//    return QSqlTableModel::headerData(section, orientation, role);
//}

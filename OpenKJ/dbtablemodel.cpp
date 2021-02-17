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
    QString thm = (settings->theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_cdgIcon = QIcon(thm + "mimetypes/22/application-x-cda.svg");
    m_zipIcon = QIcon(thm + "mimetypes/22/application-zip.svg");
    m_vidIcon = QIcon(thm + "mimetypes/22/video-mp4.svg");
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
    searchString.replace("\"", " ");
    searchString.replace(",", " ");
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
    QString searchScope;
    switch (m_searchType) {
    case SEARCH_TYPE_ALL:
        searchScope = "searchstring";
        break;
    case SEARCH_TYPE_ARTIST:
        searchScope = "artist";
        break;
    case SEARCH_TYPE_TITLE:
        searchScope = "title";
    }
    if (settings->ignoreAposInSearch())
        whereClause = "discid != \"!!BAD!!\" AND discid != \"!!DROPPED!!\" AND replace(" + searchScope + ", \"'\", \"\") LIKE \"%" + terms.at(0) + "%\"";
    else
        whereClause = "discid != \"!!BAD!!\" AND discid != \"!!DROPPED!!\" AND " + searchScope + " LIKE \"%" + terms.at(0) + "%\"";
    for (int i=1; i < terms.size(); i++)
    {
        if (settings->ignoreAposInSearch())
            whereClause = whereClause + " AND replace(" + searchScope + ", \"'\", \"\") LIKE \"%" + terms.at(i) + "%\"";
        else
            whereClause = whereClause + " AND " + searchScope + " LIKE \"%" + terms.at(i) + "%\"";
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
    if (section == 1 && role == Qt::DisplayRole)
        return tr("Artist");
    if (section == 2 && role == Qt::DisplayRole)
        return tr("Title");
    if (section == 3 && role == Qt::DisplayRole)
        return tr("SongID");
    if (section == 4 && role == Qt::DisplayRole)
        return tr("Duration");
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
    qInfo() << "Refreshing dbsongs cache";
    QSqlQuery query(db);
    query.exec("DELETE FROM mem.dbsongs");
    query.exec("VACUUM mem");
    query.exec("INSERT INTO mem.dbsongs SELECT * FROM main.dbsongs");
    setTable("mem.dbsongs");
    select();
    search(lastSearch);
}

void DbTableModel::setSearchType(DbTableModel::SearchType type)
{
    m_searchType = type;
    search(lastSearch);
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


QVariant DbTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole && index.column() == 3)
    {
        QSize sbSize(QFontMetrics(settings->applicationFont()).height(), QFontMetrics(settings->applicationFont()).height());
        QString filename = index.sibling(index.row(), 5).data().toString();
        qInfo() << filename;
        qInfo() << "decorationrole call for songid";
        if (filename.endsWith("zip", Qt::CaseInsensitive))
        {
            qInfo() << "zip file";
            return m_zipIcon.pixmap(sbSize);
        }
        else if (filename.endsWith("cdg", Qt::CaseInsensitive))
        {
            return m_cdgIcon.pixmap(sbSize);
        }
        else
        {
            qInfo() << "video file";
            return m_vidIcon.pixmap(sbSize);
        }
    }
    if (role == Qt::ToolTipRole)
        return data(index);
    return QSqlTableModel::data(index, role);
}

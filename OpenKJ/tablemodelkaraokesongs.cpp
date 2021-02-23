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

#include "tablemodelkaraokesongs.h"
#include <QMimeData>
#include <QByteArray>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QPainter>



TableModelKaraokeSongs::TableModelKaraokeSongs(QObject *parent, QSqlDatabase db) :
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


QMimeData *TableModelKaraokeSongs::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/songid", data(indexes.at(0).sibling(indexes.at(0).row(), 0)).toByteArray().data());
    return mimeData;
}

Qt::ItemFlags TableModelKaraokeSongs::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

void TableModelKaraokeSongs::search(QString searchString)
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

QString TableModelKaraokeSongs::orderByClause() const
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

QVariant TableModelKaraokeSongs::headerData(int section, Qt::Orientation orientation, int role) const
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

void TableModelKaraokeSongs::sort(int column, Qt::SortOrder order)
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

void TableModelKaraokeSongs::refreshCache()
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

void TableModelKaraokeSongs::setSearchType(TableModelKaraokeSongs::SearchType type)
{
    m_searchType = type;
    search(lastSearch);
}

int TableModelKaraokeSongs::getSongIdForPath(const QString &path)
{
    QSqlQuery query;
    query.prepare("SELECT songid FROM mem.dbSongs WHERE path = :path");
    query.bindValue(":path", path);
    query.exec();
    if (query.next())
        return query.value(0).toInt();
    return -1;
}

void TableModelKaraokeSongs::updateSongHistory(const int dbSongId)
{
    QSqlQuery query;
    query.prepare("UPDATE dbSongs set plays = plays + :incVal, lastplay = :curTs WHERE songid = :songid");
    query.bindValue(":curTs", QDateTime::currentDateTime());
    query.bindValue(":songid", dbSongId);
    query.bindValue(":incVal", 1);
    query.prepare("UPDATE mem.dbSongs set plays = plays + :incVal, lastplay = :curTs WHERE songid = :songid");
    query.bindValue(":curTs", QDateTime::currentDateTime());
    query.bindValue(":songid", dbSongId);
    query.bindValue(":incVal", 1);
    query.exec();
    select();
    qInfo() << query.lastError();
}

QVariant TableModelKaraokeSongs::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole && index.column() == 3)
    {
        QSize sbSize(QFontMetrics(settings->applicationFont()).height(), QFontMetrics(settings->applicationFont()).height());
        QString filename = index.sibling(index.row(), 5).data().toString();
        if (filename.endsWith("zip", Qt::CaseInsensitive))
        {
            return m_zipIcon.pixmap(sbSize);
        }
        else if (filename.endsWith("cdg", Qt::CaseInsensitive))
        {
            return m_cdgIcon.pixmap(sbSize);
        }
        else
        {
            return m_vidIcon.pixmap(sbSize);
        }
    }
    if (role == Qt::ToolTipRole)
        return data(index);
    return QSqlTableModel::data(index, role);
}

ItemDelegateKaraokeSongs::ItemDelegateKaraokeSongs(QObject *parent) :
    QItemDelegate(parent)
{
}

void ItemDelegateKaraokeSongs::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == 3)
    {
        QItemDelegate::paint(painter, option, index);
        return;
    }
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    if (index.column() == 4)
    {
        if (index.data().toInt() <= 0)
            return;
        QString duration = QTime(0,0,0,0).addMSecs(index.data().toInt()).toString("m:ss");
        painter->save();
        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.highlightedText().color());
        painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignCenter, duration);
        painter->restore();
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.highlightedText().color());
    painter->drawText(option.rect, Qt::TextSingleLine | Qt::AlignVCenter, " " + index.data().toString());
    painter->restore();
}

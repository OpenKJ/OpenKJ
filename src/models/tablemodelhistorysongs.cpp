#include "tablemodelhistorysongs.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

TableModelHistorySongs::TableModelHistorySongs(TableModelKaraokeSongs &songsModel) : m_karaokeSongsModel(songsModel)
{
}


int TableModelHistorySongs::rowCount([[maybe_unused]]const QModelIndex &parent) const
{
    return m_songs.size();
}

int TableModelHistorySongs::columnCount([[maybe_unused]]const QModelIndex &parent) const
{
    return 9;
}

QVariant TableModelHistorySongs::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ForegroundRole)
    {
        if (m_karaokeSongsModel.getIdForPath(m_songs.at(index.row()).filePath) == -1)
            return QColor(Qt::gray);
    }
    if (role == Qt::ToolTipRole)
    {
        if (m_karaokeSongsModel.getIdForPath(m_songs.at(index.row()).filePath) == -1)
            return QString("Song doesn't exist in current database.");
    }
    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column()) {
        case 6:
            return Qt::AlignHCenter;
        case 7:
            return Qt::AlignRight;
        case 8:
            return Qt::AlignHCenter;
        default:
            return Qt::AlignLeft;
        }
    }
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
        case 0:
            return m_songs.at(index.row()).id;
        case 1:
            return m_songs.at(index.row()).historySinger;
        case 2:
            return m_songs.at(index.row()).filePath;
        case 3:
            return m_songs.at(index.row()).artist;
        case 4:
            return m_songs.at(index.row()).title;
        case 5:
            return m_songs.at(index.row()).songid;
        case 6:
        {
            int key = m_songs.at(index.row()).keyChange;
            if (key == 0)
                return QVariant();
            if (key > 0)
                return "+" + QString::number(key);
            else
                return key;
        }
        case 7:
            return m_songs.at(index.row()).plays;
        case 8:
        {
            QLocale locale;
            return m_songs.at(index.row()).lastPlayed.toString(locale.dateFormat(QLocale::ShortFormat));
        }
        }
    }
    return QVariant();
}

void TableModelHistorySongs::loadSinger(const int historySingerId)
{
    qInfo() << "SingerHistoryTableModel::loadSinger(" << historySingerId << ") called";
    emit layoutAboutToBeChanged();
    beginInsertRows(QModelIndex(),m_songs.size(),m_songs.size());
    m_songs.clear();
    QSqlQuery query;
    query.prepare("SELECT * from historySongs WHERE historySinger = :historySinger");
    query.bindValue(":historySinger", historySingerId);
    query.exec();
    while (query.next())
    {
        HistorySong song;
        song.id = query.value(0).toUInt();
        song.historySinger = query.value(1).toUInt();
        song.filePath = query.value(2).toString();
        song.artist = query.value(3).toString();
        song.title = query.value(4).toString();
        song.songid = query.value(5).toString();
        song.keyChange = query.value(6).toInt();
        song.plays = query.value(7).toUInt();
        song.lastPlayed = (query.value(8).canConvert<QDateTime>()) ? query.value(8).toDateTime() : QDateTime();
        m_songs.emplace_back(song);
    }
    sort(m_lastSortColumn, m_lastSortOrder);
    emit layoutChanged();
    emit endInsertRows();
}

void TableModelHistorySongs::loadSinger(const QString historySingerName)
{
    qInfo() << "SingerHistoryTableModel::loadSinger(" << historySingerName << ") called";
    m_currentSinger = historySingerName;
    QSqlQuery query;
    query.prepare("SELECT id FROM historySingers WHERE name == :name LIMIT 1");
    query.bindValue(":name", historySingerName);
    query.exec();
    if (query.next())
        loadSinger(query.value(0).toUInt());
    else
    {
        qInfo() << "No history found for singer, nothing loaded";
        emit layoutAboutToBeChanged();
        m_songs.clear();
        emit layoutChanged();
    }
}

void TableModelHistorySongs::saveSong(const QString &singerName, const QString &filePath, const QString &artist, const QString &title, const QString &songid, const int keyChange)
{
    qInfo() << "filepath: " << filePath;
    if (artist == "--Dropped Song--")
    {
        qInfo() << "Song was added via drop from external source, not saving to history";
        return;
    }
    QSqlQuery query;
    auto historySingerId = getSingerId(singerName);
    if (historySingerId != -1 && songExists(historySingerId, filePath))
    {
        qInfo() << "Song already in singer history, updating existing record";
        query.prepare("UPDATE historySongs SET artist = :artist, title = :title, songid = :songid, "
                      "keychange = :keychange, plays = plays + 1, lastplay = :datetime "
                      "WHERE filePath = :filepath AND historysinger = :historysinger");
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":songid", songid);
        query.bindValue(":keychange", keyChange);
        query.bindValue(":filepath", filePath);
        query.bindValue(":historysinger", historySingerId);
        query.bindValue(":datetime", QDateTime::currentDateTime());
        query.exec();
        loadSinger(m_currentSinger);
        qInfo() << query.lastError();
        return;
    }
    if (historySingerId == -1)
    {
        qInfo() << "Singer does not exist in historySinger table, adding";
        historySingerId = addSinger(singerName);
    }
    qInfo() << "Adding new song to singer history";
    query.prepare("INSERT INTO historySongs (historySinger, filepath, artist, title, songid, keychange, plays, lastplay) "
                  "values (:historySinger, :filepath, :artist, :title, :songid, :keychange, 1, :datetime)");
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":songid", songid);
    query.bindValue(":keychange", keyChange);
    query.bindValue(":filepath", filePath);
    query.bindValue(":historySinger", historySingerId);
    query.bindValue(":datetime", QDateTime::currentDateTime());
    query.exec();
    qInfo() << query.lastError();
    loadSinger(m_currentSinger);
}

void TableModelHistorySongs::saveSong(const QString &singerName, const QString &filePath, const QString &artist, const QString &title, const QString &songid, const int keyChange, int plays, QDateTime lastPlayed)
{
    qInfo() << "filepath: " << filePath;
    QSqlQuery query;
    auto historySingerId = getSingerId(singerName);
    if (historySingerId != -1 && songExists(historySingerId, filePath))
    {
        qInfo() << "Song already in singer history, skipping";
        return;
    }
    if (historySingerId == -1)
    {
        qInfo() << "Singer does not exist in historySinger table, adding";
        historySingerId = addSinger(singerName);
    }
    qInfo() << "Adding new song to singer history";
    query.prepare("INSERT INTO historySongs (historySinger, filepath, artist, title, songid, keychange, plays, lastplay) "
                  "values (:historySinger, :filepath, :artist, :title, :songid, :keychange, :plays, :datetime)");
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":songid", songid);
    query.bindValue(":keychange", keyChange);
    query.bindValue(":filepath", filePath);
    query.bindValue(":historySinger", historySingerId);
    query.bindValue(":plays", plays);
    query.bindValue(":datetime", lastPlayed);
    query.exec();
    qInfo() << query.lastError();
    loadSinger(m_currentSinger);
}

void TableModelHistorySongs::deleteSong(const int historySongId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM historySongs WHERE id = :historySongId");
    query.bindValue(":historySongId", historySongId);
    query.exec();
    loadSinger(m_currentSinger);
}

int TableModelHistorySongs::addSinger(const QString name) const
{
    qInfo() << "Inserting singer into history: " << name;
    QSqlQuery query;
    query.prepare("INSERT INTO historySingers (name) VALUES( :name )");
    query.bindValue(":name", name);
    query.exec();
    qInfo() << query.lastError();
    return query.lastInsertId().toInt();
}

bool TableModelHistorySongs::songExists(const int historySingerId, const QString &filePath) const
{
    QSqlQuery query;
    query.prepare("SELECT id FROM historySongs WHERE historySinger = :historySinger AND filepath = :filePath LIMIT 1");
    query.bindValue(":historySinger", historySingerId);
    query.bindValue(":filePath", filePath);
    query.exec();
    if (query.next())
        return true;
    return false;
}

int TableModelHistorySongs::getSingerId(const QString &name) const
{
    int retVal = -1;
    QSqlQuery query;
    query.prepare("SELECT id FROM historySingers WHERE name = :name LIMIT 1");
    query.bindValue(":name", name);
    query.exec();
    if (query.next())
    {
        retVal = query.value(0).toInt();
    }
    return retVal;
}

std::vector<HistorySong> TableModelHistorySongs::getSingerSongs(const int historySingerId)
{
    std::vector<HistorySong> songs;
    QSqlQuery query;
    query.prepare("SELECT * from historySongs WHERE historySinger = :historySinger");
    query.bindValue(":historySinger", historySingerId);
    query.exec();
    while (query.next())
    {
        HistorySong song;
        song.id = query.value(0).toUInt();
        song.historySinger = query.value(1).toUInt();
        song.filePath = query.value(2).toString();
        song.artist = query.value(3).toString();
        song.title = query.value(4).toString();
        song.songid = query.value(5).toString();
        song.keyChange = query.value(6).toInt();
        song.plays = query.value(7).toUInt();
        song.lastPlayed = query.value(8).toDateTime();
        songs.emplace_back(song);
    }
    return songs;
}

void TableModelHistorySongs::refresh()
{
    if (getSingerId(m_currentSinger) != -1)
    {
        emit layoutAboutToBeChanged();
        m_songs.clear();
        emit layoutChanged();
        return;
    }
    loadSinger(m_currentSinger);
}


QVariant TableModelHistorySongs::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole  && orientation == Qt::Horizontal)
    {
        switch (section) {
        case 0:
            return "id";
        case 1:
            return "historySingerId";
        case 2:
            return "filepath";
        case 3:
            return "Artist";
        case 4:
            return "Title";
        case 5:
            return "SongID";
        case 6:
            return "Key";
        case 7:
            return "Plays";
        case 8:
            return "Last Play";
        }
    }
    return QVariant();
}


void TableModelHistorySongs::sort(int column, Qt::SortOrder order)
{
    m_lastSortColumn = column;
    m_lastSortOrder = order;
    emit layoutAboutToBeChanged();
    if (order == Qt::DescendingOrder)
    {
        std::sort(m_songs.begin(), m_songs.end(), [&column] (HistorySong a, HistorySong b) {
            switch (column) {
            case 3:
                return (a.artist.toLower() > b.artist.toLower());
            case 4:
                return (a.title.toLower() > b.title.toLower());
            case 5:
                return (a.songid.toLower() > b.songid.toLower());
            case 6:
                return (a.keyChange > b.keyChange);
            case 7:
                return (a.plays > b.plays);
            case 8:
                return (a.lastPlayed > b.lastPlayed);
            default:
                return (a.id > b.id);
            }
        });
    }
    else
    {
        std::sort(m_songs.begin(), m_songs.end(), [&column] (HistorySong a, HistorySong b) {
            switch (column) {
            case 3:
                return (a.artist.toLower() < b.artist.toLower());
            case 4:
                return (a.title.toLower() < b.title.toLower());
            case 5:
                return (a.songid.toLower() < b.songid.toLower());
            case 6:
                return (a.keyChange < b.keyChange);
            case 7:
                return (a.plays < b.plays);
            case 8:
                return (a.lastPlayed < b.lastPlayed);
            default:
                return (a.id < b.id);
            }
        });
    }
    emit layoutChanged();
}

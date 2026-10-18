#include "databaseworker.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <chrono>

DatabaseWorker::DatabaseWorker(const QString &dbPath, QObject *parent)
    : QObject(parent), m_dbPath(dbPath)
{
}

DatabaseWorker::~DatabaseWorker()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

void DatabaseWorker::init()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "openkj_background");
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) {
        spdlog::get("logger")->error("[DatabaseWorker] Failed to open background database connection: {}",
                                    m_db.lastError().text().toStdString());
        emit errorOccurred(m_db.lastError().text());
        return;
    }
    
    // Set fast SQLite pragmas
    QSqlQuery query(m_db);
    query.exec("PRAGMA synchronous=OFF");
    query.exec("PRAGMA cache_size=300000");
    query.exec("PRAGMA temp_store=2");
}

void DatabaseWorker::loadRotation()
{
    QList<okj::RotationSinger> singers;
    QSqlQuery query(m_db);
    
    // 1. Load basic singer details
    if (!query.exec("SELECT singerid, name, position, regular, addts FROM rotationsingers ORDER BY position")) {
        emit errorOccurred(query.lastError().text());
        return;
    }
    while (query.next()) {
        okj::RotationSinger s(
            query.value(0).toInt(),
            query.value(1).toString(),
            query.value(2).toInt(),
            query.value(3).toBool(),
            query.value(4).toDateTime()
        );
        singers.append(s);
    }
    
    // 2. Load numSongsSung counts
    query.exec("SELECT singer, COUNT(qsongid) FROM queuesongs WHERE played = 1 GROUP BY singer");
    while (query.next()) {
        int singerId = query.value(0).toInt();
        int count = query.value(1).toInt();
        for (auto &s : singers) {
            if (s.id == singerId) {
                s.cachedNumSongsSung = count;
                break;
            }
        }
    }
    
    // 3. Load numSongsUnsung counts
    query.exec("SELECT singer, COUNT(qsongid) FROM queuesongs WHERE played = 0 GROUP BY singer");
    while (query.next()) {
        int singerId = query.value(0).toInt();
        int count = query.value(1).toInt();
        for (auto &s : singers) {
            if (s.id == singerId) {
                s.cachedNumSongsUnsung = count;
                break;
            }
        }
    }
    
    // 4. Load next song details
    query.exec("SELECT q.singer, d.path, d.artist, d.title, d.discid, d.duration, q.keychg, q.qsongid "
               "FROM queuesongs q "
               "INNER JOIN dbsongs d ON d.songid = q.song "
               "WHERE q.played = 0 "
               "AND q.position = ("
               "    SELECT MIN(q2.position) "
               "    FROM queuesongs q2 "
               "    WHERE q2.singer = q.singer AND q2.played = 0"
               ")");
    while (query.next()) {
        int singerId = query.value(0).toInt();
        for (auto &s : singers) {
            if (s.id == singerId) {
                s.cachedNextSongPath = query.value(1).toString();
                s.cachedNextSongArtist = query.value(2).toString();
                s.cachedNextSongTitle = query.value(3).toString();
                s.cachedNextSongSongId = query.value(4).toString();
                s.cachedNextSongDurationSecs = (query.value(5).toInt() / 1000) + m_settings.estimationSingerPad();
                s.cachedNextSongKeyChg = query.value(6).toInt();
                s.cachedNextSongQueueId = query.value(7).toInt();
                break;
            }
        }
    }
    
    // Handle estimated empty song length for empty singers
    for (auto &s : singers) {
        if (s.cachedNextSongPath.isEmpty()) {
            if (!m_settings.estimationSkipEmptySingers()) {
                s.cachedNextSongDurationSecs = m_settings.estimationEmptySongLength() + m_settings.estimationSingerPad();
            } else {
                s.cachedNextSongDurationSecs = 0;
            }
        }
    }

    emit rotationLoaded(singers);
}

void DatabaseWorker::loadQueue(int singerId)
{
    QList<okj::QueueSong> songs;
    QSqlQuery query(m_db);
    query.prepare("SELECT queuesongs.qsongid, queuesongs.singer, queuesongs.song, queuesongs.played, "
                  "queuesongs.keychg, queuesongs.position, rotationsingers.name, dbsongs.artist, "
                  "dbsongs.title, dbsongs.discid, dbsongs.duration, dbsongs.path FROM queuesongs "
                  "INNER JOIN rotationsingers ON rotationsingers.singerid = queuesongs.singer "
                  "INNER JOIN dbsongs ON dbsongs.songid = queuesongs.song WHERE queuesongs.singer = :singerId "
                  "ORDER BY queuesongs.position");
    query.bindValue(":singerId", singerId);
    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        return;
    }
    while (query.next()) {
        songs.emplace_back(okj::QueueSong{
            query.value(0).toInt(),
            query.value(1).toInt(),
            query.value(2).toInt(),
            query.value(3).toBool(),
            query.value(4).toInt(),
            query.value(5).toInt(),
            query.value(7).toString(),
            query.value(8).toString(),
            query.value(9).toString(),
            query.value(10).toInt(),
            query.value(11).toString()
        });
    }
    emit queueLoaded(songs);
}

void DatabaseWorker::addSinger(const QString &name, int positionHint, bool makeRegular)
{
    QSqlQuery query(m_db);
    int addPos = 0;
    query.exec("SELECT COUNT(singerid) FROM rotationsingers");
    if (query.next()) {
        addPos = query.value(0).toInt();
    }
    
    QDateTime curTs = QDateTime::currentDateTime();
    query.prepare("INSERT INTO rotationsingers (name, position, regular, regularid, addts) "
                  "VALUES (:name, :pos, :regular, -1, :addts)");
    query.bindValue(":name", name);
    query.bindValue(":pos", addPos);
    query.bindValue(":regular", makeRegular);
    query.bindValue(":addts", curTs);
    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        return;
    }
    int singerId = query.lastInsertId().toInt();
    
    emit singerAdded(singerId, name, positionHint);
    loadRotation();
}

void DatabaseWorker::addSingerWithSong(const QString &name, int positionHint, int songId, int keyChange, bool makeRegular)
{
    QSqlQuery query(m_db);
    int addPos = 0;
    query.exec("SELECT COUNT(singerid) FROM rotationsingers");
    if (query.next()) {
        addPos = query.value(0).toInt();
    }
    
    QDateTime curTs = QDateTime::currentDateTime();
    query.prepare("INSERT INTO rotationsingers (name, position, regular, regularid, addts) "
                  "VALUES (:name, :pos, :regular, -1, :addts)");
    query.bindValue(":name", name);
    query.bindValue(":pos", addPos);
    query.bindValue(":regular", makeRegular);
    query.bindValue(":addts", curTs);
    query.exec();
    int singerId = query.lastInsertId().toInt();
    
    query.prepare("INSERT INTO queuesongs (singer, song, artist, title, discid, path, keychg, played, position) "
                  "VALUES (:singerId, :songId, :songId, :songId, :songId, :songId, :key, 0, 0)");
    query.bindValue(":singerId", singerId);
    query.bindValue(":songId", songId);
    query.bindValue(":key", keyChange);
    query.exec();
    
    emit singerAdded(singerId, name, positionHint);
    emit queueUpdated(singerId);
    loadRotation();
}

void DatabaseWorker::moveSinger(int oldPosition, int newPosition)
{
    // Sorting and updating position is handled by TableModelRotation::singerMove in memory,
    // which then calls commitRotation. So this slot isn't strictly needed if we use commitRotation.
}

void DatabaseWorker::renameSinger(int singerId, const QString &newName)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE rotationsingers SET name = :name WHERE singerid = :singerid");
    query.bindValue(":name", newName);
    query.bindValue(":singerid", singerId);
    query.exec();
    loadRotation();
}

void DatabaseWorker::deleteSinger(int singerId)
{
    // The model updates in-memory and then commits via commitRotation.
}

void DatabaseWorker::setSingerRegular(int singerId, bool isRegular)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE rotationsingers SET regular = :regular WHERE singerid = :singerid");
    query.bindValue(":regular", isRegular);
    query.bindValue(":singerid", singerId);
    query.exec();
    loadRotation();
}

void DatabaseWorker::clearRotation()
{
    QSqlQuery query(m_db);
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM queuesongs");
    query.exec("DELETE FROM rotationsingers");
    query.exec("COMMIT");
    loadRotation();
}

void DatabaseWorker::addSongToQueue(int singerId, int songId, int keyChange)
{
    // Use the destination queue's persisted order, independent of which singer
    // is currently displayed. Compute the append position in the same statement.
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO queuesongs (singer, song, artist, title, discid, path, keychg, played, position) "
                  "SELECT :singerId, :songId, :songId, :songId, :songId, :songId, :key, 0, "
                  "COALESCE(MAX(position), -1) + 1 FROM queuesongs WHERE singer = :singerId");
    query.bindValue(":singerId", singerId);
    query.bindValue(":songId", songId);
    query.bindValue(":key", keyChange);
    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        return;
    }
    
    emit queueUpdated(singerId);
    loadRotation();
}

void DatabaseWorker::moveQueueSong(int singerId, int oldPosition, int newPosition)
{
    // The model handles sorting in-memory and commits via commitQueue.
}

void DatabaseWorker::deleteQueueSong(int singerId, int queueSongId)
{
    // Handled via commitQueue.
}

void DatabaseWorker::setQueueSongKey(int singerId, int songId, int key)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE queuesongs SET keychg = :key WHERE qsongid = :id");
    query.bindValue(":id", songId);
    query.bindValue(":key", key);
    query.exec();
    
    emit queueUpdated(singerId);
    loadRotation();
}

void DatabaseWorker::setQueueSongPlayed(int singerId, int songId, bool played)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE queuesongs SET played = :played WHERE qsongid = :id");
    query.bindValue(":id", songId);
    query.bindValue(":played", played);
    query.exec();
    
    emit queueUpdated(singerId);
    loadRotation();
}

void DatabaseWorker::removeAllQueueSongs(int singerId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM queuesongs WHERE singer = :singerId");
    query.bindValue(":singerId", singerId);
    query.exec();
    
    emit queueUpdated(singerId);
    loadRotation();
}

void DatabaseWorker::commitRotation(const QList<okj::RotationSinger> &singers)
{
    QSqlQuery query(m_db);
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM rotationsingers");
    query.prepare("INSERT INTO rotationsingers (singerid, name, position, regular, regularid, addts) "
                  "VALUES (:singerid, :name, :pos, :regular, -1, :addts)");
    for (const auto &singer : singers) {
        query.bindValue(":singerid", singer.id);
        query.bindValue(":name", singer.name);
        query.bindValue(":pos", singer.position);
        query.bindValue(":regular", singer.regular);
        query.bindValue(":addts", singer.addTs);
        query.exec();
    }
    query.exec("COMMIT");
    loadRotation();
}

void DatabaseWorker::commitQueue(int singerId, const QList<okj::QueueSong> &songs)
{
    QSqlQuery query(m_db);
    query.exec("BEGIN TRANSACTION");
    query.prepare("DELETE FROM queuesongs WHERE singer = :singerId");
    query.bindValue(":singerId", singerId);
    query.exec();
    
    query.prepare("INSERT INTO queuesongs (qsongid, singer, song, artist, title, discid, path, keychg, played, position) "
                  "VALUES (:id, :singerId, :songId, :songId, :songId, :songId, :songId, :key, :played, :position)");
    for (const auto &song : songs) {
        query.bindValue(":id", song.id);
        query.bindValue(":singerId", song.singerId);
        query.bindValue(":songId", song.dbSongId);
        query.bindValue(":key", song.keyChange);
        query.bindValue(":played", song.played);
        query.bindValue(":position", song.position);
        query.exec();
    }
    query.exec("COMMIT");
    
    emit queueUpdated(singerId);
    loadRotation();
}

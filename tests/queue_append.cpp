#include "databaseworker.h"

#include <QApplication>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <spdlog/sinks/null_sink.h>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void exec(QSqlDatabase &db, const QString &sql)
{
    QSqlQuery query(db);
    if (!query.exec(sql))
        throw std::runtime_error(query.lastError().text().toStdString());
}

QList<int> songIds(const QList<okj::QueueSong> &songs)
{
    QList<int> ids;
    for (const auto &song : songs)
        ids.append(song.dbSongId);
    return ids;
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    spdlog::null_logger_mt("logger");

    try {
        QTemporaryDir temp;
        require(temp.isValid(), "Cannot create temporary database directory");
        DatabaseWorker worker(temp.filePath("queue.sqlite"));
        QString error;
        QObject::connect(&worker, &DatabaseWorker::errorOccurred, &app,
                         [&](const QString &message) { error = message; });
        worker.init();
        require(error.isEmpty(), "Cannot initialize database worker");
        auto db = QSqlDatabase::database("openkj_background");
        exec(db, "CREATE TABLE rotationsingers (singerid INTEGER PRIMARY KEY, name TEXT, "
                 "position INT, regular INT, regularid INT, addts TEXT)");
        exec(db, "CREATE TABLE dbsongs (songid INTEGER PRIMARY KEY, artist TEXT, title TEXT, "
                 "discid TEXT, duration INT, path TEXT)");
        exec(db, "CREATE TABLE queuesongs (qsongid INTEGER PRIMARY KEY, singer INT, song INT, "
                 "artist INT, title INT, discid INT, path INT, keychg INT, played INT, position INT)");
        exec(db, "INSERT INTO rotationsingers VALUES (1, 'Two songs', 0, 0, -1, ''), "
                 "(2, 'Seven songs', 1, 0, -1, ''), (3, 'Empty queue', 2, 0, -1, '')");
        for (int id = 1; id <= 12; ++id)
            exec(db, QString("INSERT INTO dbsongs VALUES (%1, 'Artist', 'Title', 'Disc', 1000, '/song%1')").arg(id));
        for (int pos = 0; pos < 7; ++pos) {
            exec(db, QString("INSERT INTO queuesongs (singer, song, keychg, played, position) "
                             "VALUES (2, %1, 0, %2, %3)").arg(pos + 1).arg(pos < 2 ? 1 : 0).arg(pos));
        }
        for (int pos = 0; pos < 2; ++pos)
            exec(db, QString("INSERT INTO queuesongs (singer, song, keychg, played, position) "
                             "VALUES (1, %1, 0, 0, %2)").arg(pos + 1).arg(pos));

        QList<okj::QueueSong> loaded;
        QObject::connect(&worker, &DatabaseWorker::queueLoaded, &app,
                         [&](const QList<okj::QueueSong> &songs) { loaded = songs; });
        // Loading a two-song queue must not determine the destination's append position.
        worker.loadQueue(1);
        require(loaded.size() == 2, "Source queue should have two songs");
        worker.addSongToQueue(2, 8, -2);
        worker.addSongToQueue(2, 9, 3);
        worker.loadQueue(2);
        require(songIds(loaded) == QList<int>({1, 2, 3, 4, 5, 6, 7, 8, 9}),
                "New songs must follow all seven existing songs in order");
        require(loaded[7].position == 7 && loaded[8].position == 8,
                "Repeated additions must receive distinct append positions");
        require(loaded[7].keyChange == -2 && loaded[8].keyChange == 3 && !loaded[7].played,
                "Appended songs must preserve key changes and start unplayed");
        require(loaded[0].played && loaded[1].played, "Existing played songs must be preserved");

        worker.addSongToQueue(3, 10, 0);
        worker.loadQueue(3);
        require(loaded.size() == 1 && loaded[0].position == 0, "An empty queue must start at zero");

        // Imported or previously edited queues may have gaps in their positions.
        exec(db, "UPDATE queuesongs SET position = 20 WHERE singer = 2 AND song = 9");
        worker.addSongToQueue(2, 11, 0);
        worker.loadQueue(2);
        require(loaded.last().dbSongId == 11 && loaded.last().position == 21,
                "Append must follow the last position even when there are gaps");
        worker.loadQueue(1);
        require(songIds(loaded) == QList<int>({1, 2}), "Another singer's queue must remain unchanged");
        require(error.isEmpty(), "Database worker reported an error");
        qInfo() << "Queue append regression checks passed";
    } catch (const std::exception &error) {
        qCritical() << error.what();
        return 1;
    }
    return 0;
}

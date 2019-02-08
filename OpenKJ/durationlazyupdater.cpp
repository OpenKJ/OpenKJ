#include "durationlazyupdater.h"

#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include "okarchive.h"
#include "karaokefileinfo.h"


void LazyDurationUpdateWorker::getDurations(const QStringList files) {
    OkArchive archive;
    KaraokeFileInfo parser;
    QString path;
    foreach (path, files)
    {
        int duration = 0;
        archive.setArchiveFile(path);
        duration = archive.getSongDuration();
        if (duration == 0)
            duration = parser.getDuration();
        qWarning() << "PATH: " << path << " -- DURATION: " << duration;
        emit gotDuration(path, duration);
        if (QThread::currentThread()->isInterruptionRequested())
            break;
        //QThread::msleep(50);
    }
}

LazyDurationUpdateController::LazyDurationUpdateController(QObject *parent) : QObject(parent) {
    LazyDurationUpdateWorker *worker = new LazyDurationUpdateWorker;
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &LazyDurationUpdateController::operate, worker, &LazyDurationUpdateWorker::getDurations);
    connect(worker, &LazyDurationUpdateWorker::gotDuration, this, &LazyDurationUpdateController::updateDbDuration);
    workerThread.start();
    //getDurations();
}

LazyDurationUpdateController::~LazyDurationUpdateController() {
    workerThread.quit();
    workerThread.wait();
}

void LazyDurationUpdateController::getSongsRequiringUpdate()
{
    qWarning() << "Finding songs that need durations";
    files.clear();
    QSqlQuery query;
    query.exec("SELECT path FROM dbsongs WHERE duration < 1 ORDER BY artist, title");
    while (query.next())
    {
        files.append(query.value(0).toString());
    }
    qWarning() << "Done, found " << files.size() << " songs that need durations";
}

void LazyDurationUpdateController::stopWork()
{
    qWarning() << "LazyDurationUpdateController stoWork() called";
    workerThread.requestInterruption();
}

void LazyDurationUpdateController::updateDbDuration(QString file, int duration)
{
    QSqlQuery query;
    query.prepare("UPDATE dbsongs SET duration = :duration WHERE path = :path");
    query.bindValue(":path", file);
    query.bindValue(":duration", duration);
    query.exec();
    query.prepare("UPDATE mem.dbsongs SET duration = :duration WHERE path = :path");
    query.bindValue(":path", file);
    query.bindValue(":duration", duration);
    query.exec();
}

void LazyDurationUpdateController::getDurations()
{
    getSongsRequiringUpdate();
    emit operate(files);
}

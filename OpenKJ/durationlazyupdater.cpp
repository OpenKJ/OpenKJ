#include "durationlazyupdater.h"

#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include "mzarchive.h"
#include "karaokefileinfo.h"


void LazyDurationUpdateWorker::getDurations(const QStringList files) {
    MzArchive archive;
    KaraokeFileInfo parser;
    QString path;
    foreach (path, files)
    {
        int duration = 0;
        if (path.endsWith(".zip", Qt::CaseInsensitive))
        {
            archive.setArchiveFile(path);
            duration = archive.getSongDuration();
        }
        if (duration == 0)
        {
            parser.setFileName(path);
            duration = parser.getDuration();
        }
        //qInfo() << "PATH: " << path << " -- DURATION: " << duration;
        emit gotDuration(path, duration);
        if (QThread::currentThread()->isInterruptionRequested())
            break;
    }
}

LazyDurationUpdateController::LazyDurationUpdateController(QObject *parent) : QObject(parent) {
    LazyDurationUpdateWorker *worker = new LazyDurationUpdateWorker;
    workerThread.setObjectName("DurationUpdater");
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
    qInfo() << "Finding songs that need durations";
    files.clear();
    QSqlQuery query;
    query.exec("SELECT path FROM dbsongs WHERE duration < 1 ORDER BY artist, title");
    files.reserve(query.size());
    while (query.next())
    {
        files.append(query.value(0).toString());
    }
    qInfo() << "Done, found " << files.size() << " songs that need durations";
}

void LazyDurationUpdateController::stopWork()
{
    qInfo() << "LazyDurationUpdateController stoWork() called";
    workerThread.requestInterruption();
}

void LazyDurationUpdateController::updateDbDuration(QString file, int duration)
{
    QSqlQuery query;
    query.prepare("UPDATE dbsongs SET duration = :duration WHERE path = :path");
    query.bindValue(":path", file);
    query.bindValue(":duration", duration);
    query.exec();
    emit gotDuration(file, duration);
}

void LazyDurationUpdateController::getDurations()
{
    getSongsRequiringUpdate();
    emit operate(files);
}

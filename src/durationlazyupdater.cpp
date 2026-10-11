#include "durationlazyupdater.h"

#include <QSqlQuery>
#include <QVariant>
#include "mzarchive.h"
#include "karaokefileinfo.h"
#include <QFileInfo>



void LazyDurationUpdateWorker::getDurations(const QStringList &files)
{
    if (files.isEmpty())
        return;

    const std::string loggingPrefix{"[LazyDurationThread]"};
    auto logger = spdlog::get("logger");

    logger->info("{} Starting scan of {} songs",
                 loggingPrefix, files.size());

    MzArchive archive;
    KaraokeFileInfo parser;

    qsizetype updated = 0;
    qsizetype missing = 0;
    qsizetype failed = 0;

    for (const auto &path : files)
    {
        // Check cancellation before processing each file.
        if (QThread::currentThread()->isInterruptionRequested()) {
            logger->info("{} Scan interrupted", loggingPrefix);
            break;
        }

        // Do not attempt to analyze inaccessible media.
        // Preserve the database entry for disconnected drives.
        if (!QFileInfo::exists(path)) {
            ++missing;
            continue;
        }

        unsigned int duration = 0;

        if (path.endsWith(".zip", Qt::CaseInsensitive)) {
            archive.setArchiveFile(path);
            duration = archive.getSongDuration();
        }

        if (duration == 0) {
            parser.setFile(path);
            duration = parser.getDuration();
        }

        // A failed analysis must not trigger an unnecessary
        // database write or a table-model update.
        if (duration == 0) {
            ++failed;
            continue;
        }

        ++updated;
        emit gotDuration(path, duration);
    }

    logger->info(
        "{} Scan finished: updated={}, missing={}, failed={}",
        loggingPrefix, updated, missing, failed
    );
}

LazyDurationUpdateController::LazyDurationUpdateController(QObject *parent) : QObject(parent) {
    m_logger = spdlog::get("logger");
    auto *worker = new LazyDurationUpdateWorker;
    workerThread.setObjectName("DurationUpdater");
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &LazyDurationUpdateController::operate, worker, &LazyDurationUpdateWorker::getDurations);
    connect(worker, &LazyDurationUpdateWorker::gotDuration, this, &LazyDurationUpdateController::updateDbDuration);
    workerThread.start();
    workerThread.setPriority(QThread::IdlePriority);
}

LazyDurationUpdateController::~LazyDurationUpdateController() {
    workerThread.quit();
    workerThread.wait();
}

void LazyDurationUpdateController::getSongsRequiringUpdate()
{
    m_logger->info("{} Finding songs with missing durations", m_loggingPrefix);
    files.clear();
    QSqlQuery query;
    query.exec("SELECT path FROM dbsongs WHERE duration < 1 ORDER BY artist, title");
    files.reserve(query.size());
    while (query.next())
    {
        files.append(query.value(0).toString());
    }
    m_logger->info("{} Done, found {} songs with missing durations", m_loggingPrefix, files.size());
}

void LazyDurationUpdateController::stopWork()
{
    workerThread.requestInterruption();
}

void LazyDurationUpdateController::updateDbDuration(const QString& file, int duration)
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

#include "durationlazyupdater.h"

#include <QSqlQuery>
#include <QVariant>
#include "mzarchive.h"
#include "karaokefileinfo.h"


void LazyDurationUpdateWorker::getDurations(const QStringList &files) {
    if (files.isEmpty())
        return;
    std::string m_loggingPrefix{"[LazyDurationThread]"};
    std::shared_ptr<spdlog::logger> logger;
    logger = spdlog::get("logger");
    logger->info("{} Starting scan", m_loggingPrefix);
    MzArchive archive;
    KaraokeFileInfo parser;
    for (const auto &path : files)
    {
        unsigned int duration = 0;
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
        if (duration == 0)
            logger->warn("{} Unable to get duration for file {}. - File is likely corrupted or invalid", m_loggingPrefix, path);
        else
            logger->trace("{} Got duration: {} for file: {}", m_loggingPrefix, duration, path);
        emit gotDuration(path, duration);
        if (QThread::currentThread()->isInterruptionRequested()) {
            logger->info("{} Scan interrupt requested", m_loggingPrefix);
            break;
        }
    }
    logger->info("{} Scan complete", m_loggingPrefix);
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

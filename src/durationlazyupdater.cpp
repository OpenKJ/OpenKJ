#include "durationlazyupdater.h"

#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QUuid>
#include <QElapsedTimer>
#include <QVariant>
#include <QScopeGuard>
#include "mzarchive.h"
#include "karaokefileinfo.h"
#include <QFileInfo>



void LazyDurationUpdateWorker::getDurations(const QString &dbPath, const QStringList &onlyPaths)
{
    emit scanStarted();
    QString scanError;
    const QString connectionName = "duration-" + QUuid::createUuid().toString();
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);
        db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=250");
        auto run = [&]() {
            auto logger = spdlog::get("logger");
            if (!db.open()) {
                scanError = db.lastError().text();
                logger->error("[LazyDurationThread] Database open failed: {}", db.lastError().text().toStdString());
                return;
            }
            QElapsedTimer selectionTimer;
            selectionTimer.start();
            QStringList files;
            {
                QSqlQuery query(db);
                query.setForwardOnly(true);
                if (onlyPaths.isEmpty()) {
                    if (!query.exec("SELECT path FROM dbsongs WHERE duration < 1 ORDER BY artist, title")) {
                        scanError = query.lastError().text();
                        logger->error("[LazyDurationThread] Selection failed: {}", query.lastError().text().toStdString());
                        return;
                    }
                    while (query.next()) {
                        if (QThread::currentThread()->isInterruptionRequested())
                            return;
                        files.append(query.value(0).toString());
                    }
                } else {
                    query.prepare("SELECT path FROM dbsongs WHERE path=:path AND duration < 1");
                    for (const auto &path : onlyPaths) {
                        if (QThread::currentThread()->isInterruptionRequested())
                            return;
                        query.bindValue(":path", path);
                        if (!query.exec()) {
                            scanError = query.lastError().text();
                            logger->error("[LazyDurationThread] Song selection failed: {}", query.lastError().text().toStdString());
                            return;
                        }
                        if (query.next())
                            files.append(query.value(0).toString());
                    }
                }
            }
            logger->info("[DatabaseTiming] Missing-duration selection: {} ms, songs={}", selectionTimer.elapsed(), files.size());

            const std::string loggingPrefix{"[LazyDurationThread]"};
            logger->info("{} Starting scan of {} songs",
                         loggingPrefix, files.size());

            MzArchive archive;
            KaraokeFileInfo parser;

            qsizetype updated = 0;
            qsizetype missing = 0;
            qsizetype failed = 0;
            qsizetype zipFailed = 0;
            qint64 processed = 0;
            QElapsedTimer progressTimer;
            progressTimer.start();
            const auto reportProgress = [&](bool force) {
                if (force || progressTimer.elapsed() >= 250) {
                    emit scanProgress(processed, files.size(), updated, missing, failed);
                    progressTimer.restart();
                }
            };
            reportProgress(true);
            constexpr qsizetype zipWarningLimit = 5;

            QStringList batchPaths;
            QList<int> batchDurations;
            auto flush = [&]() {
                if (batchPaths.isEmpty())
                    return true;
                QSqlQuery query(db);
                bool ok = db.transaction();
                if (ok)
                    ok = query.prepare("UPDATE dbsongs SET duration = :duration WHERE path = :path AND duration < 1");
                for (qsizetype i = 0; ok && i < batchPaths.size(); ++i) {
                    query.bindValue(":path", batchPaths[i]);
                    query.bindValue(":duration", batchDurations[i]);
                    ok = query.exec();
                }
                if (ok)
                    ok = db.commit();
                if (!ok) {
                    scanError = query.lastError().text() + " / " + db.lastError().text();
                    logger->error("{} Duration batch write failed: {} / {}", loggingPrefix,
                                  query.lastError().text().toStdString(), db.lastError().text().toStdString());
                    db.rollback();
                    return false;
                }
                updated += batchPaths.size();
                emit durationsReady(batchPaths, batchDurations);
                // At most one batch can be queued. Interruption must also work when
                // the UI is waiting for this thread during shutdown or a refresh.
                while (!batchConsumed->tryAcquire(1, 25)) {
                    if (QThread::currentThread()->isInterruptionRequested())
                        return false;
                }
                batchPaths.clear();
                batchDurations.clear();
                QThread::msleep(25);
                return true;
            };

            bool writeFailed = false;
            for (const auto &path : files)
            {
                // Check cancellation before processing each file.
                if (QThread::currentThread()->isInterruptionRequested()) {
                    logger->info("{} Scan interrupted", loggingPrefix);
                    break;
                }

                const auto progressGuard = qScopeGuard([&] {
                    ++processed;
                    reportProgress(false);
                });

                // Do not attempt to analyze inaccessible media.
                // Preserve the database entry for disconnected drives.
                if (!QFileInfo::exists(path)) {
                    ++missing;
                    continue;
                }

                unsigned int duration = 0;

                if (path.endsWith(".zip", Qt::CaseInsensitive)) {
                    archive.setArchiveFile(path);
                    // The parser would retry the same archive on failure. Attempt it once
                    // and bound warnings so a bad library cannot flood the logger.
                    duration = archive.getSongDuration(false);
                    if (duration == 0) {
                        ++zipFailed;
                        if (zipFailed <= zipWarningLimit)
                            logger->warn("{} ZIP duration failed: {} ({})", loggingPrefix,
                                         path.toStdString(), archive.getLastError().toStdString());
                    }
                } else {
                    parser.setFile(path);
                    duration = parser.getDuration();
                }

                // A failed analysis must not trigger an unnecessary
                // database write or a table-model update.
                if (duration == 0) {
                    ++failed;
                    continue;
                }

                batchPaths.append(path);
                batchDurations.append(static_cast<int>(duration));
                if (batchPaths.size() >= 32 && !flush()) {
                    writeFailed = true;
                    break;
                }
            }

            if (!writeFailed && !QThread::currentThread()->isInterruptionRequested())
                flush();

            reportProgress(true);
            logger->info(
                "{} Scan finished: updated={}, missing={}, failed={}, zipFailed={}, zipWarningsSuppressed={}",
                loggingPrefix, updated, missing, failed, zipFailed,
                zipFailed > zipWarningLimit ? zipFailed - zipWarningLimit : 0
            );
        };
        run();
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    emit scanFinished(QThread::currentThread()->isInterruptionRequested(), scanError);
}

LazyDurationUpdateController::LazyDurationUpdateController(QObject *parent) : QObject(parent) {
    m_logger = spdlog::get("logger");
    auto *worker = new LazyDurationUpdateWorker;
    workerThread.setObjectName("DurationUpdater");
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &LazyDurationUpdateController::operate, worker, &LazyDurationUpdateWorker::getDurations);
    connect(worker, &LazyDurationUpdateWorker::durationsReady, this,
            [this, consumed = worker->batchConsumed](const QStringList &paths, const QList<int> &durations) {
        if (!workerThread.isInterruptionRequested()) {
            for (qsizetype i = 0; i < paths.size(); ++i)
                emit gotDuration(paths[i], durations[i]);
        }
        consumed->release();
    });
    connect(worker, &LazyDurationUpdateWorker::scanStarted, this, [this] {
        if (!workerThread.isInterruptionRequested())
            emit scanStarted();
    });
    connect(worker, &LazyDurationUpdateWorker::scanProgress, this,
            [this](qint64 processed, qint64 total, qint64 updated, qint64 missing, qint64 failed) {
        if (!workerThread.isInterruptionRequested())
            emit scanProgress(processed, total, updated, missing, failed);
    });
    connect(worker, &LazyDurationUpdateWorker::scanFinished, this, [this](bool cancelled, const QString &error) {
        if (!workerThread.isInterruptionRequested()) {
            m_pendingJobs = qMax(0, m_pendingJobs - 1);
            if (m_pendingJobs == 0 || !error.isEmpty())
                emit scanFinished(cancelled, error);
        }
    });
    workerThread.start();
    workerThread.setPriority(QThread::IdlePriority);
}

LazyDurationUpdateController::~LazyDurationUpdateController() {
    stopWork();
    workerThread.quit();
    workerThread.wait();
}

void LazyDurationUpdateController::stopWork()
{
    workerThread.requestInterruption();
    if (m_pendingJobs > 0) {
        m_pendingJobs = 0;
        emit scanFinished(true, {});
    }
}

void LazyDurationUpdateController::getDurations()
{
    if (workerThread.isInterruptionRequested())
        return;
    if (++m_pendingJobs == 1)
        emit scanStarted();
    emit operate(QSqlDatabase::database().databaseName(), {});
}

void LazyDurationUpdateController::getSongDuration(const QString &path)
{
    if (!path.isEmpty() && !workerThread.isInterruptionRequested()) {
        if (++m_pendingJobs == 1)
            emit scanStarted();
        emit operate(QSqlDatabase::database().databaseName(), {path});
    }
}

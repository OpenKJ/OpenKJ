#ifndef DURATIONLAZYUPDATER_H
#define DURATIONLAZYUPDATER_H

#include <QObject>
#include <QThread>
#include <QSemaphore>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

class LazyDurationUpdateWorker : public QObject
{
    Q_OBJECT
public:
    std::shared_ptr<QSemaphore> batchConsumed = std::make_shared<QSemaphore>();
public slots:
    void getDurations(const QString &dbPath, const QStringList &onlyPaths);
signals:
    void scanStarted();
    void scanProgress(qint64 processed, qint64 total, qint64 updated, qint64 missing, qint64 failed);
    void scanFinished(bool cancelled, const QString &error);
    void durationsReady(const QStringList &paths, const QList<int> &durations);

};

class LazyDurationUpdateController : public QObject
{
    Q_OBJECT
    QThread workerThread;
    std::string m_loggingPrefix{"[LazyDurationController]"};
    std::shared_ptr<spdlog::logger> m_logger;
    int m_pendingJobs{0};

public:
    explicit LazyDurationUpdateController(QObject *parent = nullptr);
    ~LazyDurationUpdateController() override;
    void stopWork();
public slots:
    void getDurations();
    void getSongDuration(const QString &path);
signals:
    void scanStarted();
    void scanProgress(qint64 processed, qint64 total, qint64 updated, qint64 missing, qint64 failed);
    void scanFinished(bool cancelled, const QString &error);
    void operate(const QString &dbPath, const QStringList &onlyPaths);
    void gotDuration(const QString &path, unsigned int duration);
};


#endif // DURATIONLAZYUPDATER_H

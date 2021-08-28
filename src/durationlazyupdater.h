#ifndef DURATIONLAZYUPDATER_H
#define DURATIONLAZYUPDATER_H

#include <QObject>
#include <QThread>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

class LazyDurationUpdateWorker : public QObject
{
    Q_OBJECT
public slots:
    void getDurations(const QStringList &files);
signals:
    void gotDuration(const QString&, unsigned int);

};

class LazyDurationUpdateController : public QObject
{
    Q_OBJECT
    QThread workerThread;
    QStringList files;
    std::string m_loggingPrefix{"[LazyDurationController]"};
    std::shared_ptr<spdlog::logger> m_logger;

public:
    explicit LazyDurationUpdateController(QObject *parent = nullptr);
    ~LazyDurationUpdateController() override;
    void getSongsRequiringUpdate();
    void stopWork();
public slots:
    void updateDbDuration(const QString& file, int duration);
    void getDurations();
signals:
    void operate(const QStringList &list);
    void gotDuration(const QString &path, unsigned int duration);
};


#endif // DURATIONLAZYUPDATER_H

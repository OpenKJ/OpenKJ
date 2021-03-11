#ifndef DURATIONLAZYUPDATER_H
#define DURATIONLAZYUPDATER_H

#include <QObject>
#include <QThread>

class LazyDurationUpdateWorker : public QObject
{
    Q_OBJECT
public slots:
    void getDurations(const QStringList files);
signals:
    void gotDuration(QString, int);

};

class LazyDurationUpdateController : public QObject
{
    Q_OBJECT
    QThread workerThread;
    QStringList files;
public:
    LazyDurationUpdateController(QObject *parent = 0);
    ~LazyDurationUpdateController();
    void getSongsRequiringUpdate();
    void stopWork();
public slots:
    void updateDbDuration(QString file, int duration);
    void getDurations();
signals:
    void operate(const QStringList);
    void gotDuration(QString &path, int duration);
    void gotStopWork();
};


#endif // DURATIONLAZYUPDATER_H

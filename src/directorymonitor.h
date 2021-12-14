#ifndef DIRECTORYMONITOR_H
#define DIRECTORYMONITOR_H

#include <QFileSystemWatcher>
#include <QFutureWatcher>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

class DirectoryMonitor : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryMonitor(QObject *parent, QStringList pathsToWatch);
    ~DirectoryMonitor() override;

private:
    QFutureWatcher<QStringList> m_pathsEnumeratedWatcher;
    QFileSystemWatcher m_fsWatcher;

    QSet<QString> m_pathsWithChangedFiles;
    QTimer m_scanTimer;

    QStringList enumeratePathsAsync(QStringList paths);
    void directoriesEnumerated();
    void directoryChanged(const QString& dirPath);
    void scanPaths();

signals:
    void databaseUpdateComplete();

};

#endif // DIRECTORYMONITOR_H

#include "directorymonitor.h"
#include "dbupdater.h"
#include <QFutureWatcher>
#include <QtConcurrent>

DirectoryMonitor::DirectoryMonitor(QObject *parent, QStringList pathsToWatch) : QObject(parent)
{
    m_scanTimer.setInterval(5000);
    m_scanTimer.setSingleShot(true);
    connect(&m_scanTimer, &QTimer::timeout, this, &DirectoryMonitor::scanPaths);

    connect(&m_pathsEnumeratedWatcher, &QFutureWatcher<int>::finished, this, &DirectoryMonitor::directoriesEnumerated);
    auto future = QtConcurrent::run(this, &DirectoryMonitor::enumeratePathsAsync, pathsToWatch);
    m_pathsEnumeratedWatcher.setFuture(future);
}

DirectoryMonitor::~DirectoryMonitor()
{
    m_fsWatcher.removePaths(m_fsWatcher.directories());
}

QStringList DirectoryMonitor::enumeratePathsAsync(QStringList paths)
{
    QStringList result;
    foreach (auto path, paths) {
        QFileInfo finfo(path);
        if (finfo.isDir() && finfo.isReadable())
        {
            result.append(path);
            QDirIterator it(path, QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                result.append(it.next());
            }
        }
    }
    return result;
}

void DirectoryMonitor::directoriesEnumerated()
{
    auto paths = m_pathsEnumeratedWatcher.future().result();
    m_fsWatcher.addPaths(paths);
    connect(&m_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &DirectoryMonitor::directoryChanged);
}

void DirectoryMonitor::directoryChanged(const QString& dirPath)
{
    qInfo() << "Directory changed fired for dir: " << dirPath;
    m_pathsWithChangedFiles << dirPath;
    m_scanTimer.start();
}

void DirectoryMonitor::scanPaths()
{
    auto paths = m_pathsWithChangedFiles.values();
    m_pathsWithChangedFiles.clear();

    // Scan the folder for changes and add new files to the database.
    // Fix moved files to detect files moved between folders (in that case, both folders will be in m_pathsWithChangedFiles).
    DbUpdater dbUpdater(this);
    if (dbUpdater.process(paths, DbUpdater::ProcessingOption::FixMovedFiles)) {
        emit databaseUpdateComplete();
    }
    else {
        // scanning failed - perhaps another scan was running?
        // Queue a new run
        m_pathsWithChangedFiles.unite(QSet<QString>(paths.begin(), paths.end()));
        m_scanTimer.start();
    }
}


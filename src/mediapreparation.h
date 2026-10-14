#ifndef MEDIAPREPARATION_H
#define MEDIAPREPARATION_H

#include <QTemporaryDir>
#include <QString>
#include <atomic>
#include <memory>

struct PreparedKaraokeMedia {
    std::shared_ptr<QTemporaryDir> directory;
    QString audioFile, cdgFile, error;
    qint64 elapsedMs{0};
};

// Owns all file access and temporary files; safe to call from a worker thread.
PreparedKaraokeMedia prepareKaraokeZip(const QString &filename,
                                     const std::shared_ptr<std::atomic_bool> &cancelled);
PreparedKaraokeMedia prepareKaraokeVideo(const QString &filename,
                                       const std::shared_ptr<std::atomic_bool> &cancelled);

#endif // MEDIAPREPARATION_H

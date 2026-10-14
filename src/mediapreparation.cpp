#include "mediapreparation.h"
#include "mzarchive.h"
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>

PreparedKaraokeMedia prepareKaraokeZip(const QString &filename,
                                  const std::shared_ptr<std::atomic_bool> &cancelled)
{
    PreparedKaraokeMedia result;
    QElapsedTimer timer;
    timer.start();
    if (cancelled->load())
        return result;
    // Construction, archive access, and extraction all stay on this thread.
    MzArchive archive(filename);
    if (!archive.checkCDG() || !archive.checkAudio()) {
        result.error = QStringLiteral("Zip file does not contain a valid karaoke track.  CDG or audio file missing or corrupt.");
    } else if (!cancelled->load()) {
        result.directory = std::make_shared<QTemporaryDir>();
        if (!result.directory->isValid()) {
            result.error = QStringLiteral("Failed to create temporary directory.");
        } else {
            const auto audioName = "tmp" + archive.audioExtension();
            if (!archive.extractAudio(result.directory->path(), audioName, cancelled.get()))
                result.error = QStringLiteral("Failed to extract audio file.");
            else if (!cancelled->load() && !archive.extractCdg(result.directory->path(), "tmp.cdg", cancelled.get()))
                result.error = QStringLiteral("Failed to extract CDG file.");
            if (!cancelled->load() && result.error.isEmpty()) {
                result.audioFile = result.directory->filePath(audioName);
                result.cdgFile = result.directory->filePath("tmp.cdg");
            }
        }
    }
    if (cancelled->load() || !result.error.isEmpty()) {
        result.directory.reset();
        result.audioFile.clear();
        result.cdgFile.clear();
    }
    result.elapsedMs = timer.elapsed();
    return result;
}

PreparedKaraokeMedia prepareKaraokeVideo(const QString &filename,
                                       const std::shared_ptr<std::atomic_bool> &cancelled)
{
    PreparedKaraokeMedia result;
    QElapsedTimer timer;
    timer.start();
    if (cancelled->load())
        return result;
    QFile source(filename);
    if (!source.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("Unable to open video file: ") + source.errorString();
    } else {
        result.directory = std::make_shared<QTemporaryDir>();
        if (!result.directory->isValid()) {
            result.error = QStringLiteral("Failed to create temporary directory.");
        } else {
            // Keep a simple local filename and its real extension for GStreamer.
            const auto suffix = QFileInfo(filename).suffix();
            const auto target = result.directory->filePath("song" + (suffix.isEmpty() ? QString() : "." + suffix));
            QFile destination(target);
            if (!destination.open(QIODevice::WriteOnly)) {
                result.error = QStringLiteral("Unable to create temporary video file: ") + destination.errorString();
            } else {
                const auto expectedSize = source.size();
                qint64 copied = 0;
                QByteArray buffer(1024 * 1024, Qt::Uninitialized);
                // Check cancellation between reads rather than copying an entire
                // large video before responding to Stop or a newer selection.
                while (!cancelled->load()) {
                    const auto count = source.read(buffer.data(), buffer.size());
                    if (count < 0) {
                        result.error = QStringLiteral("Unable to read video file: ") + source.errorString();
                        break;
                    }
                    if (count == 0)
                        break;
                    if (destination.write(buffer.constData(), count) != count) {
                        result.error = QStringLiteral("Unable to copy video file: ") + destination.errorString();
                        break;
                    }
                    copied += count;
                }
                if (!cancelled->load() && result.error.isEmpty()) {
                    if (copied == 0 || copied != expectedSize)
                        result.error = QStringLiteral("Video file is empty or changed during copying.");
                    else if (!destination.flush())
                        result.error = QStringLiteral("Unable to finish copying video file: ") + destination.errorString();
                    else
                        result.audioFile = target;
                }
            }
        }
    }
    if (cancelled->load() || !result.error.isEmpty()) {
        result.directory.reset();
        result.audioFile.clear();
    }
    result.elapsedMs = timer.elapsed();
    return result;
}

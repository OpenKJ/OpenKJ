#ifndef MZARCHIVE_H
#define MZARCHIVE_H

#include <QObject>
#include <QStringList>
#include <atomic>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

class MzArchive : public QObject
{
    Q_OBJECT
public:
    explicit MzArchive(const QString &ArchiveFile, QObject *parent = nullptr);
    explicit MzArchive(QObject *parent = nullptr);
    // Batch scanners can report failures themselves without per-file warnings.
    int getSongDuration(bool logErrors = true);
    void setArchiveFile(const QString &value);
    bool checkCDG();
    bool checkAudio();
    QString audioExtension();
    bool extractAudio(const QString& destPath, const QString& destFile, const std::atomic_bool *cancelled = nullptr);
    bool extractCdg(const QString& destPath, const QString& destFile, const std::atomic_bool *cancelled = nullptr);
    bool isValidKaraokeFile();
    QString getLastError();

private:
    QString archiveFile;
    QString audioExt;
    QString lastError;
    bool findCDG(bool logErrors = true);
    bool findAudio();
    unsigned int m_audioFileIndex{0};
    unsigned int m_cdgFileIndex{0};
    int m_cdgSize{0};
    unsigned int m_audioSize{0};
    bool m_audioSupportedCompression{true};
    bool m_cdgSupportedCompression{true};
    bool m_cdgFound{false};
    bool m_audioFound{false};
    bool findEntries(bool logErrors = true);
    QStringList audioExtensions;
    std::string m_loggingPrefix{"[MZArchive]"};
    std::shared_ptr<spdlog::logger> m_logger;
};

#endif // MZARCHIVE_H

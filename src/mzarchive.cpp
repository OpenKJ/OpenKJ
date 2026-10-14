#include "mzarchive.h"
#include <QFile>
#include <QDir>
#include <zip.h>

MzArchive::MzArchive(const QString &ArchiveFile, QObject *parent) : QObject(parent)
{
    archiveFile = ArchiveFile;
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
    m_logger = spdlog::get("logger");
}

MzArchive::MzArchive(QObject *parent) : QObject(parent)
{
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
    m_logger = spdlog::get("logger");
}

int MzArchive::getSongDuration(bool logErrors)
{
    if (!findCDG(logErrors)) {
        if (lastError.isEmpty())
            lastError = "CDG not found in zip file";
        return 0;
    }
    const int duration = ((m_cdgSize / 96) / 75) * 1000;
    if (duration == 0)
        lastError = "CDG entry is empty or too short to determine duration";
    return duration;
}

void MzArchive::setArchiveFile(const QString &value)
{
    archiveFile = value;
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    lastError = "";
}

bool MzArchive::checkCDG()
{
    if (!findCDG())
        return false;
    if (m_cdgSize <= 0)
        return false;
    return true;
}

bool MzArchive::checkAudio()
{
    if (!findAudio())
        return false;
    if (m_audioSize <= 0)
        return false;
    return true;
}

QString MzArchive::audioExtension()
{
    return audioExt;
}

bool MzArchive::extractAudio(const QString& destPath, const QString& destFile, const std::atomic_bool *cancelled)
{
    m_logger->info("{} Extracting {} audio file to: {}/{}", m_loggingPrefix, archiveFile, destPath, destFile);
    if (!findAudio())
        return false;

    int err = 0;
    zip_t *z = zip_open(archiveFile.toLocal8Bit().constData(), ZIP_RDONLY, &err);
    if (!z)
    {
        m_logger->warn("{} Failed to open zip archive: {}", m_loggingPrefix, archiveFile);
        return false;
    }

    zip_file_t *zf = zip_fopen_index(z, m_audioFileIndex, 0);
    if (!zf)
    {
        m_logger->warn("{} Failed to open audio entry inside zip", m_loggingPrefix);
        zip_close(z);
        return false;
    }

    QFile dest(destPath + QDir::separator() + destFile);
    if (!dest.open(QIODevice::WriteOnly))
    {
        m_logger->warn("{} Failed to open destination file for audio extraction", m_loggingPrefix);
        zip_fclose(zf);
        zip_close(z);
        return false;
    }

    char buf[8192];
    zip_int64_t n = -1;
    while ((!cancelled || !cancelled->load()) && (n = zip_fread(zf, buf, sizeof(buf))) > 0)
    {
        if (dest.write(buf, n) != n) {
            n = -1;
            break;
        }
    }
    const bool extracted = n == 0 && dest.flush() && (!cancelled || !cancelled->load());
    dest.close();
    zip_fclose(zf);
    zip_close(z);
    return extracted;
}

bool MzArchive::extractCdg(const QString& destPath, const QString& destFile, const std::atomic_bool *cancelled)
{
    m_logger->info("{} Extracting {} cdg file to: {}/{}", m_loggingPrefix, archiveFile, destPath, destFile);
    if (!findCDG())
        return false;

    int err = 0;
    zip_t *z = zip_open(archiveFile.toLocal8Bit().constData(), ZIP_RDONLY, &err);
    if (!z)
    {
        m_logger->warn("{} Failed to open zip archive: {}", m_loggingPrefix, archiveFile);
        return false;
    }

    zip_file_t *zf = zip_fopen_index(z, m_cdgFileIndex, 0);
    if (!zf)
    {
        m_logger->warn("{} Failed to open cdg entry inside zip", m_loggingPrefix);
        zip_close(z);
        return false;
    }

    QFile dest(destPath + QDir::separator() + destFile);
    if (!dest.open(QIODevice::WriteOnly))
    {
        m_logger->warn("{} Failed to open destination file for cdg extraction", m_loggingPrefix);
        zip_fclose(zf);
        zip_close(z);
        return false;
    }

    char buf[8192];
    zip_int64_t n = -1;
    while ((!cancelled || !cancelled->load()) && (n = zip_fread(zf, buf, sizeof(buf))) > 0)
    {
        if (dest.write(buf, n) != n) {
            n = -1;
            break;
        }
    }
    const bool extracted = n == 0 && dest.flush() && (!cancelled || !cancelled->load());
    dest.close();
    zip_fclose(zf);
    zip_close(z);
    return extracted;
}

bool MzArchive::isValidKaraokeFile()
{
    if (!findEntries())
    {
        if (!m_cdgFound)
        {
            m_logger->warn("{} Missing cdg file! - {}", m_loggingPrefix, archiveFile);
            lastError = "CDG not found in zip file";
        }
        if (!m_audioFound)
        {
            m_logger->warn("{} Missing audio file! - {}", m_loggingPrefix, archiveFile);
            lastError = "Audio file not found in zip file";
        }
        return false;
    }
    if (m_audioSize <= 0)
    {
        m_logger->warn("{} Zero byte audio file! - {}", m_loggingPrefix, archiveFile);
        lastError = "Zero byte audio file";
        return false;
    }
    if (m_cdgSize <= 0)
    {
        m_logger->warn("{} Zero byte cdg file! - {}", m_loggingPrefix, archiveFile);
        lastError = "Zero byte CDG file";
        return false;
    }
    return true;
}

QString MzArchive::getLastError()
{
    return lastError;
}

bool MzArchive::findCDG(bool logErrors)
{
    if (m_cdgFound)
        return true;
    findEntries(logErrors);
    return m_cdgFound;
}

bool MzArchive::findAudio()
{
    findEntries();
    return m_audioFound;
}

bool MzArchive::findEntries(bool logErrors)
{
    if (m_audioFound && m_cdgFound)
        return true;

    int err = 0;
    zip_t *z = zip_open(archiveFile.toLocal8Bit().constData(), ZIP_RDONLY, &err);
    if (!z)
    {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, err);
        lastError = QString::fromUtf8(zip_error_strerror(&zipError));
        zip_error_fini(&zipError);
        if (logErrors)
            m_logger->warn("{} Error opening zip file: {} ({})",
                           m_loggingPrefix, archiveFile, lastError);
        return false;
    }

    zip_int64_t num_files = zip_get_num_entries(z, 0);
    for (zip_int64_t i = 0; i < num_files; i++)
    {
        zip_stat_t st;
        zip_stat_init(&st);
        if (zip_stat_index(z, i, 0, &st) == 0)
        {
            QString fileName = QString::fromUtf8(st.name);
            if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
            {
                m_cdgFileIndex = i;
                m_cdgSize = st.size;
                m_cdgFound = true;
            }
            else
            {
                for (int e = 0; e < audioExtensions.size(); e++)
                {
                    if (fileName.endsWith(audioExtensions.at(e), Qt::CaseInsensitive))
                    {
                        m_audioFileIndex = i;
                        audioExt = audioExtensions.at(e);
                        m_audioSize = st.size;
                        m_audioFound = true;
                    }
                }
            }
        }
    }
    zip_close(z);
    return m_audioFound && m_cdgFound;
}

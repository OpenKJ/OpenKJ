/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "mzarchive.h"
#include <QDebug>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#include "src/miniz/miniz.h"
#ifdef Q_OS_WIN
#include <io.h>
#endif

MzArchive::MzArchive(const QString &ArchiveFile, QObject *parent) : QObject(parent)
{
    archiveFile = ArchiveFile;
    oka.setArchiveFile(archiveFile);
    //qWarning() << "MzArchive opening file: " << archiveFile;
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
}

MzArchive::MzArchive(QObject *parent) : QObject(parent)
{
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
}

int MzArchive::getSongDuration()
{
    if (findCDG())
        return ((m_cdgSize / 96) / 75) * 1000;
    else
        return 0;
}

void MzArchive::setArchiveFile(const QString &value)
{
    //qWarning() << "MzArchive opening archive file: " << value;
    archiveFile = value;
    oka.setArchiveFile(value);
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    lastError = "";
    m_audioSupportedCompression = false;
    m_cdgSupportedCompression = false;
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

bool MzArchive::extractAudio(const QString& destPath, const QString& destFile)
{
    qInfo() << "Extracting: " << archiveFile << " audio file to: " << destPath << "/" << destFile;
    if (findAudio())
    {
        if (!m_audioSupportedCompression || !m_cdgSupportedCompression)
        {
            qWarning() << archiveFile << " - Archive using non-standard compression method, falling back to infozip based zip handling";
            return oka.extractAudio(destPath, destFile);
        }
        mz_zip_archive archive;
        memset(&archive, 0, sizeof(archive));
        //mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(), 0);
        QFile zipFile(archiveFile);
        zipFile.open(QIODevice::ReadOnly);
        QByteArray zipData = zipFile.readAll();
        zipFile.close();
        mz_zip_reader_init_mem(&archive, zipData.data(), zipData.size(), 0);
        if (mz_zip_reader_extract_to_file(&archive, m_audioFileIndex, QString(destPath + QDir::separator() + destFile).toLocal8Bit(),0))
        {
            mz_zip_reader_end(&archive);
            return true;
        }
        else
        {
            qWarning() << "Failed to extract mp3 file";
            QString err(mz_zip_get_error_string(mz_zip_get_last_error(&archive)));
            qWarning() << "unzip error: " << err;
            mz_zip_reader_end(&archive);
            qWarning() << "Attempting to fall back to external infozip method";
            return oka.extractAudio(destPath, destFile);
        }
    }
    return false;
}

bool MzArchive::extractCdg(const QString& destPath, const QString& destFile)
{
    if (findCDG())
    {
        if (!m_audioSupportedCompression || !m_cdgSupportedCompression)
        {
            qWarning() << archiveFile << " - Archive using non-standard compression method, falling back to infozip based zip handling";
            return oka.extractCdg(destPath, destFile);
        }
        mz_zip_archive archive;
        memset(&archive, 0, sizeof(archive));
        QFile zipFile(archiveFile);
        zipFile.open(QIODevice::ReadOnly);
        QByteArray zipData = zipFile.readAll();
        zipFile.close();
        mz_zip_reader_init_mem(&archive, zipData.data(), zipData.size(), 0);
        if (mz_zip_reader_extract_to_file(&archive, m_cdgFileIndex, QString(destPath + QDir::separator() + destFile).toLocal8Bit(),0))
        {
            mz_zip_reader_end(&archive);
            return true;
        }
        else
        {
            qCritical() << "Failed to extract cdg file";
            QString err(mz_zip_get_error_string(mz_zip_get_last_error(&archive)));
            qWarning() << "unzip error: " << err;
            mz_zip_reader_end(&archive);
            return false;
        }
    }
    return false;
}

bool MzArchive::isValidKaraokeFile()
{
    if (!findEntries())
    {
        if (!m_audioSupportedCompression || !m_cdgSupportedCompression)
        {
            qWarning() << archiveFile << " - Archive using non-standard compression method, falling back to infozip based zip handling";
            return oka.isValidKaraokeFile();
        }
        if (!m_cdgFound)
        {
            qWarning() << archiveFile << " - Missing CDG file";
            lastError = "CDG not found in zip file";
        }
        if (!m_audioFound)
        {
            qWarning() << archiveFile << " - Missing audio file";
            lastError = "Audio file not found in zip file";
        }
        return false;
    }
    if (m_audioSize <= 0)
    {
        qWarning() << archiveFile << " - Zero byte audio file";
        lastError = "Zero byte audio file";
        return false;
    }
    if (m_cdgSize <= 0)
    {
        qWarning() << archiveFile << " - Zero byte CDG file";
        lastError = "Zero byte CDG file";
        return false;
    }
    return true;
}

QString MzArchive::getLastError()
{
    return lastError;
}

bool MzArchive::findCDG()
{
    if (m_cdgFound)
        return true;
    findEntries();
    return m_cdgFound;
}

bool MzArchive::findAudio()
{
    findEntries();
    return m_audioFound;
}

bool MzArchive::findEntries()
{
    if (m_audioFound && m_cdgFound && m_audioSupportedCompression && m_cdgSupportedCompression)
        return true;
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    mz_zip_archive_file_stat fStat;

    QFile zipFile(archiveFile);
    if (!zipFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "Error opening zip file";
        return false;
    }
    QByteArray zipData = zipFile.readAll();
    zipFile.close();
    if (!mz_zip_reader_init_mem(&archive, zipData.data(), zipData.size(), 0))
    {
        qWarning() << "Error opening zip file";
        return false;
    }
    unsigned int files = mz_zip_reader_get_num_files(&archive);
    for (unsigned int i=0; i < files; i++)
    {
        if (mz_zip_reader_file_stat(&archive, i, &fStat))
        {
            QString fileName = fStat.m_filename;
            if (fileName.endsWith(".cdg",Qt::CaseInsensitive))
            {
                m_cdgFileIndex = fStat.m_file_index;
                m_cdgSize = fStat.m_uncomp_size;
                m_cdgSupportedCompression = fStat.m_is_supported;
                m_cdgFound = true;
            }
            else
            {
                for (int e=0; e < audioExtensions.size(); e++)
                {
                    if (fileName.endsWith(audioExtensions.at(e), Qt::CaseInsensitive))
                    {
                        m_audioFileIndex = fStat.m_file_index;
                        audioExt = audioExtensions.at(e);
                        m_audioSize = fStat.m_uncomp_size;
                        m_audioSupportedCompression = fStat.m_is_supported;
                        m_audioFound = true;
                    }
                }
            }
            if (m_audioFound && m_cdgFound && m_cdgSupportedCompression && m_audioSupportedCompression)
            {
                mz_zip_reader_end(&archive);
                return true;
            }
            else if (m_audioFound && m_cdgFound && (!m_cdgSupportedCompression || !m_audioSupportedCompression))
                return oka.isValidKaraokeFile();
        }
    }
    mz_zip_reader_end(&archive);
    return false;
}



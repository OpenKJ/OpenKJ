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


#include "okarchive.h"
#include <QDebug>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#include "miniz.h"
#ifdef Q_OS_WIN
#include <io.h>
#else
#include <unistd.h>
#endif

OkArchive::OkArchive(QString ArchiveFile, QObject *parent) : QObject(parent)
{
    archiveFile = ArchiveFile;
    qWarning() << "OkArchive opening file: " << archiveFile;
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    m_audioSupportedCompression = false;
    m_cdgSupportedCompression = false;
    lastError = "";
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
}

OkArchive::OkArchive(QObject *parent) : QObject(parent)
{
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    m_audioSupportedCompression = false;
    m_cdgSupportedCompression = false;
    lastError = "";
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
}

OkArchive::~OkArchive()
{

}

unsigned int OkArchive::getSongDuration()
{
    if ((m_cdgFound) && (m_cdgSize > 0))
        return ((m_cdgSize / 96) / 75) * 1000;
    else if (findCDG())
        return ((m_cdgSize / 96) / 75) * 1000;
    else
        return 0;
}

QByteArray OkArchive::getCDGData()
{
    QByteArray data;
    if (findCDG())
    {
        mz_zip_archive archive;
        memset(&archive, 0, sizeof(archive));
        if (!mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(), 0))
        {
             QString err(mz_zip_get_error_string(mz_zip_get_last_error(&archive)));
             qWarning() << "unzip error: " << err;
             return data;
        }

        QTemporaryDir dir;
        QString cdgTmpFile = dir.path() + QDir::separator() + "tmp.cdg";
        qWarning() << "Extracting CDG file to tmp path: " << cdgTmpFile;
        if (mz_zip_reader_extract_file_to_file(&archive, cdgFileName.toLocal8Bit(), cdgTmpFile.toLocal8Bit(),0))
        {
            QFile cdg(cdgTmpFile);
            cdg.open(QFile::ReadOnly);
            data = cdg.readAll();
            mz_zip_reader_end(&archive);
            return data;
        }
        else
        {
            QString err(mz_zip_get_error_string(mz_zip_get_last_error(&archive)));
            qWarning() << "unzip error: " << err;
        }
        mz_zip_reader_end(&archive);
        return data;
    }
    return data;
}

QString OkArchive::getArchiveFile() const
{
    return archiveFile;
}

void OkArchive::setArchiveFile(const QString &value)
{
    qWarning() << "OkArchive opening archive file: " << value;
    archiveFile = value;
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    lastError = "";
    m_audioSupportedCompression = false;
    m_cdgSupportedCompression = false;
}

bool OkArchive::checkCDG()
{
    if (!findCDG())
        return false;
    if (m_cdgSize <= 0)
        return false;
    return true;
}

bool OkArchive::checkAudio()
{
    if (!findAudio())
        return false;
    if (m_audioSize <= 0)
        return false;
    return true;
}

QString OkArchive::audioExtension()
{
    return audioExt;
}

bool OkArchive::extractAudio(QString destPath)
{
    if (findAudio())
    {
        mz_zip_archive archive;
        memset(&archive, 0, sizeof(archive));
        mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(), 0);
        if (mz_zip_reader_extract_file_to_file(&archive, audioFileName.toLocal8Bit(), destPath.toLocal8Bit(),0))
        {
            mz_zip_reader_end(&archive);
            return true;
        }
        else
        {
            qCritical() << "Failed to extract mp3 file";
            QString err(mz_zip_get_error_string(mz_zip_get_last_error(&archive)));
            qWarning() << "unzip error: " << err;
            mz_zip_reader_end(&archive);
            return false;
        }
    }
    return false;
}

bool OkArchive::isValidKaraokeFile()
{
    if (!findEntries())
    {
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
        if (!m_audioSupportedCompression)
        {
            qWarning() << archiveFile << " - Unsupported compression method on audio file";
            lastError = "Unsupported compression method";
        }
        if (!m_cdgSupportedCompression)
        {
            qWarning() << archiveFile << " - Unsupported compression method on CDG file";
            lastError = "Unsupported compression method";
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

QString OkArchive::getLastError()
{
    return lastError;
}

bool OkArchive::findCDG()
{
    findEntries();
    if (m_cdgFound && m_cdgSupportedCompression)
        return true;
    return false;
}

bool OkArchive::findAudio()
{
    findEntries();
    if (m_audioFound && m_audioSupportedCompression)
        return true;
    return false;
}

bool OkArchive::findEntries()
{
    if (m_audioFound && m_cdgFound && m_audioSupportedCompression && m_cdgSupportedCompression)
        return true;
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    mz_zip_archive_file_stat fStat;
    if (!mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(), 0))
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
                cdgFileName = fileName;
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
                        audioFileName = fileName;
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
        }
    }
    mz_zip_reader_end(&archive);
    return false;
}



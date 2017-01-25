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
#include "miniz.c"


OkArchive::OkArchive(QString ArchiveFile, QObject *parent) : QObject(parent)
{
    archiveFile = ArchiveFile;
    m_cdgFound = false;
    m_mp3Found = false;
    m_cdgSize = 0;
    m_mp3Size = 0;
}

OkArchive::OkArchive(QObject *parent) : QObject(parent)
{
    m_cdgFound = false;
    m_mp3Found = false;
    m_cdgSize = 0;
    m_mp3Size = 0;
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
        mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(),0);
        QTemporaryDir dir;
        QString cdgTmpFile = dir.path() + QDir::separator() + "tmp.cdg";
        if (mz_zip_reader_extract_file_to_file(&archive, cdgFileName.toLocal8Bit(), cdgTmpFile.toLocal8Bit(),0))
        {
            QFile cdg(cdgTmpFile);
            cdg.open(QFile::ReadOnly);
            data = cdg.readAll();
            mz_zip_reader_end(&archive);
            return data;
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
    archiveFile = value;
    m_cdgFound = false;
    m_mp3Found = false;
    m_cdgSize = 0;
    m_mp3Size = 0;
}

bool OkArchive::checkCDG()
{
    return findCDG();
}

bool OkArchive::checkMP3()
{
    return findMp3();
}

bool OkArchive::extractMP3(QString destPath)
{
    if (findMp3())
    {
        mz_zip_archive archive;
        memset(&archive, 0, sizeof(archive));
        mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(),0);
        if (mz_zip_reader_extract_file_to_file(&archive, mp3FileName.toLocal8Bit(), destPath.toLocal8Bit(),0))
        {
            mz_zip_reader_end(&archive);
            return true;
        }
        mz_zip_reader_end(&archive);
    }
    return false;
}

bool OkArchive::findCDG()
{
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    mz_zip_archive_file_stat fStat;
    mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(),0);
    unsigned int files = mz_zip_reader_get_num_files(&archive);
    for (unsigned int i=0; i < files; i++)
    {
        mz_zip_reader_file_stat(&archive, i, &fStat);
        QString fileName = fStat.m_filename;
        if (fileName.endsWith(".cdg",Qt::CaseInsensitive))
        {
            cdgFileName = fileName;
            m_cdgSize = fStat.m_uncomp_size;
            m_cdgFound = true;
            mz_zip_reader_end(&archive);
            return true;
        }
    }
    mz_zip_reader_end(&archive);
    return false;
}

bool OkArchive::findMp3()
{
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    mz_zip_archive_file_stat fStat;
    mz_zip_reader_init_file(&archive, archiveFile.toLocal8Bit(),0);
    unsigned int files = mz_zip_reader_get_num_files(&archive);
    for (unsigned int i=0; i < files; i++)
    {
        mz_zip_reader_file_stat(&archive, i, &fStat);
        QString fileName = fStat.m_filename;
        if (fileName.endsWith(".mp3",Qt::CaseInsensitive))
        {
            mp3FileName = fileName;
            m_mp3Size = fStat.m_uncomp_size;
            m_mp3Found = true;
            mz_zip_reader_end(&archive);
            return true;
        }
    }
    mz_zip_reader_end(&archive);
    return false;
}



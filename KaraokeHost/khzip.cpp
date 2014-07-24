/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#include "khzip.h"
#include <QDebug>
#include <QTemporaryDir>
#include <fstream>
#include <QFile>
#if defined(__GNUC__)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif
#endif
#include "../miniz/miniz.h"

KhZip::KhZip(QString zipFile, QObject *parent) :
    QObject(parent)
{
    m_zipFile = zipFile;
}

KhZip::KhZip(QObject *parent) :
    QObject(parent)
{
    m_zipFile = "";
}

bool KhZip::extractMp3(QDir destDir)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, m_zipFile.toStdString().c_str(), 0))
    {
        qDebug() << "mz_zip_reader_init_file() failed!";
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            qDebug() << "mz_zip_reader_file_stat() failed!";
            mz_zip_reader_end(&zip_archive);
            return false;
        }
        QString filename = file_stat.m_filename;
        if (filename.endsWith(".mp3", Qt::CaseInsensitive))
        {
            QString outFile(destDir.path() + QDir::separator() + "tmp.mp3");
            if (mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename,outFile.toLocal8Bit().data(),0))
            {
                mz_zip_reader_end(&zip_archive);
                return true;
            }
            else
            {
                qDebug() << "Error extracting file";
            }
        }

    }
    mz_zip_reader_end(&zip_archive);
    return false;
}

bool KhZip::extractCdg(QDir destDir)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, m_zipFile.toLocal8Bit().data(), 0))
    {
        qDebug() << "mz_zip_reader_init_file() failed!";
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            qDebug() << "mz_zip_reader_file_stat() failed!";
            mz_zip_reader_end(&zip_archive);
            return false;
        }
        QString filename = file_stat.m_filename;
        if (filename.endsWith(".cdg", Qt::CaseInsensitive))
        {
            QString outFile(destDir.path() + QDir::separator() + "tmp.cdg");
            if (mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename,outFile.toLocal8Bit().data(),0))
            {
                mz_zip_reader_end(&zip_archive);
                return true;
            }
            else
            {
                qDebug() << "Error extracting file";
            }
        }

    }
    mz_zip_reader_end(&zip_archive);
    return false;
}

QString KhZip::zipFile() const
{
    return m_zipFile;
}

void KhZip::setZipFile(const QString &zipFile)
{
    m_zipFile = zipFile;
}

int KhZip::getSongDuration()
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, m_zipFile.toLocal8Bit().data(), 0))
    {
        qDebug() << "mz_zip_reader_init_file() failed!";
        mz_zip_reader_end(&zip_archive);
        return 0;
    }

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            qDebug() << "mz_zip_reader_file_stat() failed!";
            mz_zip_reader_end(&zip_archive);
            return 0;
        }
        QString filename = file_stat.m_filename;
        if (filename.endsWith(".cdg", Qt::CaseInsensitive))
        {
            mz_zip_reader_end(&zip_archive);
            return ((file_stat.m_uncomp_size / 96) / 75) * 1000;
        }

    }
    mz_zip_reader_end(&zip_archive);
    return 0;
}

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
    long fileSize = 0;
    unzFile hFile = unzOpen64(m_zipFile.toUtf8().data());
    if (!hFile)
    {
        qDebug() << "Error opening zip file";
        return 0;
    }
    if (unzGoToFirstFile(hFile) != UNZ_OK)
    {
        qDebug() << "unzGoToFirstFile() failed";
        return 0;
    }
    bool cdgFound = false;
    while (!cdgFound)
    {
        char *fname = new char[256];
//        unz_file_info fileInfo;
        unzGetCurrentFileInfo(hFile,fileInfo,fname,256,NULL,0,NULL,0);
        QString filename = fname;
        delete [] fname;
        if (filename.endsWith(".cdg", Qt::CaseInsensitive))
        {
            fileSize = fileInfo->uncompressed_size;
            unzCloseCurrentFile(hFile);
            unzClose(hFile);
            cdgFound = true;
        }
        else
        {
            if (unzGoToNextFile(hFile) != UNZ_OK)
            {
                return 0;
            }
        }
    }
    // ((bytes / CD subchannel sector size) / Standard CD sectors per second) * 1000 = ms duration
    return ((fileSize / 96) / 75) * 1000;
}

KhZip::KhZip(QString zipFile, QObject *parent) :
    QObject(parent)
{
    m_zipFile = zipFile;
    fileInfo = new unz_file_info;
}

KhZip::KhZip(QObject *parent) :
    QObject(parent)
{
    fileInfo = new unz_file_info;
}

bool KhZip::extractMp3(QDir destDir)
{
    qDebug() << "Extracting to " << destDir.path();
    unzFile hFile = unzOpen64(m_zipFile.toUtf8().data());
    if (!hFile)
    {
        qDebug() << "Error opening zip file";
        return false;
    }
    if (unzGoToFirstFile(hFile) != UNZ_OK)
    {
        qDebug() << "unzGoToFirstFile() failed";
        return false;
    }
    bool mp3Found = false;
    while (!mp3Found)
    {
        char *fname = new char[256];
        unzGetCurrentFileInfo(hFile,NULL,fname,256,NULL,0,NULL,0);
        qDebug() << "Filename: " << fname;
        QString filename = fname;
        delete [] fname;
        if (filename.endsWith(".mp3", Qt::CaseInsensitive))
        {
            int unzResult = unzOpenCurrentFile(hFile);
            if (unzResult != UNZ_OK)
            {
                qDebug() << "unzOpenCurrentFile() failed " << unzResult;
                return false;
            }
            const int SizeBuffer = 32768;
            unsigned char* Buffer = new unsigned char[SizeBuffer];
            ::memset(Buffer, 0, SizeBuffer);
            int bytesRead = 0;
            std::ofstream outFile(QString(destDir.path() + QDir::separator() + "tmp.mp3").toUtf8().data());
            while ((bytesRead = unzReadCurrentFile(hFile, Buffer, SizeBuffer)) > 0)
            {
                outFile.write((const char*)Buffer, bytesRead);
            }

            if (Buffer)
            {
                delete [] Buffer;
                Buffer = NULL;
            }
            outFile.close();
            unzCloseCurrentFile(hFile);
            unzClose(hFile);
            mp3Found = true;
        }
        else
        {
            if (unzGoToNextFile(hFile) != UNZ_OK)
            {
                return false;
            }
        }

    }
    return mp3Found;
}

bool KhZip::extractCdg(QDir destDir)
{
    unzFile hFile = unzOpen64(m_zipFile.toUtf8().data());
    if (!hFile)
    {
        qDebug() << "Error opening zip file";
        return false;
    }
    if (unzGoToFirstFile(hFile) != UNZ_OK)
    {
        qDebug() << "unzGoToFirstFile() failed";
        return false;
    }
    bool cdgFound = false;
    while (!cdgFound)
    {
        char *fname = new char[256];
        unzGetCurrentFileInfo(hFile,NULL,fname,256,NULL,0,NULL,0);
        qDebug() << "Filename: " << fname;
        QString filename = fname;
        delete [] fname;
        if (filename.endsWith(".cdg", Qt::CaseInsensitive))
        {
            int unzResult = unzOpenCurrentFile(hFile);
            if (unzResult != UNZ_OK)
            {
                qDebug() << "unzOpenCurrentFile() failed " << unzResult;
                return false;
            }
            const int SizeBuffer = 32768;
            unsigned char* Buffer = new unsigned char[SizeBuffer];
            ::memset(Buffer, 0, SizeBuffer);
            int bytesRead = 0;
            std::ofstream outFile(QString(destDir.path() + QDir::separator() + "tmp.cdg").toUtf8().data());
            while ((bytesRead = unzReadCurrentFile(hFile, Buffer, SizeBuffer)) > 0)
            {
                outFile.write((const char*)Buffer, bytesRead);
            }

            if (Buffer)
            {
                delete [] Buffer;
                Buffer = NULL;
            }
            outFile.close();
            unzCloseCurrentFile(hFile);
            unzClose(hFile);
            cdgFound = true;
        }
        else
        {
            if (unzGoToNextFile(hFile) != UNZ_OK)
            {
                return false;
            }
        }
    }
    return cdgFound;
}

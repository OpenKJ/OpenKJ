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
#include <quazipfile.h>


OkArchive::OkArchive(QString ArchiveFile, QObject *parent) : QObject(parent)
{
    archiveFile = ArchiveFile;
}

OkArchive::OkArchive(QObject *parent) : QObject(parent)
{

}

OkArchive::~OkArchive()
{

}

int OkArchive::getSongDuration()
{
    QuaZip zipFile(archiveFile);
    zipFile.open(QuaZip::mdUnzip);
    if (!zipFile.isOpen()) return 0;
    QList<QuaZipFileInfo> infos = zipFile.getFileInfoList();
    for (int i=0; i < infos.count(); i++)
    {
        if (infos.at(i).name.endsWith(".cdg"))
        {
            return ((infos.at(i).uncompressedSize / 96) / 75) * 1000;
        }
    }
    zipFile.close();
    return 0;
}

QByteArray OkArchive::getCDGData()
{
    QByteArray data;
    if (findCDG())
    {
        QuaZip zipFile(archiveFile);
        zipFile.open(QuaZip::mdUnzip);
        QuaZipFile file(&zipFile);
        zipFile.setCurrentFile(cdgFileName);
        file.open(QIODevice::ReadOnly);
        data = file.readAll();
        file.close();
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
    QByteArray data;
    if (findMp3())
    {
        QuaZip zipFile(archiveFile);
        zipFile.open(QuaZip::mdUnzip);
        QuaZipFile file(&zipFile);
        zipFile.setCurrentFile(mp3FileName);
        file.open(QIODevice::ReadOnly);
        data = file.readAll();
        file.close();

        QFile destFile(destPath);
        if (!destFile.open(QFile::WriteOnly | QFile::Truncate))
        {
            qCritical() << "Unable to create destination file while extracting mp3: " << destPath;
            return false;
        }
        destFile.write(data);
        destFile.close();
        return true;//        QFile destFile(destPath);
        if (!destFile.open(QFile::WriteOnly | QFile::Truncate))
        {
            qCritical() << "Unable to create destination file while extracting mp3: " << destPath;
            return false;
        }
        destFile.write(data);
        destFile.close();
        return true;
    }
    return false;
}

bool OkArchive::findCDG()
{
    QuaZip zipFile(archiveFile);
    zipFile.open(QuaZip::mdUnzip);
    if (!zipFile.isOpen()) return false;
    QStringList filenames = zipFile.getFileNameList();
    for (int i=0; i < filenames.count(); i++)
    {
        if (filenames.at(i).endsWith(".cdg"))
        {
            cdgFileName = filenames.at(i);
            return true;
        }
    }
    zipFile.close();
    return false;
}

bool OkArchive::findMp3()
{
    QuaZip zipFile(archiveFile);
    zipFile.open(QuaZip::mdUnzip);
    if (!zipFile.isOpen()) return false;
    QStringList filenames = zipFile.getFileNameList();
    for (int i=0; i < filenames.count(); i++)
    {
        if (filenames.at(i).endsWith(".mp3"))
        {
            mp3FileName = filenames.at(i);
            return true;
        }
    }
    zipFile.close();
    return false;
}



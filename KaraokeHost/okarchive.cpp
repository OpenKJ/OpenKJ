/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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


OkArchive::OkArchive(QString ArchiveFile, QObject *parent) : QObject(parent)
{
    archiveFile = ArchiveFile;
    zipFile = new KZip(archiveFile);
    zipFile->open(QIODevice::ReadOnly);
    mp3Located = false;
    cdgLocated = false;
}

OkArchive::OkArchive(QObject *parent) : QObject(parent)
{

}

OkArchive::~OkArchive()
{
    zipFile->close();
    delete(zipFile);
}

int OkArchive::getSongDuration()
{
    findCDG(zipFile->directory());
    if (cdgLocated)
    {
        int size = cdgFile->size();
        return ((size /96) / 75) * 1000;
    }
    else
        return -1;
}

QByteArray OkArchive::getCDGData()
{
    QByteArray data;
    findCDG(zipFile->directory());
    if (cdgFile->isFile())
    {
        return cdgFile->data();
    }
    else
        qCritical() << "Error opening CDG IODevice!";
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
    findCDG(zipFile->directory());
    if ((cdgLocated) && (cdgFile->size() > 0))
        return true;
    else
        return false;
}

bool OkArchive::checkMP3()
{
    findMP3(zipFile->directory());
    if ((cdgLocated) && (cdgFile->size() > 0))
        return true;
    else
        return false;
}

bool OkArchive::extractMP3(QString destPath)
{
    findMP3(zipFile->directory());
    QByteArray data;
    if ((mp3Located) && (mp3File->size() > 0))
    {
        data = mp3File->data();
        QFile destFile(destPath);
        if (!destFile.open(QFile::WriteOnly | QFile::Truncate))
        {
            qCritical() << "Unable to create destination file while extracting mp3: " << destPath;
            return false;
        }
        destFile.write(data);
        destFile.close();
        return true;
    }
    qCritical() << "Unable to locate valid mp3 file in archive.";
    return false;
}

void OkArchive::findCDG(const KArchiveEntry *dir)
{
    if (cdgLocated)
        return;
    const KArchiveDirectory *ad = dynamic_cast<const KArchiveDirectory *>(dir);
    for (int i=0; i < ad->entries().size(); i++)
    {
        QString filename = ad->entries().at(i);
        QString filepath = ad->name() + "/" + filename;
        if (ad->entry(filename)->isFile())
        {
            if (filename.endsWith(".cdg", Qt::CaseInsensitive))
            {
                cdgLocated = true;
                cdgFile = ad->file(filename);
                return;
            }
        }
        else if (ad->entry(filename)->isDirectory())
        {
            if (filename != ad->name())
            {
                findCDG(ad->entry(filename));

            }
        }
    }
}

void OkArchive::findMP3(const KArchiveEntry *dir)
{
    if (mp3Located)
            return;
    const KArchiveDirectory *ad = dynamic_cast<const KArchiveDirectory *>(dir);
    for (int i=0; i < ad->entries().size(); i++)
    {
        QString filename = ad->entries().at(i);
        QString filepath = ad->name() + "/" + filename;
        if (ad->entry(filename)->isFile())
        {
            if (filename.endsWith(".mp3", Qt::CaseInsensitive))
            {
                mp3Located = true;
                mp3File = ad->file(filename);
                return;
            }
        }
        else if (ad->entry(filename)->isDirectory())
        {
            if (filename != ad->name())
            {
                findMP3(ad->entry(filename));

            }
        }
    }
}


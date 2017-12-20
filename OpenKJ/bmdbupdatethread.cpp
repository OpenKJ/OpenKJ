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

#include "bmdbupdatethread.h"
#include <QDir>
#include <QDirIterator>
#include <QSqlQuery>
#include <QFileInfo>
#include "tagreader.h"
#include <QtConcurrent>

BmDbUpdateThread::BmDbUpdateThread(QObject *parent) :
    QThread(parent)
{
    supportedExtensions.append(".mp3");
    supportedExtensions.append(".wav");
    supportedExtensions.append(".ogg");
    supportedExtensions.append(".flac");
    supportedExtensions.append(".m4a");
    supportedExtensions.append(".mkv");
    supportedExtensions.append(".avi");
    supportedExtensions.append(".mp4");
    supportedExtensions.append(".mpg");
    supportedExtensions.append(".mpeg");
}

QString BmDbUpdateThread::path() const
{
    return m_path;
}

void BmDbUpdateThread::setPath(const QString &path)
{
    m_path = path;
}

QStringList BmDbUpdateThread::findMediaFiles(QString directory)
{
    QStringList files;
    QDir dir(directory);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            QString filename = iterator.filePath();
            for (int i=0; i<supportedExtensions.size(); i++)
            {
                if (filename.endsWith(supportedExtensions.at(i)))
                {
                    files.append(filename);
                    break;
                }
            }
        }
    }
    return files;
}

void BmDbUpdateThread::run()
{
    TagReader reader;
    emit progressMaxChanged(0);
    emit progressChanged(0);
    emit progressMessage("Getting list of files in " + m_path);
    emit stateChanged("Finding media files...");
    QStringList files = findMediaFiles(m_path);
    emit progressMessage("Found " + QString::number(files.size()) + " files.");
    QSqlQuery query;
    emit stateChanged("Getting metadata and adding songs to the database");
    emit progressMessage("Getting metadata and adding songs to the database");
    emit progressMaxChanged(files.size());
    query.exec("BEGIN TRANSACTION");
    for (int i=0; i < files.size(); i++)
    {
        QFileInfo fi(files.at(i));
        emit progressMessage("Processing file: " + fi.fileName());
        reader.setMedia(files.at(i));
        QString duration = QString::number(reader.getDuration() / 1000);
        QString artist = reader.getArtist();
        QString title = reader.getTitle();
        QString queryString = "INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) VALUES(\"" + artist + "\",\"" + title + "\",\"" + files.at(i) + "\",\"" + files.at(i) + "\",\"" + duration + "\",\"" + artist + title + files.at(i) + "\")";
        query.exec(queryString);
        emit progressChanged(i + 1);
    }
    query.exec("COMMIT TRANSACTION");
    emit progressMessage("Finished processing files for directory: " + m_path);
}

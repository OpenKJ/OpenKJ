/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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
#include <QApplication>
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
    supportedExtensions.append(".wmv");
    supportedExtensions.append(".wma");
}

QString BmDbUpdateThread::path() const
{
    return m_path;
}

void BmDbUpdateThread::setPath(const QString &path)
{
    m_path = path;
}

QStringList BmDbUpdateThread::findMediaFiles(const QString& directory)
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
                if (filename.endsWith(supportedExtensions.at(i),Qt::CaseInsensitive))
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
    database.open();
    qInfo() << database.lastError();
    TagReader reader;
    emit progressChanged(0, 0);
    emit progressMessage("Getting list of files in " + m_path);
    emit stateChanged("Finding media files...");
    QStringList files = findMediaFiles(m_path);
    emit progressMessage("Found " + QString::number(files.size()) + " files.");
    QSqlQuery query(database);
    emit stateChanged("Getting metadata and adding songs to the database");
    emit progressMessage("Getting metadata and adding songs to the database");
    qInfo() << "Setting sqlite synchronous mode to OFF";
    query.exec("PRAGMA synchronous=OFF");
    qInfo() << query.lastError();
    qInfo() << "Increasing sqlite cache size";
    query.exec("PRAGMA cache_size=500000");
    qInfo() << query.lastError();
    query.exec("PRAGMA temp_store=2");
    qInfo() << "Beginning transaction";
    query.exec("BEGIN TRANSACTION");
    qInfo() << query.lastError();
    query.prepare("INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) VALUES(:artist, :title, :path, :filename, :duration, :searchstring)");
    for (int i=0; i < files.size(); i++)
    {
        QFileInfo fi(files.at(i));
        emit progressMessage("Processing file: " + fi.fileName());
        reader.setMedia(files.at(i));
        QString duration = QString::number(reader.getDuration() / 1000);
        QString artist = reader.getArtist();
        QString title = reader.getTitle();
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", files.at(i));
        query.bindValue(":filename", files.at(i));
        query.bindValue(":duration", duration);
        query.bindValue(":searchstring", artist + title + files.at(i));
        query.exec();
        emit progressChanged(i + 1, files.size());
    }
    query.exec("COMMIT TRANSACTION");
    qInfo() << query.lastError();
    emit progressMessage("Finished processing files for directory: " + m_path);
    database.close();
}

void BmDbUpdateThread::startUnthreaded()
{
    TagReader reader;
    emit progressChanged(0, 0);
    emit progressMessage("Getting list of files in " + m_path);
    emit stateChanged("Finding media files...");
    QStringList files = findMediaFiles(m_path);
    emit progressMessage("Found " + QString::number(files.size()) + " files.");
    QSqlQuery query;
    emit stateChanged("Getting metadata and adding songs to the database");
    emit progressMessage("Getting metadata and adding songs to the database");
    qInfo() << "Setting sqlite synchronous mode to OFF";
    query.exec("PRAGMA synchronous=OFF");
    qInfo() << query.lastError();
    qInfo() << "Increasing sqlite cache size";
    query.exec("PRAGMA cache_size=500000");
    qInfo() << query.lastError();
    query.exec("PRAGMA temp_store=2");
    qInfo() << "Beginning transaction";
    database.transaction();
    qInfo() << query.lastError();
    query.prepare("INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) VALUES(:artist, :title, :path, :filename, :duration, :searchstring)");
    for (int i=0; i < files.size(); i++)
    {
        QApplication::processEvents();
        QFileInfo fi(files.at(i));
        emit progressMessage("Processing file: " + fi.fileName());
        reader.setMedia(files.at(i));
        QString duration = QString::number(reader.getDuration() / 1000);
        QString artist = reader.getArtist();
        QString title = reader.getTitle();
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", files.at(i));
        query.bindValue(":filename", files.at(i));
        query.bindValue(":duration", duration);
        query.bindValue(":searchstring", artist + title + files.at(i));
        query.exec();
        emit progressChanged(i + 1, files.size());
    }
    database.commit();
    qInfo() << query.lastError();
    emit progressMessage("Finished processing files for directory: " + m_path);
}

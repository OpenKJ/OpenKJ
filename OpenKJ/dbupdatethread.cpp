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

#include "dbupdatethread.h"
#include <QSqlQuery>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include "sourcedirtablemodel.h"
#include <QtConcurrent>
#include "okarchive.h"
#include "tagreader.h"

int g_pattern;

DbUpdateThread::DbUpdateThread(QObject *parent) :
    QThread(parent)
{
    pattern = SourceDir::DAT;
    g_pattern = pattern;
}

int DbUpdateThread::getPattern() const
{
    return pattern;
}

void DbUpdateThread::setPattern(int value)
{
    pattern = value;
    g_pattern = value;
}

QString DbUpdateThread::getPath() const
{
    return path;
}

QStringList DbUpdateThread::findKaraokeFiles(QString directory)
{
    QStringList files;
    QDir dir(directory);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            QString fn = iterator.filePath();
            if (fn.endsWith(".zip",Qt::CaseInsensitive))
                files.append(fn);
            else if (fn.endsWith(".cdg", Qt::CaseInsensitive))
            {
                QString mp3filename = fn;
                mp3filename.chop(3);
                if ((QFile::exists(mp3filename + "mp3")) || (QFile::exists(mp3filename + "MP3")) || (QFile::exists(mp3filename + "Mp3")) || (QFile::exists(mp3filename + "mP3")))
                    files.append(fn);
            }
            else if (fn.endsWith(".mkv", Qt::CaseInsensitive) || fn.endsWith(".avi", Qt::CaseInsensitive) || fn.endsWith(".wmv", Qt::CaseInsensitive) || fn.endsWith(".mp4", Qt::CaseInsensitive) || fn.endsWith(".mpg", Qt::CaseInsensitive) || fn.endsWith(".mpeg", Qt::CaseInsensitive))
                files.append(fn);
        }
    }
    return files;
}

QMutex kDbMutex;
int processKaraokeFile(QString fileName)
{
    int duration = 0;
    // make sure the file is a valid karaoke file
    if (fileName.endsWith(".zip", Qt::CaseInsensitive))
    {
        OkArchive archive(fileName);
        if (!archive.isValidKaraokeFile())
        {
            qWarning() << "File is not a valid karaoke file: " << fileName;
            return 0;
        }
        else duration = archive.getSongDuration();
    }
    else if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QString baseFn = fileName;
        baseFn.chop(3);
        QString mp3Fn;
        if ((!QFile::exists(baseFn + "mp3")) && (!QFile::exists(baseFn + "Mp3")) && (!QFile::exists(baseFn + "MP3")) && (!QFile::exists(baseFn + "mP3")))
        {
            qWarning() << "No matching mp3 file for CDG file: " << fileName;
            return 0;
        }
        duration = ((QFile(fileName).size() / 96) / 75) * 1000;
    }
    else
    {
        TagReader reader;
        reader.setMedia(fileName);
        try {
        duration = reader.getDuration();
        } catch (...){}
    }
    QSqlQuery query;
    QString artist;
    QString title;
    QString discid;
    QFileInfo file(fileName);
    QStringList entries = file.completeBaseName().split(" - ");
    switch (g_pattern)
    {
    case SourceDir::DTA:
        if (entries.size() >= 3) artist = entries[2];
        if (entries.size() >= 2) title = entries[1];
        if (entries.size() >= 1) discid = entries[0];
        break;
    case SourceDir::DAT:
        if (entries.size() >= 2) artist = entries[1];
        if (entries.size() >= 3) title = entries[2];
        if (entries.size() >= 1) discid = entries[0];
        break;
    case SourceDir::ATD:
        if (entries.size() >= 1) artist = entries[0];
        if (entries.size() >= 2) title = entries[1];
        if (entries.size() >= 3) discid = entries[2];
        break;
    case SourceDir::TAD:
        if (entries.size() >= 2) artist = entries[1];
        if (entries.size() >= 1) title = entries[0];
        if (entries.size() >= 3) discid = entries[2];
        break;
    case SourceDir::AT:
        if (entries.size() >= 1) artist = entries[0];
        if (entries.size() >= 2) title = entries[1];
        break;
    case SourceDir::TA:
        if (entries.size() >= 2) artist = entries[1];
        if (entries.size() >= 1) title = entries[0];
        break;
    }
    QString sql = "INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration) VALUES(\"" + discid + "\",\"" + artist + "\",\""
            + title + "\",\"" + file.filePath() + "\",\"" + file.completeBaseName() + "\"," + QString::number(duration) + ")";
    kDbMutex.lock();
    query.exec(sql);
    kDbMutex.unlock();
    return 0;
}

void DbUpdateThread::setPath(const QString &value)
{
    path = value;
}



void DbUpdateThread::run()
{
    QStringList files = findKaraokeFiles(path);
    QSqlQuery query("BEGIN TRANSACTION");
    QtConcurrent::blockingMap(files, processKaraokeFile);
    query.exec("COMMIT TRANSACTION");
}

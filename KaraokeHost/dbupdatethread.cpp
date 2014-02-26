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

#include "dbupdatethread.h"
#include <QSqlQuery>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include "sourcedirtablemodel.h"

DbUpdateThread::DbUpdateThread(QObject *parent) :
    QThread(parent)
{
}

int DbUpdateThread::getPattern() const
{
    return pattern;
}

void DbUpdateThread::setPattern(int value)
{
    pattern = value;
}

QString DbUpdateThread::getPath() const
{
    return path;
}

boost::shared_ptr<QStringList> DbUpdateThread::findKaraokeFiles(QString directory)
{
    boost::shared_ptr<QStringList> files(new QStringList());
    QDir dir(directory);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            QString filename = iterator.filePath();
            if (filename.endsWith(".zip",Qt::CaseInsensitive))
                files->append(filename);
        }
    }
    return files;
}

void DbUpdateThread::setPath(const QString &value)
{
    path = value;
}

void DbUpdateThread::run()
{
    boost::shared_ptr<QStringList> files = findKaraokeFiles(path);
    QSqlQuery query("BEGIN TRANSACTION");
    for (int i=0; i < files->size(); i++)
    {
        QString artist;
        QString title;
        QString discid;
        QFileInfo file(files->at(i));
        QStringList entries = file.baseName().split(" - ");
        switch (pattern)
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
        QString sql = "INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename) VALUES(\"" + discid + "\",\"" + artist + "\",\""
                + title + "\",\"" + file.filePath() + "\",\"" + file.baseName() + "\")";
        query.exec(sql);
    }
    query.exec("COMMIT TRANSACTION");
}

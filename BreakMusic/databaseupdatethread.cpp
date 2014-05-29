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

#include "databaseupdatethread.h"
#include <QDir>
#include <QDebug>
#include <QDirIterator>
#include <tag.h>
#include <taglib.h>
#include <fileref.h>
#include <QSqlQuery>
#include <QFileInfo>

DatabaseUpdateThread::DatabaseUpdateThread(QObject *parent) :
    QThread(parent)
{
}

QString DatabaseUpdateThread::path() const
{
    return m_path;
}

void DatabaseUpdateThread::setPath(const QString &path)
{
    m_path = path;
}

QStringList *DatabaseUpdateThread::findMediaFiles(QString directory)
{
    QStringList *files = new QStringList();
    QDir dir(directory);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            QString filename = iterator.filePath();
            if (filename.endsWith(".mp3",Qt::CaseInsensitive) || filename.endsWith(".wav",Qt::CaseInsensitive) || filename.endsWith(".ogg",Qt::CaseInsensitive) || filename.endsWith(".flac",Qt::CaseInsensitive))
            {
                files->append(filename);
            }
        }
    }
    return files;
}

void DatabaseUpdateThread::run()
{
    QStringList *files = findMediaFiles(m_path);
    QSqlQuery query;
    qDebug() << "Beginning db insert";
    query.exec("BEGIN TRANSACTION");
    for (int i=0; i < files->size(); i++)
    {
        TagLib::FileRef f(files->at(i).toUtf8().data());
        if (!f.isNull())
        {
            QString artist = QString::fromStdString(f.tag()->artist().to8Bit(true));
            QString title = QString::fromStdString(f.tag()->title().to8Bit(true));
            QString duration = QString::number(f.audioProperties()->length());
            QString filename = QFileInfo(files->at(i)).fileName();
            query.exec("INSERT OR IGNORE INTO songs (artist,title,path,filename,duration) VALUES(\"" + artist + "\",\"" + title + "\",\"" + files->at(i) + "\",\"" + filename + "\"," + duration + ")");
            //        qDebug() << f.tag()->artist().toCString(true) << " - " << f.tag()->title().toCString(true);
        }
    }
    query.exec("COMMIT TRANSACTION");
    delete files;
    qDebug() << "Finished db insert";
}

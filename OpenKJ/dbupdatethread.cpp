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
#include <QStandardPaths>
#include "sourcedirtablemodel.h"
#include <QtConcurrent>
#include "okarchive.h"
#include "tagreader.h"
#include "karaokefileinfo.h"

SourceDir::NamingPattern g_pattern;
int g_customPatternId, g_artistCaptureGrp, g_titleCaptureGrp, g_discIdCaptureGrp;
QString g_artistRegex, g_titleRegex, g_discIdRegex;
QStringList errors;

bool DbUpdateThread::dbEntryExists(QString filepath)
{
    QSqlQuery query;
    query.exec("select exists(select 1 from mem.dbsongs where path = '" + filepath + "')");
    if (query.first())
        return query.value(0).toBool();
    else
        return false;
}

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

void DbUpdateThread::setPattern(SourceDir::NamingPattern value)
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
    emit progressMessage("Finding karaoke files in " + directory);
    files.clear();
    QDir dir(directory);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    int existing = 0;
    int notInDb = 0;
    int total = 0;
    QSqlQuery query;
    bool alreadyInDb = false;
    query.prepare("SELECT songid FROM mem.dbsongs WHERE path = :filepath AND discid != '!!DROPPED!!' LIMIT 1");
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            total++;
            query.bindValue(":filepath", iterator.filePath());
            query.exec();
            if (query.first())
            {
                alreadyInDb = true;
            }
            else
            {
                alreadyInDb = false;
            }
            if (alreadyInDb)
            {
                existing++;
                emit stateChanged("Finding potential karaoke files... " + QString::number(total) + " found. " + QString::number(notInDb) + " new/" + QString::number(existing) + " existing");
                continue;
            }
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
            notInDb++;
        }
        emit stateChanged("Finding potential karaoke files... " + QString::number(total) + " found. " + QString::number(notInDb) + " new/" + QString::number(existing) + " existing");

        //emit stateChanged("Finding potential karaoke files... " + QString::number(files.size()) + " found.");
    }
    emit progressMessage("Done searching for files.");
    return files;
}

QStringList DbUpdateThread::getMissingDbFiles()
{
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from mem.dbsongs");
    while (query.next())
    {
        QString path = query.value("path").toString();
        if (!QFile(path).exists())
        {
            files.append(path);
        }
    }
    return files;
}

QStringList DbUpdateThread::getDragDropFiles()
{
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from mem.dbsongs WHERE discid = '!!DROPPED!!'");
    while (query.next())
    {
        files.append(query.value("path").toString());
    }
    return files;
}

QMutex errorMutex;
QStringList DbUpdateThread::getErrors()
{
    QStringList l_errors = errors;
    errorMutex.lock();
    errors.clear();
    errorMutex.unlock();
    return l_errors;
}

void DbUpdateThread::addSingleTrack(QString path)
{
    OkArchive archive;
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration,searchstring) VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
    int duration = 0;
    QFileInfo file(path);
    archive.setArchiveFile(path);
    duration = archive.getSongDuration();
    QString artist;
    QString title;
    QString discid;
    KaraokeFileInfo parser;
    parser.setFileName(file.completeBaseName());
    parser.setDiscIdRegEx("^\\S+?(?=(\\s|_)-(\\s|_))");
    parser.setTitleRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
    parser.setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
    artist = parser.getArtist();
    title = parser.getTitle();
    discid = parser.getDiscId();
    query.bindValue(":discid", discid);
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":path", file.filePath());
    query.bindValue(":filename", file.completeBaseName());
    query.bindValue(":duration", duration);
    query.bindValue(":searchstring", QString(file.completeBaseName() + " " + artist + " " + title + " " + discid));
    query.exec();
}

int DbUpdateThread::addDroppedFile(QString path)
{
    QSqlQuery query;
    query.prepare("SELECT songid FROM dbsongs WHERE path = :path LIMIT 1");
    query.bindValue(":path", path);
    query.exec();
    if (query.first())
    {
        return query.value("songid").toInt();
    }
    query.prepare("INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename) VALUES(:discid, :artist, :title, :path, :filename)");
    QFileInfo file(path);
    QString artist = "-Dropped File-";
    QString title = QFileInfo(path).fileName();
    QString discid = "!!DROPPED!!";
    query.bindValue(":discid", discid);
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":path", file.filePath());
    query.bindValue(":filename", file.completeBaseName());
    query.exec();
    if (query.lastInsertId().isValid())
        return query.lastInsertId().toInt();
    else
        return -1;
}

void DbUpdateThread::unthreadedRun()
{

}

void DbUpdateThread::setPath(const QString &value)
{
    path = value;

}

void DbUpdateThread::run()
{
    emit progressChanged(0);
    emit progressMaxChanged(0);
    emit stateChanged("Verifing that files in DB are present on disk");
    emit stateChanged("Finding potential karaoke files...");
    QStringList missingFiles = getMissingDbFiles();
    QStringList newSongs = findKaraokeFiles(path);
    QStringList dragDropFiles = getDragDropFiles();
    QSqlQuery query;

    // Try to find out if any of the new files found have been moved and fix the db entry
    emit stateChanged("Detecting and updating moved files...");
    emit progressMaxChanged(missingFiles.size());
    int count = 0;
    query.exec("BEGIN TRANSACTION");
    for (int f=0; f<missingFiles.size(); f++)
    {
        emit progressMessage("Looking for matches to missing db song: " + missingFiles.at(f));
        qWarning() << "Looking for match for missing file: " << missingFiles.at(f);
        bool matchfound = false;
        for (int i=0; i < newSongs.size(); i++)
        {
            QString missingFile = missingFiles.at(f);
            QString newFile = newSongs.at(i);
            if (QFileInfo(newSongs.at(i)).fileName() == QFileInfo(missingFiles.at(f)).fileName())
            {
                query.prepare("UPDATE dbsongs SET path = :newpath WHERE path = :oldpath");
                query.bindValue(":newpath", newFile);
                query.bindValue(":oldpath", missingFile);
                query.exec();
                qWarning() << "Missing file found at new location";
                qWarning() << "  old: " << missingFile;
                qWarning() << "  new: " << newFile;
                newSongs.removeAt(i);
                emit progressMessage("Found match! Modifying existing song.");
                matchfound = true;
                break;
            }
        }
        if (!matchfound)
            emit progressMessage("No match found");

        count++;
        emit progressChanged(count);
    }
    query.exec("COMMIT TRANSACTION");

    for (int f=0; f < dragDropFiles.size(); f++)
    {
        QString dropFile = dragDropFiles.at(f);
        qWarning() << "Looking for matches for drop file: " << dropFile;
        for (int i=0; i < newSongs.size(); i++)
        {
            if (newSongs.at(i) == dropFile)
            {
                qWarning() << "Found match for drop file: " << dropFile;
                QFileInfo file(dropFile);
                int duration = 0;
                query.prepare("UPDATE dbsongs SET discid = :discid, artist = :artist, title = :title, filename = :filename, duration = :duration, searchstring = :searchstring WHERE path = :path");
                QString artist;
                QString title;
                QString discid;
                KaraokeFileInfo parser;
                parser.setFileName(dropFile);
                parser.setPattern(g_pattern, path);
                artist = parser.getArtist();
                title = parser.getTitle();
                discid = parser.getDiscId();
                duration = parser.getDuration();

                if (artist == "" && title == "" && discid == "")
                {
                    // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
                    if (g_pattern != SourceDir::METADATA)
                    {
                        parser.setPattern(SourceDir::METADATA, path);
                        artist = parser.getArtist();
                        title = parser.getTitle();
                        discid = parser.getDiscId();
                    }
                    // If we still don't have any metadata, just throw filename into the title field
                    if (artist == "" && title == "" && discid == "")
                        title = file.completeBaseName();
                }
                query.bindValue(":discid", discid);
                query.bindValue(":artist", artist);
                query.bindValue(":title", title);
                query.bindValue(":path", file.filePath());
                query.bindValue(":filename", file.completeBaseName());
                query.bindValue(":duration", duration);
                query.bindValue(":searchstring", QString(file.completeBaseName() + " " + artist + " " + title + " " + discid));
                query.exec();
                newSongs.removeAt(i);
                break;
            }
        }
    }

    // Add new songs to the database
    emit progressMaxChanged(newSongs.size());
    emit progressMessage("Found " + QString::number(newSongs.size()) + " potential karaoke files.");
    QString fileName;
    query.exec("BEGIN TRANSACTION");
    emit progressMessage("Checking if files are valid and getting durations...");
    emit stateChanged("Validating karaoke files and getting song durations...");
    query.prepare("INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration,searchstring) VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
    QProcess *process = new QProcess(this);
    OkArchive *archive = new OkArchive(this);
    for (int i=0; i < newSongs.count(); i++)
    {
        fileName = newSongs.at(i);
        int duration = 0;
        QFileInfo file(fileName);
        //emit progressMessage("Processing file: " + file.fileName());
#ifdef Q_OS_WIN
        if (fileName.contains("*") || fileName.contains("?") || fileName.contains("<") || fileName.contains(">") || fileName.contains("|"))
        {
            // illegal character
            errorMutex.lock();
            errors.append("Illegal character in filename: " + fileName);
            errorMutex.unlock();
            emit progressMessage("Illegal character in filename: " + fileName);
            emit progressChanged(i + 1);
            continue;
        }
#endif
        if (fileName.endsWith(".zip", Qt::CaseInsensitive))
        {
            //OkArchive *archive = new OkArchive(process);
            archive->setArchiveFile(fileName);

            if (!archive->isValidKaraokeFile())
            {
                errorMutex.lock();
                errors.append(archive->getLastError() + ": " + fileName);
                errorMutex.unlock();
                emit progressMessage(archive->getLastError() + ": " + fileName);
                emit progressChanged(i + 1);
                continue;
            }

            duration = archive->getSongDuration();
        }

        QString artist;
        QString title;
        QString discid;

        KaraokeFileInfo parser;
        parser.setFileName(fileName);

        parser.setPattern(g_pattern, path);
        artist = parser.getArtist();
        title = parser.getTitle();
        discid = parser.getDiscId();
        if (!fileName.endsWith(".zip", Qt::CaseInsensitive))
            duration = parser.getDuration();

        if (artist == "" && title == "" && discid == "")
        {
            // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
            if (g_pattern != SourceDir::METADATA)
            {
                parser.setPattern(SourceDir::METADATA, path);
                artist = parser.getArtist();
                title = parser.getTitle();
                discid = parser.getDiscId();
            }
            // If we still don't have any metadata, just throw filename into the title field
            if (artist == "" && title == "" && discid == "")
                title = file.completeBaseName();
        }

        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", file.filePath());
        query.bindValue(":filename", file.completeBaseName());
        query.bindValue(":duration", duration);
        query.bindValue(":searchstring", QString(file.completeBaseName() + " " + artist + " " + title + " " + discid));
        query.exec();
        emit progressChanged(i + 1);
        emit stateChanged("Validating karaoke files and getting song durations... " + QString::number(i + 1) + " of " + QString::number(newSongs.size()));
       // msleep(60);

    }
    delete process;
    delete archive;
    query.exec("COMMIT TRANSACTION");
    emit progressMessage("Done processing new files.");
    if (errors.size() > 0)
    {
        emit errorsGenerated(errors);
    }

}

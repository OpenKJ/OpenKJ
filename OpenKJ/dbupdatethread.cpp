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
#include <QApplication>
#include "sourcedirtablemodel.h"
#include <QtConcurrent>
#include "okarchive.h"
#include "tagreader.h"
#include "karaokefileinfo.h"

SourceDir::NamingPattern g_pattern;
int g_customPatternId, g_artistCaptureGrp, g_titleCaptureGrp, g_songIdCaptureGrp;
QString g_artistRegex, g_titleRegex, g_songIdRegex;
QStringList errors;

bool DbUpdateThread::dbEntryExists(QString filepath)
{
//    qWarning() << "DbUpdateThread::dbEntryExists(" << filepath << ") called";
//    qWarning() << "Creating thread db connection";
//    QSqlDatabase database = genUniqueDbConn();
//    database.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "openkj.sqlite");
//    database.open();
//    qWarning() << "Created";
    QSqlQuery query;
    query.prepare("select exists(select 1 from dbsongs where path = :filepath)");
    query.bindValue(":filepath", filepath);
    query.exec();
    if (query.first())
    {
//        qWarning() << "Removing thread db connection";
//        QSqlDatabase::removeDatabase(database.connectionName());
//        qWarning() << "Removed";
        bool result = query.value(0).toBool();
        query.clear();
        //qWarning() << "dbentryexists returning" << result << " for " << filepath;
//        qWarning() << "DbUpdateThread::dbEntryExists(" << filepath << ") ended";
        return result;
    }
    else
    {
        query.clear();
//        qWarning() << "Removing thread db connection";
//        QSqlDatabase::removeDatabase(database.connectionName());
//        qWarning() << "Removed";
//        qWarning() << "DbUpdateThread::dbEntryExists(" << filepath << ") ended";
        return false;
    }
}

QString DbUpdateThread::findMatchingAudioFile(QString cdgFilePath)
{
    qWarning() << "findMatchingAudioFile(" << cdgFilePath << ") called";
    QStringList audioExtensions;
    audioExtensions.append("mp3");
    audioExtensions.append("wav");
    audioExtensions.append("ogg");
    audioExtensions.append("mov");
    audioExtensions.append("flac");
    QFileInfo cdgInfo(cdgFilePath);
    QDir srcDir = cdgInfo.absoluteDir();
    QDirIterator it(srcDir);
    while (it.hasNext())
    {
        it.next();
        if (it.fileInfo().completeBaseName() != cdgInfo.completeBaseName())
            continue;
        if (it.fileInfo().suffix().toLower() == "cdg")
            continue;
        QString ext;
        foreach (ext, audioExtensions)
        {
            if (it.fileInfo().suffix().toLower() == ext)
            {
                qWarning() << "findMatchingAudioFile found match: " << it.filePath();
                return it.filePath();
            }
        }
    }
    qWarning() << "findMatchingAudioFile found no matches";
    return QString();
}

DbUpdateThread::DbUpdateThread(QSqlDatabase tdb, QObject *parent) :
    QThread(parent)
{
    database = tdb;
    pattern = SourceDir::SAT;
    g_pattern = pattern;
    settings = new Settings(this);
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
    qWarning() << "DbUpdateThread::findKaraokeFiles(" << directory << ") called";
//    qWarning() << "Creating thread db connection";
//    QSqlDatabase database = genUniqueDbConn();
////    database.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "openkj.sqlite");
//    database.open();
//    qWarning() << "Created";
    QStringList files;
    emit progressMessage("Finding karaoke files in " + directory);
    files.clear();
    QDir dir(directory);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    int existing = 0;
    int notInDb = 0;
    int total = 0;
    qWarning() << "Creating query";
    QSqlQuery query;
    qWarning() << "Created";
    bool alreadyInDb = false;
    query.prepare("SELECT songid FROM dbsongs WHERE path = :filepath AND discid != '!!DROPPED!!' LIMIT 1");
    while (iterator.hasNext()) {
        QApplication::processEvents();
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            total++;
            query.bindValue(":filepath", iterator.filePath());
            query.exec();
            qWarning() << query.lastError();
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
                if (findMatchingAudioFile(fn) != "")
                    files.append(fn);
//                QString audioFilename = fn;
//                audioFilename.chop(3);
//                if ((QFile::exists(audioFilename + "mp3")) || (QFile::exists(audioFilename + "MP3")) || (QFile::exists(audioFilename + "Mp3")) || (QFile::exists(audioFilename + "mP3")))
//                    files.append(fn);
            }
            else if (fn.endsWith(".mkv", Qt::CaseInsensitive) || fn.endsWith(".avi", Qt::CaseInsensitive) || fn.endsWith(".wmv", Qt::CaseInsensitive) || fn.endsWith(".mp4", Qt::CaseInsensitive) || fn.endsWith(".mpg", Qt::CaseInsensitive) || fn.endsWith(".mpeg", Qt::CaseInsensitive))
                files.append(fn);
            notInDb++;
        }
        emit stateChanged("Finding potential karaoke files... " + QString::number(total) + " found. " + QString::number(notInDb) + " new/" + QString::number(existing) + " existing");

        //emit stateChanged("Finding potential karaoke files... " + QString::number(files.size()) + " found.");
    }
    qWarning() << "File search results - Potential files: " << files.size() << " - Already in DB: " << existing << " - New: " << notInDb;
    emit progressMessage("Done searching for files.");
//    qWarning() << "Removing thread db connection";
//    query.clear();
//    QSqlDatabase::removeDatabase(database.connectionName());
//    qWarning() << "Removed";
    qWarning() << "DbUpdateThread::findKaraokeFiles(" << directory << ") ended";
    return files;
}

QStringList DbUpdateThread::getMissingDbFiles()
{
    qWarning() << "DbUpdateThread::getMissingDbFiles() called";
//    qWarning() << "Creating thread db connection";
//    QSqlDatabase database = genUniqueDbConn();
////    database.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "openkj.sqlite");
//    database.open();
//    qWarning() << "Created";
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from dbsongs");
    while (query.next())
    {
        QString path = query.value("path").toString();
        if (!QFile(path).exists())
        {
            files.append(path);
        }
    }
    query.clear();
//    database.close();
//    qWarning() << "Removing thread db connection";
//    QSqlDatabase::removeDatabase(database.connectionName());
//    qWarning() << "Removed";
//    qWarning() << "DbUpdateThread::getMissingDbFiles() ended";
    return files;
}

QStringList DbUpdateThread::getDragDropFiles()
{
    qWarning() << "DbUpdateThread::getDragDropFiles() called";
//    qWarning() << "Creating thread db connection";
//    QSqlDatabase database = genUniqueDbConn();
////    database.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "openkj.sqlite");
//    database.open();
//    qWarning() << "Created";
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from dbsongs WHERE discid = '!!DROPPED!!'");
    while (query.next())
    {
        files.append(query.value("path").toString());
    }
    query.clear();
//    qWarning() << "Removing thread db connection";
//    QSqlDatabase::removeDatabase(database.connectionName());
//    qWarning() << "Removed";
    qWarning() << "DbUpdateThread::getDragDropFiles() ended";
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
    if (settings->dbLazyLoadDurations())
        duration = -2;
    else
        duration = archive.getSongDuration();
    QString artist;
    QString title;
    QString discid;
    KaraokeFileInfo parser;
    parser.setFileName(path);
    parser.setPattern(SourceDir::SAT);
//    parser.setSongIdRegEx("^\\S+?(?=(\\s|_)-(\\s|_))");
//    parser.setTitleRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
//    parser.setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
    if (duration == 0)
        duration = parser.getDuration();
    artist = parser.getArtist();
    title = parser.getTitle();
    discid = parser.getSongId();
    query.bindValue(":discid", discid);
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":path", file.filePath());
    query.bindValue(":filename", file.completeBaseName());
    query.bindValue(":duration", duration);
    query.bindValue(":searchstring", QString(file.completeBaseName() + " " + artist + " " + title + " " + discid));
    query.exec();
    if (query.lastInsertId().isValid())
    {
        int lastInsertId = query.lastInsertId().toInt();
        query.prepare("INSERT OR IGNORE INTO mem.dbsongs (rowid,discid,artist,title,path,filename,duration,searchstring) VALUES(:rowid,:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
        query.bindValue(":rowid", lastInsertId);
        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", file.filePath());
        query.bindValue(":filename", file.completeBaseName());
        query.bindValue(":duration", duration);
        query.bindValue(":searchstring", QString(file.completeBaseName() + " " + artist + " " + title + " " + discid));
        query.exec();
    }
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
    {
        int lastInsertId = query.lastInsertId().toInt();
        query.clear();
        query.prepare("INSERT OR IGNORE INTO mem.dbSongs (rowid,discid,artist,title,path,filename) VALUES(:rowid,:discid, :artist, :title, :path, :filename)");
        query.bindValue(":rowid", lastInsertId);
        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", file.filePath());
        query.bindValue(":filename", file.completeBaseName());
        query.exec();
        return lastInsertId;
    }
    else
    {
        query.clear();
        return -1;
    }
}

void DbUpdateThread::startUnthreaded()
{
    emit progressChanged(0);
    emit progressMaxChanged(0);
    emit stateChanged("Verifing that files in DB are present on disk");
    emit stateChanged("Finding potential karaoke files...");
    QStringList missingFiles = getMissingDbFiles();
    QStringList newSongs = findKaraokeFiles(path);
    QStringList dragDropFiles = getDragDropFiles();
    qWarning() << "Creating QSqlQuery";
    QSqlQuery query;
    qWarning() << "Created";

    // Try to find out if any of the new files found have been moved and fix the db entry
    emit stateChanged("Detecting and updating moved files...");
    emit progressMaxChanged(missingFiles.size());
    int count = 0;
    qWarning() << "Looking for missing files";
    database.transaction();
    for (int f=0; f<missingFiles.size(); f++)
    {
        QApplication::processEvents();
        emit progressMessage("Looking for matches to missing db song: " + missingFiles.at(f));
        qWarning() << "Looking for match for missing file: " << missingFiles.at(f);
        bool matchfound = false;
        QString newFile;
        QString missingFile;
        query.prepare("UPDATE dbsongs SET path = :newpath WHERE path = :oldpath");
        for (int i=0; i < newSongs.size(); i++)
        {
            QApplication::processEvents();
            missingFile = missingFiles.at(f);
            newFile = newSongs.at(i);
            if (QFileInfo(newSongs.at(i)).fileName() == QFileInfo(missingFiles.at(f)).fileName())
            {
//                query.prepare("UPDATE dbsongs SET path = :newpath WHERE path = :oldpath");
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
    qWarning() << "Committing transaction";
    database.commit();
    qWarning() << "Processing dragged/dropped files";
    for (int f=0; f < dragDropFiles.size(); f++)
    {
        QApplication::processEvents();
        QString dropFile = dragDropFiles.at(f);
        qWarning() << "Looking for matches for drop file: " << dropFile;
        query.prepare("UPDATE dbsongs SET discid = :discid, artist = :artist, title = :title, filename = :filename, duration = :duration, searchstring = :searchstring WHERE path = :path");
        QString artist;
        QString title;
        QString discid;
        QString filePath;
        QString fileName;
        int duration = 0;
        QString searchString;

        for (int i=0; i < newSongs.size(); i++)
        {
            QApplication::processEvents();
            if (newSongs.at(i) == dropFile)
            {
                qWarning() << "Found match for drop file: " << dropFile;
                QFileInfo file(dropFile);
                filePath = file.filePath();
                fileName = file.completeBaseName();
                KaraokeFileInfo parser;
                parser.setFileName(dropFile);
                parser.setPattern(g_pattern, path);
                artist = parser.getArtist();
                title = parser.getTitle();
                discid = parser.getSongId();
                duration = parser.getDuration();

                if (artist == "" && title == "" && discid == "")
                {
                    // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
                    if (g_pattern != SourceDir::METADATA)
                    {
                        parser.setPattern(SourceDir::METADATA, path);
                        artist = parser.getArtist();
                        title = parser.getTitle();
                        discid = parser.getSongId();
                    }
                    // If we still don't have any metadata, just throw filename into the title field
                    if (artist == "" && title == "" && discid == "")
                        title = file.completeBaseName();
                }
                searchString = QString(file.completeBaseName() + " " + artist + " " + title + " " + discid);
                query.bindValue(":discid", discid);
                query.bindValue(":artist", artist);
                query.bindValue(":title", title);
                query.bindValue(":path", filePath);
                query.bindValue(":filename", fileName);
                query.bindValue(":duration", duration);
                query.bindValue(":searchstring", searchString);
                query.exec();
                newSongs.removeAt(i);
                break;
            }
        }
    }
    qWarning() << "Adding new songs";
    emit progressMaxChanged(newSongs.size());
    emit progressMessage("Found " + QString::number(newSongs.size()) + " potential karaoke files.");
    QString fName;
    QString filePath;
    QString discid;
    QString artist;
    QString title;
    QString searchString;
    int duration = 0;
    qWarning() << "Setting sqlite synchronous mode to OFF";
    query.exec("PRAGMA synchronous=OFF");
    qWarning() << query.lastError();
    qWarning() << "Increasing sqlite cache size";
    query.exec("PRAGMA cache_size=500000");
    qWarning() << query.lastError();
    query.exec("PRAGMA temp_store=2");
    qWarning() << "Beginning transaction";
    database.transaction();
    emit progressMessage("Checking if files are valid and getting durations...");
    emit stateChanged("Validating karaoke files and getting song durations...");
    qWarning() << "Preparing statement";
    query.prepare("INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration,searchstring) VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
    qWarning() << "Statement prepared";
    qWarning() << "Creating OkArchive instance";
    OkArchive archive;
    KaraokeFileInfo parser;
    qWarning() << "looping over songs";
    for (int i=0; i < newSongs.count(); i++)
    {
        QApplication::processEvents();
        QString fileName = newSongs.at(i);
        qWarning() << "Beginning processing file: " << fileName;
        QFileInfo file(fileName);
        emit progressMessage("Processing file: " + file.completeBaseName());
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
            archive.setArchiveFile(fileName);
            if (!settings->dbSkipValidation())
            {
                if (!archive.isValidKaraokeFile())
                {
                    errorMutex.lock();
                    errors.append(archive.getLastError() + ": " + fileName);
                    errorMutex.unlock();
                    emit progressMessage(archive.getLastError() + ": " + fileName);
                    emit progressChanged(i + 1);
                    continue;
                }
            }
            if (settings->dbLazyLoadDurations())
                duration = -2;
            else
                duration = archive.getSongDuration();
        }
        parser.setFileName(fileName);
        parser.setPattern(g_pattern, path);
        artist = parser.getArtist();
        title = parser.getTitle();
        discid = parser.getSongId();
        if (!fileName.endsWith(".zip", Qt::CaseInsensitive))
        {
            if (settings->dbLazyLoadDurations())
                duration = -3;
            else
                duration = parser.getDuration();
        }
        if (artist == "" && title == "" && discid == "")
        {
            // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
            if (g_pattern != SourceDir::METADATA)
            {
                parser.setPattern(SourceDir::METADATA, path);
                artist = parser.getArtist();
                title = parser.getTitle();
                discid = parser.getSongId();
            }
            // If we still don't have any metadata, just throw filename into the title field
            if (artist == "" && title == "" && discid == "")
                title = file.completeBaseName();
        }
        fName = file.completeBaseName();
        filePath = fileName;
        searchString = QString(fName + " " + artist + " " + title + " " + discid);
        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", filePath);
        query.bindValue(":filename", fName);
        query.bindValue(":duration", duration);
        query.bindValue(":searchstring", searchString);
        query.exec();
        emit progressChanged(i + 1);
        emit stateChanged("Validating karaoke files and getting song durations... " + QString::number(i + 1) + " of " + QString::number(newSongs.size()));
        qWarning() << "Done processing file: " << fileName;
    }
    qWarning() << "Done looping";
    qWarning() << "Committing transaction";
    database.commit();
    qWarning() << "QSqlDatabase last error: " << database.lastError();
    qWarning() << "QSqlQuery last error: " << query.lastError();
    emit progressMessage("Done processing new files.");
    if (errors.size() > 0)
    {
        emit errorsGenerated(errors);
    }
    //emit databaseUpdateComplete();
}

void DbUpdateThread::setPath(const QString &value)
{
    path = value;

}

void DbUpdateThread::run()
{
    emit databaseAboutToUpdate();

    database.open();
    emit progressChanged(0);
    emit progressMaxChanged(0);
    emit stateChanged("Verifing that files in DB are present on disk");
    emit stateChanged("Finding potential karaoke files...");
    QStringList missingFiles = getMissingDbFiles();
    QStringList newSongs = findKaraokeFiles(path);
    QStringList dragDropFiles = getDragDropFiles();

//    qWarning() << "Creating thread db connection";
//    QSqlDatabase database = genUniqueDbConn();
////    database.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "openkj.sqlite");
//    database.open();
//    qWarning() << "Created";
    qWarning() << "Creating QSqlQuery";
    QSqlQuery query(database);
    qWarning() << "Created";

    // Try to find out if any of the new files found have been moved and fix the db entry
    emit stateChanged("Detecting and updating moved files...");
    emit progressMaxChanged(missingFiles.size());
    int count = 0;
    qWarning() << "Looking for missing files";
    query.exec("BEGIN TRANSACTION");
    for (int f=0; f<missingFiles.size(); f++)
    {
        emit progressMessage("Looking for matches to missing db song: " + missingFiles.at(f));
        qWarning() << "Looking for match for missing file: " << missingFiles.at(f);
        bool matchfound = false;
        QString newFile;
        QString missingFile;
        query.prepare("UPDATE dbsongs SET path = :newpath WHERE path = :oldpath");
        for (int i=0; i < newSongs.size(); i++)
        {
            missingFile = missingFiles.at(f);
            newFile = newSongs.at(i);
            if (QFileInfo(newSongs.at(i)).fileName() == QFileInfo(missingFiles.at(f)).fileName())
            {
//                query.prepare("UPDATE dbsongs SET path = :newpath WHERE path = :oldpath");
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
    qWarning() << "Committing transaction";
    query.exec("COMMIT TRANSACTION");
    qWarning() << "Processing dragged/dropped files";
    for (int f=0; f < dragDropFiles.size(); f++)
    {
        QString dropFile = dragDropFiles.at(f);
        qWarning() << "Looking for matches for drop file: " << dropFile;
        query.prepare("UPDATE dbsongs SET discid = :discid, artist = :artist, title = :title, filename = :filename, duration = :duration, searchstring = :searchstring WHERE path = :path");
        QString artist;
        QString title;
        QString discid;
        QString filePath;
        QString fileName;
        int duration = 0;
        QString searchString;

        for (int i=0; i < newSongs.size(); i++)
        {
            if (newSongs.at(i) == dropFile)
            {
                qWarning() << "Found match for drop file: " << dropFile;
                QFileInfo file(dropFile);
                filePath = file.filePath();
                fileName = file.completeBaseName();
//                int duration = 0;
//                query.prepare("UPDATE dbsongs SET discid = :discid, artist = :artist, title = :title, filename = :filename, duration = :duration, searchstring = :searchstring WHERE path = :path");
//                QString artist;
//                QString title;
//                QString discid;
                KaraokeFileInfo parser;
                parser.setFileName(dropFile);
                parser.setPattern(g_pattern, path);
                artist = parser.getArtist();
                title = parser.getTitle();
                discid = parser.getSongId();
                duration = parser.getDuration();

                if (artist == "" && title == "" && discid == "")
                {
                    // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
                    if (g_pattern != SourceDir::METADATA)
                    {
                        parser.setPattern(SourceDir::METADATA, path);
                        artist = parser.getArtist();
                        title = parser.getTitle();
                        discid = parser.getSongId();
                    }
                    // If we still don't have any metadata, just throw filename into the title field
                    if (artist == "" && title == "" && discid == "")
                        title = file.completeBaseName();
                }
                searchString = QString(file.completeBaseName() + " " + artist + " " + title + " " + discid);
                query.bindValue(":discid", discid);
                query.bindValue(":artist", artist);
                query.bindValue(":title", title);
                query.bindValue(":path", filePath);
                query.bindValue(":filename", fileName);
                query.bindValue(":duration", duration);
                query.bindValue(":searchstring", searchString);
                query.exec();
                newSongs.removeAt(i);
                break;
            }
        }
    }
    qWarning() << "Adding new songs";
    // Add new songs to the database
    emit progressMaxChanged(newSongs.size());
    emit progressMessage("Found " + QString::number(newSongs.size()) + " potential karaoke files.");
    QString fName;
    QString filePath;
    QString discid;
    QString artist;
    QString title;
    QString searchString;
    int duration = 0;
    qWarning() << "Setting sqlite synchronous mode to OFF";
    query.exec("PRAGMA synchronous=OFF");
    qWarning() << query.lastError();
    qWarning() << "Increasing sqlite cache size";
    query.exec("PRAGMA cache_size=500000");
    qWarning() << query.lastError();
    query.exec("PRAGMA temp_store=2");
    qWarning() << "Beginning transaction";
    query.exec("BEGIN TRANSACTION");
    //database.transaction();
    emit progressMessage("Checking if files are valid and getting durations...");
    emit stateChanged("Validating karaoke files and getting song durations...");
    qWarning() << "Preparing statement";
    query.prepare("INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration,searchstring) VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
    qWarning() << "Statement prepared";
//    qWarning() << "Creating QProcess";
//    QProcess *process = new QProcess(this);
    qWarning() << "Creating OkArchive instance";
    OkArchive archive;
    KaraokeFileInfo parser;
    qWarning() << "looping over songs";
    for (int i=0; i < newSongs.count(); i++)
    {
        QString fileName = newSongs.at(i);
        QFileInfo file(fileName);
        emit progressMessage("Processing file: " + file.completeBaseName());
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
            archive.setArchiveFile(fileName);

            if (!archive.isValidKaraokeFile())
            {
                errorMutex.lock();
                errors.append(archive.getLastError() + ": " + fileName);
                errorMutex.unlock();
                emit progressMessage(archive.getLastError() + ": " + fileName);
                emit progressChanged(i + 1);
                continue;
            }

            duration = archive.getSongDuration();
        }

//        KaraokeFileInfo parser;
        parser.setFileName(fileName);

        parser.setPattern(g_pattern, path);
        artist = parser.getArtist();
        title = parser.getTitle();
        discid = parser.getSongId();
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
                discid = parser.getSongId();
            }
            // If we still don't have any metadata, just throw filename into the title field
            if (artist == "" && title == "" && discid == "")
                title = file.completeBaseName();
        }
        fName = file.completeBaseName();
        filePath = fileName;
        searchString = QString(fName + " " + artist + " " + title + " " + discid);
        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", filePath);
        query.bindValue(":filename", fName);
        query.bindValue(":duration", duration);
        query.bindValue(":searchstring", searchString);
        query.exec();
        //qWarning() << query.lastError();
        emit progressChanged(i + 1);
        emit stateChanged("Validating karaoke files and getting song durations... " + QString::number(i + 1) + " of " + QString::number(newSongs.size()));
       // msleep(60);

    }
    qWarning() << "Done looping";
//    delete process;
//    delete archive;
    qWarning() << "Committing transaction";
    //database.commit();
    query.exec("COMMIT TRANSACTION");
    qWarning() << "QSqlDatabase last error: " << database.lastError();
    qWarning() << "QSqlQuery last error: " << query.lastError();
    emit progressMessage("Done processing new files.");
    if (errors.size() > 0)
    {
        emit errorsGenerated(errors);
    }
    database.close();
    emit databaseUpdateComplete();
//    qWarning() << "Removing thread db connection";
//    query.clear();
//    QSqlDatabase::removeDatabase(database.connectionName());
//    qWarning() << "Removed";
}

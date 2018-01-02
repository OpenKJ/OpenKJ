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
#include "filenameparser.h"

int g_pattern;
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
    emit progressMessage("Finding karaoke files in " + directory);
    QStringList files;
    QDir dir(directory);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    int existing = 0;
    int notInDb = 0;
    int total = 0;
    QSqlQuery query;
    bool alreadyInDb = false;
    query.prepare("SELECT EXISTS(SELECT 1 FROM mem.dbsongs WHERE path = :filepath LIMIT 1)");
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            total++;
            query.bindValue(":filepath", iterator.filePath());
            query.exec();
            if (query.first())
                alreadyInDb = query.value(0).toBool();
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
            qWarning() << "Not in DB: " << iterator.filePath();
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

QMutex errorMutex;
QStringList DbUpdateThread::getErrors()
{
    QStringList l_errors = errors;
    errorMutex.lock();
    errors.clear();
    errorMutex.unlock();
    return l_errors;
}

void DbUpdateThread::setPath(const QString &value)
{
    path = value;

}



void DbUpdateThread::run()
{
    TagReader reader;
    OkArchive archive;
    if (pattern == SourceDir::CUSTOM)
    {
        QSqlQuery query;
        query.exec("SELECT custompattern FROM sourcedirs WHERE path == \"" + path + "\"" );
        if (query.first())
            g_customPatternId = query.value(0).toInt();
        if (g_customPatternId < 1)
        {
            qCritical() << "Custom pattern set for path, but pattern ID is invalid!  Bailing out!";
            return;
        }
        query.exec("SELECT * FROM custompatterns WHERE patternid == " + QString::number(g_customPatternId));
        if (query.first())
        {
            g_artistRegex = query.value("artistregex").toString();
            g_titleRegex  = query.value("titleregex").toString();
            g_discIdRegex = query.value("discidregex").toString();
            g_artistCaptureGrp = query.value("artistcapturegrp").toInt();
            g_titleCaptureGrp  = query.value("titlecapturegrp").toInt();
            g_discIdCaptureGrp = query.value("discidcapturegrp").toInt();
        }
    }
    emit progressChanged(0);
    emit progressMaxChanged(0);
    emit stateChanged("Finding potential karaoke files...");
    QStringList files = findKaraokeFiles(path);
    emit progressMaxChanged(files.size());
    emit progressMessage("Found " + QString::number(files.size()) + " potential karaoke files.");
    QString fileName;
    QSqlQuery query("BEGIN TRANSACTION");
    emit progressMessage("Checking if files are valid and getting durations...");
    emit stateChanged("Validating karaoke files and getting song durations...");
    query.prepare("INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration) VALUES(:discid, :artist, :title, :path, :filename, :duration)");
    for (int i=0; i < files.count(); i++)
    {
        fileName = files.at(i);
        QString mediaFile;
        int duration = 0;
        QFileInfo file(fileName);
        emit progressMessage("Processing file: " + file.fileName());
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
            if (!archive.isValidKaraokeFile())
            {
                errorMutex.lock();
                errors.append("Bad or invalid karaoke file: " + fileName);
                errorMutex.unlock();
                emit progressMessage("Bad or invalid karaoke file: " + fileName);
                emit progressChanged(i + 1);
                continue;
            }
            else duration = archive.getSongDuration();
        }
        else if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
        {
            QString baseFn = fileName;
            baseFn.chop(3);
            if (QFile::exists(baseFn + "mp3"))
                mediaFile = baseFn + "mp3";
            else if (QFile::exists(baseFn + "Mp3"))
                mediaFile = baseFn + "Mp3";
            else if (QFile::exists(baseFn + "MP3"))
                mediaFile = baseFn + "MP3";
            else if (QFile::exists(baseFn + "mP3"))
                mediaFile = baseFn + "mP3";
            else
            {
                errorMutex.lock();
                errors.append("Missing CDG file for mp3 file: " + fileName);
                errorMutex.unlock();
                emit progressMessage("Missing CDG file for mp3 file: " + fileName);
                emit progressChanged(i + 1);
                continue;
            }
            duration = ((QFile(fileName).size() / 96) / 75) * 1000;
        }
        else
        {
            reader.setMedia(fileName);
            try
            {
                duration = reader.getDuration();
            }
            catch (...)
            {
                errorMutex.lock();
                errors.append("Unable to get duration for file: " + fileName);
                errorMutex.unlock();
                emit progressMessage("Unable to get duration for file: " + fileName);
            }
        }
        QString artist;
        QString title;
        QString discid;
        FilenameParser parser;
        parser.setFileName(file.completeBaseName());
        switch (g_pattern)
        {
        case SourceDir::DTA:
            parser.setDiscIdRegEx("^\\S+?(?=(\\s|_)-(\\s|_))");
            parser.setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
            parser.setArtistRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
            artist = parser.getArtist();
            title = parser.getTitle();
            discid = parser.getDiscId();
            break;
        case SourceDir::DAT:
            parser.setDiscIdRegEx("^\\S+?(?=(\\s|_)-(\\s|_))");
            parser.setTitleRegEx("(?:^\\S+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|_))(.+)",1);
            parser.setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))", 0);
            artist = parser.getArtist();
            title = parser.getTitle();
            discid = parser.getDiscId();
            break;
        case SourceDir::ATD:
            parser.setArtistRegEx(".+?(?=(\\s|_)-(\\s|_))",0);
            parser.setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))");
            parser.setDiscIdRegEx("(?:^.+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|))(.+)", 1);
            artist = parser.getArtist();
            title = parser.getTitle();
            discid = parser.getDiscId();
            break;
        case SourceDir::TAD:
            parser.setTitleRegEx(".+?(?=(\\s|_)-(\\s|_))",0);
            parser.setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*?)(?=(\\s|_)-(\\s|_))");
            parser.setDiscIdRegEx("(?:^.+(?:\\s|_)-(?:\\s|_).+(?:\\s|_)-(?:\\s|))(.+)", 1);
            artist = parser.getArtist();
            title = parser.getTitle();
            discid = parser.getDiscId();
            break;
        case SourceDir::AT:
            parser.setArtistRegEx(".+?(?=(\\s|_)-(\\s|_))");
            parser.setTitleRegEx("(?<=(\\s|_)-(\\s|_))(.*)");
            artist = parser.getArtist();
            title = parser.getTitle();
            discid = "";
            break;
        case SourceDir::TA:
            parser.setTitleRegEx(".+?(?=(\\s|_)-(\\s|_))");
            parser.setArtistRegEx("(?<=(\\s|_)-(\\s|_))(.*)");
            artist = parser.getArtist();
            title = parser.getTitle();
            discid = "";
            break;
        case SourceDir::CUSTOM:
            parser.setTitleRegEx(g_titleRegex, g_titleCaptureGrp);
            parser.setArtistRegEx(g_artistRegex, g_artistCaptureGrp);
            parser.setDiscIdRegEx(g_discIdRegex, g_discIdCaptureGrp);
            artist = parser.getArtist();
            title = parser.getTitle();
            discid = parser.getDiscId();
            break;
        case SourceDir::METADATA:
            if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
            {
                TagReader reader;
                reader.setMedia(mediaFile);
                artist = reader.getArtist();
                title = reader.getTitle();
                discid = reader.getAlbum();
                QString track = reader.getTrack();
                if (track != "")
                {
                    discid.append("-" + track);
                }
            }
            else if (fileName.endsWith(".zip", Qt::CaseInsensitive))
            {
                QTemporaryDir dir;
                archive.setArchiveFile(fileName);
                archive.checkAudio();
                QString audioFile = "temp" + archive.audioExtension();
                archive.extractAudio(dir.path() + QDir::separator() + audioFile);
                TagReader reader;
                reader.setMedia(dir.path() + QDir::separator() + audioFile);
                artist = reader.getArtist();
                title = reader.getTitle();
                discid = reader.getAlbum();
                QString track = reader.getTrack();
                if (track != "")
                {
                    discid.append("-" + track);
                }
            }
            break;
        }
        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", file.filePath());
        query.bindValue(":filename", file.completeBaseName());
        query.bindValue(":duration", duration);
        query.exec();
        emit progressChanged(i + 1);
        emit stateChanged("Validating karaoke files and getting song durations... " + QString::number(i + 1) + " of " + QString::number(files.size()));
    }
    query.exec("COMMIT TRANSACTION");
    emit progressMessage("Done processing files.");
    if (errors.size() > 0)
    {
        emit errorsGenerated(errors);
    }
}

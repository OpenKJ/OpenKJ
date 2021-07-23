/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
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
#include <array>
#include <QSqlQuery>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QApplication>
#include "src/models/tablemodelkaraokesourcedirs.h"
#include "mzarchive.h"
#include "karaokefileinfo.h"

bool DbUpdater::dbEntryExists(const QString &filepath, const bool includeDropped) {
    QSqlQuery query;
    if (includeDropped)
        query.prepare("SELECT EXISTS(SELECT 1 FROM dbsongs WHERE path = :filepath)");
    else
        query.prepare("SELECT EXISTS(SELECT 1 FROM dbsongs WHERE path = :filepath AND discid != '!!DROPPED!!')");
    query.bindValue(":filepath", filepath);
    query.exec();
    if (query.first()) {
        return query.value(0).toBool();
    }
    return false;
}

QString DbUpdater::findMatchingAudioFile(const QString &cdgFilePath) {
    std::array<QString, 5> audioExtensions{"mp3", "wav", "ogg", "mov", "flac"};
    QFileInfo cdgInfo(cdgFilePath);
    for (const auto &ext : audioExtensions) {
        QString testPath = cdgInfo.absolutePath() + QDir::separator() + cdgInfo.completeBaseName() + '.' + ext;
        if (QFile::exists(testPath))
            return testPath;
    }
    return QString();
}

DbUpdater::DbUpdater(QObject *parent) :
        QObject(parent) {
}

void DbUpdater::setPattern(SourceDir::NamingPattern value) {
    m_pattern = value;
}

QStringList DbUpdater::findKaraokeFiles(const QString &directory) {
    qInfo() << "DbUpdater::findKaraokeFiles(" << directory << ") called";
    QStringList files;
    emit progressMessage("Finding karaoke files in " + directory);
    files.reserve(200000);
    QDir dir(directory);
    int existing = 0;
    int notInDb = 0;
    int total = 0;
    int loops = 0;
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        QApplication::processEvents();
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            total++;
            QString fn = iterator.filePath();
            if (isSupportedMediaFile(fn)) {
                if (dbEntryExists(iterator.filePath(), false)) {
                    existing++;
                } else {
                    files.append(fn);
                    notInDb++;
                }
            }
        }
        if (loops >= 5) {
            emit stateChanged("Finding potential karaoke files... " + QString::number(total) + " found. " +
                              QString::number(notInDb) + " new/" + QString::number(existing) + " existing");
            loops = 0;
            continue;
        }
        loops++;
    }
    emit stateChanged(
            "Finding potential karaoke files... " + QString::number(total) + " found. " + QString::number(notInDb) +
            " new/" + QString::number(existing) + " existing");
    qInfo() << "File search results - Potential files: " << files.size() << " - Already in DB: " << existing
            << " - New: " << notInDb;
    emit progressMessage("Done searching for files.");
    qInfo() << "DbUpdater::findKaraokeFiles(" << directory << ") ended";
    return files;
}

QStringList DbUpdater::getMissingDbFiles() {
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from dbsongs");
    while (query.next()) {
        QString path = query.value("path").toString();
        if (!QFile(path).exists()) {
            files.append(path);
        }
    }
    return files;
}

QStringList DbUpdater::getDragDropFiles() {
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from dbsongs WHERE discid = '!!DROPPED!!'");
    while (query.next()) {
        files.append(query.value("path").toString());
    }
    return files;
}

void DbUpdater::addSingleTrack(const QString &filePath) {
    MzArchive archive;
    QSqlQuery query;
    query.prepare(
            "INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration,searchstring) VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
    int duration{-2};
    QFileInfo file(filePath);
    QString artist;
    QString title;
    QString discid;
    KaraokeFileInfo parser;
    parser.setFileName(filePath);
    parser.setPattern(SourceDir::SAT);
    archive.setArchiveFile(filePath);
    if (!m_settings.dbLazyLoadDurations()) {
        duration = parser.getDuration();
    }
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
    if (query.lastInsertId().isValid()) {
        int lastInsertId = query.lastInsertId().toInt();
        query.prepare(
                "INSERT OR IGNORE INTO mem.dbsongs (rowid,discid,artist,title,path,filename,duration,searchstring) VALUES(:rowid,:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
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

int DbUpdater::addDroppedFile(const QString &filePath) {
    QSqlQuery query;
    query.prepare("SELECT songid FROM dbsongs WHERE path = :path LIMIT 1");
    query.bindValue(":path", filePath);
    query.exec();
    if (query.first()) {
        return query.value("songid").toInt();
    }
    query.prepare(
            "INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename) VALUES(:discid, :artist, :title, :path, :filename)");
    QFileInfo file(filePath);
    QString artist = "-Dropped File-";
    QString title = QFileInfo(filePath).fileName();
    QString discid = "!!DROPPED!!";
    query.bindValue(":discid", discid);
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":path", file.filePath());
    query.bindValue(":filename", file.completeBaseName());
    query.exec();
    if (query.lastInsertId().isValid()) {
        int lastInsertId = query.lastInsertId().toInt();
        query.clear();
        query.prepare(
                "INSERT OR IGNORE INTO mem.dbSongs (rowid,discid,artist,title,path,filename) VALUES(:rowid,:discid, :artist, :title, :path, :filename)");
        query.bindValue(":rowid", lastInsertId);
        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", file.filePath());
        query.bindValue(":filename", file.completeBaseName());
        query.exec();
        return lastInsertId;
    } else {
        return -1;
    }
}

void DbUpdater::process() {
    emit progressChanged(0);
    emit progressMaxChanged(0);
    emit stateChanged("Verifying that files in DB are present on disk");
    emit stateChanged("Finding potential karaoke files...");
    QStringList newSongs = findKaraokeFiles(m_path);
    fixMissingFiles(newSongs);
    importDragDropSongs(newSongs);
    QSqlQuery query;
    qInfo() << "Adding new songs";
    emit progressMaxChanged(newSongs.size());
    emit progressMessage("Found " + QString::number(newSongs.size()) + " potential karaoke files.");
    QString fName;
    QString discid;
    QString artist;
    QString title;
    QString searchString;
    int duration{-2};
    query.exec("PRAGMA synchronous=OFF");
    query.exec("PRAGMA cache_size=500000");
    query.exec("PRAGMA temp_store=2");
    qInfo() << "Beginning transaction";
    query.exec("BEGIN TRANSACTION");
    emit progressMessage("Checking if files are valid and getting durations...");
    emit stateChanged("Validating karaoke files and getting song durations...");
    qInfo() << "Preparing statement";
    query.prepare(
            "INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration,searchstring) VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
    qInfo() << "Statement prepared";
    qInfo() << "Creating MzArchive instance";
    MzArchive archive;
    KaraokeFileInfo parser;
    qInfo() << "looping over songs";
    int loops{0};
    for (const auto &filePath : newSongs) {
        QApplication::processEvents();
        QFileInfo fileInfo(filePath);
#ifdef Q_OS_WIN
        if (filePath.contains("*") || filePath.contains("?") || filePath.contains("<") || filePath.contains(">") || filePath.contains("|"))
        {
            // illegal character
            errors.append("Illegal character in filename: " + filePath);
            emit progressMessage("Illegal character in filename: " + filePath);
            emit progressChanged(loops + 1);
            continue;
        }
#endif
        parser.setFileName(filePath);
        parser.setPattern(m_pattern, m_path);
        if (filePath.endsWith(".zip", Qt::CaseInsensitive) && !m_settings.dbSkipValidation()) {
            archive.setArchiveFile(filePath);
            if (!archive.isValidKaraokeFile()) {
                errors.append(archive.getLastError() + ": " + filePath);
                continue;
            }
        }
        if (!m_settings.dbLazyLoadDurations())
            duration = parser.getDuration();
        artist = parser.getArtist();
        title = parser.getTitle();
        discid = parser.getSongId();
        if (artist == "" && title == "" && discid == "") {
            // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
            if (m_pattern != SourceDir::METADATA) {
                parser.setPattern(SourceDir::METADATA, m_path);
                artist = parser.getArtist();
                title = parser.getTitle();
                discid = parser.getSongId();
            }
            // If we still don't have any metadata, just throw filename into the title field
            if (artist == "" && title == "" && discid == "")
                title = fileInfo.completeBaseName();
        }
        fName = fileInfo.completeBaseName();
        searchString = QString(fName + " " + artist + " " + title + " " + discid);
        query.bindValue(":discid", discid);
        query.bindValue(":artist", artist);
        query.bindValue(":title", title);
        query.bindValue(":path", filePath);
        query.bindValue(":filename", fName);
        query.bindValue(":duration", duration);
        query.bindValue(":searchstring", searchString);
        query.exec();
        if (loops % 5 == 0) {
            emit progressChanged(loops + 1);
            emit stateChanged(
                    "Validating karaoke files and getting song durations... " + QString::number(loops + 1) + " of " +
                    QString::number(newSongs.size()));
        }
        loops++;
    }
    qInfo() << "Done looping";
    qInfo() << "Committing transaction";
    query.exec("COMMIT");
    emit progressMessage("Done processing new files.");
    if (!errors.empty()) {
        emit errorsGenerated(errors);
    }
}

void DbUpdater::setPath(const QString &value) {
    m_path = value;
}

bool DbUpdater::isSupportedMediaFile(const QString &filePath) {
    std::array<QString, 7> supportedVideoExtensions{".mp4", ".mkv", ".avi", ".wmv", ".m4v", ".mpg", ".mpeg"};
    if (filePath.endsWith(".zip", Qt::CaseInsensitive) ||
        (filePath.endsWith(".cdg", Qt::CaseInsensitive) && !findMatchingAudioFile(filePath).isEmpty()))
        return true;
    bool vidMatch = std::any_of(supportedVideoExtensions.begin(), supportedVideoExtensions.end(),
                                [filePath](const auto &val) {
                                    return filePath.endsWith(val, Qt::CaseInsensitive);
                                });
    if (vidMatch)
        return true;
    return false;
}

void DbUpdater::fixMissingFiles(QStringList &existingFiles) {
    QStringList missingFiles = getMissingDbFiles();
    QSqlQuery query;
    emit stateChanged("Detecting and updating moved files...");
    emit progressMaxChanged(missingFiles.size());
    int count = 0;
    qInfo() << "Looking for missing files";
    query.exec("BEGIN TRANSACTION");
    for (const auto &missingFile : missingFiles) {
        QApplication::processEvents();
        emit progressMessage("Looking for matches to missing db song: " + missingFile);
        qInfo() << "Looking for match for missing file: " << missingFile;
        bool matchFound = false;
        QString newFile;
        query.prepare("UPDATE dbsongs SET path = :newpath WHERE path = :oldpath");
        for (int i = 0; i < existingFiles.size(); i++) {
            QApplication::processEvents();
            newFile = existingFiles.at(i);
            if (QFileInfo(existingFiles.at(i)).fileName() == QFileInfo(missingFile).fileName()) {
                query.bindValue(":newpath", newFile);
                query.bindValue(":oldpath", missingFile);
                query.exec();
                qInfo() << "Missing file found at new location";
                qInfo() << "  old: " << missingFile;
                qInfo() << "  new: " << newFile;
                existingFiles.removeAt(i);
                emit progressMessage("Found match! Modifying existing song.");
                matchFound = true;
                break;
            }
        }
        if (!matchFound)
                emit progressMessage("No match found");
        count++;
        emit progressChanged(count);
    }
    query.exec("COMMIT");
}

void DbUpdater::importDragDropSongs(QStringList &existingFiles) {
    QStringList dragDropFiles = getDragDropFiles();
    QSqlQuery query;
    query.prepare(
            "UPDATE dbsongs SET discid = :discid, artist = :artist, title = :title, filename = :filename, duration = :duration, searchstring = :searchstring WHERE path = :path");
    qInfo() << "Processing dragged/dropped files";
    for (int f = 0; f < dragDropFiles.size(); f++) {
        QApplication::processEvents();
        const auto &dropFile = dragDropFiles.at(f);
        qInfo() << "Looking for matches for drop file: " << dropFile;
        QString artist;
        QString title;
        QString discid;
        QString filePath;
        QString fileName;
        int duration;
        QString searchString;
        for (int i = 0; i < existingFiles.size(); i++) {
            QApplication::processEvents();
            if (existingFiles.at(i) == dropFile) {
                qInfo() << "Found match for drop file: " << dropFile;
                QFileInfo file(dropFile);
                filePath = file.filePath();
                fileName = file.completeBaseName();
                KaraokeFileInfo parser;
                parser.setFileName(dropFile);
                parser.setPattern(m_pattern, m_path);
                artist = parser.getArtist();
                title = parser.getTitle();
                discid = parser.getSongId();
                duration = parser.getDuration();

                if (artist == "" && title == "" && discid == "") {
                    // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
                    if (m_pattern != SourceDir::METADATA) {
                        parser.setPattern(SourceDir::METADATA, m_path);
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
                existingFiles.removeAt(i);
                break;
            }
        }
    }
}

QStringList DbUpdater::getErrors() {
    return errors;
}

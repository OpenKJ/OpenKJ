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

#include "dbupdater.h"
#include <array>
#include <QSqlQuery>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QApplication>
#include "models/tablemodelkaraokesourcedirs.h"
#include "mzarchive.h"
#include "karaokefileinfo.h"

// Checks to see if a given path currently exists in the database
// Does include songs marked bad, but not songs dropped via drag and drop
bool DbUpdater::dbEntryExists(const QString &filepath, bool includeDropped) {
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

// Given a cdg file path, tries to find a matching supported audio file
// Returns an empty QString if no match is found
// Optimized for finding most common file extensions first
QString DbUpdater::findMatchingAudioFile(const QString &cdgFilePath) {
    std::array<QString, 41> audioExtensions{
        "mp3",
        "MP3",
        "wav",
        "WAV",
        "ogg",
        "OGG",
        "mov",
        "MOV",
        "flac",
        "FLAC",
        "Mp3","mP3",
        "Wav","wAv","waV","WAv","wAV","WaV",
        "Ogg","oGg","ogG","OGg","oGG","OgG",
        "Mov","mOv","moV","MOv","mOV","MoV",
        "Flac","fLac","flAc","flaC","FLac","FLAc",
        "flAC","fLAC","FlaC", "FLaC", "FlAC"
    };
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

// Set the naming pattern to be used when processing the current path
void DbUpdater::setPattern(SourceDir::NamingPattern value) {
    m_pattern = value;
}

// Finds all potential supported karaoke files in a given directory
QStringList DbUpdater::findKaraokeFiles(const QString &directory) {
    qInfo() << "DbUpdater::findKaraokeFiles(" << directory << ") called";
    QStringList files;
    emit progressMessage("Finding karaoke files in " + directory);
    files.reserve(200000);
    QDir dir(directory);
    int existing{0};
    int notInDb{0};
    int total{0};
    int loops{0};
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        QApplication::processEvents();
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            total++;
            if (isSupportedMediaFile(iterator.filePath())) {
                if (dbEntryExists(iterator.filePath())) {
                    existing++;
                } else {
                    files.append(iterator.filePath());
                    notInDb++;
                }
            }
        }
        if (loops >= 10) {
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

// Checks all files in the db to see if they still exist
// Returns a list of any that it determines are missing
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

// Returns a list of all of the files added to the db via drag & drop
QStringList DbUpdater::getDragDropFiles() {
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from dbsongs WHERE discid = '!!DROPPED!!'");
    while (query.next()) {
        files.append(query.value("path").toString());
    }
    return files;
}

// Adds a single media file to the database.
// Typically used for files purchased from the song shop,
// or files added through the directory watch feature.
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
}

// Adds the given media file path to the database as a drag and dropped file
// Returns the db ID of the created entry
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
        return query.lastInsertId().toInt();
    }
    return -1;
}

// Process files and do database update on the current directory
void DbUpdater::process() {
    emit progressChanged(0);
    emit progressMaxChanged(0);
    emit stateChanged("Finding potential karaoke files...");
    QStringList newSongs = findKaraokeFiles(m_path);
    fixMissingFiles(newSongs);
    importDragDropSongs(newSongs);
    qInfo() << "Adding new songs";
    emit progressMaxChanged(newSongs.size());
    emit progressMessage("Found " + QString::number(newSongs.size()) + " potential karaoke files.");
    emit progressMessage("Importing new files into the karaoke database...");
    emit stateChanged("Importing new files into the karaoke database...");    QSqlQuery query;
    query.exec("PRAGMA synchronous=OFF");
    query.exec("PRAGMA cache_size=500000");
    query.exec("PRAGMA temp_store=2");
    query.exec("BEGIN TRANSACTION");
    query.prepare(
            "INSERT OR IGNORE INTO dbSongs (discid,artist,title,path,filename,duration,searchstring) VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)");
    MzArchive archive;
    KaraokeFileInfo parser;
    QFileInfo fileInfo;
    int loops{0};
    for (const auto &filePath : newSongs) {
        QApplication::processEvents();
        int duration{-2};
        fileInfo.setFile(filePath);
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
        parser.getMetadata();
        if (!parser.parseSuccess()) {
            // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
            if (m_pattern != SourceDir::METADATA) {
                parser.setPattern(SourceDir::METADATA, m_path);
                parser.getMetadata();
            }
        }
        if (!m_settings.dbLazyLoadDurations())
            duration = parser.getDuration();
        if (filePath.endsWith(".zip", Qt::CaseInsensitive) && !m_settings.dbSkipValidation()) {
            archive.setArchiveFile(filePath);
            if (!archive.isValidKaraokeFile()) {
                m_errors.append(archive.getLastError() + ": " + filePath);
                continue;
            }
        }
        query.bindValue(":discid", parser.getSongId());
        query.bindValue(":artist", parser.getArtist());
        // If metadata parse wasn't successful, just put the filename in the title field
        query.bindValue(":title", (parser.parseSuccess()) ? parser.getTitle() : fileInfo.completeBaseName());
        query.bindValue(":path", filePath);
        query.bindValue(":filename", fileInfo.completeBaseName());
        query.bindValue(":duration", duration);
        // searchString contains the metadata plus the basename to work around people's libraries that are
        // misnamed and don't import properly or who use media tags and have bad tags.
        query.bindValue(":searchstring", fileInfo.completeBaseName() + " " + parser.getArtist() + " " + parser.getTitle() + " " + parser.getSongId());
        query.exec();
        if (loops % 10 == 0) {
            emit progressChanged(loops + 1);
            emit stateChanged(
                    "Importing new files into the karaoke database... " + QString::number(loops + 1) + " of " +
                    QString::number(newSongs.size()));
        }
        loops++;
    }
    query.exec("COMMIT");
    emit progressMessage("Done processing new files.");
    if (!m_errors.empty()) {
        emit errorsGenerated(m_errors);
    }
}

// Set the current path for processing
void DbUpdater::setPath(const QString &value) {
    m_path = value;
}

// Checks the provided file path to see whether it's a file type that's supported by OpenKJ
// Returns true if supported, false otherwise
bool DbUpdater::isSupportedMediaFile(const QString &filePath) {
    const std::array<std::string, 9> extensions{
        "zip",
        "mp4",
        "cdg",
        "mkv",
        "avi",
        "wmv",
        "m4v",
        "mpg",
        "mpeg"
    };
    const std::string ext = QFileInfo(filePath).suffix().toLower().toStdString();
    return std::any_of(
            extensions.begin(),
            extensions.end(),
            [&ext,&filePath](const auto &val) {
                if (val == "cdg")
                {
                    return (val == ext && !findMatchingAudioFile(filePath).isEmpty());
                }
                return (ext == val);
            }
            );
}

// Given a list of files found on disk, checks them against files that are
// currently missing to determine if they've just been moved.  For any that have
// been determined to have moved, the existing db entry is updated with the new path
// and the entry is removed from the provided existing files list.
void DbUpdater::fixMissingFiles(QStringList &existingFiles) {
    auto missingFiles = getMissingDbFiles();
    QSqlQuery query;
    emit stateChanged("Detecting and updating missing and moved files...");
    emit progressMaxChanged(missingFiles.size());
    int count{0};
    qInfo() << "Looking for missing files";
    query.exec("BEGIN TRANSACTION");
    for (const auto &missingFile : missingFiles) {
        QApplication::processEvents();
        emit progressMessage("Looking for matches to missing db song: " + missingFile);
        qInfo() << "Looking for match for missing file: " << missingFile;
        bool matchFound{false};
        query.prepare("UPDATE dbsongs SET path = :newpath WHERE path = :oldpath");
        for (int i{0}; i < existingFiles.size(); i++) {
            QApplication::processEvents();
            if (QFileInfo(existingFiles.at(i)).fileName() == QFileInfo(missingFile).fileName()) {
                query.bindValue(":newpath", existingFiles.at(i));
                query.bindValue(":oldpath", missingFile);
                query.exec();
                qInfo() << "Missing file found at new location";
                qInfo() << "  old: " << missingFile;
                qInfo() << "  new: " << existingFiles.at(i);
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

// Checks to see if any files in the provided list of existing files match
// drag & drop entries in the database.  Converts the entries to normal db
// entries if they match.  Any matches will be removed from the provided
// existing files list
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
        QString searchString;
        for (int i = 0; i < existingFiles.size(); i++) {
            QApplication::processEvents();
            if (existingFiles.at(i) == dropFile) {
                qInfo() << "Found match for drop file: " << dropFile;
                QFileInfo file(dropFile);
                KaraokeFileInfo parser;
                parser.setFileName(dropFile);
                parser.setPattern(m_pattern, m_path);
                parser.getMetadata();
                if (!parser.parseSuccess()) {
                    // Something went wrong, no metadata found. File is probably named wrong. If we didn't try media tags, give it a shot
                    if (m_pattern != SourceDir::METADATA) {
                        parser.setPattern(SourceDir::METADATA, m_path);
                        parser.getMetadata();
                    }
                }
                searchString = QString(file.completeBaseName() + " " + parser.getArtist() + " " + parser.getTitle() + " " + parser.getSongId());
                query.bindValue(":discid", parser.getSongId());
                query.bindValue(":artist", parser.getArtist());
                query.bindValue(":title", (parser.parseSuccess()) ? parser.getTitle() : file.completeBaseName());
                query.bindValue(":path", file.filePath());
                query.bindValue(":filename", file.completeBaseName());
                query.bindValue(":duration", parser.getDuration());
                query.bindValue(":searchstring", searchString);
                query.exec();
                existingFiles.removeAt(i);
                break;
            }
        }
    }
}

// Returns a list of errors encountered while processing
QStringList DbUpdater::getErrors() {
    return m_errors;
}

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

#define SQL(...) #__VA_ARGS__
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

// TODO: remove?
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

DbUpdater::DbUpdater(QObject *parent) :
        QObject(parent) {
}

// Finds all potential supported karaoke files in a given directory
void DbUpdater::findKaraokeFilesOnDisk() {

    // cdg and zip files
    QStringList karaoke_files;
    // audio files to match with cdg files
    QStringList audio_files;
    karaoke_files.reserve(200000);
    audio_files.reserve(200000);

    int existing{0};
    int notInDb{0};
    int total{0};
    int loops{0};

    foreach(auto path, m_paths ) {

        emit progressMessage("Finding karaoke files in " + path);
        QDir dir(path);
        QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            if (!iterator.fileInfo().isDir()) {
                total++;
                const std::string ext = iterator.fileInfo().suffix().toLower().toStdString();

                if (std::binary_search(karaoke_file_extensions.begin(), karaoke_file_extensions.end(), ext)) {
                    karaoke_files.append(iterator.filePath());
                }
                else if (std::binary_search(audio_file_extensions.begin(), audio_file_extensions.end(), ext)) {
                    const QString filePath = iterator.filePath();
                    audio_files.append(filePath.left(filePath.lastIndexOf('.')));
                }
            }
            if (loops++ % 10 == 0) {
                emit stateChanged("Finding potential karaoke files... " + QString::number(total) + " found. " +
                                  QString::number(notInDb) + " new/" + QString::number(existing) + " existing");
                QApplication::processEvents();
            }
        }
        emit stateChanged(
                "Finding potential karaoke files... " + QString::number(total) + " found. " + QString::number(notInDb) +
                " new/" + QString::number(existing) + " existing");
    //    qInfo() << "File search results - Potential files: " << files.size() << " - Already in DB: " << existing
    //            << " - New: " << notInDb;
    }
    emit progressMessage("Done searching for files.");

    m_karaokeFilesOnDisk = karaoke_files;
    m_audioFilesOnDisk = audio_files;
}

// Checks all files in the db to see if they still exist
// Returns a list of any that it determines are missing
QStringList DbUpdater::getMissingDbFiles() {
    QStringList files;
    QSqlQuery query;
    query.exec("SELECT path from dbsongs");
    while (query.next()) {
        QString path = query.value("path").toString();
        if (!QFileInfo::exists(path)) {
            files.append(path);
        }
    }
    return files;
}

/*
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
*/

// Adds a single media file to the database.
// Typically used for files purchased from the song m_songShop,
// or files added through the directory watch feature.
void DbUpdater::addSingleTrack(const QString &filePath) {
    // TODO: athom
    /*MzArchive archive;
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
    query.exec();*/
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

// Process files and do database update on the current directory.
// Constraint: access the filesystem as little as possible to optimize performance for NAS scenarios.
// Strategy: read list of files to memory and compare list to database to find added or removed files.
void DbUpdater::process(const QList<QString> &paths, bool handleMissingFiles)
{
    m_paths = QStringList(paths);
    m_paths.sort();

    emit progressChanged(0);
    emit progressMaxChanged(0);

    emit stateChanged("Scanning disk for files...");
    findKaraokeFilesOnDisk();

    emit stateChanged("Sorting files...");
    m_karaokeFilesOnDisk.sort();
    m_audioFilesOnDisk.sort();

    // todo:
    //fixMissingFiles(newSongs);
    //importDragDropSongs(newSongs);

    qInfo() << "Checking for new songs";
    emit progressMaxChanged(MAX(m_karaokeFilesOnDisk.count(), m_audioFilesOnDisk.count()));
    //emit progressMessage("Found " + QString::number(newSongs.size()) + " potential karaoke files.");
    emit progressMessage("Importing new files into the karaoke database...");
    emit stateChanged("Importing new files into the karaoke database...");

    int i_kar = -1;
    int i_aud = 0;

    QStringList newSongs; newSongs.reserve(20000);
    QVector<int> nonExistentSongIds; nonExistentSongIds.reserve(20000);

    QSqlQuery dbSongs;
    QStringList sql_path_filter;
    for(int i=0; i<m_paths.size(); i++) {
        sql_path_filter.append(QString("path LIKE :pathfilter%1").arg(i));
    }
    dbSongs.setForwardOnly(true);
    dbSongs.prepare("SELECT songid, path, CASE discid WHEN '!!DROPPED!!' THEN 1 ELSE 0 END FROM dbsongs WHERE " + sql_path_filter.join(" OR ") + " ORDER BY path");
    for(int i=0; i<m_paths.size(); i++) {
        auto key = QString(":pathfilter%1").arg(i);
        dbSongs.bindValue(key, getPathWithTrailingSeparator(m_paths[i]) + "%");
    }
    dbSongs.exec();

    QString disk_path, db_path = nullptr;
    bool db_is_dropped_file = false;
    int run = 0;

    do {
        // Comparison result:
        //   when negative: file is on disk but not in database.
        //   when positive: file is in database but not on disk.
        //   when 0: file is on disk AND in databse.
        int comp_result = 0;

        if (run > 0) {
            if (disk_path != nullptr && db_path != nullptr) {
                comp_result = QString::compare(disk_path, db_path);
            }
            else {
                comp_result = (int)(db_path != nullptr) - (int)(disk_path != nullptr);
            }

            if (comp_result < 0) {
                newSongs.append(disk_path);
            }

            if (comp_result > 0) {
                nonExistentSongIds.append(dbSongs.value(0).toInt());
            }

            if (comp_result == 0 && db_is_dropped_file) {
                // Add drag'n'dropped files to the list of new songs so they
                // will be properly added (upserted) to the database.
                // TODO: needs testing - I can't make drag'n'drop work currently.../athom
                newSongs.append(disk_path);
            }
        }

        if (comp_result <= 0) {
            // move to next file on disk
            bool invalid_file_found;
            do {
                i_kar++;
                disk_path = i_kar < m_karaokeFilesOnDisk.size() ? m_karaokeFilesOnDisk.at(i_kar) : nullptr;

                if (disk_path != nullptr && disk_path.endsWith(".cdg", Qt::CaseInsensitive)) {

                    // File type is "cdg" and is only valid if there is an audio file with the same filename.
                    // Look for an entry with the same filename in the list of audio files.
                    invalid_file_found = true;
                    const QStringRef disk_path_without_ext = QStringRef(&disk_path, 0, disk_path.length() - 4);

                    while (i_aud < m_audioFilesOnDisk.size()) {
                        int comp_result_audio = disk_path_without_ext.compare(m_audioFilesOnDisk.at(i_aud));
                        if (comp_result_audio == 0) {
                            // match found!
                            invalid_file_found = false;
                            i_aud++;
                            break;
                        }
                        if (comp_result_audio < 0) {
                            // no match found...
                            break;
                        }

                        // keep looking - advance to the next audio file in the list
                        i_aud++;
                    }
                }
                else {
                    invalid_file_found = false;
                }

            } while (invalid_file_found);
        }

        if (comp_result >= 0) {
            // move to next database record
            if (dbSongs.next()) {
                db_path = dbSongs.value(1).toString();
                db_is_dropped_file = dbSongs.value(2).toBool();
            }
            else {
                db_path = nullptr;
                db_is_dropped_file = false;
            }
        }
        run++;
    }
    while (disk_path != nullptr || db_path != nullptr);


    QSqlQuery query;
    query.exec("PRAGMA synchronous=OFF");
    query.exec("PRAGMA cache_size=500000");
    query.exec("PRAGMA temp_store=2");
    query.exec("BEGIN TRANSACTION");
    query.prepare(SQL(
            INSERT INTO dbSongs (discid, artist, title, path, filename, duration, searchstring)
            VALUES(:discid, :artist, :title, :path, :filename, :duration, :searchstring)
            ON CONFLICT(path) DO UPDATE SET
                discid = :discid,
                artist = :artist,
                title = :title,
                filename = :filename,
                duration = :duration,
                searchstring = :searchstring
           ));
    MzArchive archive;
    // TODO: maybe just call the default constructor...
    auto patternResolver = std::make_shared<KaraokeFilePatternResolver>();
    KaraokeFileInfo parser(this, patternResolver);
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
            m_errors.append("Illegal character in filename: " + filePath);
            emit progressMessage("Illegal character in filename: " + filePath);
            emit progressChanged(loops + 1);
            continue;
        }
#endif
        parser.setFile(filePath);

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
        query.bindValue(":title", (parser.parsedSuccessfully()) ? parser.getTitle() : fileInfo.completeBaseName());
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

QString DbUpdater::getPathWithTrailingSeparator(const QString &path) {
    return path.endsWith(QDir::separator())
            ? path
            : path + QDir::separator();
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

/*// Checks to see if any files in the provided list of existing files match
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
*/

// Returns a list of errors encountered while processing
QStringList DbUpdater::getErrors() {
    return m_errors;
}

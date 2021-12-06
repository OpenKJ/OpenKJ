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

DbUpdater::DbUpdater(QObject *parent) :
        QObject(parent) {
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
// Constraint: access the filesystem as little as possible to optimize performance for slow file access/network.
// Strategy: read list of files to memory and compare list to database to find added or removed files.
bool DbUpdater::process(const QList<QString> &paths, ProcessingOptions options)
{
    // Make sure only one dbupdater runs at a time.
    // Even though the program is primarily single threaded, exessive use of
    // QApplication::processEvents can cause reentrant calls.

    static std::mutex mutex;
    if (!mutex.try_lock()) {
        m_errors.append("Scanner already running");
        return false;
    }
    const std::lock_guard<std::mutex> locker(mutex, std::adopt_lock);

    m_missingFilesSongIds.clear();
    setPaths(paths);

    emit stateChanged("Scanning disk for files...");
    DiskEnumerator diskEnumerator(*this);
    diskEnumerator.findKaraokeFilesOnDisk();

    emit stateChanged("Scanning database for files...");
    QApplication::processEvents();
    DbEnumerator dbEnumerator(*this);
    dbEnumerator.prepareQuery(!options.testFlag(FixMovedFilesSearchInWholeDB));

    emit stateChanged("Checking files against database...");
    qInfo() << "Checking for new songs";
    int progressMax = MAX(diskEnumerator.count(), dbEnumerator.count());

    QStringList newFilesOnDisk; newFilesOnDisk.reserve(20000);
    QVector<DbSongRecord> filesMissingOnDisk;
    bool keepTrackOfMissig = options.testFlag(FixMovedFiles) || options.testFlag(PrepareForRemovalOfMissing);

    int run = 0;
    do {
        // Comparison result:
        //   when negative: file is on disk but not in database.
        //   when positive: file is in database but not on disk.
        //   when 0: file is on disk AND in databse.
        int comp_result = 0;

        if (run > 0) {
            if (diskEnumerator.IsValid && dbEnumerator.IsValid) {
                comp_result = QString::compare(diskEnumerator.CurrentFile, dbEnumerator.CurrentRecord.path);
            }
            else {
                comp_result = (int)(dbEnumerator.IsValid) - (int)(diskEnumerator.IsValid);
            }

            if (comp_result < 0) {
                newFilesOnDisk.append(diskEnumerator.CurrentFile);
            }

            if (comp_result > 0 && keepTrackOfMissig) {
                filesMissingOnDisk.append(dbEnumerator.CurrentRecord);
            }

            if (comp_result == 0 && dbEnumerator.CurrentRecord.isDropped) {
                // Add drag'n'dropped files to the list of new songs so they
                // will be properly added (upserted) to the database.
                newFilesOnDisk.append(diskEnumerator.CurrentFile);
            }
        }

        if (comp_result <= 0) {
            diskEnumerator.readNextDiskFile();
        }

        if (comp_result >= 0) {
            dbEnumerator.readNextRecord();
        }
        run++;
        if (shouldUpdateGui()) {
            emit progressChanged(run, progressMax);
            QApplication::processEvents();
        }
    }
    while (diskEnumerator.IsValid || dbEnumerator.IsValid);


    if (options.testFlag(FixMovedFiles) && !newFilesOnDisk.empty() && !filesMissingOnDisk.empty()) {
        fixMissingFiles(filesMissingOnDisk, newFilesOnDisk);
    }

    addFilesToDatabase(newFilesOnDisk);

    if (options.testFlag(PrepareForRemovalOfMissing)) {
        m_missingFilesSongIds.reserve(filesMissingOnDisk.size());
        foreach(const auto &rec, filesMissingOnDisk) {
            m_missingFilesSongIds.append(rec.id);
        }
    }

    return true;
}

void DbUpdater::addFilesToDatabase(const QList<QString> &files)
{
    if (files.empty())
        return;

    emit stateChanged("Adding new files to database...")    ;

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
    KaraokeFileInfo parser(this);
    QFileInfo fileInfo;
    int loops = 0;

    for (const auto &filePath : files) {
        loops++;
        int duration{-2};
        fileInfo.setFile(filePath);
#ifdef Q_OS_WIN
        if (filePath.contains("*") || filePath.contains("?") || filePath.contains("<") || filePath.contains(">") || filePath.contains("|"))
        {
            // illegal character
            m_errors.append("Illegal character in filename: " + filePath);
            emit progressMessage("Illegal character in filename: " + filePath);
            emit progressChanged(loops, files.length());
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
        if (shouldUpdateGui()) {
            emit progressChanged(loops, files.length());
            //emit stateChanged(QString("Importing new files into the karaoke database... %1 of %2").arg(loops).arg(files.length()));
            QApplication::processEvents();
        }
    }
    query.exec("COMMIT");

    emit progressMessage("Done processing new files.");

    if (!m_errors.empty()) {
        emit errorsGenerated(m_errors);
    }
}

int DbUpdater::missingFilesCount()
{
    return m_missingFilesSongIds.length();
}

void DbUpdater::removeMissingFilesFromDatabase()
{
    if (m_missingFilesSongIds.empty())
        return;

    emit stateChanged("Removing missing files from database...");

    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.prepare("DELETE FROM dbSongs WHERE [songid] = :id");

    foreach(const int id, m_missingFilesSongIds) {
        query.bindValue(":id", id);
        query.exec();
    }

    query.exec("DELETE FROM queueSongs WHERE [song] NOT IN (SELECT [songid] FROM dbSongs)");
    query.exec("DELETE FROM regularSongs WHERE [songid] NOT IN (SELECT [songid] FROM dbSongs)");

    query.exec("COMMIT");
    m_missingFilesSongIds.clear();
}

// Finds all potential supported karaoke files in a given directory
void DbUpdater::DiskEnumerator::findKaraokeFilesOnDisk() {

    emit m_parent.progressChanged(0, 0);

    // cdg and zip files
    QStringList karaoke_files;
    // audio files to match with cdg files
    QStringList audio_files;
    karaoke_files.reserve(200000);
    audio_files.reserve(200000);

    foreach(auto path, m_parent.m_paths ) {
        emit m_parent.stateChanged("Finding karaoke files in " + path);
        QApplication::processEvents();

        int foundInPath = 0;
        QDir dir(path);
        QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            if (!iterator.fileInfo().isDir()) {
                const std::string ext = iterator.fileInfo().suffix().toLower().toStdString();

                if (std::binary_search(m_parent.karaoke_file_extensions.begin(), m_parent.karaoke_file_extensions.end(), ext)) {
                    karaoke_files.append(iterator.filePath());
                    foundInPath++;
                }
                else if (std::binary_search(m_parent.audio_file_extensions.begin(), m_parent.audio_file_extensions.end(), ext)) {
                    const QString filePath = iterator.filePath();
                    audio_files.append(filePath.left(filePath.lastIndexOf('.')));
                }
            }
            if (m_parent.shouldUpdateGui()) {
                emit m_parent.stateChanged(QString("Scanning %1\n    %2 found, %3 total...")
                                  .arg(path)
                                  .arg(foundInPath)
                                  .arg(karaoke_files.length()));
                QApplication::processEvents();
            }
        }
    }

    emit m_parent.stateChanged("Sorting...");
    QApplication::processEvents();

    karaoke_files.sort();
    audio_files.sort();

    emit m_parent.stateChanged("Done searching for files.");

    m_karaokeFilesOnDisk = karaoke_files;
    m_audioFilesOnDisk = audio_files;
}

void DbUpdater::DiskEnumerator::readNextDiskFile()
{
    bool invalid_file_found;
    do {
        m_i_kar++;
        CurrentFile = m_i_kar < m_karaokeFilesOnDisk.size() ? m_karaokeFilesOnDisk.at(m_i_kar) : nullptr;

        if (CurrentFile != nullptr && CurrentFile.endsWith(".cdg", Qt::CaseInsensitive)) {

            // File type is "cdg" and is only valid if there is an audio file with the same filename.
            // Look for an entry with the same filename in the list of audio files.
            invalid_file_found = true;
            const QStringRef disk_path_without_ext = QStringRef(&CurrentFile, 0, CurrentFile.length() - 4);

            while (m_i_aud < m_audioFilesOnDisk.size()) {
                int comp_result_audio = disk_path_without_ext.compare(m_audioFilesOnDisk.at(m_i_aud));
                if (comp_result_audio == 0) {
                    // match found!
                    invalid_file_found = false;
                    m_i_aud++;
                    break;
                }
                if (comp_result_audio < 0) {
                    // no match found...
                    break;
                }

                // keep looking - advance to the next audio file in the list
                m_i_aud++;
            }
        }
        else {
            invalid_file_found = false;
        }

    } while (invalid_file_found);
    IsValid = CurrentFile != nullptr;
}

void DbUpdater::DbEnumerator::prepareQuery(bool limitToPaths)
{
    if (!limitToPaths) {
        m_dbSongs.prepare("SELECT songid, path, CASE discid WHEN '!!DROPPED!!' THEN 1 ELSE 0 END FROM dbsongs ORDER BY path");
    }
    else {
        QStringList sql_path_filter;
        for(int i = 0; i < m_parent.m_paths.size(); i++) {
            sql_path_filter.append(QString("path LIKE :pathfilter%1").arg(i));
        }

        m_dbSongs.prepare("SELECT songid, path, CASE discid WHEN '!!DROPPED!!' THEN 1 ELSE 0 END FROM dbsongs WHERE " + sql_path_filter.join(" OR ") + " ORDER BY path");
        for(int i = 0; i < m_parent.m_paths.size(); i++) {
            auto key = QString(":pathfilter%1").arg(i);
            m_dbSongs.bindValue(key, m_parent.m_paths[i] + "%");
        }
    }
    m_dbSongs.exec();

    // trick to do count in SQlite:
    m_count = 0;
    if(m_dbSongs.last())
    {
        m_count =  m_dbSongs.at() + 1;
        m_dbSongs.first();
        m_dbSongs.previous();
    }
}

void DbUpdater::DbEnumerator::readNextRecord()
{
    if ((IsValid = m_dbSongs.next())) {
        CurrentRecord = DbSongRecord {
            .id =        m_dbSongs.value(0).toInt(),
            .isDropped = m_dbSongs.value(2).toBool(),
            .path =      m_dbSongs.value(1).toString()
        };
    }
}

void DbUpdater::setPaths(const QList<QString> &paths)
{
    // Normalize paths:
    //  * Ensure ending separator
    //  * Remove any subpath as parent paths are scanned recursively
    m_paths = QStringList();
    foreach(auto path, paths) {
        m_paths << (path.endsWith("/")
                ? path
                : path + "/");
    }

    m_paths.sort();

    int i=1;
    while (i < m_paths.length()) {
        if (m_paths[i].startsWith(m_paths[i-1]) && m_paths[i].length() > m_paths[i-1].length()) {
            m_paths.removeAt(i);
        }
        else {
            i++;
        }
    }
}

// Given a list of files found on disk, checks them against files that are
// currently missing to determine if they've just been moved or or their caps changed.  For any that have
// been determined to have moved, the existing db entry is updated with the new path
// and the entry is removed from the provided existing files list.
void DbUpdater::fixMissingFiles(QVector<DbSongRecord> &filesMissingOnDisk, QStringList &newFilesOnDisk) {

    emit stateChanged("Detecting and updating missing or moved files...");

    int count{0};
    qInfo() << "Looking for missing files";

    // Strategy: create new list of only the filenames (without paths) of all the new files found.
    //   Instead of creating new strings, use QStringRef of the full path string.
    //   Use that new, sorted list as a lookup table for the missing files in the database.

    // Keep a copy of "newFilesOnDisk" so the QStrings are not destructed, causing QStringRefs to fail.
    QStringList newFilesOnDiskCopy(newFilesOnDisk);
    QVector<QStringRef> filesOnDiskFilenamesOnlySorted;
    filesOnDiskFilenamesOnlySorted.reserve(newFilesOnDiskCopy.size());
    foreach(const QString &s, newFilesOnDiskCopy) {
        int filenameBeginsAt = s.lastIndexOf('/') + 1;
        filesOnDiskFilenamesOnlySorted.append(QStringRef(&s, filenameBeginsAt, s.length() - filenameBeginsAt));
    }

    // Sort the list case insensitive
    auto caseInsensitiveSort = [](const QStringRef &a, const QString &b) -> bool { return QStringRef::compare(a, b, Qt::CaseInsensitive) < 0; };
    auto caseInsensitiveSortStringRef = [](const QStringRef &a, const QStringRef &b) -> bool { return QStringRef::compare(a, b, Qt::CaseInsensitive) < 0; };

    std::sort(filesOnDiskFilenamesOnlySorted.begin(), filesOnDiskFilenamesOnlySorted.end(), caseInsensitiveSortStringRef);

    // Copy records that are still missing to a new list instead of removing them from filesMissingOnDisk. It's faster that way.
    QVector<DbSongRecord> filesMissingOnDisk_still;
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.prepare("UPDATE dbsongs SET path = :newpath WHERE songid = :id");

    foreach(auto missingFile, filesMissingOnDisk) {

        emit progressMessage("Looking for matches to missing db song: " + missingFile.path + "...");
        qInfo() << "Looking for match for missing file: " << missingFile.path;

        QApplication::processEvents();

        bool matchFound = false;
        auto filenameWithoutPath = QFileInfo(missingFile.path).fileName();
        auto const lb = std::lower_bound(filesOnDiskFilenamesOnlySorted.begin(), filesOnDiskFilenamesOnlySorted.end(), filenameWithoutPath, caseInsensitiveSort);
        if (lb->compare(filenameWithoutPath, Qt::CaseInsensitive) == 0) {
            query.bindValue(":newpath", *lb->string());
            query.bindValue(":id", missingFile.id);

            if (query.exec()) {
                emit progressMessage("Found match! Modifying existing song.");
                qInfo() << "Missing file found at new location";
                qInfo() << "  old: " << missingFile.path;
                qInfo() << "  new: " << lb->string();

                newFilesOnDisk.removeOne(*lb->string());
                matchFound = true;
            }
            else {
                qInfo() << "Error updating database: " << query.lastError();
            }
        }

        if (!matchFound) {
            filesMissingOnDisk_still.append(missingFile);
            emit progressMessage("No match found");
        }
        count++;
        if (shouldUpdateGui()) {
            emit progressChanged(count, filesMissingOnDisk.size());
        }
    }
    query.exec("COMMIT");
    filesMissingOnDisk = filesMissingOnDisk_still;
}

bool DbUpdater::shouldUpdateGui()
{
    if (!m_guiUpdateTimer.isValid())
        m_guiUpdateTimer.start();

    if (m_guiUpdateTimer.elapsed() > 200) {
        m_guiUpdateTimer.restart();
        return true;
    }
    return false;
}

// Returns a list of errors encountered while processing
QStringList DbUpdater::getErrors() {
    return m_errors;
}


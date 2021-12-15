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

#ifndef DBUPDATER_H
#define DBUPDATER_H

#include <QObject>
#include <QStringList>
#include <QtSql>
#include "src/models/tablemodelkaraokesourcedirs.h"
#include "settings.h"
#include <array>


class DbUpdater : public QObject
{
    Q_OBJECT

private:

    struct DbSongRecord {
       int id{-1};
       bool isDropped{false};
       QString path;
    };

    // file extension list must be sorted and in lower case:
    const std::array<std::string, 9> karaoke_file_extensions {
        "avi",
        "cdg",
        "m4v",
        "mkv",
        "mp4",
        "mpeg",
        "mpg",
        "wmv",
        "zip"
    };

    std::array<std::string, 5> audio_file_extensions {
        "flac",
        "mov",
        "mp3",
        "ogg",
        "wav"
    };

    class DiskEnumerator
    {
    private:
        DbUpdater& m_parent;
        QStringList m_karaokeFilesOnDisk;
        QStringList m_audioFilesOnDisk;
        int m_i_kar{-1};
        int m_i_aud{0};

    public:
        bool IsValid{false};
        QString CurrentFile;
        explicit DiskEnumerator(DbUpdater& parent) : m_parent(parent) { reset(); }
        void findKaraokeFilesOnDisk();
        void readNextDiskFile();
        void reset() { m_i_kar = -1; m_i_aud = 0; IsValid = false; }
        int count() { return m_karaokeFilesOnDisk.length(); }
    };

    class DbEnumerator
    {
    private:
        DbUpdater& m_parent;
        QSqlQuery m_dbSongs;
        int m_count{0};

    public:
        bool IsValid = false;
        DbSongRecord CurrentRecord;
        explicit DbEnumerator(DbUpdater& parent) : m_parent(parent) {}
        void prepareQuery(bool limitToPaths);
        void readNextRecord();
        [[nodiscard]] int count() const { return m_count; }
    };

    Settings m_settings;
    QStringList m_paths;
    QStringList m_errors;
    QVector<int> m_missingFilesSongIds;
    QElapsedTimer m_guiUpdateTimer;

    void setPaths(const QList<QString> &paths);
    void fixMissingFiles(QVector<DbSongRecord> &filesMissingOnDisk, QStringList &newFilesOnDisk);
    bool shouldUpdateGui();

public:

    enum ProcessingOption
    {
        None                          = 0x0,
        FixMovedFiles                 = 0x01,
        FixMovedFilesSearchInWholeDB  = FixMovedFiles | 0x02,
        PrepareForRemovalOfMissing    = 0x04
    };
    Q_DECLARE_FLAGS(ProcessingOptions, ProcessingOption)

    explicit DbUpdater(QObject *parent = nullptr);

    QStringList getErrors();
    bool process(const QList<QString> &paths, ProcessingOptions options);
    void addFilesToDatabase(const QList<QString> &files);
    int missingFilesCount();
    void removeMissingFilesFromDatabase();

signals:
    void errorsGenerated(QStringList);
    void progressMessage(const QString &msg);
    void stateChanged(QString state);
    void progressChanged(int progress, int max);

};

Q_DECLARE_OPERATORS_FOR_FLAGS(DbUpdater::ProcessingOptions)

#endif // DBUPDATER_H

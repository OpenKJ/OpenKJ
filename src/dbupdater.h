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


class DbUpdater : public QObject
{
    Q_OBJECT

private:

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

    QString m_path;
    Settings m_settings;
    QStringList m_errors;
    QStringList m_karaokeFilesOnDisk;
    QStringList m_audioFilesOnDisk;
    void fixMissingFiles(QStringList &existingFiles);
//    void importDragDropSongs(QStringList &existingFiles);
    void findKaraokeFilesOnDisk();
    void findKaraokeFilesInDB();
    QString getPathWithTrailingSeparator();

public:
    explicit DbUpdater(QObject *parent = nullptr);
    void setPath(const QString &value);

    static QStringList getMissingDbFiles();
    //static QStringList getDragDropFiles();
    QStringList getErrors();
    void addSingleTrack(const QString& filePath);
    static int addDroppedFile(const QString& filePath);
    void process();
    static bool dbEntryExists(const QString &filepath, bool includeDropped = false);

signals:
    void errorsGenerated(QStringList);
    void progressMessage(QString msg);
    void stateChanged(QString state);
    void progressChanged(int progress);
    void progressMaxChanged(int max);

};

#endif // DBUPDATER_H

/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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


#ifndef OKARCHIVE_H
#define OKARCHIVE_H

#include <QObject>
#include <QStringList>
#include <QProcess>

struct zipEntry
{
    QString fileName;
    int fileSize;
};

typedef QList<zipEntry> zipEntries;

class OkArchive : public QObject
{
    Q_OBJECT
public:
    explicit OkArchive(QString ArchiveFile, QObject *parent = 0);
    explicit OkArchive(QObject *parent = 0);
    ~OkArchive();
    unsigned int getSongDuration();
    QByteArray getCDGData();
    QString getArchiveFile() const;
    void setArchiveFile(const QString &value);
    bool checkCDG();
    bool checkAudio();
    QString audioExtension();
    bool extractAudio(QString destPath, QString destFile);
    bool extractCdg(QString destPath, QString destFile);
    bool isValidKaraokeFile();
    QString getLastError();

private:
    QString archiveFile;
    QString cdgFileName;
    QString audioFileName;
    QString audioExt;
    QString lastError;
    bool findCDG();
    bool findAudio();
    int cdgSize();
    int audioSize();
    int m_cdgSize;
    int m_audioSize;
    bool m_cdgFound;
    bool m_audioFound;
    bool m_entriesProcessed;
    zipEntries m_entries;
    bool findEntries();
    QStringList audioExtensions;
    zipEntries getZipContents();
    bool extractFile(QString fileName, QString destDir, QString destFile);
    bool zipIsValid();
    QProcess *process;
    bool goodArchive;

signals:

public slots:
};

#endif // KHARCHIVE_H

/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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


#ifndef KHARCHIVE_H
#define KHARCHIVE_H

#include <QObject>
//#include <quazip.h>


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
    bool checkMP3();
    bool extractMP3(QString destPath);

private:
    QString archiveFile;
    QString cdgFileName;
    QString mp3FileName;
    bool findCDG();
    bool findMp3();
    int cdgSize();
    int mp3Size();
    int m_cdgSize;
    int m_mp3Size;
    bool m_cdgFound;
    bool m_mp3Found;

signals:

public slots:
};

#endif // KHARCHIVE_H

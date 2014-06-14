/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#ifndef KHZIP_H
#define KHZIP_H

#include <QObject>
#include <QDir>
//#ifdef Q_OS_WIN
//#define ZLIB_WINAPI
//#endif
//extern "C" {
//    #include <unzip.h>
//}

//class KhZip : public QObject
//{
//    Q_OBJECT
//private:
//    QString m_zipFile;
//public:
//    explicit KhZip(QString zipFile, QObject *parent = 0);
//    explicit KhZip(QObject *parent = 0);
//    ~KhZip();
//    bool extractMp3(QDir destDir);
//    bool extractCdg(QDir destDir);

//    QString zipFile() const;
//    void setZipFile(const QString &zipFile);
//    int getSongDuration();
//    unz_file_info *fileInfo;

//signals:

//public slots:

//};

class KhZip : public QObject
{
    Q_OBJECT
private:
    QString m_zipFile;
public:
    explicit KhZip(QString zipFile, QObject *parent = 0);
    explicit KhZip(QObject *parent = 0);
    //~KhZip2();
    bool extractMp3(QDir destDir);
    bool extractCdg(QDir destDir);

    QString zipFile() const;
    void setZipFile(const QString &zipFile);
    int getSongDuration();

signals:

public slots:

};

#endif // KHZIP_H

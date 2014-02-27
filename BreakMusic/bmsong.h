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

#ifndef KHBMSONG_H
#define KHBMSONG_H

#include <QObject>
#include <vector>
#include <QStringList>

class BmSong : public QObject
{
    Q_OBJECT
public:
    explicit BmSong(QObject *parent = 0);
    BmSong(int songIndex, QObject *parent = 0);

    QString artist() const;
    void setArtist(const QString &artist);

    QString title() const;
    void setTitle(const QString &title);

    QString path() const;
    void setPath(const QString &path);

    QString filename() const;
    void setFilename(const QString &filename);

    QString getSearchableString() const;

    int duration() const;
    void setDuration(int duration);

    QString durationStr();

    int index() const;
    void setIndex(int index);

signals:
    
public slots:

private:
    QString m_artist;
    QString m_title;
    QString m_path;
    QString m_filename;
    int m_duration;
    int m_index;
    
};

class BmSongs : public QObject
{
    Q_OBJECT
public:
    explicit BmSongs(QObject *parent = 0);
    ~BmSongs();
    void loadFromDB();
    void setFilterTerms(QStringList terms);
    void clear();
    BmSong *at(int vectorIndex);
    BmSong *getSongByIndex(int songIndex);
    unsigned int size();
signals:
    void dataAboutToChange();
    void dataChanged();

public slots:

private:
    QList<BmSong *> *allSongs;
    QList<BmSong *> *filteredSongs;
    QStringList filterTerms;
};

#endif // KHBMSONG_H

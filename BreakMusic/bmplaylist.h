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

#ifndef BmPLAYLIST_H
#define BmPLAYLIST_H

#include <QObject>
#include <bmsong.h>


class BmPlaylistSong : public QObject
{
    Q_OBJECT
public:
    explicit BmPlaylistSong(QObject *parent=0);
    unsigned int position() const;
    void setPosition(unsigned int position, bool skipDb = false);
    BmSong *song() const;
    void setSong(BmSong *song);

    unsigned int index() const;
    void setIndex(unsigned int index);
    bool valid() const;
    void setValid(bool valid);

signals:

public slots:

private:
    unsigned int m_position;
    BmSong *m_song;
    unsigned int m_index;
    bool m_valid;
};


class BmPlaylist : public QObject
{
    Q_OBJECT
public:
    explicit BmPlaylist(QObject *parent = 0);
    ~BmPlaylist();
    void loadSongs();
    unsigned int size();
    BmPlaylistSong *at(int vectorPos);
    BmPlaylistSong *getCurrentSong();
    BmPlaylistSong *getNextSong();
    BmPlaylistSong *getSongByPosition(unsigned int position);
    void insertSong(BmSong *song, unsigned int position);
    void insertSong(int songid, unsigned int position);
    void addSong(BmSong *song);
    void moveSong(unsigned int oldPosition, unsigned int newPosition);
    void moveSongAfterCurrent(int oldPos);
    void removeSong(unsigned int position);
    unsigned int plIndex() const;
    void setPlIndex(unsigned int plIndex);
    void setCurrentSongByPosition(int position);
    QString title() const;
    void setTitle(const QString &title);
    void next();
    void sort();


signals:
    void dataAboutToChange();
    void dataChanged();
public slots:

private:
    QList<BmPlaylistSong *> *songs;
    unsigned int m_plIndex;
    QString m_title;
    BmPlaylistSong *m_currentSong;
    
};


class BmPlaylists : public QObject
{
    Q_OBJECT
public:
    explicit BmPlaylists(QObject *parent = 0);
    ~BmPlaylists();
    /// Add playlist and return plIndex of new playlist or zero on failure
    unsigned int addPlaylist(QString title);
    /// Remove playlist by plIndex
    void removePlaylist(int plIndex);
    /// Remove playlist by playlist title.
    void removePlaylist(QString title);
    /// Return pointer to the current playlist as set by setCurrent(int)
    BmPlaylist *getCurrent();
    /// Return pointer to the playlist matching plIndex
    BmPlaylist *getByIndex(unsigned int plIndex);
    /// Return pointer to the playlist matching plTitle
    BmPlaylist *getByTitle(QString plTitle);
    /// Set the currently active playlist
    void setCurrent(int plIndex);
    /// Set the currently active playlist
    void setCurrent(QString plTitle);
    /// Get a pointer to the playlist at vector position vectorIndex.  This is
    /// only really meant to be used when iterating over the playlists.  This is
    /// not based on the plIndex
    BmPlaylist *at(int vectorIndex);
    /// Return the number of playlists
    unsigned int size();
    /// Check to see if a playlist title exists
    bool exists(QString title);
    /// Return a QStringList of the playlist titles
    QStringList getTitleList();

signals:
    void dataAboutToChange();
    void dataChanged();
    void playlistsChanged();
    void currentPlaylistChanged(QString playlistTitle);

public slots:

private:
    QList<BmPlaylist *> *playlists;
    /// Load in the playlists from the database
    void loadFromDB();
    BmPlaylist *currentPlaylist;
};

#endif // BmPLAYLIST_H

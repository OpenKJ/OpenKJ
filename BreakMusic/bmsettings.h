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

#ifndef BMSETTINGS_H
#define BMSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QWidget>
#include <QTableView>
#include <QTreeView>
#include <QSplitter>

class BmSettings : public QObject
{
    Q_OBJECT
public:
    explicit BmSettings(QObject *parent = 0);
    void saveWindowState(QWidget *window);
    void restoreWindowState(QWidget *window);
    bool showFilenames();
    void setShowFilenames(bool show);
    bool showMetadata();
    void setShowMetadata(bool show);
    int volume();
    void setVolume(int volume);
    int playlistIndex();
    void setPlaylistIndex(int index);
    void saveColumnWidths(QTreeView *treeView);
    void saveColumnWidths(QTableView *tableView);
    void restoreColumnWidths(QTreeView *treeView);
    void restoreColumnWidths(QTableView *tableView);
    void saveSplitterState(QSplitter *splitter);
    void restoreSplitterState(QSplitter *splitter);

signals:

public slots:

private:
    QSettings *settings;


};

#endif // BMSETTINGS_H

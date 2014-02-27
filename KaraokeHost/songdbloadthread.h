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

#ifndef SONGDBLOADTHREAD_H
#define SONGDBLOADTHREAD_H

#include <QThread>
#include <boost/shared_ptr.hpp>
#include "khsong.h"

class SongDBLoadThread : public QThread
{
    Q_OBJECT
public:
    explicit SongDBLoadThread(KhSongs *songsVectorPointer, QObject *parent = 0);
    void run();
//    void setSharedPointer(boost::shared_ptr<)

signals:
    
public slots:
    
private:
    KhSongs *songs;
};

#endif // SONGDBLOADTHREAD_H

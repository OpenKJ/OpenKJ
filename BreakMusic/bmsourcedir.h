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

#ifndef KHBMSOURCEDIR_H
#define KHBMSOURCEDIR_H

#include <QObject>
#include <boost/shared_ptr.hpp>
#include <vector>

class BmSourceDir : public QObject
{
    Q_OBJECT
public:
    explicit BmSourceDir(QObject *parent = 0);
    BmSourceDir(QString path, QObject *parent = 0);
    BmSourceDir(QString path, int index, QObject *parent = 0);
    QString getPath() const;
    void setPath(const QString &value);
    int getIndex() const;
    void setIndex(int value);

signals:
    
public slots:
    
private:
    int m_index;
    QString m_path;
};

class BmSourceDirs : public QObject
{
    Q_OBJECT
public:
    explicit BmSourceDirs(QObject *parent = 0);
    int size();
    boost::shared_ptr<BmSourceDir> at(int vectorPos);
    bool add(QString path);
    void deleteByIndex(int index);
    void deleteByPath(QString path);
    void debugPrintEntries();
signals:

public slots:

private:
    std::vector<boost::shared_ptr<BmSourceDir> > srcDirs;
    void loadFromDB();
};
#endif // KHBMSOURCEDIR_H

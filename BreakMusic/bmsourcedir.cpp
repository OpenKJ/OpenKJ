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

#include "bmsourcedir.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

BmSourceDir::BmSourceDir(QObject *parent) :
    QObject(parent)
{
    m_index = -1;
}

BmSourceDir::BmSourceDir(QString path, QObject *parent) :
    QObject(parent)
{
    m_path = path;
    m_index = -1;
}

BmSourceDir::BmSourceDir(QString path, int index, QObject *parent) :
    QObject(parent)
{
    m_path = path;
    m_index = index;
}

int BmSourceDir::getIndex() const
{
    return m_index;
}

void BmSourceDir::setIndex(int value)
{
    m_index = value;
}

QString BmSourceDir::getPath() const
{
    return m_path;
}

void BmSourceDir::setPath(const QString &value)
{
    m_path = value;
}


int BmSourceDirs::size()
{
    return srcDirs->size();
}

BmSourceDir *BmSourceDirs::at(int vectorPos)
{
    return srcDirs->at(vectorPos);
}

bool BmSourceDirs::add(QString path)
{
    QSqlQuery query;
    if (query.exec("INSERT INTO srcdirs (path) VALUES(\"" + path + "\")"))
    {
        int index = query.lastInsertId().toInt();
        srcDirs->push_back(new BmSourceDir(path,index,this));
        return true;
    }
    else
        return false;
}

void BmSourceDirs::deleteByIndex(int index)
{
    QSqlQuery query;
    query.exec("DELETE FROM srcdirs WHERE ROWID == " + QString::number(index));
    loadFromDB();
}

void BmSourceDirs::deleteByPath(QString path)
{
    QSqlQuery query;
    query.exec("DELETE FROM srcdirs WHERE path == " + path);
    loadFromDB();
}

void BmSourceDirs::debugPrintEntries()
{
    for (int i=0; i < srcDirs->size(); i++)
        qDebug() << "Src Path: " << srcDirs->at(i)->getPath();
}

void BmSourceDirs::loadFromDB()
{
    qDeleteAll(srcDirs->begin(),srcDirs->end());
    srcDirs->clear();
    QSqlQuery query("SELECT ROWID,path FROM srcDirs ORDER BY path");
    int sourcedirid = query.record().indexOf("ROWID");
    int path = query.record().indexOf("path");
    while (query.next()) {
//        BmSourceDir *dir = new BmSourceDir();
//        dir->setIndex(query.value(sourcedirid).toInt());
//        dir->setPath(query.value(path).toString());
//        srcDirs->push_back(dir);
        srcDirs->push_back(new BmSourceDir(query.value(path).toString(),query.value(sourcedirid).toInt()));
    }
}


BmSourceDirs::BmSourceDirs(QObject *parent) :
    QObject(parent)
{
    srcDirs = new QList<BmSourceDir *>;
    loadFromDB();
}

BmSourceDirs::~BmSourceDirs()
{
    qDeleteAll(srcDirs->begin(),srcDirs->end());
    delete srcDirs;
}

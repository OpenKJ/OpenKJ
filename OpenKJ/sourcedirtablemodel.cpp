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

#include "sourcedirtablemodel.h"
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>

#define UNUSED(x) (void)x


SourceDirTableModel::SourceDirTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    mydata = new QList<SourceDir *>;
}

SourceDirTableModel::~SourceDirTableModel()
{
    qDeleteAll(mydata->begin(),mydata->end());
    delete mydata;
}

int SourceDirTableModel::rowCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return mydata->size();
}

int SourceDirTableModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return 2;
}

QVariant SourceDirTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= mydata->size() || index.row() < 0)
        return QVariant();
    QSqlQuery query;
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case PATH:
            return mydata->at(index.row())->getPath();
        case PATTERN:
            switch (mydata->at(index.row())->getPattern())
            {
            case SourceDir::SAT:
                return QString("SongID - Artist - Title");
            case SourceDir::STA:
                return QString("SongID - Title - Artist");
            case SourceDir::ATS:
                return QString("Artist - Title - SongID");
            case SourceDir::TAS:
                return QString("Title - Artist - SongID");
            case SourceDir::AT:
                return QString("Artist - Title");
            case SourceDir::TA:
                return QString("Title - Artist");
            case SourceDir::S_T_A:
                return QString("SongID_Title_Artist");
            case SourceDir::METADATA:
                return QString("Media Tags");
            case SourceDir::CUSTOM:
                QString customName;
                query.exec("SELECT name FROM custompatterns WHERE patternid == " + QString::number(mydata->at(index.row())->getCustomPattern()));
                if (query.first())
                    customName = query.value("name").toString();
                return QString("Custom: " + customName);
            }
        }
    }
    return QVariant();
}

QVariant SourceDirTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case PATH:
            return tr("Path");
        case PATTERN:
            return tr("Pattern");
        }
    }
    return QVariant();
}

Qt::ItemFlags SourceDirTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SourceDirTableModel::loadFromDB()
{
    mydata->clear();
    QSqlQuery query("SELECT ROWID,path,pattern,custompattern FROM sourceDirs ORDER BY path");
    while (query.next()) {
        SourceDir *dir = new SourceDir();
        dir->setIndex(query.value("ROWID").toInt());
        dir->setPath(query.value("path").toString());
        dir->setPattern(static_cast<SourceDir::NamingPattern>(query.value("pattern").toInt()));
        dir->setCustomPattern(query.value("custompattern").toInt());
        addSourceDir(dir);
    }
}

int SourceDir::getIndex() const
{
    return index;
}

void SourceDir::setIndex(int value)
{
    index = value;
}

QString SourceDir::getPath() const
{
    return path;
}

void SourceDir::setPath(const QString &value)
{
    path = value;
}

SourceDir::NamingPattern SourceDir::getPattern() const
{
    return pattern;
}

void SourceDir::setPattern(SourceDir::NamingPattern value)
{
    pattern = value;
}


void SourceDirTableModel::addSourceDir(SourceDir *dir)
{
    if(std::find(mydata->begin(),mydata->end(),dir) != mydata->end())
        return;
    beginInsertRows(QModelIndex(),mydata->size(),mydata->size());
    mydata->push_back(dir);
    endInsertRows();
}

void SourceDirTableModel::addSourceDir(QString dirpath, int pattern, int customPattern = 0)
{
    layoutAboutToBeChanged();
    QSqlQuery query;
    query.prepare("INSERT INTO sourceDirs (path,pattern,custompattern) VALUES(:path,:pattern,:custompattern)");
    query.bindValue(":path", dirpath);
    query.bindValue(":pattern", pattern);
    query.bindValue(":custompattern", customPattern);
    query.exec();
    loadFromDB();
    layoutChanged();
}

void SourceDirTableModel::delSourceDir(int index)
{
    int dbid = mydata->at(index)->getIndex();
    QSqlQuery query;
    query.exec("DELETE FROM sourceDirs WHERE ROWID == " + QString::number(dbid));
    layoutAboutToBeChanged();
    loadFromDB();
    layoutChanged();
}

int SourceDirTableModel::size()
{
    return mydata->size();
}


SourceDir *SourceDirTableModel::getDirByIndex(int index)
{
    return mydata->at(index);
}

SourceDir *SourceDirTableModel::getDirByPath(QString path)
{
    loadFromDB();
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();
    while (dir.absolutePath() != dir.rootPath())
    {
        for (int i=0; i<mydata->size(); i++)
        {
            if (mydata->at(i)->getPath() == dir.absolutePath())
            {
                qInfo() << "Match found - " << mydata->at(i)->getPath() << " - " << mydata->at(i)->getPattern();
                return mydata->at(i);
            }
        }
        dir.cdUp();
    }
    qInfo() << "No Match Found";
    return new SourceDir();
}

QStringList SourceDirTableModel::getSourceDirs()
{
    QStringList dirs;
    for (int i=0; i < mydata->size(); i++)
    {
        dirs.append(mydata->at(i)->getPath());
    }
    return dirs;
}

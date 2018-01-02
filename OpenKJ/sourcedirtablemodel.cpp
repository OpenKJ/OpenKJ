/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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
#include <QSqlQuery>
#include <QSqlRecord>

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
            case SourceDir::DAT:
                return QString("DiscID - Artist - Title");
            case SourceDir::DTA:
                return QString("DiscID - Title - Artist");
            case SourceDir::ATD:
                return QString("Artist - Title - DiscID");
            case SourceDir::TAD:
                return QString("Title - Artist - DiscID");
            case SourceDir::AT:
                return QString("Artist - Title");
            case SourceDir::TA:
                return QString("Title - Artist");
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
        dir->setPattern(query.value("pattern").toInt());
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

int SourceDir::getPattern() const
{
    return pattern;
}

void SourceDir::setPattern(int value)
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
    query.exec("INSERT INTO sourceDirs (path,pattern,custompattern) VALUES('" + dirpath + "'," + QString::number(pattern) + "," + QString::number(customPattern) + ")");
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

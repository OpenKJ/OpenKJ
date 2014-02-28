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

#ifndef SOURCEDIRTABLEMODEL_H
#define SOURCEDIRTABLEMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>

class SourceDir
{
public:
    enum {DAT=0,DTA,ATD,TAD,AT,TA};

    int getPattern() const;
    void setPattern(int value);

    QString getPath() const;
    void setPath(const QString &value);

    int getIndex() const;
    void setIndex(int value);

private:
    int index;
    QString path;
    int pattern;
};

class SourceDirTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SourceDirTableModel(QObject *parent = 0);
    ~SourceDirTableModel();
    enum {PATH=0,PATTERN};
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void addSourceDir(QString dirpath, int pattern);
    void delSourceDir(int index);
    int size();


    void loadFromDB();
    QSqlDatabase *getDBObject() const;
    void setDBObject(QSqlDatabase *value);
    void clear();
    SourceDir *getDirByIndex(int index);

private:
    QList<SourceDir *> *mydata;
    void addSourceDir(SourceDir *dir);
    
signals:
    
public slots:
    
};

#endif // SOURCEDIRTABLEMODEL_H

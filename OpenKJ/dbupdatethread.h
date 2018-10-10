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

#ifndef DBUPDATETHREAD_H
#define DBUPDATETHREAD_H

#include <QThread>
#include <QStringList>
#include <QtSql>
#include "sourcedirtablemodel.h"

class DbUpdateThread : public QThread
{
    Q_OBJECT

private:
    QString path;
    SourceDir::NamingPattern pattern;
    //QSqlDatabase database;
    QSqlDatabase database;


public:
    explicit DbUpdateThread(QSqlDatabase tdb, QObject *parent = 0);
    void run();
    QString getPath() const;
    void setPath(const QString &value);
    int getPattern() const;
    void setPattern(SourceDir::NamingPattern value);
    QStringList findKaraokeFiles(QString directory);
    QStringList getMissingDbFiles();
    QStringList getDragDropFiles();
    QStringList getErrors();
    void addSingleTrack(QString path);
    int addDroppedFile(QString path);
    void startUnthreaded();
    bool dbEntryExists(QString filepath);

signals:
    void threadFinished();
    void errorsGenerated(QStringList);
    void progressMessage(QString msg);
    void stateChanged(QString state);
    void progressChanged(int progress);
    void progressMaxChanged(int max);
    void databaseAboutToUpdate();
    void databaseUpdateComplete();

};

#endif // DBUPDATETHREAD_H

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

#ifndef DLGDURATIONSCAN_H
#define DLGDURATIONSCAN_H

#include <QDialog>
#include "khdb.h"
#include <QStringList>
#include "okarchive.h"
#include <ThreadWeaver/ThreadWeaver>
#include <QSqlQuery>
#include <QTimer>


class DurationUpdater : public ThreadWeaver::Job
{
public:
    DurationUpdater(QString filename) : m_fileName(filename) {}
protected:
    void run(ThreadWeaver::JobPointer, ThreadWeaver::Thread*);
private:
    QString m_fileName;
};


namespace Ui {
class DlgDurationScan;
}

class DlgDurationScan : public QDialog
{
    Q_OBJECT

private:
    QStringList findNeedUpdateSongs();
    int numUpdatesNeeded();
    Ui::DlgDurationScan *ui;
    OkArchive archiveFile;
    bool stopProcessing;
    bool queueing;
    KhDb db;
    ThreadWeaver::Queue *threadQueue;
    QSqlQuery *m_query;
    QTimer *uiUpdateTimer;
    int needed;
    int processed;
    bool transactionStarted;

public:
    explicit DlgDurationScan(QWidget *parent = 0);
    ~DlgDurationScan();

private slots:
    void on_buttonClose_clicked();
    void on_buttonStop_clicked();
    void on_buttonStart_clicked();
    void on_durationProcessed(int duration, QString filePath);
    void on_uiUpdateTimer();

};



#endif // DLGDURATIONSCAN_H



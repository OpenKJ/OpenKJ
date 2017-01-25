/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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

#include "dlgdurationscan.h"
#include "ui_dlgdurationscan.h"
#include <QDebug>
#include <QSqlQuery>
#include <QFile>


DlgDurationScan::DlgDurationScan(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDurationScan)
{
    ui->setupUi(this);
    ui->lblTotal->setText(QString::number(numUpdatesNeeded()));
    ui->buttonStop->setDisabled(true);
    connect(&watcher, SIGNAL(progressRangeChanged(int,int)), ui->progressBar, SLOT(setRange(int,int)));
    connect(&watcher, SIGNAL(progressValueChanged(int)), this, SLOT(progressValueChanged(int)));
    connect(&watcher, SIGNAL(progressRangeChanged(int,int)), this, SLOT(progressRangeChanged(int,int)));
    connect(&watcher, SIGNAL(finished()), this, SLOT(processingComplete()));
    connect(&watcher, SIGNAL(canceled()), this, SLOT(processingStopped()));
    connect(&watcher, SIGNAL(paused()), this, SLOT(processingPaused()));
}

DlgDurationScan::~DlgDurationScan()
{
    delete ui;
}

void DlgDurationScan::on_buttonClose_clicked()
{
    close();
}

void DlgDurationScan::on_buttonStop_clicked()
{
    watcher.cancel();
}

void DlgDurationScan::on_buttonStart_clicked()
{
    needDurationFiles = findNeedUpdateSongs();
    db.beginTransaction();
    ui->buttonStop->setDisabled(false);
    ui->buttonStart->setDisabled(true);
    watcher.setFuture(QtConcurrent::map(needDurationFiles, &DlgDurationScan::getDuration));
}

void DlgDurationScan::progressRangeChanged(int min, int max)
{
    if (watcher.isRunning())
    {
        ui->lblTotal->setText(QString::number(max));
        ui->progressBar->setRange(min,max);
    }
}

void DlgDurationScan::processingComplete()
{
    db.endTransaction();
    ui->lblProcessed->setText("0");
    ui->lblTotal->setText(QString::number(numUpdatesNeeded()));
    ui->progressBar->setValue(0);
}

void DlgDurationScan::processingStopped()
{
    db.endTransaction();
    ui->lblProcessed->setText("0");
    ui->lblTotal->setText(QString::number(numUpdatesNeeded()));
    ui->progressBar->setValue(0);
    ui->buttonStart->setDisabled(false);
    ui->buttonStop->setDisabled(true);
}

void DlgDurationScan::processingPaused()
{

}

void DlgDurationScan::processingStarted()
{
    ui->buttonStart->setDisabled(true);
    ui->buttonStop->setDisabled(false);
}

void DlgDurationScan::progressValueChanged(int progress)
{
    ui->lblProcessed->setText(QString::number(progress));
    ui->progressBar->setValue(progress);
}

QStringList DlgDurationScan::findNeedUpdateSongs()
{
    QSqlQuery query;
    QString sql = "SELECT path FROM dbsongs WHERE duration IS NULL ORDER BY discid";
    QStringList nullDurations;
    query.exec(sql);
    while (query.next())
    {
        nullDurations << query.value(0).toString();
    }
    return nullDurations;
}

int DlgDurationScan::numUpdatesNeeded()
{
    int needed = -1;
    QSqlQuery query;
    query.exec("SELECT COUNT(*) FROM dbsongs WHERE duration IS NULL");
    while (query.next())
    {
        needed = query.value(0).toInt();
    }
    return needed;
}

QMutex mutex;
void DlgDurationScan::getDuration(const QString filename)
{
    if (filename.endsWith(".zip", Qt::CaseInsensitive))
    {

        OkArchive archiveFile(filename);
        int duration = archiveFile.getSongDuration();
        if (duration == -1)
        {
            qWarning() << "Unable to get duration for file: " << filename;
            return;
        }
        mutex.lock();
        QSqlQuery query;
        query.exec("UPDATE dbsongs SET duration = " + QString::number(duration) + " WHERE path == \"" + filename + "\"");
        mutex.unlock();
    }
    else if (filename.endsWith(".cdg", Qt::CaseInsensitive))
    {
        QFile cdgFile(filename);
        int size = cdgFile.size();
        int duration = ((size / 96) / 75) * 1000;
        mutex.lock();
        QSqlQuery query;
        query.exec("UPDATE dbsongs SET duration = " + QString::number(duration) + " WHERE path == \"" + filename + "\"");
        mutex.unlock();
    }
}

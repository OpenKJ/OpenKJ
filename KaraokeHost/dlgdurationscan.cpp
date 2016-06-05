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
#include <QApplication>
#include <QFile>


DlgDurationScan::DlgDurationScan(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDurationScan)
{
    ui->setupUi(this);
    queueing = false;
    transactionStarted = false;
    stopProcessing = false;
    threadQueue = new ThreadWeaver::Queue(this);
    uiUpdateTimer = new QTimer(this);
    uiUpdateTimer->start(500);
    needed = numUpdatesNeeded();
    processed = 0;
    ui->lblTotal->setText(QString::number(needed));
    connect(uiUpdateTimer, SIGNAL(timeout()), this, SLOT(on_uiUpdateTimer()));
}

DlgDurationScan::~DlgDurationScan()
{
    delete ui;
}

void DlgDurationScan::on_buttonClose_clicked()
{
    stopProcessing = true;
    close();
}

void DlgDurationScan::on_buttonStop_clicked()
{
    threadQueue->dequeue();
    stopProcessing = true;
}

void DlgDurationScan::on_buttonStart_clicked()
{
    stopProcessing = false;
    QStringList needDurationFiles = findNeedUpdateSongs();
    qDebug() << "Found " << needDurationFiles.size() << " songs that need duration";
    needed = needDurationFiles.size();
    processed = 0;
    ui->lblTotal->setText(QString::number(needDurationFiles.size()));
    db.beginTransaction();
    queueing = true;
    for (int i=0; i < needDurationFiles.size(); i++)
    {
        QApplication::processEvents();
        if (needDurationFiles.at(i).endsWith(".zip", Qt::CaseInsensitive))
            threadQueue->stream() << new DurationUpdaterZip(needDurationFiles.at(i));
        else if (needDurationFiles.at(i).endsWith(".cdg", Qt::CaseInsensitive))
            threadQueue->stream() << new DurationUpdaterCdg(needDurationFiles.at(i));
        QApplication::processEvents();
        transactionStarted = true;
        if (stopProcessing)
        {
            stopProcessing = false;
            break;
        }
    }
    queueing = false;
//    db.endTransaction();

}

void DlgDurationScan::on_durationProcessed(int duration, QString filepath)
{
    processed++;
    QSqlQuery query;
    query.exec("UPDATE dbsongs SET duration = " + QString::number(duration) + " WHERE path == \"" + filepath + "\"");

}

void DlgDurationScan::on_uiUpdateTimer()
{
    if ((!threadQueue->isEmpty()) && (!queueing))
    {
        processed = needed - threadQueue->queueLength();
        ui->lblProcessed->setText(QString::number(processed));
        ui->progressBar->setValue(((float)processed / (float)needed) * 100);
    }
    else
    {
        if (transactionStarted)
        {
            db.endTransaction();
            transactionStarted = false;
            needed = numUpdatesNeeded();
            processed = 0;
            ui->lblProcessed->setText(QString::number(processed));
            ui->lblTotal->setText(QString::number(needed));
            ui->progressBar->setValue(((float)processed / (float)needed) * 100);
        }
    }
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

void DurationUpdaterZip::run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *)
{
    OkArchive archiveFile(m_fileName);
    int duration = archiveFile.getSongDuration();
    if (duration == -1)
    {
        qWarning() << "Unable to get duration for file: " << m_fileName;
        return;
    }
    QSqlQuery query;
    query.exec("UPDATE dbsongs SET duration = " + QString::number(duration) + " WHERE path == \"" + m_fileName + "\"");
}

void DurationUpdaterCdg::run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *)
{
    QFile cdgFile(m_fileName);
    int size = cdgFile.size();
    int duration = ((size / 96) / 75) * 1000;
    QSqlQuery query;
    query.exec("UPDATE dbsongs SET duration = " + QString::number(duration) + " WHERE path == \"" + m_fileName + "\"");
}

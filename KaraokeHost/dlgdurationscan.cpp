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

#include "dlgdurationscan.h"
#include "ui_dlgdurationscan.h"
#include <QDebug>
#include <QSqlQuery>
#include <QApplication>


DlgDurationScan::DlgDurationScan(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDurationScan)
{
    ui->setupUi(this);
    stopProcessing = false;
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
    stopProcessing = true;
}

void DlgDurationScan::on_buttonStart_clicked()
{
    stopProcessing = false;
    QStringList needDurationFiles = findNeedUpdateSongs();
    qDebug() << "Found " << needDurationFiles.size() << " songs that need duration";
    ui->lblTotal->setText(QString::number(needDurationFiles.size()));
    db.beginTransaction();
    QSqlQuery query;
    for (int i=0; i < needDurationFiles.size(); i++)
    {
        QApplication::processEvents();
        zip.setZipFile(needDurationFiles.at(i));
        int duration = zip.getSongDuration();
        QApplication::processEvents();
        query.exec("UPDATE dbsongs SET duration = " + QString::number(duration) + " WHERE path == \"" + needDurationFiles.at(i) + "\"");
        ui->progressBar->setValue(((float)(i + 1) / (float)needDurationFiles.size()) * 100);
        ui->lblProcessed->setText(QString::number(i));
        QApplication::processEvents();
        if (stopProcessing)
        {
            stopProcessing = false;
            break;
        }
    }
    db.endTransaction();

}

QStringList DlgDurationScan::findNeedUpdateSongs()
{
    QSqlQuery query;
    QString sql = "SELECT path FROM dbsongs WHERE duration IS NULL";
    QStringList nullDurations;
    query.exec(sql);
    while (query.next())
    {
        nullDurations << query.value(0).toString();
    }
    return nullDurations;
}

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

#include "dlgregularexport.h"
#include "ui_dlgregularexport.h"
#include <QFileDialog>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>
#include <QXmlStreamWriter>
#include <QSqlQuery>
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

DlgRegularExport::DlgRegularExport(TableModelKaraokeSongs &karaokeSongsModel, QWidget *parent) :
        m_karaokeSongsModel(karaokeSongsModel),
        QDialog(parent),
    ui(new Ui::DlgRegularExport)
{
    ui->setupUi(this);
    ui->tableViewRegulars->setModel(&m_historySingersModel);
    ui->tableViewRegulars->hideColumn(0);
    ui->tableViewRegulars->hideColumn(2);
    ui->tableViewRegulars->hideColumn(3);
    ui->tableViewRegulars->hideColumn(4);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
}

DlgRegularExport::~DlgRegularExport()
{
    qInfo() << "dlgregularexport destructor called";
    delete ui;
}

void DlgRegularExport::on_pushButtonClose_clicked()
{
    close();
}

void DlgRegularExport::on_pushButtonExport_clicked()
{
    qInfo() << "export - export button pressed";
    auto rowIndexes = ui->tableViewRegulars->selectionModel()->selectedRows();
    if (rowIndexes.size() == 0)
        return;
    std::vector<int> historySingerIds;
    std::for_each(rowIndexes.begin(), rowIndexes.end(), [&historySingerIds] (QModelIndex index) {
        historySingerIds.emplace_back(index.data().toInt());
        qInfo() << "export - singer id" << index.data().toInt();
    });
    qInfo() << "export - done looping over singer ids";
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "KhRegularSingersExport.json";
    qInfo() << "export - Default save location: " << defaultFilePath;
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select file to save regulars to"), defaultFilePath, "JSON files (*.json)", nullptr, QFileDialog::DontUseNativeDialog);
    qInfo() << "export - file select dialog should have shown";
    if (saveFilePath != "")
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setStandardButtons(QFlags<QMessageBox::StandardButton>());
        msgBox->setText(tr("Exporting regular singers, please wait..."));
        msgBox->show();
        exportSingers(historySingerIds, saveFilePath);
        msgBox->close();
        delete msgBox;
        QMessageBox::information(this, tr("Export complete"), tr("Regular singer export complete."));
        ui->tableViewRegulars->clearSelection();
    }
}

void DlgRegularExport::on_pushButtonExportAll_clicked()
{
    std::vector<int> singerIds;
    auto singers = m_historySingersModel.singers();
    std::for_each(singers.begin(), singers.end(), [&singerIds] (HistorySinger singer) {
        singerIds.emplace_back(singer.historySingerId);
    });
    if (singerIds.size() > 0)
    {
        QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "KhRegularSingersExport.json";
        qDebug() << "Default save location: " << defaultFilePath;
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select file to save regulars to"), defaultFilePath, "JSON Files (*.json)", nullptr, QFileDialog::DontUseNativeDialog);
        if (saveFilePath != "")
        {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setStandardButtons(QFlags<QMessageBox::StandardButton>());
            msgBox->setText(tr("Exporting regular singers, please wait..."));
            msgBox->show();
            exportSingers(singerIds, saveFilePath);
            msgBox->close();
            delete msgBox;
            QMessageBox::information(this, tr("Export complete"), tr("Regular singer export complete."));
            ui->tableViewRegulars->clearSelection();
        }
    }
}

void DlgRegularExport::exportSingers(const std::vector<int> &historySingerIds, const QString &savePath)
{
    QFile outfile(savePath);
    QJsonArray jArr;
    std::for_each(historySingerIds.begin(), historySingerIds.end(), [&] (auto singerId) {
         QJsonObject jSinger;
         auto singer = m_historySingersModel.getSinger(singerId);
         jSinger.insert("name", singer.name);
         auto songs = m_historySongsModel.getSingerSongs(singer.historySingerId);
         QJsonArray jSongs;
         std::for_each(songs.begin(), songs.end(), [&jSongs] (HistorySong song) {
            QJsonObject jSong;
            jSong.insert("filepath", song.filePath);
            jSong.insert("artist", song.artist);
            jSong.insert("title", song.title);
            jSong.insert("songid", song.songid);
            jSong.insert("keychange", song.keyChange);
            jSong.insert("plays", song.plays);
            jSong.insert("lastplay", song.lastPlayed.toString());
            jSongs.append(jSong);
         });
         jSinger.insert("songs", jSongs);
         jArr.append(jSinger);
    });
    outfile.open(QFile::WriteOnly);
    QJsonDocument jDoc(jArr);
    outfile.write(jDoc.toJson(QJsonDocument::Indented));
    outfile.close();
}


void DlgRegularExport::closeEvent([[maybe_unused]]QCloseEvent *event)
{
    deleteLater();
}


void DlgRegularExport::done(int)
{
    deleteLater();
}

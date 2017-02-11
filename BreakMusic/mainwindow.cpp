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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSqlQuery>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include "bmsettings.h"
#include "tagreader.h"

Settings *settings;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    bmCurrentPosition = 0;
    ui->setupUi(this);
    settings = new Settings(this);
    settings->restoreWindowState(this);
    //mPlayer = new BmAudioBackendGStreamer(this);
    bmAudioBackend = new AudioBackendGstreamer(this);
    bmAudioBackend->setUseFader(true);
    bmAudioBackend->setUseSilenceDetection(true);
    ipcServer = new BmIPCServer("bmControl",this); 
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("BreakMusic");
    khDir = new QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!khDir->exists())
    {
        khDir->mkpath(khDir->absolutePath());
    }
    database = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    database->setDatabaseName(khDir->absolutePath() + QDir::separator() + "breakmusic.sqlite");
    database->open();
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS bmsongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, path VARCHAR(700) NOT NULL UNIQUE, Filename COLLATE NOCASE, Duration TEXT, searchstring TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS bmplaylists ( playlistid INTEGER PRIMARY KEY AUTOINCREMENT, title COLLATE NOCASE NOT NULL UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS bmplsongs ( plsongid INTEGER PRIMARY KEY AUTOINCREMENT, playlist INT, position INT, Artist INT, Title INT, Filename INT, Duration INT, path INT)");
    query.exec("CREATE TABLE IF NOT EXISTS bmsrcdirs ( path NOT NULL)");
    bmDbDialog = new BmDbDialog(database,this);
    bmPlaylistsModel = new QSqlTableModel(this, *database);
    bmPlaylistsModel->setTable("bmplaylists");
    bmPlaylistsModel->sort(2, Qt::AscendingOrder);
    bmDbModel = new BmDbTableModel(this, *database);
    bmDbModel->setTable("bmsongs");
    bmDbModel->select();
    bmCurrentPlaylist = settings->bmPlaylistIndex();
    bmPlModel = new BmPlTableModel(this, *database);
    bmPlModel->select();
    ui->actionShow_Filenames->setChecked(settings->bmShowFilenames());
    ui->actionShow_Metadata->setChecked(settings->bmShowMetadata());
    ui->comboBoxBmPlaylists->setModel(bmPlaylistsModel);
    ui->comboBoxBmPlaylists->setModelColumn(1);
    ui->comboBoxBmPlaylists->setCurrentIndex(settings->bmPlaylistIndex());
    if (bmPlaylistsModel->rowCount() == 0)
    {
        bmAddPlaylist("Default");
        ui->comboBoxBmPlaylists->setCurrentIndex(0);
    }
    bmDbDelegate = new BmDbItemDelegate(this);
    ui->tableViewBmDb->setModel(bmDbModel);
    ui->tableViewBmDb->setItemDelegate(bmDbDelegate);
    ui->tableViewBmPlaylist->setModel(bmPlModel);
    bmPlDelegate = new BmPlItemDelegate(this);
    ui->tableViewBmPlaylist->setItemDelegate(bmPlDelegate);
    settings->restoreSplitterState(ui->splitter);
    settings->restoreColumnWidths(ui->tableViewBmDb);
    settings->restoreColumnWidths(ui->tableViewBmPlaylist);
    ui->tableViewBmDb->setColumnHidden(0, true);
    ui->tableViewBmDb->setColumnHidden(3, true);
    ui->tableViewBmDb->setColumnHidden(6, true);
    ui->tableViewBmDb->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(5,75);
    ui->tableViewBmPlaylist->setColumnHidden(0, true);
    ui->tableViewBmPlaylist->setColumnHidden(1, true);
    ui->tableViewBmPlaylist->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Fixed);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(2,25);
    ui->tableViewBmPlaylist->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(7,25);
    bmAudioBackend->setVolume(settings->bmVolume());

    connect(ipcServer, SIGNAL(messageReceived(int)), this, SLOT(ipcMessageReceived(int)));
    connect(ui->actionShow_Filenames, SIGNAL(triggered(bool)), this, SLOT(bmShowFilenames(bool)));
    connect(ui->actionShow_Metadata, SIGNAL(triggered(bool)), this, SLOT(bmShowMetadata(bool)));
    connect(ui->actionManage_Database, SIGNAL(triggered()), bmDbDialog, SLOT(show()));
    connect(bmAudioBackend, SIGNAL(stateChanged(BmAbstractAudioBackend::State)), this, SLOT(bmMediaStateChanged(BmAbstractAudioBackend::State)));
    connect(bmAudioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(bmMediaPositionChanged(qint64)));
    connect(bmAudioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(bmMediaDurationChanged(qint64)));
    connect(bmAudioBackend, SIGNAL(volumeChanged(int)), ui->sliderBmVolume, SLOT(setValue(int)));
    connect(bmDbDialog, SIGNAL(bmDbUpdated()), this, SLOT(bmDbUpdated()));
    connect(bmDbDialog, SIGNAL(bmDbCleared()), this, SLOT(bmDbCleared()));
}

MainWindow::~MainWindow()
{
    settings->saveWindowState(this);
    settings->saveSplitterState(ui->splitter);
    settings->saveColumnWidths(ui->tableViewBmDb);
    settings->saveColumnWidths(ui->tableViewBmPlaylist);
    settings->bmSetVolume(ui->sliderBmVolume->value());
    settings->bmSetPlaylistIndex(ui->comboBoxBmPlaylists->currentIndex());
    delete database;
    delete khDir;
    delete ui;
}

void MainWindow::ipcMessageReceived(int ipcCommand)
{
    qDebug() << "Received IPC: " << ipcCommand;
    switch (ipcCommand) {
    case BmIPCServer::CMD_FADE_OUT:
        //fader->fadeOut();
        qDebug() << "Received IPC command CMD_FADE_OUT";
        bmAudioBackend->fadeOut();
        break;
    case BmIPCServer::CMD_FADE_IN:
        qDebug() << "Received IPC command CMD_FADE_IN";
        bmAudioBackend->fadeIn();
        break;
    case BmIPCServer::CMD_PLAY:
        bmAudioBackend->play();
        qDebug() << "Received IPC command CMD_PLAY";
        break;
    case BmIPCServer::CMD_PAUSE:
        bmAudioBackend->pause();
        qDebug() << "Received IPC command CMD_PAUSE";
        break;
    case BmIPCServer::CMD_STOP:
        bmAudioBackend->stop(false);
        qDebug() << "Received IPC command CMD_STOP";
        break;
    default:
        qDebug() << "Received unknown IPC command.  NOOP!";
        break;
    }
}

void MainWindow::onActionManageDatabase()
{
    bmDbDialog->show();
}

void MainWindow::on_tableViewBmDb_activated(const QModelIndex &index)
{
    int songId = index.sibling(index.row(), 0).data().toInt();
    bmPlModel->addSong(songId);
}

void MainWindow::on_buttonBmStop_clicked()
{
    bmAudioBackend->stop(false);
}

void MainWindow::on_lineEditBmSearch_returnPressed()
{
    bmDbModel->search(ui->lineEditBmSearch->text());
}

void MainWindow::on_tableViewBmPlaylist_activated(const QModelIndex &index)
{
    bmAudioBackend->stop(false);
    bmCurrentPosition = index.row();
    QString path = index.sibling(index.row(), 7).data().toString();
    QString song = index.sibling(index.row(), 3).data().toString() + " - " + index.sibling(index.row(), 4).data().toString();
    QString nextSong;
    if (!ui->checkBoxBmBreak->isChecked())
    {
        if (bmCurrentPosition == bmPlModel->rowCount() - 1)
            nextSong = bmPlModel->index(0, 3).data().toString() + " - " + bmPlModel->index(0, 4).data().toString();
        else
            nextSong = bmPlModel->index(bmCurrentPosition + 1, 3).data().toString() + " - " + bmPlModel->index(bmCurrentPosition + 1, 4).data().toString();
    }
    else
        nextSong = "None - Breaking after current song";
    bmAudioBackend->setMedia(path);
    bmAudioBackend->play();
    ui->labelBmPlaying->setText(song);
    ui->labelBmNext->setText(nextSong);
    bmPlDelegate->setCurrentSong(index.row());
    bmPlModel->select();
}

void MainWindow::on_sliderBmVolume_valueChanged(int value)
{
    bmAudioBackend->setVolume(value);
}

void MainWindow::on_sliderBmPosition_sliderMoved(int position)
{
    bmAudioBackend->setPosition(position);
}

void MainWindow::bmMediaPositionChanged(qint64 position)
{
    ui->sliderBmPosition->setValue(position);
    ui->labelBmPosition->setText(QTime(0,0,0,0).addMSecs(position).toString("m:ss"));
    ui->labelBmRemaining->setText(QTime(0,0,0,0).addMSecs(bmAudioBackend->duration() - position).toString("m:ss"));
}

void MainWindow::bmMediaDurationChanged(qint64 duration)
{
    ui->sliderBmPosition->setMaximum(duration);
    ui->labelBmDuration->setText(QTime(0,0,0,0).addMSecs(duration).toString("m:ss"));
}

bool MainWindow::bmPlaylistExists(QString name)
{
    for (int i=0; i < bmPlaylistsModel->rowCount(); i++)
    {
        if (bmPlaylistsModel->index(i,1).data().toString().toLower() == name.toLower())
            return true;
    }
    return false;
}

void MainWindow::bmAddPlaylist(QString title)
{
    if (bmPlaylistsModel->insertRow(bmPlaylistsModel->rowCount()))
    {
        QModelIndex index = bmPlaylistsModel->index(bmPlaylistsModel->rowCount() - 1, 1);
        bmPlaylistsModel->setData(index, title);
        bmPlaylistsModel->submitAll();
        bmPlaylistsModel->select();
        ui->comboBoxBmPlaylists->setCurrentIndex(index.row());
    }
}

void MainWindow::on_buttonBmPause_clicked(bool checked)
{
    if (checked)
        bmAudioBackend->pause();
    else
        bmAudioBackend->play();
}

void MainWindow::bmShowMetadata(bool checked)
{
    ui->tableViewBmDb->setColumnHidden(1, !checked);
    ui->tableViewBmDb->setColumnHidden(2, !checked);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(1, 100);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(2, 100);
    ui->tableViewBmPlaylist->setColumnHidden(3, !checked);
    ui->tableViewBmPlaylist->setColumnHidden(4, !checked);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(3, 100);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(4, 100);
    settings->bmSetShowMetadata(checked);
}


void MainWindow::bmShowFilenames(bool checked)
{
    ui->tableViewBmDb->setColumnHidden(4, !checked);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(4, 100);
    ui->tableViewBmPlaylist->setColumnHidden(5, !checked);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(5, 100);
    settings->bmSetShowFilenames(checked);
}

void MainWindow::on_actionImport_Playlist_triggered()
{
    QString importFile = QFileDialog::getOpenFileName(this,tr("Select playlist to import"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("(*.m3u)"));
    if (importFile != "")
    {
        QStringList files;
        QFile textFile;
        textFile.setFileName(importFile);
        textFile.open(QFile::ReadOnly);
        QTextStream textStream(&textFile);
        while (true)
        {
            QString line = textStream.readLine();
            if (line.isNull())
                break;
            else
            {
                if (!line.startsWith("#"))
                    files.append(line.replace("\\", "/"));
            }
        }
        bool ok;
        QString title = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal, tr("New Playlist"), &ok);

        if (ok && !title.isEmpty())
        {
            if (!bmPlaylistExists(title))
            {
                if (bmPlaylistsModel->insertRow(bmPlaylistsModel->rowCount()))
                {
                    QModelIndex index = bmPlaylistsModel->index(bmPlaylistsModel->rowCount() - 1, 1);
                    bmPlaylistsModel->setData(index, title);
                    bmPlaylistsModel->submitAll();
                    bmPlaylistsModel->select();
                    ui->comboBoxBmPlaylists->setCurrentIndex(index.row());
                    bmPlModel->setCurrentPlaylist(bmPlaylistsModel->index(index.row(),0).data().toInt());

                }
            }
        }
        QSqlQuery query;
        query.exec("BEGIN TRANSACTION");
        TagReader reader;
        for (int i=0; i < files.size(); i++)
        {
            if (QFile(files.at(i)).exists())
            {
                reader.setMedia(files.at(i).toLocal8Bit());
                QString duration = QString::number(reader.getDuration() / 1000);
                QString artist = reader.getArtist();
                QString title = reader.getTitle();
                QString filename = QFileInfo(files.at(i)).fileName();
                QString queryString = "INSERT OR IGNORE INTO songs (artist,title,path,filename,duration,searchstring) VALUES(\"" + artist + "\",\"" + title + "\",\"" + files.at(i) + "\",\"" + filename + "\",\"" + duration + "\",\"" + artist + title + filename + "\")";
                query.exec(queryString);
            }
        }
        query.exec("COMMIT TRANSACTION");
        bmDbModel->select();
        QApplication::processEvents();
        QList<int> songIds;
        for (int i=0; i < files.size(); i++)
        {
            int songId = bmPlModel->getSongIdByFilePath(files.at(i));
            if (songId >= 0)
                songIds.push_back(songId);
        }
        query.exec("BEGIN TRANSACTION");
        for (int i=0; i < songIds.size(); i++)
        {
            QString sIdStr = QString::number(songIds.at(i));
            QString sql = "INSERT INTO plsongs (playlist,position,artist,title,filename,duration,path) VALUES(" + QString::number(bmCurrentPlaylist) + "," + QString::number(i) + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + ")";
            query.exec(sql);
        }
        query.exec("COMMIT TRANSACTION");
        bmPlModel->select();
    }
}

void MainWindow::on_actionExport_Playlist_triggered()
{
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + ui->comboBoxBmPlaylists->currentText() + ".m3u";
    qDebug() << "Default save location: " << defaultFilePath;
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select filename to save playlist as"), defaultFilePath, tr("(*.m3u)"));
    if (saveFilePath != "")
    {
        QFile file(saveFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, tr("Error saving file"), tr("Unable to open selected file for writing.  Please verify that you have the proper permissions to write to that location."),QMessageBox::Close);
            return;
        }
        QTextStream out(&file);
        for (int i=0; i < bmPlModel->rowCount(); i++)
        {
            out << bmPlModel->index(i, 7).data().toString() << "\n";
        }
    }
}

void MainWindow::on_actionNew_Playlist_triggered()
{
    bool ok;
    QString title = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal, tr("New Playlist"), &ok);
    if (ok && !title.isEmpty())
    {
        bmAddPlaylist(title);
    }
}

void MainWindow::on_actionRemove_Playlist_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("Are you sure you want to delete the current playlist?  If you have not exported it, you will not be able to undo this action!");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        QSqlQuery query;
        query.exec("DELETE FROM plsongs WHERE playlist == " + QString::number(bmPlModel->currentPlaylist()));
        query.exec("DELETE FROM playlists WHERE playlistid == " + QString::number(bmPlModel->currentPlaylist()));
        bmPlaylistsModel->select();
        if (bmPlaylistsModel->rowCount() == 0)
        {
            bmAddPlaylist("Default");
        }
        ui->comboBoxBmPlaylists->setCurrentIndex(0);
        bmPlModel->setCurrentPlaylist(bmPlaylistsModel->index(0,0).data().toInt());
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QString title;
    QString text;
    title = "About OpenKJ BreakMusic";
    text.append("OpenKJ BreakMusic\n\n");
    text.append("http://openkj.org\n\n");
    text.append("This is open source software and is licensed under the terms of the GNU General Public License v3");
    QMessageBox::about(this, title, text);
}

void MainWindow::bmMediaStateChanged(AbstractAudioBackend::State newState)
{
    static AbstractAudioBackend::State lastState = AbstractAudioBackend::StoppedState;
    if (newState != lastState)
    {
        lastState = newState;
        if (newState == AbstractAudioBackend::EndOfMediaState)
        {
            if (ui->checkBoxBmBreak->isChecked())
            {
                bmAudioBackend->stop(true);
                return;
            }
            if (bmCurrentPosition < bmPlModel->rowCount() - 1)
                bmCurrentPosition++;
            else
                bmCurrentPosition = 0;
            bmAudioBackend->stop(true);
            QString path = bmPlModel->index(bmCurrentPosition, 7).data().toString();
            QString song = bmPlModel->index(bmCurrentPosition, 3).data().toString() + " - " + bmPlModel->index(bmCurrentPosition, 4).data().toString();
            QString nextSong;
            if (!ui->checkBoxBmBreak->isChecked())
            {
            if (bmCurrentPosition == bmPlModel->rowCount() - 1)
                nextSong = bmPlModel->index(0, 3).data().toString() + " - " + bmPlModel->index(0, 4).data().toString();
            else
                nextSong = bmPlModel->index(bmCurrentPosition + 1, 3).data().toString() + " - " + bmPlModel->index(bmCurrentPosition + 1, 4).data().toString();
            }
            else
                nextSong = "None - Breaking after current song";
            bmAudioBackend->setMedia(path);
            bmAudioBackend->play();
            ui->labelBmPlaying->setText(song);
            ui->labelBmNext->setText(nextSong);
            bmPlDelegate->setCurrentSong(bmCurrentPosition);
            bmPlModel->select();
        }
    }
}

void MainWindow::bmDbUpdated()
{
    bmDbModel->select();
}

void MainWindow::bmDbCleared()
{
    bmDbModel->select();
    bmAddPlaylist("Default");
    ui->comboBoxBmPlaylists->setCurrentIndex(0);
}

void MainWindow::on_tableViewBmPlaylist_clicked(const QModelIndex &index)
{
    if (index.column() == 7)
    {
        bmPlModel->deleteSong(index.row());
    }
}

void MainWindow::on_comboBoxBmPlaylists_currentIndexChanged(int index)
{
    bmCurrentPlaylist = bmPlaylistsModel->index(index, 0).data().toInt();
    bmPlModel->setCurrentPlaylist(bmCurrentPlaylist);
}

void MainWindow::on_checkBoxBmBreak_toggled(bool checked)
{
    QString nextSong;
    if (!checked)
    {
        if (bmCurrentPosition == bmPlModel->rowCount() - 1)
            nextSong = bmPlModel->index(0, 3).data().toString() + " - " + bmPlModel->index(0, 4).data().toString();
        else
            nextSong = bmPlModel->index(bmCurrentPosition + 1, 3).data().toString() + " - " + bmPlModel->index(bmCurrentPosition + 1, 4).data().toString();
    }
    else
        nextSong = "None - Breaking after current song";
    ui->labelBmNext->setText(nextSong);
}


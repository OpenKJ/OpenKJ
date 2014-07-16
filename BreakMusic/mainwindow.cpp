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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSqlQuery>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <tag.h>
#include <fileref.h>
#include "bmsettings.h"

BmSettings *settings;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    currentPosition = 0;
    ui->setupUi(this);
    settings = new BmSettings(this);
    settings->restoreWindowState(this);
    mPlayer = new BmAudioBackendGStreamer(this);
    mPlayer->setUseFader(true);
    mPlayer->setUseSilenceDetection(true);
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
    query.exec("CREATE TABLE IF NOT EXISTS songs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, path VARCHAR(700) NOT NULL UNIQUE, Filename COLLATE NOCASE, Duration TEXT, searchstring TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS playlists ( playlistid INTEGER PRIMARY KEY AUTOINCREMENT, title COLLATE NOCASE NOT NULL UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS plsongs ( plsongid INTEGER PRIMARY KEY AUTOINCREMENT, playlist INT, position INT, Artist INT, Title INT, Filename INT, Duration INT, path INT)");
    query.exec("CREATE TABLE IF NOT EXISTS srcdirs ( path NOT NULL)");
    dbDialog = new DatabaseDialog(database,this);
    playlistsModel = new QSqlTableModel(this, *database);
    playlistsModel->setTable("playlists");
    playlistsModel->sort(2, Qt::AscendingOrder);
    dbModel = new DbTableModel(this, *database);
    dbModel->setTable("songs");
    dbModel->select();
    currentPlaylist = settings->playlistIndex();
    plModel = new PlTableModel(this, *database);
    plModel->select();
    ui->actionShow_Filenames->setChecked(settings->showFilenames());
    ui->actionShow_Metadata->setChecked(settings->showMetadata());
    ui->comboBoxPlaylists->setModel(playlistsModel);
    ui->comboBoxPlaylists->setModelColumn(1);
    ui->comboBoxPlaylists->setCurrentIndex(settings->playlistIndex());
    if (playlistsModel->rowCount() == 0)
    {
        addPlaylist("Default");
        ui->comboBoxPlaylists->setCurrentIndex(0);
    }
    ui->tableViewDB->setModel(dbModel);
    ui->tableViewDB->setColumnHidden(0, true);
    ui->tableViewDB->setColumnHidden(3, true);
    ui->tableViewDB->setColumnHidden(6, true);
    ui->tableViewPlaylist->setModel(plModel);
    ui->tableViewPlaylist->setColumnHidden(0, true);
    ui->tableViewPlaylist->setColumnHidden(1, true);
    ui->tableViewPlaylist->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Fixed);
    ui->tableViewPlaylist->horizontalHeader()->resizeSection(2,16);
    ui->tableViewPlaylist->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    ui->tableViewPlaylist->horizontalHeader()->resizeSection(7,16);
    ui->tableViewPlaylist->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    plDelegate = new PlItemDelegate(this);
    ui->tableViewPlaylist->setItemDelegate(plDelegate);
    showFilenames(settings->showFilenames());
    showMetadata(settings->showMetadata());
    ui->tableViewDB->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    mPlayer->setVolume(settings->volume());

    connect(ipcServer, SIGNAL(messageReceived(int)), this, SLOT(ipcMessageReceived(int)));
    connect(ui->actionShow_Filenames, SIGNAL(triggered(bool)), this, SLOT(showFilenames(bool)));
    connect(ui->actionShow_Metadata, SIGNAL(triggered(bool)), this, SLOT(showMetadata(bool)));
    connect(ui->actionManage_Database, SIGNAL(triggered()), dbDialog, SLOT(show()));
    connect(mPlayer, SIGNAL(stateChanged(BmAbstractAudioBackend::State)), this, SLOT(mediaStateChanged(BmAbstractAudioBackend::State)));
    connect(mPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(on_mediaPositionChanged(qint64)));
    connect(mPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(on_mediaDurationChanged(qint64)));
    connect(mPlayer, SIGNAL(volumeChanged(int)), ui->sliderVolume, SLOT(setValue(int)));
    connect(dbDialog, SIGNAL(dbUpdated()), this, SLOT(dbUpdated()));
    connect(dbDialog, SIGNAL(dbCleared()), this, SLOT(dbCleared()));
}

MainWindow::~MainWindow()
{
    settings->saveWindowState(this);
    settings->setVolume(ui->sliderVolume->value());
    settings->setPlaylistIndex(ui->comboBoxPlaylists->currentIndex());
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
        mPlayer->fadeOut();
        break;
    case BmIPCServer::CMD_FADE_IN:
        qDebug() << "Received IPC command CMD_FADE_IN";
        mPlayer->fadeIn();
        break;
    case BmIPCServer::CMD_PLAY:
        mPlayer->play();
        qDebug() << "Received IPC command CMD_PLAY";
        break;
    case BmIPCServer::CMD_PAUSE:
        mPlayer->pause();
        qDebug() << "Received IPC command CMD_PAUSE";
        break;
    case BmIPCServer::CMD_STOP:
        mPlayer->stop();
        qDebug() << "Received IPC command CMD_STOP";
        break;
    default:
        qDebug() << "Received unknown IPC command.  NOOP!";
        break;
    }
}

void MainWindow::onActionManageDatabase()
{
    dbDialog->show();
}

void MainWindow::on_tableViewDB_activated(const QModelIndex &index)
{
    int songId = index.sibling(index.row(), 0).data().toInt();
    plModel->addSong(songId);
}

void MainWindow::on_buttonStop_clicked()
{
    mPlayer->stop();
}

void MainWindow::on_lineEditSearch_returnPressed()
{
    dbModel->search(ui->lineEditSearch->text());
}

void MainWindow::on_tableViewPlaylist_activated(const QModelIndex &index)
{
    mPlayer->stop();
    currentPosition = index.row();
    QString path = index.sibling(index.row(), 7).data().toString();
    QString song = index.sibling(index.row(), 3).data().toString() + " - " + index.sibling(index.row(), 4).data().toString();
    QString nextSong;
    if (!ui->checkBoxBreak->isChecked())
    {
        if (currentPosition == plModel->rowCount() - 1)
            nextSong = plModel->index(0, 3).data().toString() + " - " + plModel->index(0, 4).data().toString();
        else
            nextSong = plModel->index(currentPosition + 1, 3).data().toString() + " - " + plModel->index(currentPosition + 1, 4).data().toString();
    }
    else
        nextSong = "None - Breaking after current song";
    mPlayer->setMedia(path);
    mPlayer->play();
    ui->labelPlaying->setText(song);
    ui->labelNext->setText(nextSong);
    plDelegate->setCurrentSong(index.row());
    plModel->select();
}

void MainWindow::on_sliderVolume_valueChanged(int value)
{
    mPlayer->setVolume(value);
}

void MainWindow::on_sliderPosition_sliderMoved(int position)
{
    mPlayer->setPosition(position);
}

void MainWindow::on_mediaPositionChanged(qint64 position)
{
    ui->sliderPosition->setValue(position);
    ui->labelPosition->setText(QTime(0,0,0,0).addMSecs(position).toString("m:ss"));
    ui->labelRemaining->setText(QTime(0,0,0,0).addMSecs(mPlayer->duration() - position).toString("m:ss"));
}

void MainWindow::on_mediaDurationChanged(qint64 duration)
{
    ui->sliderPosition->setMaximum(duration);
    ui->labelDuration->setText(QTime(0,0,0,0).addMSecs(duration).toString("m:ss"));
}

bool MainWindow::playlistExists(QString name)
{
    for (int i=0; i < playlistsModel->rowCount(); i++)
    {
        if (playlistsModel->index(i,1).data().toString().toLower() == name.toLower())
            return true;
    }
    return false;
}

void MainWindow::addPlaylist(QString title)
{
    if (playlistsModel->insertRow(playlistsModel->rowCount()))
    {
        QModelIndex index = playlistsModel->index(playlistsModel->rowCount() - 1, 1);
        playlistsModel->setData(index, title);
        playlistsModel->submitAll();
        playlistsModel->select();
        ui->comboBoxPlaylists->setCurrentIndex(index.row());
    }
}

void MainWindow::on_buttonPause_clicked(bool checked)
{
    if (checked)
        mPlayer->pause();
    else
        mPlayer->play();
}

void MainWindow::showMetadata(bool checked)
{
    ui->tableViewDB->setColumnHidden(1, !checked);
    ui->tableViewDB->setColumnHidden(2, !checked);
    ui->tableViewPlaylist->setColumnHidden(3, !checked);
    ui->tableViewPlaylist->setColumnHidden(4, !checked);
    settings->setShowMetadata(checked);
}


void MainWindow::showFilenames(bool checked)
{
    ui->tableViewDB->setColumnHidden(4, !checked);
    ui->tableViewPlaylist->setColumnHidden(5, !checked);
    settings->setShowFilenames(checked);
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
            if (!playlistExists(title))
            {
                if (playlistsModel->insertRow(playlistsModel->rowCount()))
                {
                    QModelIndex index = playlistsModel->index(playlistsModel->rowCount() - 1, 1);
                    playlistsModel->setData(index, title);
                    playlistsModel->submitAll();
                    playlistsModel->select();
                    ui->comboBoxPlaylists->setCurrentIndex(index.row());
                    plModel->setCurrentPlaylist(playlistsModel->index(index.row(),0).data().toInt());

                }
            }
        }
        QSqlQuery query;
        query.exec("BEGIN TRANSACTION");
        for (int i=0; i < files.size(); i++)
        {
            TagLib::FileRef f(files.at(i).toUtf8().data());
            if (!f.isNull())
            {
                QString artist = QString::fromStdString(f.tag()->artist().to8Bit(true));
                QString title = QString::fromStdString(f.tag()->title().to8Bit(true));
                QString duration = QString::number(f.audioProperties()->length());
                QString filename = QFileInfo(files.at(i)).fileName();
                query.exec("INSERT OR IGNORE INTO songs (artist,title,path,filename,duration) VALUES(\"" + artist + "\",\"" + title + "\",\"" + files.at(i) + "\",\"" + filename + "\"," + duration + ")");
            }
        }
        query.exec("COMMIT TRANSACTION");
        dbModel->select();
        QApplication::processEvents();
        QList<int> songIds;
        for (int i=0; i < files.size(); i++)
        {
            int songId = plModel->getSongIdByFilePath(files.at(i));
            if (songId >= 0)
                songIds.push_back(songId);
        }
        query.exec("BEGIN TRANSACTION");
        for (int i=0; i < songIds.size(); i++)
        {
            QString sIdStr = QString::number(songIds.at(i));
            QString sql = "INSERT INTO plsongs (playlist,position,artist,title,filename,duration,path) VALUES(" + QString::number(currentPlaylist) + "," + QString::number(i) + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + ")";
            query.exec(sql);
        }
        query.exec("COMMIT TRANSACTION");
        plModel->select();
    }
}

void MainWindow::on_actionExport_Playlist_triggered()
{
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + ui->comboBoxPlaylists->currentText() + ".m3u";
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
        for (int i=0; i < plModel->rowCount(); i++)
        {
            out << plModel->index(i, 7).data().toString() << "\n";
        }
    }
}

void MainWindow::on_actionNew_Playlist_triggered()
{
    bool ok;
    QString title = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal, tr("New Playlist"), &ok);
    if (ok && !title.isEmpty())
    {
        addPlaylist(title);
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
        query.exec("DELETE FROM plsongs WHERE playlist == " + QString::number(plModel->currentPlaylist()));
        query.exec("DELETE FROM playlists WHERE playlistid == " + QString::number(plModel->currentPlaylist()));
        playlistsModel->select();
        if (playlistsModel->rowCount() == 0)
        {
            addPlaylist("Default");
        }
        ui->comboBoxPlaylists->setCurrentIndex(0);
        plModel->setCurrentPlaylist(playlistsModel->index(0,0).data().toInt());
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

void MainWindow::mediaStateChanged(BmAbstractAudioBackend::State newState)
{
    static BmAbstractAudioBackend::State lastState = BmAbstractAudioBackend::StoppedState;
    if (newState != lastState)
    {
        lastState = newState;
        if (newState == BmAbstractAudioBackend::EndOfMediaState)
        {
            if (ui->checkBoxBreak->isChecked())
            {
                mPlayer->stop(true);
                return;
            }
            if (currentPosition < plModel->rowCount() - 1)
                currentPosition++;
            else
                currentPosition = 0;
            mPlayer->stop(true);
            QString path = plModel->index(currentPosition, 7).data().toString();
            QString song = plModel->index(currentPosition, 3).data().toString() + " - " + plModel->index(currentPosition, 4).data().toString();
            QString nextSong;
            if (!ui->checkBoxBreak->isChecked())
            {
            if (currentPosition == plModel->rowCount() - 1)
                nextSong = plModel->index(0, 3).data().toString() + " - " + plModel->index(0, 4).data().toString();
            else
                nextSong = plModel->index(currentPosition + 1, 3).data().toString() + " - " + plModel->index(currentPosition + 1, 4).data().toString();
            }
            else
                nextSong = "None - Breaking after current song";
            mPlayer->setMedia(path);
            mPlayer->play();
            ui->labelPlaying->setText(song);
            ui->labelNext->setText(nextSong);
            plDelegate->setCurrentSong(currentPosition);
            plModel->select();
        }
    }

}

void MainWindow::dbUpdated()
{
    dbModel->select();
}

void MainWindow::dbCleared()
{
    dbModel->select();
    addPlaylist("Default");
    ui->comboBoxPlaylists->setCurrentIndex(0);
}

void MainWindow::on_tableViewPlaylist_clicked(const QModelIndex &index)
{
    if (index.column() == 7)
    {
        plModel->deleteSong(index.row());
    }
}

void MainWindow::on_comboBoxPlaylists_currentIndexChanged(int index)
{
    currentPlaylist = playlistsModel->index(index, 0).data().toInt();
    plModel->setCurrentPlaylist(currentPlaylist);
}

void MainWindow::on_checkBoxBreak_toggled(bool checked)
{
    QString nextSong;
    if (!checked)
    {
        if (currentPosition == plModel->rowCount() - 1)
            nextSong = plModel->index(0, 3).data().toString() + " - " + plModel->index(0, 4).data().toString();
        else
            nextSong = plModel->index(currentPosition + 1, 3).data().toString() + " - " + plModel->index(currentPosition + 1, 4).data().toString();
    }
    else
        nextSong = "None - Breaking after current song";
    ui->labelNext->setText(nextSong);
}

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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sharedMemory = new QSharedMemory("KhControl",this);
    mPlayer = new QMediaPlayer(this);
    sharedMemory->lock();
    ipcServer = new BmIPCServer("bmControl",this);
    khDir = new QDir(QDir::homePath() + QDir::separator() + ".KaraokeHost");
    database = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    database->setDatabaseName(khDir->absolutePath() + QDir::separator() + "khbreakmusic.sqlite");
    database->open();
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS songs ( artist VARCHAR(100), title VARCHAR(100), path VARCHAR(700) NOT NULL UNIQUE, filename VARCHAR(200), duration INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS playlists ( title VARCHAR(100) NOT NULL UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS plsongs ( playlist INTEGER, song INTEGER, position INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS srcdirs ( path VARCHAR(700) NOT NULL UNIQUE)");
    dbDialog = new DatabaseDialog(this);
    songs = new BmSongs(this);
    songdbmodel = new SongdbTableModel(songs,this);
    playlists = new BmPlaylists(this);
    playlistmodel = new PlaylistTableModel(playlists,this);
    playlistmodel->showFilenames(false);
    playlistmodel->showMetadata(true);
    songdbmodel->showFilenames(false);
    songdbmodel->showMetadata(true);
    ui->actionShow_Filenames->setChecked(false);
    ui->actionShow_Metadata->setChecked(true);
    ui->comboBoxPlaylists->addItems(playlists->getTitleList());
    ui->comboBoxPlaylists->setCurrentIndex(0);
    ui->treeViewDB->setModel(songdbmodel);
    ui->treeViewPlaylist->setModel(playlistmodel);
    ui->treeViewPlaylist->header()->resizeSection(0,18);
    ui->treeViewPlaylist->header()->setSectionResizeMode(0,QHeaderView::Fixed);
    ui->treeViewPlaylist->header()->resizeSection(playlistmodel->getColumnCount() - 1,18);
    ui->treeViewPlaylist->header()->setSectionResizeMode(playlistmodel->getColumnCount() - 1,QHeaderView::Fixed);
    songs->loadFromDB();
    mPlayer->setVolume(25);
    ui->sliderVolume->setValue(25);
    fader = new Fader(mPlayer,this);

    connect(ipcServer, SIGNAL(messageReceived(int)), this, SLOT(ipcMessageReceived(int)));
    connect(ui->actionShow_Filenames, SIGNAL(triggered(bool)), this, SLOT(on_actionShow_Filenames(bool)));
    connect(ui->actionShow_Metadata, SIGNAL(triggered(bool)), this, SLOT(on_actionShow_Metadata(bool)));
    connect(ui->actionManage_Database, SIGNAL(triggered()), dbDialog, SLOT(show()));
    connect(mPlayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(on_mediaStatusChanged(QMediaPlayer::MediaStatus)));
    connect(mPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(on_mediaPositionChanged(qint64)));
    connect(mPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(on_mediaDurationChanged(qint64)));
    connect(mPlayer, SIGNAL(volumeChanged(int)), ui->sliderVolume, SLOT(setValue(int)));
    connect(playlists, SIGNAL(playlistsChanged()), this, SLOT(on_playlistsChanged()));
    connect(playlists, SIGNAL(currentPlaylistChanged(QString)), ui->comboBoxPlaylists, SLOT(setCurrentText(QString)));
    connect(playlists, SIGNAL(dataChanged()), this, SLOT(on_playlistChanged()));
    connect(ui->sliderVolume, SIGNAL(sliderMoved(int)), fader, SLOT(setBaseVolume(int)));

    on_actionShow_Filenames(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}



//void MainWindow::ipcMessageReceived(QString ipcMessage)
//{
//    qDebug() << "Received IPC: " << ipcMessage;
//}

void MainWindow::ipcMessageReceived(int ipcCommand)
{
    qDebug() << "Received IPC: " << ipcCommand;
    switch (ipcCommand) {
    case BmIPCServer::CMD_FADE_OUT:
        fader->fadeOut();
        qDebug() << "Received IPC command CMD_FADE_OUT";
        break;
    case BmIPCServer::CMD_FADE_IN:
        fader->fadeIn();
        qDebug() << "Received IPC command CMD_FADE_IN";
        break;
    case BmIPCServer::CMD_PLAY:
        fader->fadePlay();
        qDebug() << "Received IPC command CMD_PLAY";
        break;
    case BmIPCServer::CMD_PAUSE:
        fader->fadePause();
        qDebug() << "Received IPC command CMD_PAUSE";
        break;
    case BmIPCServer::CMD_STOP:
        fader->fadeStop();
        qDebug() << "Received IPC command CMD_STOP";
        break;
    default:
        qDebug() << "Received unknown IPC command.  NOOP!";
        break;
    }
}

void MainWindow::on_actionManageDatabase_triggered()
{
    dbDialog->show();
}

void MainWindow::on_playlistsChanged()
{
    qDebug() << "on_playlistsChanged() fired";
    QString cursel = playlists->getCurrent()->title();
    ui->comboBoxPlaylists->clear();
    ui->comboBoxPlaylists->addItems(playlists->getTitleList());
    ui->comboBoxPlaylists->setCurrentText(cursel);
}

void MainWindow::on_treeViewDB_activated(const QModelIndex &index)
{
    playlists->getCurrent()->addSong(songs->at(index.row()));
//    mPlayer->setMedia(QUrl::fromLocalFile(songs->at(index.row())->path()));
//    mPlayer->play();
}

void MainWindow::on_buttonStop_clicked()
{
    fader->fadeStop();
}

void MainWindow::on_lineEditSearch_returnPressed()
{
    QStringList terms;
    terms = ui->lineEditSearch->text().split(" ",QString::SkipEmptyParts);
    songs->setFilterTerms(terms);
}

void MainWindow::on_buttonAddPlaylist_clicked()
{
    bool ok;
    QString title = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal, tr("New Playlist"), &ok);
    if (ok && !title.isEmpty())
    {
        qDebug() << "Would create playlist: " + title;
        if (!playlists->exists(title))
            playlists->setCurrent(playlists->addPlaylist(title));
    }
}

void MainWindow::on_treeViewPlaylist_activated(const QModelIndex &index)
{

    playlists->getCurrent()->setCurrentSongByPosition(index.row());
    playCurrent();
    ui->buttonPause->setChecked(false);
}

void MainWindow::on_sliderVolume_valueChanged(int value)
{
    mPlayer->setVolume(value);
}

void MainWindow::on_mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    qDebug() << "mediaStatusChanged fired: " << status;
    if ((status == QMediaPlayer::EndOfMedia) && (ui->checkBoxBreak->checkState() != Qt::Checked))
    {
        playlists->getCurrent()->next();
        playCurrent();
    }
}

void MainWindow::on_sliderPosition_sliderMoved(int position)
{
    mPlayer->setPosition(position);
}

void MainWindow::on_mediaPositionChanged(qint64 position)
{
    ui->sliderPosition->setValue(position);
    ui->labelPosition->setText(msToMMSS(position));
    ui->labelRemaining->setText(msToMMSS(mPlayer->duration() - position));
}

void MainWindow::on_mediaDurationChanged(qint64 duration)
{
    ui->sliderPosition->setMaximum(duration);
    ui->labelDuration->setText(msToMMSS(duration));
}

void MainWindow::playCurrent()
{
    BmPlaylistSong *song = playlists->getCurrent()->getCurrentSong();
    BmPlaylistSong *next = playlists->getCurrent()->getNextSong();
    qDebug() << "Playing song at position: " << song->position() << " Artist: " << song->song()->artist() << " Title: " << song->song()->title();
    if (mPlayer->state() == QMediaPlayer::PlayingState)
        fader->fadeStop();
    while (fader->isFading())
        QApplication::processEvents();
    mPlayer->setMedia(QUrl::fromLocalFile(song->song()->path()));
    fader->fadePlay();
    ui->sliderPosition->setMaximum(mPlayer->duration());
    ui->sliderPosition->setValue(0);
    ui->labelPlaying->setText(song->song()->artist() + " - " + song->song()->title());
    ui->labelNext->setText(next->song()->artist() + " - " + next->song()->title());
}

QString MainWindow::msToMMSS(qint64 ms)
{
    QString sec;
    QString min;
    int seconds = (int) (ms / 1000) % 60 ;
    int minutes = (int) ((ms / (1000*60)) % 60);

    if (seconds < 10)
        sec = "0" + QString::number(seconds);
    else
        sec = QString::number(seconds);
    if (minutes < 10)
        min = "0" + QString::number(minutes);
    else
        min = QString::number(minutes);

    return QString(min + ":" + sec);
}

void MainWindow::on_buttonPause_clicked(bool checked)
{
    if (checked)
        fader->fadePause();
    else
        fader->fadePlay();
}

void MainWindow::on_comboBoxPlaylists_currentIndexChanged(const QString &arg1)
{
    qDebug() << "Changing active playlist to: " << arg1;
    playlists->setCurrent(arg1);
}

void MainWindow::on_playlistChanged()
{
    if (playlists->getCurrent()->getNextSong()->valid())
    {
    BmSong *nextSong = playlists->getCurrent()->getNextSong()->song();
    ui->labelNext->setText(nextSong->artist() + " - " + nextSong->title());
    }
}

void MainWindow::on_actionShow_Metadata(bool checked)
{
    if (!checked)
    {
        ui->treeViewDB->header()->hideSection(0);
        ui->treeViewDB->header()->hideSection(1);
        ui->treeViewPlaylist->header()->hideSection(1);
        ui->treeViewPlaylist->header()->hideSection(2);
    }
    else
    {
        ui->treeViewDB->header()->showSection(0);
        ui->treeViewDB->header()->showSection(1);
        ui->treeViewPlaylist->header()->showSection(1);
        ui->treeViewPlaylist->header()->showSection(2);
    }
}


void MainWindow::on_actionShow_Filenames(bool checked)
{
    if (!checked)
    {
        ui->treeViewDB->header()->hideSection(2);
        ui->treeViewPlaylist->header()->hideSection(3);
    }
    else
    {
        ui->treeViewDB->header()->showSection(2);
        ui->treeViewPlaylist->header()->showSection(3);
    }
}

void MainWindow::on_treeViewPlaylist_clicked(const QModelIndex &index)
{
    if (index.column() == 5)
    {
        qDebug() << "Delete clicked on row: " << index.row();
        playlists->getCurrent()->removeSong(index.row());
    }
}

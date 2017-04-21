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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <iostream>
#include <QTemporaryDir>
#include <QDir>
#include "audiobackendgstreamer.h"
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include "khdb.h"
#include "okarchive.h"
#include "tagreader.h"
#include <QSvgRenderer>


Settings *settings;
KhDb *db;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    sliderPositionPressed = false;
    m_rtClickQueueSongId = -1;
    m_rtClickRotationSingerId = -1;
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");
    ui->setupUi(this);
#ifdef Q_OS_WIN
    ui->sliderBmPosition->setMaximumHeight(12);
    ui->sliderBmVolume->setMaximumWidth(12);
    ui->sliderProgress->setMaximumHeight(12);
    ui->sliderVolume->setMaximumWidth(12);
#endif
    db = new KhDb(this);
    labelSingerCount = new QLabel(ui->statusBar);
    khDir = new QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!khDir->exists())
    {
        khDir->mkpath(khDir->absolutePath());
    }
    settings = new Settings(this);
    int initialKVol = settings->audioVolume();
    int initialBMVol = settings->bmVolume();
    qWarning() << "Initial volumes - K: " << initialKVol << " BM: " << initialBMVol;
    settings->restoreWindowState(this);
    database = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    database->setDatabaseName(khDir->absolutePath() + QDir::separator() + "openkj.sqlite");
    database->open();
    QSqlQuery query("CREATE TABLE IF NOT EXISTS dbSongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, DiscId COLLATE NOCASE, 'Duration' INTEGER, path VARCHAR(700) NOT NULL UNIQUE, filename COLLATE NOCASE)");
    query.exec("CREATE TABLE IF NOT EXISTS rotationSingers ( singerid INTEGER PRIMARY KEY AUTOINCREMENT, name COLLATE NOCASE UNIQUE, 'position' INTEGER NOT NULL, 'regular' LOGICAL DEFAULT(0), 'regularid' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS queueSongs ( qsongid INTEGER PRIMARY KEY AUTOINCREMENT, singer INT, song INTEGER NOT NULL, artist INT, title INT, discid INT, path INT, keychg INT, played LOGICAL DEFAULT(0), 'position' INT)");
    query.exec("CREATE TABLE IF NOT EXISTS regularSingers ( regsingerid INTEGER PRIMARY KEY AUTOINCREMENT, Name COLLATE NOCASE UNIQUE, ph1 INT, ph2 INT, ph3 INT)");
    query.exec("CREATE TABLE IF NOT EXISTS regularSongs ( regsongid INTEGER PRIMARY KEY AUTOINCREMENT, regsingerid INTEGER NOT NULL, songid INTEGER NOT NULL, 'keychg' INTEGER, 'position' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS sourceDirs ( path VARCHAR(255) UNIQUE, pattern INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS bmsongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, path VARCHAR(700) NOT NULL UNIQUE, Filename COLLATE NOCASE, Duration TEXT, searchstring TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS bmplaylists ( playlistid INTEGER PRIMARY KEY AUTOINCREMENT, title COLLATE NOCASE NOT NULL UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS bmplsongs ( plsongid INTEGER PRIMARY KEY AUTOINCREMENT, playlist INT, position INT, Artist INT, Title INT, Filename INT, Duration INT, path INT)");
    query.exec("CREATE TABLE IF NOT EXISTS bmsrcdirs ( path NOT NULL)");
    query.exec("PRAGMA synchronous = OFF");
    sortColDB = 1;
    sortDirDB = 0;
    dbModel = new DbTableModel(this, *database);
    dbModel->select();
    qModel = new QueueModel(this, *database);
    qModel->select();
    qDelegate = new QueueItemDelegate(this);
    rotModel = new RotationModel(this, *database);
    rotModel->select();
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    ui->tableViewRotation->setModel(rotModel);
    rotDelegate = new RotationItemDelegate(this);
    ui->tableViewRotation->setItemDelegate(rotDelegate);
    ui->tableViewQueue->setModel(qModel);
    ui->tableViewQueue->setItemDelegate(qDelegate);
    khTmpDir = new QTemporaryDir();
    dbDialog = new DlgDatabase(this);
    dlgKeyChange = new DlgKeyChange(qModel, this);
    regularSingersDialog = new DlgRegularSingers(rotModel, this);
    regularExportDialog = new DlgRegularExport(rotModel, this);
    regularImportDialog = new DlgRegularImport(rotModel, this);
    requestsDialog = new DlgRequests(rotModel, this);
    cdgWindow = new DlgCdg(this, Qt::Window);
    cdg = new CDG;
    ui->tableViewDB->setModel(dbModel);
    dbDelegate = new DbItemDelegate(this);
    ui->tableViewDB->setItemDelegate(dbDelegate);
//    ipcClient = new KhIPCClient("bmControl",this);
    bmAudioBackend = new AudioBackendGstreamer(false, this);
    audioBackends = new KhAudioBackends;
    audioBackends->push_back(new AudioBackendGstreamer(true, this));
    if (audioBackends->count() < 1)
        qCritical("No audio backends available!");
    if (settings->audioBackend() < audioBackends->count())
        activeAudioBackend = audioBackends->at(settings->audioBackend());
    else
    {
        settings->setAudioBackend(0);
        activeAudioBackend = audioBackends->at(0);
    }
    if (activeAudioBackend->canFade())
        activeAudioBackend->setUseFader(settings->audioUseFader());
    if (!activeAudioBackend->canPitchShift())
    {
        ui->spinBoxKey->hide();
        ui->lblKey->hide();
        ui->tableViewQueue->hideColumn(7);
    }
    audioRecorder = new KhAudioRecorder(this);
    settingsDialog = new DlgSettings(audioBackends, this);
    connect(rotModel, SIGNAL(songDroppedOnSinger(int,int,int)), this, SLOT(songDroppedOnSinger(int,int,int)));
    connect(activeAudioBackend, SIGNAL(volumeChanged(int)), ui->sliderVolume, SLOT(setValue(int)));
    connect(dbDialog, SIGNAL(databaseUpdated()), this, SLOT(songdbUpdated()));
    connect(dbDialog, SIGNAL(databaseCleared()), this, SLOT(databaseCleared()));
    connect(dbDialog, SIGNAL(databaseCleared()), regularSingersDialog, SLOT(regularsChanged()));
    connect(activeAudioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(audioBackend_positionChanged(qint64)));
    connect(activeAudioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(audioBackend_durationChanged(qint64)));
    connect(activeAudioBackend, SIGNAL(stateChanged(AbstractAudioBackend::State)), this, SLOT(audioBackend_stateChanged(AbstractAudioBackend::State)));
    connect(activeAudioBackend, SIGNAL(pitchChanged(int)), ui->spinBoxKey, SLOT(setValue(int)));
    qDebug() << "Setting volume to " << settings->audioVolume();
    ui->sliderBmVolume->setValue(settings->audioVolume());
    connect(rotModel, SIGNAL(rotationModified()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(tickerOutputModeChanged()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(audioBackendChanged(int)), this, SLOT(audioBackendChanged(int)));
    connect(settings, SIGNAL(cdgBgImageChanged()), this, SLOT(onBgImageChange()));
    connect(activeAudioBackend, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()));
    connect(settingsDialog, SIGNAL(audioUseFaderChanged(bool)), activeAudioBackend, SLOT(setUseFader(bool)));
    activeAudioBackend->setUseFader(settings->audioUseFader());
    connect(settingsDialog, SIGNAL(audioSilenceDetectChanged(bool)), activeAudioBackend, SLOT(setUseSilenceDetection(bool)));
    activeAudioBackend->setUseSilenceDetection(settings->audioDetectSilence());
    connect(settingsDialog, SIGNAL(audioDownmixChanged(bool)), activeAudioBackend, SLOT(setDownmix(bool)));
    activeAudioBackend->setDownmix(settings->audioDownmix());
    connect(qModel, SIGNAL(queueModified(int)), rotModel, SLOT(queueModified(int)));
    connect(requestsDialog, SIGNAL(addRequestSong(int,int)), qModel, SLOT(songAdd(int,int)));
    cdgWindow->setShowBgImage(true);
    setShowBgImage(true);
    settings->restoreWindowState(cdgWindow);
    settings->restoreWindowState(requestsDialog);
    settings->restoreWindowState(regularSingersDialog);
    settings->restoreSplitterState(ui->splitter);
    settings->restoreSplitterState(ui->splitter_2);
    settings->restoreColumnWidths(ui->tableViewDB);
    settings->restoreColumnWidths(ui->tableViewQueue);
    settings->restoreColumnWidths(ui->tableViewRotation);
    if ((settings->cdgWindowFullscreen()) && (settings->showCdgWindow()))
    {
        cdgWindow->makeFullscreen();
    }
    rotationDataChanged();
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    ui->tableViewDB->horizontalHeader()->resizeSection(4,75);
    ui->tableViewQueue->hideColumn(0);
    ui->tableViewQueue->hideColumn(1);
    ui->tableViewQueue->hideColumn(2);
    ui->tableViewQueue->hideColumn(6);
    ui->tableViewQueue->hideColumn(9);
    ui->tableViewQueue->hideColumn(10);
    ui->tableViewQueue->hideColumn(11);
    ui->tableViewQueue->hideColumn(12);
    ui->tableViewQueue->horizontalHeader()->resizeSection(8, 25);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    if (activeAudioBackend->canPitchShift())
    {
        ui->tableViewQueue->horizontalHeader()->showSection(7);
    }
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
//    ui->tableViewQueue->horizontalHeader()->resizeSection(7,25);
    qModel->setHeaderData(8, Qt::Horizontal,"");
    qModel->setHeaderData(7, Qt::Horizontal, "Key");
    ui->tableViewRotation->horizontalHeader()->resizeSection(0, 25);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableViewRotation->horizontalHeader()->resizeSection(3,25);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableViewRotation->horizontalHeader()->resizeSection(4,25);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    rotModel->setHeaderData(0,Qt::Horizontal,"");
    rotModel->setHeaderData(1,Qt::Horizontal,"Singer");
    rotModel->setHeaderData(2,Qt::Horizontal,"Next Song");
    rotModel->setHeaderData(3,Qt::Horizontal,"");
    rotModel->setHeaderData(4,Qt::Horizontal,"");
    ui->statusBar->addWidget(labelSingerCount);



    bmCurrentPosition = 0;
    bmAudioBackend->setUseFader(true);
    bmAudioBackend->setUseSilenceDetection(true);

    bmPlaylistsModel = new QSqlTableModel(this, *database);
    bmPlaylistsModel->setTable("bmplaylists");
    bmPlaylistsModel->sort(2, Qt::AscendingOrder);
    bmDbDialog = new BmDbDialog(database,this);
    bmDbModel = new BmDbTableModel(this, *database);
    bmDbModel->setTable("bmsongs");
    bmDbModel->select();
    bmCurrentPlaylist = settings->bmPlaylistIndex();
    bmPlModel = new BmPlTableModel(this, *database);
    bmPlModel->select();
//    ui->actionShow_Filenames->setChecked(settings->bmShowFilenames());
//    ui->actionShow_Metadata->setChecked(settings->bmShowMetadata());
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
    ui->actionDisplay_Filenames->setChecked(settings->bmShowFilenames());
    ui->actionDisplay_Metadata->setChecked(settings->bmShowMetadata());
    settings->restoreSplitterState(ui->splitterBm);
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


    connect(bmAudioBackend, SIGNAL(stateChanged(AbstractAudioBackend::State)), this, SLOT(bmMediaStateChanged(AbstractAudioBackend::State)));
    connect(bmAudioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(bmMediaPositionChanged(qint64)));
    connect(bmAudioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(bmMediaDurationChanged(qint64)));
    connect(bmAudioBackend, SIGNAL(volumeChanged(int)), ui->sliderBmVolume, SLOT(setValue(int)));
    connect(bmDbDialog, SIGNAL(bmDbUpdated()), this, SLOT(bmDbUpdated()));
    connect(bmDbDialog, SIGNAL(bmDbCleared()), this, SLOT(bmDbCleared()));

    ui->sliderBmVolume->setValue(initialBMVol);
    ui->sliderVolume->setValue(initialKVol);
//    bmAudioBackend->setVolume(initialBMVol);
//    activeAudioBackend->setVolume(initialKVol);

}

void MainWindow::play(QString karaokeFilePath)
{
    if (activeAudioBackend->state() != AbstractAudioBackend::PausedState)
    {
        if (activeAudioBackend->state() == AbstractAudioBackend::PlayingState)
            activeAudioBackend->stop();
        if (karaokeFilePath.endsWith(".zip", Qt::CaseInsensitive))
        {
            OkArchive archive(karaokeFilePath);
            if ((archive.checkCDG()) && (archive.checkMP3()))
            {
                if (!archive.extractMP3(khTmpDir->path() + QDir::separator() + "tmp.mp3"))
                {
                    QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract mp3 file."),QMessageBox::Ok);
                    return;
                }
                cdg->FileOpen(archive.getCDGData());
                cdg->Process();
                cdgWindow->setShowBgImage(false);
                setShowBgImage(false);
                activeAudioBackend->setMedia(khTmpDir->path() + QDir::separator() + "tmp.mp3");
//                ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_OUT);
                bmAudioBackend->fadeOut(false);
                activeAudioBackend->play();
            }
            else
            {
                QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG or mp3 file missing or corrupt."),QMessageBox::Ok);
            }
        }
        else if (karaokeFilePath.endsWith(".cdg", Qt::CaseInsensitive))
        {
            QFile cdgFile(karaokeFilePath);
            if (!cdgFile.exists())
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file missing."),QMessageBox::Ok);
                return;
            }
            else if (cdgFile.size() == 0)
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"),QMessageBox::Ok);
                return;
            }
            QString mp3fn;
            QString baseFn = karaokeFilePath;
            baseFn.chop(3);
            if (QFile::exists(baseFn + "mp3"))
                mp3fn = baseFn + "mp3";
            else if (QFile::exists(baseFn + "Mp3"))
                mp3fn = baseFn + "Mp3";
            else if (QFile::exists(baseFn + "MP3"))
                mp3fn = baseFn + "MP3";
            else if (QFile::exists(baseFn + "mP3"))
                mp3fn = baseFn + "mP3";
            else
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("mp3 file missing."),QMessageBox::Ok);
                return;
            }
            QFile mp3File(mp3fn);
            if (mp3File.size() == 0)
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("mp3 file contains no data"),QMessageBox::Ok);
                return;
            }
            cdg->FileOpen(cdgFile.fileName().toStdString());
            cdg->Process();
            activeAudioBackend->setMedia(mp3fn);
//            ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_OUT);
            bmAudioBackend->fadeOut();
            activeAudioBackend->play();
        }
        else
            return;
    }
    else if (activeAudioBackend->state() == AbstractAudioBackend::PausedState)
    {
        if (settings->recordingEnabled())
            audioRecorder->unpause();
        activeAudioBackend->play();
    }
}

MainWindow::~MainWindow()
{
    settings->bmSetVolume(ui->sliderBmVolume->value());
    settings->setAudioVolume(ui->sliderBmVolume->value());
    qWarning() << "Saving volumes - K: " << settings->audioVolume() << " BM: " << settings->bmVolume();
    settings->saveSplitterState(ui->splitter);
    settings->saveSplitterState(ui->splitter_2);
    settings->saveColumnWidths(ui->tableViewDB);
    settings->saveColumnWidths(ui->tableViewRotation);
    settings->saveColumnWidths(ui->tableViewQueue);
    if (!settings->cdgWindowFullscreen())
        settings->saveWindowState(cdgWindow);
    settings->saveWindowState(requestsDialog);
    settings->saveWindowState(regularSingersDialog);
    settings->saveWindowState(this);
    settings->setShowCdgWindow(cdgWindow->isVisible());

    settings->saveSplitterState(ui->splitterBm);
    settings->saveColumnWidths(ui->tableViewBmDb);
    settings->saveColumnWidths(ui->tableViewBmPlaylist);
    settings->bmSetPlaylistIndex(ui->comboBoxBmPlaylists->currentIndex());

    delete cdg;
    delete khDir;
    delete database;
    delete ui;
    qDeleteAll(audioBackends->begin(), audioBackends->end());
    delete audioBackends;
    delete khTmpDir;
}

void MainWindow::search()
{
    dbModel->search(ui->lineEdit->text());
}

void MainWindow::songdbUpdated()
{
    dbModel->select();
}

void MainWindow::databaseCleared()
{
    dbModel->select();
    rotModel->select();
    qModel->setSinger(-1);
}

void MainWindow::on_buttonStop_clicked()
{
    activeAudioBackend->stop();
    bmAudioBackend->fadeIn();
//    ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
}

void MainWindow::on_buttonPause_clicked()
{
    if (activeAudioBackend->state() == AbstractAudioBackend::PausedState)
    {
        activeAudioBackend->play();
    }
    else
        activeAudioBackend->pause();
}

void MainWindow::on_lineEdit_returnPressed()
{
    search();
}

void MainWindow::on_tableViewDB_activated(const QModelIndex &index)
{
    if (qModel->singer() >= 0)
    {
        qModel->songAdd(index.sibling(index.row(),0).data().toInt());
    }
}

void MainWindow::on_buttonAddSinger_clicked()
{
    rotModel->singerAdd(ui->editAddSinger->text());
    ui->editAddSinger->clear();
}

void MainWindow::on_editAddSinger_returnPressed()
{
    rotModel->singerAdd(ui->editAddSinger->text());
    ui->editAddSinger->clear();
}

void MainWindow::on_tableViewRotation_activated(const QModelIndex &index)
{
    if (index.column() < 3)
    {
        int singerId = index.sibling(index.row(),0).data().toInt();
        QString nextSongPath = rotModel->nextSongPath(singerId);
        if (nextSongPath != "")
        {
            play(nextSongPath);
            activeAudioBackend->setPitchShift(rotModel->nextSongKeyChg(singerId));
            rotDelegate->setCurrentSinger(singerId);
            rotModel->setCurrentSinger(singerId);
            ui->labelArtist->setText(rotModel->nextSongArtist(singerId));
            ui->labelTitle->setText(rotModel->nextSongTitle(singerId));
            ui->labelSinger->setText(rotModel->getSingerName(singerId));
            qModel->songSetPlayed(rotModel->nextSongQueueId(singerId));
        }
    }
}

void MainWindow::on_tableViewRotation_clicked(const QModelIndex &index)
{
    if (index.column() == 4)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Are you sure you want to remove this singer?");
        msgBox.setInformativeText("Unless this singer is a tracked regular, you will be unable retrieve any queue data for this singer once they are deleted.");
        QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();
        if (msgBox.clickedButton() == yesButton)
        {
            int singerId = index.sibling(index.row(),0).data().toInt();
            qModel->setSinger(-1);
            if (rotModel->currentSinger() == singerId)
            {
                rotModel->setCurrentSinger(-1);
                rotDelegate->setCurrentSinger(-1);
            }
            rotModel->singerDelete(singerId);
            ui->tableViewRotation->clearSelection();
            ui->tableViewQueue->clearSelection();
            return;
        }
    }
    if (index.column() == 3)
    {
        if (!rotModel->singerIsRegular(index.sibling(index.row(),0).data().toInt()))
        {
            QString name = index.sibling(index.row(),1).data().toString();
            if (rotModel->regularExists(name))
                QMessageBox::warning(this, "Naming conflict!","A regular singer named " + name + " already exists. You must either rename or delete the existing regular singer, or rename the singer being saved as a regular. The operation has been cancelled.",QMessageBox::Ok);
            else
                rotModel->singerMakeRegular(index.sibling(index.row(),0).data().toInt());
        }
        else
        {
            QMessageBox msgBox(this);
            msgBox.setText("Are you sure you want to disable regular tracking for this singer?");
            msgBox.setInformativeText("Doing so will not remove the regular singer entry, but it will prevent any changes made to the singer's queue from being saved to the regular singer until the regular singer is either reloaded or the rotation singer is re-merged with the regular singer.");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton)
            {
                rotModel->singerDisableRegularTracking(index.sibling(index.row(),0).data().toInt());
            }
        }
    }
    qModel->setSinger(index.sibling(index.row(),0).data().toInt());
}

void MainWindow::on_tableViewQueue_activated(const QModelIndex &index)
{

    play(index.sibling(index.row(), 6).data().toString());
    activeAudioBackend->setPitchShift(index.sibling(index.row(),7).data().toInt());
    ui->labelSinger->setText(rotModel->getSingerName(index.sibling(index.row(),1).data().toInt()));
    ui->labelArtist->setText(index.sibling(index.row(),3).data().toString());
    ui->labelTitle->setText(index.sibling(index.row(),4).data().toString());
    rotModel->setCurrentSinger(index.sibling(index.row(),1).data().toInt());
    rotDelegate->setCurrentSinger(index.sibling(index.row(),1).data().toInt());
    qModel->songSetPlayed(index.sibling(index.row(),0).data().toInt());
}

void MainWindow::on_actionManage_DB_triggered()
{
    dbDialog->showNormal();
}

void MainWindow::on_actionExport_Regulars_triggered()
{
    regularExportDialog->show();
}

void MainWindow::on_actionImport_Regulars_triggered()
{
    regularImportDialog->show();
}

void MainWindow::on_actionSettings_triggered()
{
    settingsDialog->show();
}

void MainWindow::on_actionRegulars_triggered()
{
    regularSingersDialog->show();
}

void MainWindow::on_actionIncoming_Requests_triggered()
{
    requestsDialog->show();
}

void MainWindow::songDroppedOnSinger(int singerId, int songId, int dropRow)
{
    qModel->setSinger(singerId);
    qModel->songAdd(songId);
    ui->tableViewRotation->clearSelection();
    QItemSelectionModel *selmodel = ui->tableViewRotation->selectionModel();
    QModelIndex topLeft;
    QModelIndex bottomRight;
    topLeft = rotModel->index(dropRow, 0, QModelIndex());
    bottomRight = rotModel->index(dropRow, 4, QModelIndex());
    QItemSelection selection(topLeft, bottomRight);
    selmodel->select(selection, QItemSelectionModel::Select);
}

void MainWindow::on_pushButton_clicked()
{
    search();
}

void MainWindow::on_tableViewQueue_clicked(const QModelIndex &index)
{
    if (index.column() == 8)
    {
        qModel->songDelete(index.sibling(index.row(),0).data().toInt());
    }
}

void MainWindow::notify_user(QString message)
{
    QMessageBox msgbox(this);
    msgbox.setText(message);
    msgbox.exec();
}

void MainWindow::on_buttonClearRotation_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("This action will clear all rotation singers and queues. This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton)
    {
        rotModel->clearRotation();
        qModel->setSinger(-1);
    }
}

void MainWindow::clearQueueSort()
{
    ui->tableViewQueue->sortByColumn(-1);
}


void MainWindow::on_buttonClearQueue_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("This action will clear all queued songs for the selected singer.  If the singer is a regular singer, it will delete their saved regular songs as well! This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        qModel->clearQueue();
    }
}

void MainWindow::on_spinBoxKey_valueChanged(int arg1)
{
    if ((activeAudioBackend->state() == AbstractAudioBackend::PlayingState) || (activeAudioBackend->state() == AbstractAudioBackend::PausedState))
    {
        activeAudioBackend->setPitchShift(arg1);
        if (arg1 > 0)
            ui->spinBoxKey->setPrefix("+");
        else
            ui->spinBoxKey->setPrefix("");
    }
    else
        ui->spinBoxKey->setValue(0);
}

void MainWindow::on_sliderVolume_valueChanged(int value)
{
    activeAudioBackend->setVolume(value);
}

void MainWindow::audioBackend_positionChanged(qint64 position)
{
    if (activeAudioBackend->state() == AbstractAudioBackend::PlayingState)
    {
        if (cdg->GetLastCDGUpdate() >= position)
        {
            if (!cdg->SkipFrame(position))
            {
                unsigned char* rgbdata;
                rgbdata = cdg->GetImageByTime(position);
                QImage img(rgbdata, 300, 216, QImage::Format_RGB888);
//                ui->cdgOutput->setPixmap(QPixmap::fromImage(img));
                ui->cdgVideoWidget->videoSurface()->present(QVideoFrame(img));
                cdgWindow->updateCDG(img);
                free(rgbdata);
            }
        }
        if (!sliderPositionPressed)
        {
            ui->sliderProgress->setMaximum(activeAudioBackend->duration());
            ui->sliderProgress->setValue(position);
        }
        ui->labelElapsedTime->setText(activeAudioBackend->msToMMSS(position));
        ui->labelRemainTime->setText(activeAudioBackend->msToMMSS(activeAudioBackend->duration() - position));
    }
}

void MainWindow::audioBackend_durationChanged(qint64 duration)
{
    ui->labelTotalTime->setText(activeAudioBackend->msToMMSS(duration));
}

void MainWindow::audioBackend_stateChanged(AbstractAudioBackend::State state)
{
    if (state == AbstractAudioBackend::StoppedState)
    {
        audioRecorder->stop();
        cdg->VideoClose();
        ui->labelArtist->setText("None");
        ui->labelTitle->setText("None");
        ui->labelSinger->setText("None");
        ui->labelElapsedTime->setText("0:00");
        ui->labelRemainTime->setText("0:00");
        ui->labelTotalTime->setText("0:00");
        ui->sliderProgress->setValue(0);
        setShowBgImage(true);
        cdgWindow->setShowBgImage(true);
        bmAudioBackend->fadeIn(false);
    }
    if (state == AbstractAudioBackend::EndOfMediaState)
    {
        audioRecorder->stop();
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        activeAudioBackend->stop(true);
    }
    if (state == AbstractAudioBackend::PausedState)
    {
            audioRecorder->pause();
    }
    if (state == AbstractAudioBackend::PlayingState)
    {
        if (settings->recordingEnabled())
        {
            int curSingerId = rotModel->currentSinger();
            QString singerName = rotModel->getSingerName(curSingerId);
            QString artist = ui->labelArtist->text();
            QString title = ui->labelTitle->text();
            audioRecorder->record(singerName + " - " + artist + " - " + title);
        }
    }
}

void MainWindow::on_sliderProgress_sliderMoved(int position)
{
    Q_UNUSED(position);
}

void MainWindow::on_buttonRegulars_clicked()
{
    regularSingersDialog->show();
}

void MainWindow::rotationDataChanged()
{
    QString statusBarText = "Singers: ";
    statusBarText += QString::number(rotModel->rowCount());
    labelSingerCount->setText(statusBarText);
    QString tickerText = "Singers: ";
    tickerText += QString::number(rotModel->rowCount());
    tickerText += " | Current: ";
    int displayPos;
    QString curSinger = rotModel->getSingerName(rotModel->currentSinger());
    if (curSinger != "")
    {
        tickerText += curSinger;
        displayPos = rotModel->getSingerPosition(rotModel->currentSinger());
    }
    else
    {
        tickerText += "None";
        displayPos = -1;
    }
    int listSize;
    if (settings->tickerFullRotation() || (rotModel->rowCount() < settings->tickerShowNumSingers()))
    {
        listSize = rotModel->rowCount();
        tickerText += " | Upcoming: ";
    }
    else
    {
        listSize = settings->tickerShowNumSingers();
        tickerText += " | Next ";
        tickerText += QString::number(settings->tickerShowNumSingers());
        tickerText += " Singers: ";
    }
    for (int i=0; i < listSize; i++)
    {
        if (displayPos + 1 < rotModel->rowCount())
            displayPos++;
        else
            displayPos = 0;
        tickerText += QString::number(i + 1);
        tickerText += ") ";
        tickerText += rotModel->getSingerName(rotModel->singerIdAtPosition(displayPos));
        tickerText += "  ";
    }
    if (rotModel->rowCount() == 0)
        tickerText += " None";
    cdgWindow->setTickerText(tickerText);
}

void MainWindow::silenceDetected()
{
    if (cdg->GetLastCDGUpdate() < activeAudioBackend->position())
    {
        activeAudioBackend->stop(true);
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        bmAudioBackend->fadeIn();
    }
}

void MainWindow::audioBackendChanged(int index)
{
    activeAudioBackend = audioBackends->at(index);
}

void MainWindow::on_tableViewDB_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewDB->indexAt(pos);
    if (index.isValid())
    {
        previewZip = index.sibling(index.row(), 5).data().toString();
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", this, SLOT(previewCdg()));
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_tableViewRotation_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewRotation->indexAt(pos);
    if (index.isValid())
    {
           m_rtClickRotationSingerId = index.sibling(index.row(),0).data().toInt();
           QMenu contextMenu(this);
           contextMenu.addAction("Rename", this, SLOT(renameSinger()));
           contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::renameSinger()
{
    bool ok;
    QString currentName = rotModel->getSingerName(m_rtClickRotationSingerId);
    QString name = QInputDialog::getText(this, "Rename singer", "New name:", QLineEdit::Normal, currentName, &ok);
    if (ok && !name.isEmpty())
    {
        if ((name.toLower() == currentName.toLower()) && (name != currentName))
        {
            // changing capitalization only
            rotModel->singerSetName(m_rtClickRotationSingerId, name);
        }
        else if (rotModel->singerExists(name))
        {
            QMessageBox::warning(this, "Singer exists!","A singer named " + name + " already exists. Please choose a unique name and try again. The operation has been cancelled.",QMessageBox::Ok);
        }
        else
        {
            rotModel->singerSetName(m_rtClickRotationSingerId, name);
        }

    }
}

void MainWindow::on_tableViewQueue_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewQueue->indexAt(pos);
    if (index.isValid())
    {   
        previewZip = index.sibling(index.row(), 6).data().toString();
        m_rtClickQueueSongId = index.sibling(index.row(), 0).data().toInt();
        dlgKeyChange->setActiveSong(m_rtClickQueueSongId);
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", this, SLOT(previewCdg()));
        contextMenu.addAction("Set Key Change", this, SLOT(setKeyChange()));
        contextMenu.addAction("Toggle played", this, SLOT(toggleQueuePlayed()));
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_sliderProgress_sliderPressed()
{
    sliderPositionPressed = true;
}

void MainWindow::on_sliderProgress_sliderReleased()
{
    activeAudioBackend->setPosition(ui->sliderProgress->value());
    sliderPositionPressed = false;
}

void MainWindow::setKeyChange()
{
    dlgKeyChange->show();
}

void MainWindow::toggleQueuePlayed()
{
    qModel->songSetPlayed(m_rtClickQueueSongId, !qModel->getSongPlayed(m_rtClickQueueSongId));
}

void MainWindow::regularNameConflict(QString name)
{
    QMessageBox::warning(this, "Regular singer exists!","A regular singer named " + name + " already exists. You must either delete or rename the existing regular first, or rename the new regular singer and try again. The operation has been cancelled.",QMessageBox::Ok);
}

void MainWindow::regularAddError(QString errorText)
{
    QMessageBox::warning(this, "Error adding regular singer!", errorText,QMessageBox::Ok);
}

void MainWindow::previewCdg()
{
    cdgPreviewDialog = new DlgCdgPreview(this);
    cdgPreviewDialog->setAttribute(Qt::WA_DeleteOnClose);
    cdgPreviewDialog->setSourceFile(previewZip);
    cdgPreviewDialog->preview();
}

void MainWindow::setShowBgImage(bool show)
{
    if (show)
    {
        QImage bgImage(ui->cdgVideoWidget->size(), QImage::Format_ARGB32);
        QPainter painter(&bgImage);
        QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
        renderer.render(&painter);
        ui->cdgVideoWidget->videoSurface()->present(QVideoFrame(bgImage));
    }
}

void MainWindow::onBgImageChange()
{
   if (activeAudioBackend->state() == AbstractAudioBackend::StoppedState)
       cdgWindow->setShowBgImage(true);
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

void MainWindow::on_actionManage_Break_DB_triggered()
{
    bmDbDialog->show();
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
            bmAudioBackend->setVolume(ui->sliderBmVolume->value());
            ui->labelBmPlaying->setText(song);
            ui->labelBmNext->setText(nextSong);
            bmPlDelegate->setCurrentSong(bmCurrentPosition);
            bmPlModel->select();
        }
        if (newState == AbstractAudioBackend::StoppedState)
        {
            ui->labelBmPlaying->setText("None");
            ui->labelBmNext->setText("None");
            ui->labelBmDuration->setText("00:00");
            ui->labelBmRemaining->setText("00:00");
            ui->labelBmPosition->setText("00:00");
            ui->sliderBmPosition->setValue(0);
        }
    }
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

void MainWindow::on_buttonBmPause_clicked(bool checked)
{
    if (checked)
        bmAudioBackend->pause();
    else
        bmAudioBackend->play();
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

void MainWindow::on_actionDisplay_Metadata_toggled(bool arg1)
{
    ui->tableViewBmDb->setColumnHidden(1, !arg1);
    ui->tableViewBmDb->setColumnHidden(2, !arg1);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(1, 100);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(2, 100);
    ui->tableViewBmPlaylist->setColumnHidden(3, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(4, !arg1);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(3, 100);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(4, 100);
    settings->bmSetShowMetadata(arg1);
}

void MainWindow::on_actionDisplay_Filenames_toggled(bool arg1)
{
    ui->tableViewBmDb->setColumnHidden(4, !arg1);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(4, 100);
    ui->tableViewBmPlaylist->setColumnHidden(5, !arg1);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(5, 100);
    settings->bmSetShowFilenames(arg1);
}

void MainWindow::on_actionManage_Karaoke_DB_triggered()
{
    dbDialog->showNormal();
}

void MainWindow::on_actionPlaylistNew_triggered()
{
    bool ok;
    QString title = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal, tr("New Playlist"), &ok);
    if (ok && !title.isEmpty())
    {
        bmAddPlaylist(title);
    }
}

void MainWindow::on_actionPlaylistImport_triggered()
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
                QString queryString = "INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) VALUES(\"" + artist + "\",\"" + title + "\",\"" + files.at(i) + "\",\"" + filename + "\",\"" + duration + "\",\"" + artist + title + filename + "\")";
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
            QString sql = "INSERT INTO bmplsongs (playlist,position,artist,title,filename,duration,path) VALUES(" + QString::number(bmCurrentPlaylist) + "," + QString::number(i) + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + "," + sIdStr + ")";
            query.exec(sql);
        }
        query.exec("COMMIT TRANSACTION");
        bmPlModel->select();
    }
}

void MainWindow::on_actionPlaylistExport_triggered()
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

void MainWindow::on_actionPlaylistDelete_triggered()
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
        query.exec("DELETE FROM bmplsongs WHERE playlist == " + QString::number(bmPlModel->currentPlaylist()));
        query.exec("DELETE FROM bmplaylists WHERE playlistid == " + QString::number(bmPlModel->currentPlaylist()));
        bmPlaylistsModel->select();
        if (bmPlaylistsModel->rowCount() == 0)
        {
            bmAddPlaylist("Default");
        }
        ui->comboBoxBmPlaylists->setCurrentIndex(0);
        bmPlModel->setCurrentPlaylist(bmPlaylistsModel->index(0,0).data().toInt());
    }
}



void MainWindow::on_buttonBmSearch_clicked()
{
    bmDbModel->search(ui->lineEditBmSearch->text());
}

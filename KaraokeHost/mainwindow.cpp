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
#include <QMessageBox>
#include <iostream>
#include <QTemporaryDir>
#include <QDir>
#include "khaudiobackendqmediaplayer.h"
#ifdef USE_GSTREAMER
#include "khaudiobackendgstreamer.h"
#endif
#include "khzip.h"
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QMenu>
#include "khdb.h"


KhSettings *settings;
KhDb *db;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    sliderPositionPressed = false;
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("KaraokeHost");
    ui->setupUi(this);
    db = new KhDb(this);
    labelSingerCount = new QLabel(ui->statusBar);
    khDir = new QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!khDir->exists())
    {
        khDir->mkpath(khDir->absolutePath());
    }
    settings = new KhSettings(this);
    settings->restoreWindowState(this);
    database = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    database->setDatabaseName(khDir->absolutePath() + QDir::separator() + "karaokehost.sqlite");
    database->open();
    QSqlQuery query("CREATE TABLE IF NOT EXISTS dbSongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, DiscId COLLATE NOCASE, 'Duration' INTEGER, path VARCHAR(700) NOT NULL UNIQUE, filename COLLATE NOCASE)");
    query.exec("CREATE TABLE IF NOT EXISTS rotationSingers ( singerid INTEGER PRIMARY KEY AUTOINCREMENT, name COLLATE NOCASE UNIQUE, 'position' INTEGER NOT NULL, 'regular' LOGICAL DEFAULT(0), 'regularid' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS queueSongs ( qsongid INTEGER PRIMARY KEY AUTOINCREMENT, singer INT, song INTEGER NOT NULL, artist INT, title INT, discid INT, path INT, keychg INT, played LOGICAL DEFAULT(0), 'position' INT, 'regsong' LOGICAL DEFAULT(0), 'regsongid' INTEGER DEFAULT(-1), 'regsingerid' INTEGER DEFAULT(-1))");
    query.exec("CREATE TABLE IF NOT EXISTS regularSingers ( name VARCHAR(30) NOT NULL UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS regularSongs ( singer INTEGER NOT NULL, song INTEGER NOT NULL, 'keychg' INTEGER, 'position' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS sourceDirs ( path VARCHAR(255) UNIQUE, pattern INTEGER)");
    query.exec("PRAGMA synchronous = OFF");
//    query.exec("PRAGMA journal_mode = OFF");
    sortColDB = 1;
    sortDirDB = 0;
    songdbmodel = new SongDBTableModel(this);
    songdbmodel->loadFromDB();
    dbModel = new DbTableModel(this, *database);
    dbModel->select();
    qModel = new QueueModel(this, *database);
    qModel->select();
    rotModel = new RotationModel(this, *database);
    rotModel->select();
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    regularSingers = new KhRegularSingers(songdbmodel->getDbSongs(),this);
    //singers = new KhSingers(regularSingers, this);
    songCurrent = NULL;
    rotationmodel = new RotationTableModel(regularSingers, this);
    ui->tableViewRotation->setModel(rotModel);
    rotDelegate = new RotationItemDelegate(this);
    ui->tableViewRotation->setItemDelegate(rotDelegate);
//    ui->treeViewRotation->header()->setSectionResizeMode(0,QHeaderView::Fixed);
//    ui->treeViewRotation->header()->setSectionResizeMode(3,QHeaderView::Fixed);
//    ui->treeViewRotation->header()->setSectionResizeMode(4,QHeaderView::Fixed);
//    ui->treeViewRotation->header()->setSectionResizeMode(5,QHeaderView::Fixed);
//    ui->treeViewRotation->header()->resizeSection(0,22);
//    ui->treeViewRotation->header()->resizeSection(3,22);
//    ui->treeViewRotation->header()->resizeSection(4,22);
//    ui->treeViewRotation->header()->resizeSection(5,22);
    queuemodel = new QueueTableModel(rotationmodel, this);
//    ui->treeViewQueue->sortByColumn(-1);
    ui->tableViewQueue->setModel(qModel);
//    ui->treeViewQueue->header()->setSectionResizeMode(3,QHeaderView::Fixed);
//    ui->treeViewQueue->header()->setSectionResizeMode(4,QHeaderView::Fixed);
//    ui->treeViewQueue->header()->resizeSection(4,22);
//    ui->treeViewQueue->header()->resizeSection(3,50);

    khTmpDir = new QTemporaryDir();
    dbDialog = new DlgDatabase(this);
    dlgKeyChange = new DlgKeyChange(rotationmodel, this);
    regularSingersDialog = new DlgRegularSingers(regularSingers, rotationmodel, this);
    regularExportDialog = new DlgRegularExport(regularSingers, this);
    regularImportDialog = new DlgRegularImport(songdbmodel->getDbSongs(), regularSingers, this);
    requestsDialog = new DlgRequests(rotModel, this);
    cdgPreviewDialog = new DlgCdgPreview(this);
    cdgWindow = new DlgCdg(this, Qt::Window);
    if (settings->showCdgWindow())
    {
        cdgWindow->show();
        if (settings->cdgWindowFullscreen())
        {
            QPoint topLeft = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor()).topLeft();
            cdgWindow->move(topLeft);
        }
    }
    cdg = new CDG;
//    songdbmodel = new SongDBTableModel(this);
//    songdbmodel->loadFromDB();
    ui->tableViewDB->setModel(dbModel);
    dbDelegate = new DbItemDelegate(this);
    ui->tableViewDB->setItemDelegate(dbDelegate);

//    ui->treeViewDB->header()->setSectionResizeMode(3,QHeaderView::Fixed);
//    ui->treeViewDB->header()->resizeSection(3,60);
    ipcClient = new KhIPCClient("bmControl",this);
    audioBackends = new KhAudioBackends;
#ifdef USE_GSTREAMER
    qDebug() << "Initializing audio backend: GStreamer";
    audioBackends->push_back(new KhAudioBackendGStreamer(this));
#endif
#ifdef USE_QMEDIAPLAYER
    qDebug() << "Initializing audio backend: QMediaPlayer (QtMultimedia)";
    audioBackends->push_back(new KhAudioBackendQMediaPlayer(this));
#endif
    if (audioBackends->count() < 1)
        qCritical("No audio backends available!");
    if (settings->audioBackend() < audioBackends->count())
        activeAudioBackend = audioBackends->at(settings->audioBackend());
    else
    {
        settings->setAudioBackend(0);
        activeAudioBackend = audioBackends->at(0);
    }
    qDebug() << "Audio backend: " << activeAudioBackend->backendName();
    if (activeAudioBackend->canFade())
        activeAudioBackend->setUseFader(settings->audioUseFader());
    if (!activeAudioBackend->canPitchShift())
    {
        ui->groupBoxKey->hide();
        ui->tableViewQueue->hideColumn(7);
    }
    audioRecorder = new KhAudioRecorder(this);
    settingsDialog = new DlgSettings(audioBackends, this);
    connect(rotModel, SIGNAL(songDroppedOnSinger(int,int,int)), this, SLOT(songDroppedOnSinger(int,int,int)));
    connect(activeAudioBackend, SIGNAL(volumeChanged(int)), ui->sliderVolume, SLOT(setValue(int)));
    connect(dbDialog, SIGNAL(databaseUpdated()), this, SLOT(songdbUpdated()));
    connect(dbDialog, SIGNAL(databaseCleared()), this, SLOT(databaseCleared()));
    connect(rotationmodel, SIGNAL(songDroppedOnSinger(int,int,int)), this, SLOT(songDroppedOnSinger(int,int,int)));
    connect(rotationmodel, SIGNAL(notify_user(QString)), this, SLOT(notify_user(QString)));
    connect(queuemodel, SIGNAL(itemMoved()), this, SLOT(clearQueueSort()));
    connect(activeAudioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(audioBackend_positionChanged(qint64)));
    connect(activeAudioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(audioBackend_durationChanged(qint64)));
    connect(activeAudioBackend, SIGNAL(stateChanged(KhAbstractAudioBackend::State)), this, SLOT(audioBackend_stateChanged(KhAbstractAudioBackend::State)));
    connect(activeAudioBackend, SIGNAL(pitchChanged(int)), ui->spinBoxKey, SLOT(setValue(int)));
    qDebug() << "Setting volume to " << settings->audioVolume();
    activeAudioBackend->setVolume(settings->audioVolume());
    ui->sliderVolume->setValue(settings->audioVolume());
    connect(settingsDialog, SIGNAL(showCdgWindowChanged(bool)), cdgWindow, SLOT(setVisible(bool)));
    connect(settingsDialog, SIGNAL(cdgWindowFullScreenChanged(bool)), cdgWindow, SLOT(setFullScreen(bool)));
    connect(regularSingers, SIGNAL(dataChanged()), rotationmodel, SIGNAL(layoutChanged()));
    connect(regularSingers, SIGNAL(dataAboutToChange()), rotationmodel, SIGNAL(layoutAboutToBeChanged()));
    connect(rotModel, SIGNAL(rotationModified()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(tickerOutputModeChanged()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(audioBackendChanged(int)), this, SLOT(audioBackendChanged(int)));
    connect(activeAudioBackend, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()));
    connect(settingsDialog, SIGNAL(audioUseFaderChanged(bool)), activeAudioBackend, SLOT(setUseFader(bool)));
    activeAudioBackend->setUseFader(settings->audioUseFader());
    connect(settingsDialog, SIGNAL(audioSilenceDetectChanged(bool)), activeAudioBackend, SLOT(setUseSilenceDetection(bool)));
    activeAudioBackend->setUseSilenceDetection(settings->audioDetectSilence());
    connect(settingsDialog, SIGNAL(audioDownmixChanged(bool)), activeAudioBackend, SLOT(setDownmix(bool)));
    activeAudioBackend->setDownmix(settings->audioDownmix());
    connect(qModel, SIGNAL(queueModified()), rotModel, SLOT(queueModified()));
    connect(requestsDialog, SIGNAL(addRequestSong(int,int)), qModel, SLOT(songAdd(int,int)));
    QImage cdgBg;
    if (settings->cdgDisplayBackgroundImage() != "")
    {
        qDebug() << "Attempting to load CDG background: " << settings->cdgDisplayBackgroundImage();
        if (!cdgBg.load(settings->cdgDisplayBackgroundImage()))
        {
            qDebug() << "Failed to load, loading default resource";
            cdgBg.load(":/icons/Icons/openkjlogo1.png");
        }
        else
            qDebug() << "Loaded OK";
    }
    else
    {
        cdgBg.load(":/icons/Icons/openkjlogo1.png");
        qDebug() << "No CDG background image specified, loading default resource";
    }
    cdgWindow->updateCDG(cdgBg,true);
    ui->cdgOutput->setPixmap(QPixmap::fromImage(QImage(":/icons/Icons/openkjlogo1.png")));
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
    ui->tableViewQueue->hideColumn(0);
    ui->tableViewQueue->hideColumn(1);
    ui->tableViewQueue->hideColumn(2);
    ui->tableViewQueue->hideColumn(6);
    ui->tableViewQueue->hideColumn(8);
    //ui->tableViewQueue->hideColumn(9);
    ui->tableViewQueue->hideColumn(11);
    ui->tableViewQueue->hideColumn(12);
//    ui->tableViewRotation->hideColumn(4);
    ui->tableViewRotation->horizontalHeader()->resizeSection(0, 20);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableViewRotation->horizontalHeader()->resizeSection(3,20);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableViewRotation->horizontalHeader()->resizeSection(4,20);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    rotModel->setHeaderData(0,Qt::Horizontal,"");
    rotModel->setHeaderData(1,Qt::Horizontal,"Singer");
    rotModel->setHeaderData(2,Qt::Horizontal,"Next Song");
    rotModel->setHeaderData(3,Qt::Horizontal,"");
    rotModel->setHeaderData(4,Qt::Horizontal,"");



    ui->statusBar->addWidget(labelSingerCount);
    rtClickQueueSong = NULL;
}

void MainWindow::play(QString zipFilePath)
{

    if (activeAudioBackend->state() != KhAbstractAudioBackend::PausedState)
    {
        if (activeAudioBackend->state() == KhAbstractAudioBackend::PlayingState)
            activeAudioBackend->stop();
        KhZip zip(zipFilePath);
        int duration = zip.getSongDuration();
        bool cdgOk = zip.extractCdg(QDir(khTmpDir->path()));
        bool mp3Ok = zip.extractMp3(QDir(khTmpDir->path()));
        if (mp3Ok && cdgOk)
        {
            QFile cdgFile(khTmpDir->path() + QDir::separator() + "tmp.cdg");
            if (cdgFile.size() == 0)
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"),QMessageBox::Ok);
                return;
            }
            QFile mp3File(khTmpDir->path() + QDir::separator() + "tmp.mp3");
            if (mp3File.size() == 0)
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("mp3 file contains no data"),QMessageBox::Ok);
                return;
            }

            cdg->FileOpen(khTmpDir->path().toStdString() + QDir::separator().toLatin1() + "tmp.cdg");
            cdg->Process();
            activeAudioBackend->setMedia(khTmpDir->path() + QDir::separator() + "tmp.mp3");
            ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_OUT);
            activeAudioBackend->play();
//            if (settings->recordingEnabled())
//                audioRecorder->record(rotationmodel->getCurrent()->name() + " - " + songCurrent->Artist + " - " + songCurrent->Title);
//            ui->labelArtist->setText(songCurrent->Artist);
//            ui->labelTitle->setText(songCurrent->Title);
            qDebug() << "Duration from cdg: " << activeAudioBackend->msToMMSS(duration);
        }
        else
        {
            QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG or mp3 file missing."),QMessageBox::Ok);
        }
    }
    else if (activeAudioBackend->state() == KhAbstractAudioBackend::PausedState)
    {
        if (settings->recordingEnabled())
            audioRecorder->unpause();
        activeAudioBackend->play();
    }
}

MainWindow::~MainWindow()
{
    settings->saveSplitterState(ui->splitter);
    settings->saveSplitterState(ui->splitter_2);
    settings->saveColumnWidths(ui->tableViewDB);
    settings->saveColumnWidths(ui->tableViewRotation);
    settings->saveColumnWidths(ui->tableViewQueue);
    settings->saveWindowState(cdgWindow);
    settings->saveWindowState(requestsDialog);
    settings->saveWindowState(regularSingersDialog);
    settings->saveWindowState(this);
    settings->setShowCdgWindow(cdgWindow->isVisible());
    settings->setAudioVolume(ui->sliderVolume->value());
    delete cdg;
    delete khDir;
    delete database;
    delete regularSingers;
    //delete songCurrent;
    delete ui;
    qDeleteAll(audioBackends->begin(), audioBackends->end());
    delete audioBackends;
    delete khTmpDir;
}

void MainWindow::search()
{
//    QString termsstr;
//    termsstr = ui->lineEdit->text();
//    songdbmodel->applyFilter(termsstr);
    dbModel->search(ui->lineEdit->text());
}

void MainWindow::songdbUpdated()
{
    dbModel->select();
}

void MainWindow::databaseCleared()
{
    songdbmodel->loadFromDB();
    rotationmodel->clear();
    regularSingers->clear();
    ui->tableViewDB->clearSelection();
    ui->tableViewRotation->clearSelection();

}

void MainWindow::on_buttonStop_clicked()
{
    activeAudioBackend->stop();
    ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
}

void MainWindow::on_buttonPause_clicked()
{
    if (activeAudioBackend->state() == KhAbstractAudioBackend::PausedState)
        activeAudioBackend->play();
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
//    if (rotationmodel->getSelectedSingerPosition() != -1)
//    {
//        int songid;
//        songid = songdbmodel->getRowSong(index.row())->ID;
//        rotationmodel->layoutAboutToBeChanged();
//        queuemodel->layoutAboutToBeChanged();
//        rotationmodel->getSelected()->addSongAtEnd(songid);
//        queuemodel->layoutChanged();
//        rotationmodel->layoutChanged();
//    }
}

void MainWindow::on_buttonAddSinger_clicked()
{
//    if (rotationmodel->exists(ui->editAddSinger->text()))
//    {
//        qDebug() << "Singer exists";
//        return;
//    }
//    if (!rotationmodel->add(ui->editAddSinger->text()))
//    {
//        qDebug() << "Failed to add singer!!!";
//        return;
//    }
    rotModel->singerAdd(ui->editAddSinger->text());
    ui->editAddSinger->clear();
}

void MainWindow::on_editAddSinger_returnPressed()
{
//    if (rotationmodel->exists(ui->editAddSinger->text()))
//    {
//        qDebug() << "Singer exists";
//        return;
//    }
//    if (!rotationmodel->add(ui->editAddSinger->text()))
//    {
//        qDebug() << "Failed to add singer!!!";
//        return;
//    }
    rotModel->singerAdd(ui->editAddSinger->text());
    ui->editAddSinger->clear();
}

void MainWindow::on_tableViewRotation_activated(const QModelIndex &index)
{
    if (index.column() < 3)
    {
        int singerId = index.sibling(index.row(),0).data().toInt();
        play(rotModel->nextSongPath(singerId));
        rotDelegate->setCurrentSinger(singerId);
        rotModel->setCurrentSinger(singerId);
        ui->labelArtist->setText(rotModel->nextSongArtist(singerId));
        ui->labelTitle->setText(rotModel->nextSongTitle(singerId));
        ui->labelSinger->setText(rotModel->getSingerName(singerId));
        qModel->markSongPlayed(rotModel->nextSongQueueId(singerId));
    }
}

void MainWindow::on_tableViewRotation_clicked(const QModelIndex &index)
{
    if (index.column() == 4)
    {
        // Deleting singer
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
    qModel->setSinger(index.sibling(index.row(),0).data().toInt());
//    if (index.column() == 5)
//    {
//        KhSinger *singer = rotationmodel->getSingerByPosition(index.row() + 1);
//        if (!singer->regular())
//        {
//            if (regularSingers->exists(singer->name()))
//            {
//                QMessageBox msgBox(this);
//                msgBox.setText("A regular singer with this name already exists!");
//                msgBox.setInformativeText("Would you like to merge their saved queue with the current singer's, replace the saved queue completely, or cancel?");
//                QPushButton *mergeButton = msgBox.addButton(tr("Merge"), QMessageBox::ActionRole);
//                QPushButton *replaceButton     = msgBox.addButton("Replace",QMessageBox::ActionRole);
//                QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);

//                msgBox.exec();

//                if (msgBox.clickedButton() == mergeButton)
//                {
//                    // merge the singer queue with the regular's queue
//                }
//                else if (msgBox.clickedButton() == replaceButton)
//                {
//                    // replace the exising regular's queue with the current singer's
//                }
//                else if (msgBox.clickedButton() == cancelButton)
//                {
//                    // abort, so nothing.
//                    return;
//                }
//            }
//            else
//            {
//                rotationmodel->layoutAboutToBeChanged();
//                qDebug() << "Set regular clicked, singer is not currently regular";
//                rotationmodel->createRegularForSinger(singer->index());
//                rotationmodel->layoutChanged();
//                //for (unsigned int i=0; i < queuemodel->rowCount(); i++)
//                // {
//                // Need more functionality in queuemodel or need to split out queue data into KhSingers
//                // }
//                return;
//            }
//        }
//        else
//        {
//            QMessageBox msgBox(this);
//            msgBox.setText("Are you sure you want to disable regular tracking for this singer?");
//            msgBox.setInformativeText("Doing so will not remove the regular singer entry, but it will prevent any changes made to the singer's queue from being saved to the regular singer until the regular singer is either reloaded or the rotation singer is re-merged with the regular singer.");
//            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
//            QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
//            msgBox.exec();
//            if (msgBox.clickedButton() == yesButton)
//            {
//                rotationmodel->layoutAboutToBeChanged();
//                singer->setRegular(false);
//                singer->setRegularIndex(-1);
//                rotationmodel->layoutChanged();
//                return;
//            }
//            else if (msgBox.clickedButton() == cancelButton)
//                return;
//        }
//    }
//    else if (rowclicked != index.row())
//    {
//        ui->tableViewQueue->clearSelection();
//        int singerid = rotationmodel->getSingerByPosition(index.row() + 1)->index();
//        qModel->setSinger(singerid);
//        queuemodel->layoutAboutToBeChanged();
//        rotationmodel->setSelectedSingerIndex(singerid);
//        rowclicked = index.row();
//        queuemodel->layoutChanged();
//    }
}

void MainWindow::on_tableViewQueue_activated(const QModelIndex &index)
{
    play(index.sibling(index.row(), 6).data().toString());
    rotDelegate->setCurrentSinger(index.sibling(index.row(),1).data().toInt());
    rotModel->setCurrentSinger(index.sibling(index.row(),1).data().toInt());
    qModel->markSongPlayed(index.sibling(index.row(),0).data().toInt());
    ui->labelSinger->setText(rotModel->getSingerName(index.sibling(index.row(),1).data().toInt()));
    ui->labelArtist->setText(index.sibling(index.row(),3).data().toString());
    ui->labelTitle->setText(index.sibling(index.row(),4).data().toString());
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

void MainWindow::on_treeViewQueue_clicked(const QModelIndex &index)
{
    if (index.column() == 4)
    {
        queuemodel->layoutAboutToBeChanged();
        rotationmodel->getSelected()->queueObject()->deleteSongByPosition(index.row());
        queuemodel->layoutChanged();
        ui->tableViewQueue->clearSelection();
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
        ui->tableViewQueue->clearSelection();
        ui->tableViewRotation->clearSelection();
        //queuemodel->clear();
        queuemodel->layoutAboutToBeChanged();
        rotationmodel->clear();
        queuemodel->layoutChanged();
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
        if (rotationmodel->getSelected() != NULL)
        {
            rotationmodel->layoutAboutToBeChanged();
            ui->tableViewQueue->clearSelection();
            queuemodel->clear();
            rotationmodel->layoutChanged();
        }
    }
}

void MainWindow::on_spinBoxKey_valueChanged(int arg1)
{
    if ((activeAudioBackend->state() == KhAbstractAudioBackend::PlayingState) || (activeAudioBackend->state() == KhAbstractAudioBackend::PausedState))
        activeAudioBackend->setPitchShift(arg1);
    else
        ui->spinBoxKey->setValue(0);
}

void MainWindow::on_sliderVolume_valueChanged(int value)
{
    activeAudioBackend->setVolume(value);
}

void MainWindow::audioBackend_positionChanged(qint64 position)
{
    if (activeAudioBackend->state() == KhAbstractAudioBackend::PlayingState)
    {
        if (cdg->GetLastCDGUpdate() >= position)
        {
            if (!cdg->SkipFrame(position))
            {
                unsigned char* rgbdata;
                rgbdata = cdg->GetImageByTime(position);
                QImage img(rgbdata, 300, 216, QImage::Format_RGB888);
                ui->cdgOutput->setPixmap(QPixmap::fromImage(img));
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

void MainWindow::audioBackend_stateChanged(KhAbstractAudioBackend::State state)
{
    if (state == KhAbstractAudioBackend::StoppedState)
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
        //ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        QImage cdgBg;
        if (settings->cdgDisplayBackgroundImage() != "")
        {
            qDebug() << "Attempting to load CDG background: " << settings->cdgDisplayBackgroundImage();
            if (!cdgBg.load(settings->cdgDisplayBackgroundImage()))
            {
                qDebug() << "Failed to load, loading default resource";
                cdgBg.load(":/icons/Icons/openkjlogo1.png");
            }
            else
                qDebug() << "Loaded OK";
        }
        else
        {
            cdgBg.load(":/icons/Icons/openkjlogo1.png");
            qDebug() << "No CDG background image specified, loading default resource";
        }
        cdgWindow->updateCDG(cdgBg, true);
        ui->cdgOutput->setPixmap(QPixmap::fromImage(QImage(":/icons/Icons/openkjlogo1.png")));

    }
    if (state == KhAbstractAudioBackend::EndOfMediaState)
    {
        audioRecorder->stop();
        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        activeAudioBackend->stop(true);
    }
    if (state == KhAbstractAudioBackend::PausedState)
    {
            audioRecorder->pause();
    }

}

void MainWindow::on_sliderProgress_sliderMoved(int position)
{
    Q_UNUSED(position);
   // audioBackend->setPosition(position);
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
    QString tickerText = "Singers in rotation: ";
    tickerText += QString::number(rotModel->rowCount());
    tickerText += " | Current singer: ";
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
        tickerText += " | Upcoming Singers: ";
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
        qDebug() << "Silence detected for > 2s after last CDG draw command... Stopping.";
        activeAudioBackend->stop(true);
        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
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
        QString zipPath = index.sibling(index.row(), 5).data().toString();
        cdgPreviewDialog->setZipFile(zipPath);
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", cdgPreviewDialog, SLOT(preview()));
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_tableViewQueue_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewQueue->indexAt(pos);
    if (index.isValid())
    {   
//        QString zipPath = rotationmodel->getSelected()->getSongByPosition(index.row())->getSourceFile();
        QString zipPath = index.sibling(index.row(), 6).data().toString();
        dlgKeyChange->setActiveSong(rotationmodel->getSelected()->getSongByPosition(index.row()));
        rtClickQueueSong = rotationmodel->getSelected()->getSongByPosition(index.row());
        cdgPreviewDialog->setZipFile(zipPath);
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", cdgPreviewDialog, SLOT(preview()));
        contextMenu.addAction("Set Key Change", this, SLOT(setKeyChange()));
        contextMenu.addAction("Toggle played", this, SLOT(toggleQueuePlayed()));
        contextMenu.exec(QCursor::pos());
        //contextMenu->exec(ui->treeView->mapToGlobal(point));
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
    rotationmodel->layoutAboutToBeChanged();
    queuemodel->layoutAboutToBeChanged();
    rtClickQueueSong->setPlayed(!rtClickQueueSong->getPlayed());
    rotationmodel->layoutChanged();
    queuemodel->layoutChanged();
}

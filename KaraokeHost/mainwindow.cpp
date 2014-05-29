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
#ifdef USE_FMOD
#include "khaudiobackendfmod.h"
#endif
#ifdef USE_QMEDIAPLAYER
#include "khaudiobackendqmediaplayer.h"
#endif
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
    QSqlQuery query("CREATE TABLE IF NOT EXISTS dbSongs ( discid VARCHAR(25), artist VARCHAR(100), title VARCHAR(100), path VARCHAR(700) NOT NULL UNIQUE, filename VARCHAR(200), 'length' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS rotationSingers ( name VARCHAR(30) NOT NULL UNIQUE, 'position' INTEGER NOT NULL, 'regular' LOGICAL DEFAULT(0), 'regularid' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS queueSongs ( singer INTEGER NOT NULL, song INTEGER NOT NULL, keychg INTEGER, played INTEGER NOT NULL, 'position' INTEGER, 'regsong' LOGICAL DEFAULT(0), 'regsongid' INTEGER DEFAULT(-1), 'regsingerid' INTEGER DEFAULT(-1))");
    query.exec("CREATE TABLE IF NOT EXISTS regularSingers ( name VARCHAR(30) NOT NULL UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS regularSongs ( singer INTEGER NOT NULL, song INTEGER NOT NULL, 'keychg' INTEGER, 'position' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS sourceDirs ( path VARCHAR(255) UNIQUE, pattern INTEGER)");
    query.exec("PRAGMA synchronous = OFF");
//    query.exec("PRAGMA journal_mode = OFF");
    sortColDB = 1;
    sortDirDB = 0;
    songdbmodel = new SongDBTableModel(this);
    songdbmodel->loadFromDB();
    regularSingers = new KhRegularSingers(songdbmodel->getDbSongs(),this);
    //singers = new KhSingers(regularSingers, this);
    songCurrent = NULL;
    rotationmodel = new RotationTableModel(regularSingers, this);
    ui->treeViewRotation->setModel(rotationmodel);
    ui->treeViewRotation->header()->resizeSection(0,18);
    ui->treeViewRotation->header()->resizeSection(3,18);
    ui->treeViewRotation->header()->resizeSection(4,18);
    ui->treeViewRotation->header()->resizeSection(5,18);
    queuemodel = new QueueTableModel(rotationmodel, this);
    ui->treeViewQueue->sortByColumn(-1);
    ui->treeViewQueue->setModel(queuemodel);
    ui->treeViewQueue->header()->resizeSection(4,18);

    khTmpDir = new QTemporaryDir();
    dbDialog = new DatabaseDialog(this);
    dlgKeyChange = new DlgKeyChange(rotationmodel, this);
    regularSingersDialog = new RegularSingersDialog(regularSingers, rotationmodel, this);
    regularExportDialog = new RegularExportDialog(regularSingers, this);
    regularImportDialog = new RegularImportDialog(songdbmodel->getDbSongs(), regularSingers, this);
    requestsDialog = new KhRequestsDialog(songdbmodel->getDbSongs(), rotationmodel, this);
    cdgPreviewDialog = new CdgPreviewDialog(this);
    cdgWindow = new CdgWindow(this, Qt::Window);
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
    ui->treeViewDB->sortByColumn(-1);
    ui->treeViewDB->setModel(songdbmodel);
    ipcClient = new KhIPCClient("bmControl",this);
    audioBackends = new KhAudioBackends;
#ifdef USE_FMOD
    qDebug() << "Initializing audio backend: Fmod";
    audioBackends->push_back(new KhAudioBackendFMOD(settings->audioDownmix(), this));
#endif
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
        ui->treeViewQueue->hideColumn(3);
    }
    settingsDialog = new SettingsDialog(audioBackends, this);
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
    connect(rotationmodel, SIGNAL(layoutChanged()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(tickerOutputModeChanged()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(audioBackendChanged(int)), this, SLOT(audioBackendChanged(int)));
    connect(activeAudioBackend, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()));
    connect(settingsDialog, SIGNAL(audioUseFaderChanged(bool)), activeAudioBackend, SLOT(setUseFader(bool)));
    activeAudioBackend->setUseFader(settings->audioUseFader());
    connect(settingsDialog, SIGNAL(audioSilenceDetectChanged(bool)), activeAudioBackend, SLOT(setUseSilenceDetection(bool)));
    activeAudioBackend->setUseSilenceDetection(settings->audioDetectSilence());
    connect(settingsDialog, SIGNAL(audioDownmixChanged(bool)), activeAudioBackend, SLOT(setDownmix(bool)));
    activeAudioBackend->setDownmix(settings->audioDownmix());
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
    settings->restoreColumnWidths(ui->treeViewDB);
    settings->restoreColumnWidths(ui->treeViewQueue);
    settings->restoreColumnWidths(ui->treeViewRotation);
    if ((settings->cdgWindowFullscreen()) && (settings->showCdgWindow()))
    {
        cdgWindow->makeFullscreen();
    }
    rotationDataChanged();
    ui->statusBar->addWidget(labelSingerCount);
}

void MainWindow::play(QString zipFilePath)
{

    if ((activeAudioBackend->state() != KhAbstractAudioBackend::PlayingState) && (activeAudioBackend->state() != KhAbstractAudioBackend::PausedState))
    {
        KhZip zip(zipFilePath);
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
            activeAudioBackend->play();
            ui->labelArtist->setText(songCurrent->Artist);
            ui->labelTitle->setText(songCurrent->Title);
        }
        else
        {
            QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG or mp3 file missing."),QMessageBox::Ok);
        }
    }
    else if (activeAudioBackend->state() == KhAbstractAudioBackend::PausedState)
    {
        activeAudioBackend->play();
    }
}

MainWindow::~MainWindow()
{
    settings->saveSplitterState(ui->splitter);
    settings->saveSplitterState(ui->splitter_2);
    settings->saveColumnWidths(ui->treeViewDB);
    settings->saveColumnWidths(ui->treeViewRotation);
    settings->saveColumnWidths(ui->treeViewQueue);
    settings->saveWindowState(cdgWindow);
    settings->saveWindowState(requestsDialog);
    settings->saveWindowState(regularSingersDialog);
    settings->saveWindowState(this);
    settings->setShowCdgWindow(cdgWindow->isVisible());
    settings->setAudioVolume(ui->sliderVolume->value());
    delete cdg;
    delete khDir;
    delete khTmpDir;
    delete database;
    delete regularSingers;
    //delete songCurrent;
    delete ui;
}

void MainWindow::search()
{
    QString termsstr;
    termsstr = ui->lineEdit->text();
    songdbmodel->applyFilter(termsstr);
}

void MainWindow::songdbUpdated()
{
    songdbmodel->loadFromDB();
}

void MainWindow::databaseCleared()
{
    songdbmodel->loadFromDB();
    rotationmodel->clear();
    regularSingers->clear();
    ui->treeViewDB->clearSelection();
    ui->treeViewRotation->clearSelection();

}

void MainWindow::on_buttonStop_clicked()
{
    activeAudioBackend->stop();
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

void MainWindow::on_treeViewDB_activated(const QModelIndex &index)
{
    if (rotationmodel->getSelectedSingerPosition() != -1)
    {
        int songid;
        songid = songdbmodel->getRowSong(index.row())->ID;
        rotationmodel->layoutAboutToBeChanged();
        queuemodel->layoutAboutToBeChanged();
        rotationmodel->getSelected()->addSongAtEnd(songid);
        queuemodel->layoutChanged();
        rotationmodel->layoutChanged();
    }
}

void MainWindow::on_buttonAddSinger_clicked()
{
    if (rotationmodel->exists(ui->editAddSinger->text()))
    {
        qDebug() << "Singer exists";
        return;
    }
    if (!rotationmodel->add(ui->editAddSinger->text()))
    {
        qDebug() << "Failed to add singer!!!";
        return;
    }
    ui->editAddSinger->clear();
}

void MainWindow::on_editAddSinger_returnPressed()
{
    if (rotationmodel->exists(ui->editAddSinger->text()))
    {
        qDebug() << "Singer exists";
        return;
    }
    if (!rotationmodel->add(ui->editAddSinger->text()))
    {
        qDebug() << "Failed to add singer!!!";
        return;
    }
    ui->editAddSinger->clear();
}

void MainWindow::on_treeViewRotation_activated(const QModelIndex &index)
{
    if (index.column() < 3)
    {
        rotationmodel->setCurrentSingerPosition(index.row() + 1);
        activeAudioBackend->stop();
        KhQueueSong *qsong = rotationmodel->getSelected()->getNextSong();
        KhSong *song = songdbmodel->getSongByID(qsong->getSongID());
        songCurrent = song;
        delete khTmpDir;
        khTmpDir = new QTemporaryDir();
        play(song->path);
        queuemodel->layoutAboutToBeChanged();
        qsong->setPlayed(true);
        queuemodel->layoutChanged();
    }
}

void MainWindow::on_treeViewRotation_clicked(const QModelIndex &index)
{
    ui->treeViewQueue->sortByColumn(-1);
    static int rowclicked = -1;
    if (index.column() == 3)
    {
        QModelIndex child = rotationmodel->index(index.row(), 1, index.parent());
        ui->treeViewRotation->selectionModel()->setCurrentIndex(child,QItemSelectionModel::SelectCurrent);
        ui->treeViewRotation->edit(child);
    }
    else if (index.column() == 4)
    {
        queuemodel->layoutAboutToBeChanged();
        rotationmodel->deleteSingerByPosition(index.row() + 1);
        ui->treeViewRotation->clearSelection();
        rotationmodel->setSelectedSingerIndex(-1);
        rowclicked = -1;
        queuemodel->layoutChanged();
    }
    else if (index.column() == 5)
    {
        KhSinger *singer = rotationmodel->getSingerByPosition(index.row() + 1);
        if (!singer->regular())
        {
            if (regularSingers->exists(singer->name()))
            {
                QMessageBox msgBox(this);
                msgBox.setText("A regular singer with this name already exists!");
                msgBox.setInformativeText("Would you like to merge their saved queue with the current singer's, replace the saved queue completely, or cancel?");
                QPushButton *mergeButton = msgBox.addButton(tr("Merge"), QMessageBox::ActionRole);
                QPushButton *replaceButton     = msgBox.addButton("Replace",QMessageBox::ActionRole);
                QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);

                msgBox.exec();

                if (msgBox.clickedButton() == mergeButton)
                {
                    // merge the singer queue with the regular's queue
                }
                else if (msgBox.clickedButton() == replaceButton)
                {
                    // replace the exising regular's queue with the current singer's
                }
                else if (msgBox.clickedButton() == cancelButton)
                {
                    // abort, so nothing.
                    return;
                }
            }
            else
            {
                rotationmodel->layoutAboutToBeChanged();
                qDebug() << "Set regular clicked, singer is not currently regular";
                rotationmodel->createRegularForSinger(singer->index());
                rotationmodel->layoutChanged();
                //for (unsigned int i=0; i < queuemodel->rowCount(); i++)
                // {
                // Need more functionality in queuemodel or need to split out queue data into KhSingers
                // }
                return;
            }
        }
        else
        {
            QMessageBox msgBox(this);
            msgBox.setText("Are you sure you want to disable regular tracking for this singer?");
            msgBox.setInformativeText("Doing so will not remove the regular singer entry, but it will prevent any changes made to the singer's queue from being saved to the regular singer until the regular singer is either reloaded or the rotation singer is re-merged with the regular singer.");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton)
            {
                rotationmodel->layoutAboutToBeChanged();
                singer->setRegular(false);
                singer->setRegularIndex(-1);
                rotationmodel->layoutChanged();
                return;
            }
            else if (msgBox.clickedButton() == cancelButton)
                return;
        }
    }
    else if (rowclicked != index.row())
    {
        ui->treeViewQueue->clearSelection();
        int singerid = rotationmodel->getSingerByPosition(index.row() + 1)->index();
        queuemodel->layoutAboutToBeChanged();
        rotationmodel->setSelectedSingerIndex(singerid);
        rowclicked = index.row();
        queuemodel->layoutChanged();
    }
}

void MainWindow::on_treeViewQueue_activated(const QModelIndex &index)
{
    activeAudioBackend->stop();
    KhQueueSong *queuesong = rotationmodel->getSelected()->getSongByPosition(index.row());
    KhSong *song = new KhSong();
    song->Artist = queuesong->getArtist();
    song->Title = queuesong->getTitle();
    song->path = queuesong->getSourceFile();
    song->DiscID = queuesong->getDiscID();
    if (songCurrent != NULL)
        delete songCurrent;
    songCurrent = song;
    delete khTmpDir;
    khTmpDir = new QTemporaryDir();
    play(song->path);
    if (activeAudioBackend->canPitchShift()) activeAudioBackend->setPitchShift(queuesong->getKeyChange());
    queuemodel->layoutAboutToBeChanged();
    queuesong->setPlayed(true);
    queuemodel->layoutChanged();
    rotationmodel->layoutAboutToBeChanged();
    rotationmodel->setCurrentSingerPosition(rotationmodel->getSelected()->position());
    rotationmodel->layoutChanged();
    ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_OUT);
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

void MainWindow::songDroppedOnSinger(int singer, int song, int row)
{
    ui->treeViewRotation->clearSelection();
    QItemSelectionModel *selmodel = ui->treeViewRotation->selectionModel();
    QModelIndex topLeft;
    QModelIndex bottomRight;
    topLeft = rotationmodel->index(row, 0, QModelIndex());
    bottomRight = rotationmodel->index(row, 2, QModelIndex());
    QItemSelection selection(topLeft, bottomRight);
    selmodel->select(selection, QItemSelectionModel::Select);
    queuemodel->layoutAboutToBeChanged();
    rotationmodel->setSelectedSingerIndex(singer);
    if (rotationmodel->getSingerByIndex(singer) != NULL)
        rotationmodel->getSingerByIndex(singer)->addSongAtEnd(song);
    queuemodel->layoutChanged();
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
        ui->treeViewQueue->clearSelection();
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
        ui->treeViewQueue->clearSelection();
        ui->treeViewRotation->clearSelection();
        //queuemodel->clear();
        queuemodel->layoutAboutToBeChanged();
        rotationmodel->clear();
        queuemodel->layoutChanged();
    }
}

void MainWindow::clearQueueSort()
{
    ui->treeViewQueue->sortByColumn(-1);
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
            ui->treeViewQueue->clearSelection();
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
        cdg->VideoClose();
        ui->labelArtist->setText("None");
        ui->labelTitle->setText("None");
        ui->labelElapsedTime->setText("0:00");
        ui->labelRemainTime->setText("0:00");
        ui->labelTotalTime->setText("0:00");
        ui->sliderProgress->setValue(0);
        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
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
    statusBarText += QString::number(rotationmodel->size());
    labelSingerCount->setText(statusBarText);
    QString tickerText = "Singers in rotation: ";
    tickerText += QString::number(rotationmodel->size());
    tickerText += " | Current singer: ";
    int displayPos;
    if (rotationmodel->getCurrent() != NULL)
    {
        tickerText += rotationmodel->getCurrent()->name();
        displayPos = rotationmodel->getCurrent()->position();
    }
    else
    {
        tickerText += "None";
        displayPos = 0;
    }
//    tickerText += " | Upcoming Singers: ";
    int listSize;
    if (settings->tickerFullRotation() || (rotationmodel->size() < settings->tickerShowNumSingers()))
{
        listSize = rotationmodel->size();
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
        if (displayPos + 1 <= rotationmodel->size())
            displayPos++;
        else
            displayPos = 1;
        tickerText += QString::number(i + 1);
        tickerText += ") ";
        tickerText += rotationmodel->getSingerByPosition(displayPos)->name();
        tickerText += "  ";
    }
    if (rotationmodel->size() == 0)
        tickerText += " None";
    cdgWindow->setTickerText(tickerText);
}

void MainWindow::silenceDetected()
{
    if (cdg->GetLastCDGUpdate() < activeAudioBackend->position())
    {
        qDebug() << "Silence detected for > 2s after last CDG draw command... Stopping.";
        activeAudioBackend->stop(true);
    }
}

void MainWindow::audioBackendChanged(int index)
{
    activeAudioBackend = audioBackends->at(index);
}

void MainWindow::on_treeViewDB_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->treeViewDB->indexAt(pos);
    if (index.isValid())
    {
        QString zipPath = songdbmodel->getRowSong(index.row())->path;
        cdgPreviewDialog->setZipFile(zipPath);
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", cdgPreviewDialog, SLOT(preview()));
        contextMenu.exec(QCursor::pos());
        //contextMenu->exec(ui->treeView->mapToGlobal(point));
    }
}

void MainWindow::on_treeViewQueue_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->treeViewQueue->indexAt(pos);
    if (index.isValid())
    {   
        QString zipPath = rotationmodel->getSelected()->getSongByPosition(index.row())->getSourceFile();
        dlgKeyChange->setActiveSong(rotationmodel->getSelected()->getSongByPosition(index.row()));
        cdgPreviewDialog->setZipFile(zipPath);
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", cdgPreviewDialog, SLOT(preview()));
        contextMenu.addAction("Set Key Change", this, SLOT(setKeyChange()));
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

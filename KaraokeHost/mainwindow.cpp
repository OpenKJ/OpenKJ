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
#include <khaudiobackendfmod.h>
#else
#include <khaudiobackendqmediaplayer.h>
#endif
#include <khzip.h>
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QMenu>


KhSettings *settings;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("KaraokeHost");
    ui->setupUi(this);
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
//    query.exec("PRAGMA synchronous = OFF");
    query.exec("PRAGMA journal_mode = OFF");
    sortColDB = 1;
    sortDirDB = 0;
    songdbmodel = new SongDBTableModel(this);
    songdbmodel->loadFromDB();
    regularSingers = new KhRegularSingers(songdbmodel->getDbSongs(),this);
    singers = new KhRotationSingers(regularSingers, this);
    songCurrent = NULL;
    rotationmodel = new RotationTableModel(singers, this);
    ui->treeViewRotation->setModel(rotationmodel);
    ui->treeViewRotation->header()->resizeSection(0,18);
    ui->treeViewRotation->header()->resizeSection(3,18);
    ui->treeViewRotation->header()->resizeSection(4,18);
    ui->treeViewRotation->header()->resizeSection(5,18);
    queuemodel = new QueueTableModel(singers, this);
    ui->treeViewQueue->sortByColumn(-1);
    ui->treeViewQueue->setModel(queuemodel);
    ui->treeViewQueue->header()->resizeSection(4,18);

    khTmpDir = new QTemporaryDir();
    dbDialog = new DatabaseDialog(this);
    settingsDialog = new SettingsDialog(this);
    regularSingersDialog = new RegularSingersDialog(regularSingers, singers, this);
    regularExportDialog = new RegularExportDialog(regularSingers, this);
    regularImportDialog = new RegularImportDialog(songdbmodel->getDbSongs(), regularSingers, this);
    requestsDialog = new KhRequestsDialog(songdbmodel->getDbSongs(), singers, this);
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
#ifdef USE_FMOD
    audioBackend = new KhAudioBackendFMOD(this);
    qDebug() << "Audio backend: FMOD";
#else
    audioBackend = new KhAudioBackendQMediaPlayer(this);
    qDebug() << "Audio backend: QMediaPlayer";
#endif
    if (audioBackend->canFade())
        audioBackend->setUseFader(settings->audioUseFader());
    if (!audioBackend->canPitchShift())
    {
        ui->groupBoxKey->hide();
        ui->treeViewQueue->hideColumn(3);
    }

    connect(audioBackend, SIGNAL(volumeChanged(int)), ui->sliderVolume, SLOT(setValue(int)));
    connect(dbDialog, SIGNAL(databaseUpdated()), this, SLOT(songdbUpdated()));
    connect(dbDialog, SIGNAL(databaseCleared()), this, SLOT(databaseCleared()));
    connect(rotationmodel, SIGNAL(songDroppedOnSinger(int,int,int)), this, SLOT(songDroppedOnSinger(int,int,int)));
    connect(rotationmodel, SIGNAL(notify_user(QString)), this, SLOT(notify_user(QString)));
    connect(queuemodel, SIGNAL(itemMoved()), this, SLOT(clearQueueSort()));
    connect(audioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(audioBackend_positionChanged(qint64)));
    connect(audioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(audioBackend_durationChanged(qint64)));
    connect(audioBackend, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(audioBackend_stateChanged(QMediaPlayer::State)));
    qDebug() << "Setting volume to " << settings->audioVolume();
    audioBackend->setVolume(settings->audioVolume());
    ui->sliderVolume->setValue(settings->audioVolume());
    connect(settingsDialog, SIGNAL(showCdgWindowChanged(bool)), cdgWindow, SLOT(setVisible(bool)));
    connect(settingsDialog, SIGNAL(cdgWindowFullScreenChanged(bool)), cdgWindow, SLOT(setFullScreen(bool)));
    connect(regularSingers, SIGNAL(dataChanged()), rotationmodel, SIGNAL(layoutChanged()));
    connect(regularSingers, SIGNAL(dataAboutToChange()), rotationmodel, SIGNAL(layoutAboutToBeChanged()));
    connect(singers, SIGNAL(dataChanged()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(tickerOutputModeChanged()), this, SLOT(rotationDataChanged()));
    cdgWindow->updateCDG(QImage(":/icons/Icons/openkjlogo1.png"));
    ui->cdgOutput->setPixmap(QPixmap::fromImage(QImage(":/icons/Icons/openkjlogo1.png")));
    settings->restoreWindowState(cdgWindow);
    if ((settings->cdgWindowFullscreen()) && (settings->showCdgWindow()))
    {
        cdgWindow->makeFullscreen();
    }
    rotationDataChanged();

//    QStringList singers = regularSingers->importSingersList("/tmp/regtest.xml");
//    for (int i=0; i < singers.size(); i++)
//        regularSingers->importSinger(singers.at(i), "/tmp/regtest.xml");
//    close();
//    cdgPreviewDialog->show();
//    cdgPreviewDialog->preview("/storage/karaoke/Sound Choice/SC1000 Series/SC1736-04 - 3 Doors Down - Kryptonite.zip");
}

void MainWindow::play(QString zipFilePath)
{

    if ((audioBackend->state() != QMediaPlayer::PlayingState) && (audioBackend->state() != QMediaPlayer::PausedState))
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
            audioBackend->setMedia(khTmpDir->path() + QDir::separator() + "tmp.mp3");
            audioBackend->play();
            ui->labelArtist->setText(songCurrent->Artist);
            ui->labelTitle->setText(songCurrent->Title);
        }
        else
        {
            QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG or mp3 file missing."),QMessageBox::Ok);
        }
    }
    else if (audioBackend->state() == QMediaPlayer::PausedState)
    {
        audioBackend->play();
    }
}

MainWindow::~MainWindow()
{
    settings->saveWindowState(cdgWindow);
    settings->saveWindowState(this);
    settings->setShowCdgWindow(cdgWindow->isVisible());
    settings->setAudioVolume(audioBackend->volume());
    delete cdg;
    delete khDir;
    delete khTmpDir;
    delete database;
    delete regularSingers;
    delete songCurrent;
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
    singers->clear();
    ui->treeViewDB->clearSelection();
    ui->treeViewRotation->clearSelection();

}

void MainWindow::on_buttonStop_clicked()
{
    audioBackend->stop();
}

void MainWindow::on_buttonPlay_clicked()
{
}

void MainWindow::on_buttonPause_clicked()
{
    if (audioBackend->state() == QMediaPlayer::PausedState)
        audioBackend->play();
    else
        audioBackend->pause();
}

void MainWindow::on_lineEdit_returnPressed()
{
    search();
}

void MainWindow::on_treeViewDB_activated(const QModelIndex &index)
{
    if (singers->getSelectedSingerPosition() != -1)
    {
        int songid;
        songid = songdbmodel->getRowSong(index.row())->ID;
        rotationmodel->layoutAboutToBeChanged();
        queuemodel->layoutAboutToBeChanged();
        singers->getSelected()->addSongAtEnd(songid);
        queuemodel->layoutChanged();
        rotationmodel->layoutChanged();
    }
}

void MainWindow::on_buttonAddSinger_clicked()
{
    if (singers->exists(ui->editAddSinger->text()))
    {
        qDebug() << "Singer exists";
        return;
    }
    if (!singers->add(ui->editAddSinger->text()))
    {
        qDebug() << "Failed to add singer!!!";
        return;
    }
    ui->editAddSinger->clear();
}

void MainWindow::on_editAddSinger_returnPressed()
{
    if (singers->exists(ui->editAddSinger->text()))
    {
        qDebug() << "Singer exists";
        return;
    }
    if (!singers->add(ui->editAddSinger->text()))
    {
        qDebug() << "Failed to add singer!!!";
        return;
    }
    ui->editAddSinger->clear();
}

void MainWindow::on_treeViewRotation_activated(const QModelIndex &index)
{

    singers->setCurrentSingerPosition(index.row() + 1);
    audioBackend->stop();
    KhQueueSong *qsong = singers->getSelected()->getNextSong();
    KhSong *song = songdbmodel->getSongByID(qsong->getSongID());
    songCurrent = song;
    delete khTmpDir;
    khTmpDir = new QTemporaryDir();
    play(song->path);
    queuemodel->layoutAboutToBeChanged();
    qsong->setPlayed(true);
    queuemodel->layoutChanged();
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
        singers->deleteSingerByPosition(index.row() + 1);
        ui->treeViewRotation->clearSelection();
        singers->setSelectedSingerIndex(-1);
        rowclicked = -1;
        queuemodel->layoutChanged();
    }
    else if (index.column() == 5)
    {
        KhSinger *singer = singers->getSingerByPosition(index.row() + 1);
        if (!singer->isRegular())
        {
            if (regularSingers->exists(singer->getSingerName()))
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
                singers->createRegularForSinger(singer->getSingerIndex());
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
        int singerid = singers->getSingerByPosition(index.row() + 1)->getSingerIndex();
        queuemodel->layoutAboutToBeChanged();
        singers->setSelectedSingerIndex(singerid);
        rowclicked = index.row();
        queuemodel->layoutChanged();
    }
}

void MainWindow::on_treeViewQueue_activated(const QModelIndex &index)
{
    audioBackend->stop();
    KhQueueSong *queuesong = singers->getSelected()->getSongByPosition(index.row());
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
    queuemodel->layoutAboutToBeChanged();
    queuesong->setPlayed(true);
    queuemodel->layoutChanged();
    rotationmodel->layoutAboutToBeChanged();
    singers->setCurrentSingerPosition(singers->getSelected()->getSingerPosition());
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
    singers->setSelectedSingerIndex(singer);
    if (singers->getSingerByIndex(singer) != NULL)
        singers->getSingerByIndex(singer)->addSongAtEnd(song);
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
        singers->getSelected()->getQueueObject()->deleteSongByPosition(index.row());
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
    ui->treeViewQueue->clearSelection();
    ui->treeViewRotation->clearSelection();
    //queuemodel->clear();
    queuemodel->layoutAboutToBeChanged();
    singers->clear();
    queuemodel->layoutChanged();
}

void MainWindow::clearQueueSort()
{
    ui->treeViewQueue->sortByColumn(-1);
}


void MainWindow::on_buttonClearQueue_clicked()
{
    if (singers->getSelected() != NULL)
    {
        rotationmodel->layoutAboutToBeChanged();
        ui->treeViewQueue->clearSelection();
        queuemodel->clear();
        rotationmodel->layoutChanged();
    }
}

void MainWindow::on_spinBoxKey_valueChanged(int arg1)
{
    if ((audioBackend->state() == QMediaPlayer::PlayingState) || (audioBackend->state() == QMediaPlayer::PausedState))
        audioBackend->setPitchShift(arg1);
    else
        ui->spinBoxKey->setValue(0);
}

void MainWindow::on_sliderVolume_valueChanged(int value)
{
    audioBackend->setVolume(value);
}

void MainWindow::audioBackend_positionChanged(qint64 position)
{
    if (audioBackend->state() == QMediaPlayer::PlayingState)
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
        ui->sliderProgress->setMaximum(audioBackend->duration());
        ui->sliderProgress->setValue(position);
        ui->labelElapsedTime->setText(audioBackend->msToMMSS(position));
        ui->labelRemainTime->setText(audioBackend->msToMMSS(audioBackend->duration() - position));
    }
}

void MainWindow::audioBackend_durationChanged(qint64 duration)
{
    ui->labelTotalTime->setText(audioBackend->msToMMSS(duration));
}

void MainWindow::audioBackend_stateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::StoppedState)
    {
        cdg->VideoClose();
        ui->labelArtist->setText("None");
        ui->labelTitle->setText("None");
        ui->labelElapsedTime->setText("0:00");
        ui->labelRemainTime->setText("0:00");
        ui->labelTotalTime->setText("0:00");
        ui->sliderProgress->setValue(0);
        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        cdgWindow->updateCDG(QImage(":/icons/Icons/openkjlogo1.png"));
        ui->cdgOutput->setPixmap(QPixmap::fromImage(QImage(":/icons/Icons/openkjlogo1.png")));

    }
}

void MainWindow::on_sliderProgress_sliderMoved(int position)
{
    audioBackend->setPosition(position);
}

void MainWindow::on_buttonRegulars_clicked()
{
    regularSingersDialog->show();
}

void MainWindow::rotationDataChanged()
{
    QString tickerText = "Singers in rotation: ";
    tickerText += QString::number(singers->getSingers()->size());
    tickerText += " | Current singer: ";
    int displayPos;
    if (singers->getCurrent() != NULL)
    {
        tickerText += singers->getCurrent()->getSingerName();
        displayPos = singers->getCurrent()->getSingerPosition();
    }
    else
    {
        tickerText += "None";
        displayPos = 0;
    }
//    tickerText += " | Upcoming Singers: ";
    int listSize;
    if (settings->tickerFullRotation() || (singers->getSingers()->size() < settings->tickerShowNumSingers()))
{
        listSize = singers->getSingers()->size();
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
        if (displayPos + 1 <= singers->getSingers()->size())
            displayPos++;
        else
            displayPos = 1;
        tickerText += QString::number(i + 1);
        tickerText += ") ";
        tickerText += singers->getSingerByPosition(displayPos)->getSingerName();
        tickerText += "  ";
    }
    if (singers->getSingers()->size() == 0)
        tickerText += " None";
    cdgWindow->setTickerText(tickerText);
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

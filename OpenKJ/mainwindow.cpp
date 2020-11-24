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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <iostream>
#include <QTemporaryDir>
#include <QDir>
#include "mediabackend.h"
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QImageReader>
#include "mzarchive.h"
#include "tagreader.h"
#include <QSvgRenderer>
#include "audiorecorder.h"
#include "okjsongbookapi.h"
#include "updatechecker.h"
#include "okjversion.h"
#include "dlgeditsong.h"
#include <QKeySequence>
#include "soundfxbutton.h"
#include "tableviewtooltipfilter.h"
#include <tickernew.h>
#include "dbupdatethread.h"
#include <chrono>
#include "okjutil.h"
#include <algorithm>
#include <random>
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
#include <QRandomGenerator>
#endif

#ifdef Q_OS_WIN
#define NOMINMAX
#include <Windows.h>
#endif

Settings *settings;
OKJSongbookAPI *songbookApi;
int remainSecs = 240;

auto startTime = std::chrono::high_resolution_clock::now();

void MainWindow::addSfxButton(const QString &filename, const QString &label, const bool &reset)
{
    static int numButtons = 0;
    if (reset)
        numButtons = 0;
    qInfo() << "sfxButtonGrid contains " << numButtons << " children";
    int col = 0;
    if (numButtons % 2)
    {
        col = 1;
    }
    int row = numButtons / 2;
    qInfo() << "Adding button " << label << "at row: " << row << " col: " << col;
    SoundFxButton* button = new SoundFxButton();
    button->setButtonData(filename);
    button->setText(label);
    ui->sfxButtonGrid->addWidget(button,row,col);
    connect(button, &SoundFxButton::clicked, this, &MainWindow::sfxButtonPressed);
    connect(button, &SoundFxButton::customContextMenuRequested, this, &MainWindow::sfxButton_customContextMenuRequested);
    numButtons++;
}

void MainWindow::refreshSfxButtons()
{
    QLayoutItem* item;
    while ( ( item = ui->sfxButtonGrid->takeAt( 0 ) ) != NULL )
    {
        delete item->widget();
        delete item;
    }
    SfxEntryList list = settings->getSfxEntries();
    bool first = true;
    qInfo() << "SfxEntryList size: " << list.size();
    foreach (SfxEntry entry, list) {
        if (first)
        {
            first = false;
            addSfxButton(entry.path, entry.name, true);
            continue;
        }
        addSfxButton(entry.path, entry.name);
    }
}

void MainWindow::updateIcons()
{
    QString thm = (settings->theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    ui->buttonClearRotation->setIcon(QIcon(thm + "actions/22/edit-clear-all.svg"));
    ui->buttonAddSinger->setIcon(QIcon(thm + "actions/22/list-add-user.svg"));
    ui->buttonRegulars->setIcon(QIcon(thm + "actions/22/user-others.svg"));
    ui->btnRotTop->setIcon(QIcon(thm + "actions/22/go-top.svg"));
    ui->btnRotUp->setIcon(QIcon(thm + "actions/22/go-up.svg"));
    ui->btnRotBottom->setIcon(QIcon(thm + "actions/22/go-bottom.svg"));
    ui->btnRotDown->setIcon(QIcon(thm + "actions/22/go-down.svg"));
    ui->pushButton->setIcon(QIcon(thm + "actions/22/edit-find.svg"));
    ui->buttonClearQueue->setIcon(QIcon(thm + "actions/22/edit-clear-all.svg"));
    ui->btnQTop->setIcon(QIcon(thm + "actions/22/go-top.svg"));
    ui->btnQUp->setIcon(QIcon(thm + "actions/22/go-up.svg"));
    ui->btnQBottom->setIcon(QIcon(thm + "actions/22/go-bottom.svg"));
    ui->btnQDown->setIcon(QIcon(thm + "actions/22/go-down.svg"));
    ui->buttonPause->setIcon(QIcon(thm + "actions/22/media-playback-pause.svg"));
    ui->buttonStop->setIcon(QIcon(thm + "actions/22/media-playback-stop.svg"));
    ui->labelVolume->setPixmap(QPixmap(thm + "actions/16/player-volume.svg"));
    ui->pushButtonTempoDn->setIcon(QIcon(thm + "actions/22/downindicator.svg"));
    ui->pushButtonTempoUp->setIcon(QIcon(thm + "actions/22/upindicator.svg"));
    ui->pushButtonKeyDn->setIcon(QIcon(thm + "actions/22/downindicator.svg"));
    ui->pushButtonKeyUp->setIcon(QIcon(thm + "actions/22/upindicator.svg"));
    ui->btnSfxStop->setIcon(QIcon(thm + "actions/22/media-playback-stop.svg"));

    ui->buttonBmPause->setIcon(QIcon(thm + "actions/22/media-playback-pause.svg"));
    ui->buttonBmStop->setIcon(QIcon(thm + "actions/22/media-playback-stop.svg"));
    ui->btnPlTop->setIcon(QIcon(thm + "actions/22/go-top.svg"));
    ui->btnPlUp->setIcon(QIcon(thm + "actions/22/go-up.svg"));
    ui->btnPlBottom->setIcon(QIcon(thm + "actions/22/go-bottom.svg"));
    ui->btnPlDown->setIcon(QIcon(thm + "actions/22/go-down.svg"));
    ui->labelVolumeBm->setPixmap(QPixmap(thm + "actions/16/player-volume.svg"));
    ui->buttonBmSearch->setIcon(QIcon(thm + "actions/22/edit-find.svg"));

    requestsDialog->updateIcons();
}



bool MainWindow::isSingleInstance()
{
    if(_singular->attach(QSharedMemory::ReadOnly)){
        _singular->detach();
        return false;
    }
    if(_singular->create(1))
        return true;
    return false;
}

QFile *logFile;
QStringList *logContents;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    bool loggingEnabled = settings->logEnabled();
    auto currentTime = std::chrono::high_resolution_clock::now();
    unsigned int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    if (loggingEnabled && !logFile->isOpen())
    {
        QString logDir = settings->logDir();
        QDir dir;
        QString logFilePath;
        QString filename = "openkj-debug-" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm") + "-log";
        dir.mkpath(logDir);
        //  m_filename = filename;
#ifdef Q_OS_WIN
        logFilePath = logDir + QDir::separator() + filename;
#else
        logFilePath = logDir + QDir::separator() + filename;
#endif
        //    logFile = new QFile(logFilePath);
        logFile->setFileName(logFilePath);
        logFile->open(QFile::WriteOnly);
    }


    QTextStream logStream(logFile);
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "DEBG: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "DEBG: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString::number(elapsed) + QString(" - DEBG: " + localMsg + " (" + context.function + ")"));
        break;
    case QtInfoMsg:
        fprintf(stderr, "INFO: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "INFO: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString::number(elapsed) + QString(" - INFO: " + localMsg + " (" + context.function + ")"));
        break;
    case QtWarningMsg:
        fprintf(stderr, "WARN: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "WARN: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString::number(elapsed) + QString(" - WARN: " + localMsg + " (" + context.function + ")"));
        break;
    case QtCriticalMsg:
        fprintf(stderr, "CRIT: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "CRIT: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString::number(elapsed) + QString(" - CRIT: " + localMsg + " (" + context.function + ")"));
        break;
    case QtFatalMsg:
        fprintf(stderr, "FATAL!!: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "FATAL!!: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString::number(elapsed) + QString(" - FATAL!!: " + localMsg + " (" + context.function + ")"));
        abort();
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
#ifdef Q_OS_WIN
    timeBeginPeriod(1);
#endif
    settings = new Settings();
    logContents = new QStringList();
    debugDialog = new DlgDebugOutput(this);
    debugDialog->setVisible(settings->logShow());
    logFile = new QFile();
    qInstallMessageHandler(myMessageOutput);
#ifndef Q_OS_WIN
    // This fixes the program refusing to start and reporting another running instance after a crash on Linux
    // and Mac until you try it twice.
    _singular = new QSharedMemory("SharedMemorySingleInstanceProtectorOpenKJ", this);
    _singular->attach();
    delete _singular;
#endif
    _singular = new QSharedMemory("SharedMemorySingleInstanceProtectorOpenKJ", this);
    shop = new SongShop(this);
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");
    ui->setupUi(this);
    ui->actionShow_Debug_Log->setChecked(settings->logShow());
    //ui->videoPreview->setAspectRatio(16.0,9.0);
#ifdef Q_OS_WIN
    ui->sliderBmPosition->setMaximumHeight(12);
    ui->sliderBmVolume->setMaximumWidth(12);
    ui->sliderProgress->setMaximumHeight(12);
    //ui->sliderVolume->setMaximumWidth(12);
#endif
    QDir khDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!khDir.exists())
    {
        khDir.mkpath(khDir.absolutePath());
    }
    if (settings->theme() != 0)
    {
        ui->pushButtonIncomingRequests->setStyleSheet("");
        update();
    }
    songbookApi = new OKJSongbookAPI(this);
    int initialKVol = settings->audioVolume();
    int initialBMVol = settings->bmVolume();
    qInfo() << "Initial volumes - K: " << initialKVol << " BM: " << initialBMVol;
    settings->restoreWindowState(this);
    database = QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    database.setDatabaseName(khDir.absolutePath() + QDir::separator() + "openkj.sqlite");
    database.open();
    QSqlQuery query("CREATE TABLE IF NOT EXISTS dbSongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, DiscId COLLATE NOCASE, 'Duration' INTEGER, path VARCHAR(700) NOT NULL UNIQUE, filename COLLATE NOCASE, searchstring TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS rotationSingers ( singerid INTEGER PRIMARY KEY AUTOINCREMENT, name COLLATE NOCASE UNIQUE, 'position' INTEGER NOT NULL, 'regular' LOGICAL DEFAULT(0), 'regularid' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS queueSongs ( qsongid INTEGER PRIMARY KEY AUTOINCREMENT, singer INT, song INTEGER NOT NULL, artist INT, title INT, discid INT, path INT, keychg INT, played LOGICAL DEFAULT(0), 'position' INT)");
    query.exec("CREATE TABLE IF NOT EXISTS regularSingers ( regsingerid INTEGER PRIMARY KEY AUTOINCREMENT, Name COLLATE NOCASE UNIQUE, ph1 INT, ph2 INT, ph3 INT)");
    query.exec("CREATE TABLE IF NOT EXISTS regularSongs ( regsongid INTEGER PRIMARY KEY AUTOINCREMENT, regsingerid INTEGER NOT NULL, songid INTEGER NOT NULL, 'keychg' INTEGER, 'position' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS sourceDirs ( path VARCHAR(255) UNIQUE, pattern INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS bmsongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, path VARCHAR(700) NOT NULL UNIQUE, Filename COLLATE NOCASE, Duration TEXT, searchstring TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS bmplaylists ( playlistid INTEGER PRIMARY KEY AUTOINCREMENT, title COLLATE NOCASE NOT NULL UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS bmplsongs ( plsongid INTEGER PRIMARY KEY AUTOINCREMENT, playlist INT, position INT, Artist INT, Title INT, Filename INT, Duration INT, path INT)");
    query.exec("CREATE TABLE IF NOT EXISTS bmsrcdirs ( path NOT NULL)");
    query.exec("PRAGMA synchronous=OFF");
    query.exec("PRAGMA cache_size=300000");
    query.exec("PRAGMA temp_store=2");

    int schemaVersion = 0;
    query.exec("PRAGMA user_version");
    if (query.first())
         schemaVersion = query.value(0).toInt();
    qInfo() << "Database schema version: " << schemaVersion;

    if (schemaVersion < 100)
    {
        qInfo() << "Updating database schema to version 101";
        query.exec("ALTER TABLE sourceDirs ADD COLUMN custompattern INTEGER");
        query.exec("PRAGMA user_version = 100");
        qInfo() << "DB Schema update to v100 completed";
    }
    if (schemaVersion < 101)
    {
        qInfo() << "Updating database schema to version 101";
        query.exec("CREATE TABLE custompatterns ( patternid INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, artistregex TEXT, artistcapturegrp INT, titleregex TEXT, titlecapturegrp INT, discidregex TEXT, discidcapturegrp INT)");
        query.exec("PRAGMA user_version = 101");
        qInfo() << "DB Schema update to v101 completed";
    }
    if (schemaVersion < 102)
    {
        qInfo() << "Updating database schema to version 102";
        query.exec("CREATE UNIQUE INDEX idx_path ON dbsongs(path)");
        query.exec("PRAGMA user_version = 102");
        qInfo() << "DB Schema update to v102 completed";
    }
    if (schemaVersion < 103)
    {
        qInfo() << "Updating database schema to version 103";
        query.exec("ALTER TABLE dbsongs ADD COLUMN searchstring TEXT");
        query.exec("UPDATE dbsongs SET searchstring = filename || ' ' || artist || ' ' || title || ' ' || discid");
        query.exec("PRAGMA user_version = 103");
        qInfo() << "DB Schema update to v103 completed";

    }
    if (schemaVersion < 105)
    {
        qInfo() << "Updating database schema to version 105";
        query.exec("ALTER TABLE rotationSingers ADD COLUMN addts TIMESTAMP");
        query.exec("PRAGMA user_version = 105");
        qInfo() << "DB Schema update to v105 completed";
    }

//    query.exec("ATTACH DATABASE ':memory:' AS mem");
//    query.exec("CREATE TABLE mem.dbsongs AS SELECT * FROM main.dbsongs");
//    refreshSongDbCache();

    dbModel = new DbTableModel(this, database);
    dbModel->select();
    qModel = new QueueModel(this, database);
    qModel->select();
    qDelegate = new QueueItemDelegate(this);
    rotModel = new RotationModel(this, database);
    rotModel->select();
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    ui->tableViewRotation->setModel(rotModel);
    rotDelegate = new RotationItemDelegate(this);
    ui->tableViewRotation->setItemDelegate(rotDelegate);
    ui->tableViewQueue->setModel(qModel);
    ui->tableViewQueue->setItemDelegate(qDelegate);
    ui->tableViewQueue->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewQueue));
    khTmpDir = new QTemporaryDir();
    dbDialog = new DlgDatabase(database, this);
    dlgKeyChange = new DlgKeyChange(qModel, this);
    regularSingersDialog = new DlgRegularSingers(rotModel, this);
    regularExportDialog = new DlgRegularExport(rotModel, this);
    regularImportDialog = new DlgRegularImport(rotModel, this);
    requestsDialog = new DlgRequests(rotModel);
    requestsDialog->setModal(false);
    dlgBookCreator = new DlgBookCreator(this);
    dlgEq = new DlgEq(this);
    dlgAddSinger = new DlgAddSinger(rotModel, this);
    connect(dlgAddSinger, &DlgAddSinger::newSingerAdded, [&] (auto pos) {
       ui->tableViewRotation->selectRow(pos);
       ui->lineEdit->setFocus();
    });
    dlgSongShop = new DlgSongShop(shop);
    dlgSongShop->setModal(false);
    ui->tableViewDB->setModel(dbModel);
    ui->tableViewDB->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewDB));
    dbDelegate = new DbItemDelegate(this);
    ui->tableViewDB->setItemDelegate(dbDelegate);
//    ipcClient = new KhIPCClient("bmControl",this);
    if (kMediaBackend.canFade())
        kMediaBackend.setUseFader(settings->audioUseFader());
    if (!kMediaBackend.canPitchShift())
    {
        ui->spinBoxKey->hide();
        ui->lblKey->hide();
        ui->tableViewQueue->hideColumn(7);
    }
    if (!kMediaBackend.canChangeTempo())
    {
        ui->spinBoxTempo->hide();
        ui->lblTempo->hide();
    }
    ui->videoPreview->setMediaBackends(&kMediaBackend, &bmMediaBackend);
    settingsDialog = new DlgSettings(&kMediaBackend, &bmMediaBackend, this);
    cdgWindow = new DlgCdg(&kMediaBackend, &bmMediaBackend, 0, Qt::Window);
    connect(rotModel, &RotationModel::songDroppedOnSinger, this, &MainWindow::songDroppedOnSinger);
    connect(dbDialog, &DlgDatabase::databaseUpdateComplete, this, &MainWindow::databaseUpdated);
    connect(dbDialog, &DlgDatabase::databaseAboutToUpdate, this, &MainWindow::databaseAboutToUpdate);
    connect(dbDialog, &DlgDatabase::databaseSongAdded, dbModel, &DbTableModel::select);
    connect(dbDialog, &DlgDatabase::databaseSongAdded, requestsDialog, &DlgRequests::databaseSongAdded);
    connect(dbDialog, &DlgDatabase::databaseCleared, this, &MainWindow::databaseCleared);
    connect(dbDialog, &DlgDatabase::databaseCleared, regularSingersDialog, &DlgRegularSingers::regularsChanged);
    connect(&kMediaBackend, &MediaBackend::volumeChanged, ui->sliderVolume, &QSlider::setValue);
    connect(&kMediaBackend, &MediaBackend::positionChanged, this, &MainWindow::audioBackend_positionChanged);
    connect(&kMediaBackend, &MediaBackend::durationChanged, this, &MainWindow::audioBackend_durationChanged);
    connect(&kMediaBackend, &MediaBackend::stateChanged, this, &MainWindow::audioBackend_stateChanged);
    connect(&kMediaBackend, &MediaBackend::pitchChanged, ui->spinBoxKey, &QSpinBox::setValue);
    connect(&kMediaBackend, &MediaBackend::audioError, this, &MainWindow::audioError);
    connect(&kMediaBackend, &MediaBackend::silenceDetected, this, &MainWindow::silenceDetected);
    connect(&bmMediaBackend, &MediaBackend::audioError, this, &MainWindow::audioError);
    connect(&bmMediaBackend, &MediaBackend::silenceDetected, this, &MainWindow::silenceDetectedBm);
    connect(&sfxMediaBackend, &MediaBackend::positionChanged, this, &MainWindow::sfxAudioBackend_positionChanged);
    connect(&sfxMediaBackend, &MediaBackend::durationChanged, this, &MainWindow::sfxAudioBackend_durationChanged);
    connect(&sfxMediaBackend, &MediaBackend::stateChanged, this, &MainWindow::sfxAudioBackend_stateChanged);
    connect(rotModel, &RotationModel::rotationModified, this, &MainWindow::rotationDataChanged);
    connect(settings, &Settings::tickerOutputModeChanged, this, &MainWindow::rotationDataChanged);
    connect(settings, &Settings::audioBackendChanged, this, &MainWindow::audioBackendChanged);
    connect(settings, &Settings::cdgBgImageChanged, this, &MainWindow::onBgImageChange);
    connect(settingsDialog, &DlgSettings::audioUseFaderChanged, &kMediaBackend, &MediaBackend::setUseFader);
    connect(shop, &SongShop::karaokeSongDownloaded, dbDialog, &DlgDatabase::singleSongAdd);
    kMediaBackend.setUseFader(settings->audioUseFader());
    connect(settingsDialog, &DlgSettings::audioSilenceDetectChanged, &kMediaBackend, &MediaBackend::setUseSilenceDetection);
    kMediaBackend.setUseSilenceDetection(settings->audioDetectSilence());
    connect(ui->pushButtonTempoDn, &QPushButton::clicked, ui->spinBoxTempo, &QSpinBox::stepDown);
    connect(ui->pushButtonTempoUp, &QPushButton::clicked, ui->spinBoxTempo, &QSpinBox::stepUp);
    connect(ui->pushButtonKeyDn, &QPushButton::clicked, ui->spinBoxKey, &QSpinBox::stepDown);
    connect(ui->pushButtonKeyUp, &QPushButton::clicked, ui->spinBoxKey, &QSpinBox::stepUp);
    connect(settingsDialog, &DlgSettings::audioUseFaderChangedBm, &bmMediaBackend, &MediaBackend::setUseFader);
    bmMediaBackend.setUseFader(settings->audioUseFaderBm());
    connect(settingsDialog, &DlgSettings::audioSilenceDetectChangedBm, &bmMediaBackend, &MediaBackend::setUseSilenceDetection);
    bmMediaBackend.setUseSilenceDetection(settings->audioDetectSilenceBm());

    connect(settingsDialog, &DlgSettings::audioDownmixChanged, &kMediaBackend, &MediaBackend::setDownmix);
    connect(settingsDialog, &DlgSettings::audioDownmixChangedBm, &bmMediaBackend, &MediaBackend::setDownmix);
    kMediaBackend.setDownmix(settings->audioDownmix());
    bmMediaBackend.setDownmix(settings->audioDownmixBm());
    connect(qModel, &QueueModel::queueModified, rotModel, &RotationModel::queueModified);
    connect(requestsDialog, &DlgRequests::addRequestSong, qModel, &QueueModel::songAddSlot);
    connect(settings, &Settings::tickerCustomStringChanged, this, &MainWindow::rotationDataChanged);
    cdgWindow->setShowBgImage(true);
    kMediaBackend.setVideoWinId2(ui->videoPreview->winId());
    kMediaBackend.setVideoWinId(cdgWindow->getCdgWinId());
    bmMediaBackend.setVideoWinId2(ui->videoPreview->winId());
    bmMediaBackend.setVideoWinId(cdgWindow->getCdgWinId());
    setShowBgImage(true);
    settings->restoreWindowState(cdgWindow);
    settings->restoreWindowState(requestsDialog);
    settings->restoreWindowState(regularSingersDialog);
    settings->restoreWindowState(dlgSongShop);
    settings->restoreWindowState(dbDialog);
    settings->restoreWindowState(settingsDialog);
    settings->restoreSplitterState(ui->splitter);
    settings->restoreSplitterState(ui->splitter_2);
    settings->restoreSplitterState(ui->splitterBm);
//    settings->restoreColumnWidths(ui->tableViewDB);
//    settings->restoreColumnWidths(ui->tableViewQueue);
//    settings->restoreColumnWidths(ui->tableViewRotation);
    settings->restoreWindowState(dlgSongShop);
    rotationDataChanged();
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    ui->tableViewDB->hideColumn(7);
    ui->tableViewQueue->hideColumn(0);
    ui->tableViewQueue->hideColumn(1);
    ui->tableViewQueue->hideColumn(2);
    ui->tableViewQueue->hideColumn(6);
    ui->tableViewQueue->hideColumn(9);
    ui->tableViewQueue->hideColumn(10);
    ui->tableViewQueue->hideColumn(11);
    ui->tableViewQueue->hideColumn(12);
    if (!kMediaBackend.canPitchShift())
    {
        ui->tableViewQueue->hideColumn(7);
    }
    qModel->setHeaderData(8, Qt::Horizontal,"");
    qModel->setHeaderData(7, Qt::Horizontal, "Key");
    rotModel->setHeaderData(0,Qt::Horizontal,"");
    rotModel->setHeaderData(1,Qt::Horizontal,"Singer");
    rotModel->setHeaderData(3,Qt::Horizontal,"");
    rotModel->setHeaderData(4,Qt::Horizontal,"");
    ui->tableViewRotation->hideColumn(2);
    ui->tableViewRotation->hideColumn(5);
    qInfo() << "Adding singer count to status bar";
    ui->statusBar->addWidget(&labelSingerCount);
    ui->statusBar->addWidget(&labelRotationDuration);



    bmMediaBackend.setUseFader(true);
    bmPlaylistsModel = new QSqlTableModel(this, database);
    bmPlaylistsModel->setTable("bmplaylists");
    bmPlaylistsModel->sort(2, Qt::AscendingOrder);
    bmDbDialog = new BmDbDialog(database,this);
    bmDbModel = new BmDbTableModel(this, database);
    bmDbModel->setTable("mem.bmsongs");
    bmDbModel->select();
    bmCurrentPlaylist = settings->bmPlaylistIndex();
    bmPlModel = new BmPlTableModel(this, database);
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
    ui->tableViewBmDb->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewBmDb));
    ui->tableViewBmPlaylist->setModel(bmPlModel);
    ui->tableViewBmPlaylist->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewBmPlaylist));
    bmPlDelegate = new BmPlItemDelegate(this);
    ui->tableViewBmPlaylist->setItemDelegate(bmPlDelegate);
    ui->actionDisplay_Filenames->setChecked(settings->bmShowFilenames());
    ui->actionDisplay_Metadata->setChecked(settings->bmShowMetadata());
    settings->restoreSplitterState(ui->splitterBm);
    ui->tableViewBmDb->setColumnHidden(0, true);
    ui->tableViewBmDb->setColumnHidden(3, true);
    ui->tableViewBmDb->setColumnHidden(6, true);
    ui->tableViewBmPlaylist->setColumnHidden(0, true);
    ui->tableViewBmPlaylist->setColumnHidden(1, true);
    settings->restoreSplitterState(ui->splitter_3);
    on_actionDisplay_Filenames_toggled(settings->bmShowFilenames());
    on_actionDisplay_Metadata_toggled(settings->bmShowMetadata());


    connect(&bmMediaBackend, &MediaBackend::stateChanged, this, &MainWindow::bmMediaStateChanged);
    connect(&bmMediaBackend, &MediaBackend::positionChanged, this, &MainWindow::bmMediaPositionChanged);
    connect(&bmMediaBackend, &MediaBackend::durationChanged, this, &MainWindow::bmMediaDurationChanged);
    connect(&bmMediaBackend, &MediaBackend::volumeChanged, ui->sliderBmVolume, &QSlider::setValue);
    connect(bmDbDialog, &BmDbDialog::bmDbUpdated, this, &MainWindow::bmDbUpdated);
    connect(bmDbDialog, &BmDbDialog::bmDbCleared, this, &MainWindow::bmDbCleared);
    connect(bmDbDialog, &BmDbDialog::bmDbAboutToUpdate, this, &MainWindow::bmDatabaseAboutToUpdate);
    ui->sliderBmVolume->setValue(initialBMVol);
    bmMediaBackend.setVolume(initialBMVol);
    ui->sliderVolume->setValue(initialKVol);
    kMediaBackend.setVolume(initialKVol);
    if (settings->mplxMode() == Multiplex_Normal)
        ui->pushButtonMplxBoth->setChecked(true);
    else if (settings->mplxMode() == Multiplex_LeftChannel)
        ui->pushButtonMplxLeft->setChecked(true);
    else if (settings->mplxMode() == Multiplex_RightChannel)
        ui->pushButtonMplxRight->setChecked(true);
    connect(&m_timerKaraokeAA, &QTimer::timeout, this, &MainWindow::karaokeAATimerTimeout);
    ui->actionAutoplay_mode->setChecked(settings->karaokeAutoAdvance());
    connect(ui->actionAutoplay_mode, &QAction::toggled, settings, &Settings::setKaraokeAutoAdvance);
    connect(settings, &Settings::karaokeAutoAdvanceChanged, ui->actionAutoplay_mode, &QAction::setChecked);

    if ((settings->bmAutoStart()) && (bmPlModel->rowCount() > 0))
    {
        qInfo() << "Playlist contains " << bmPlModel->rowCount() << " songs";
        bmCurrentPosition = 0;
        QString path = bmPlModel->index(bmCurrentPosition, 7).data().toString();
        if (QFile::exists(path))
        {
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
            bmMediaBackend.setMedia(path);
            bmMediaBackend.play();
            bmMediaBackend.setVolume(ui->sliderBmVolume->value());
            ui->labelBmPlaying->setText(song);
            ui->labelBmNext->setText(nextSong);
            bmPlDelegate->setCurrentSong(bmCurrentPosition);
            bmPlModel->select();
        }
        else
        {
            QMessageBox::warning(this, tr("Break music autostart failure"), tr("Break music is set to autostart but the first song in the current playlist was not found.\n\nAborting playback."),QMessageBox::Ok);
        }
    }
    cdgOffset = settings->cdgDisplayOffset();

    connect(settings, &Settings::eqBBypassChanged, &bmMediaBackend, &MediaBackend::setEqBypass);
    connect(settings, &Settings::eqBLevel1Changed, &bmMediaBackend, &MediaBackend::setEqLevel1);
    connect(settings, &Settings::eqBLevel2Changed, &bmMediaBackend, &MediaBackend::setEqLevel2);
    connect(settings, &Settings::eqBLevel3Changed, &bmMediaBackend, &MediaBackend::setEqLevel3);
    connect(settings, &Settings::eqBLevel4Changed, &bmMediaBackend, &MediaBackend::setEqLevel4);
    connect(settings, &Settings::eqBLevel5Changed, &bmMediaBackend, &MediaBackend::setEqLevel5);
    connect(settings, &Settings::eqBLevel6Changed, &bmMediaBackend, &MediaBackend::setEqLevel6);
    connect(settings, &Settings::eqBLevel7Changed, &bmMediaBackend, &MediaBackend::setEqLevel7);
    connect(settings, &Settings::eqBLevel8Changed, &bmMediaBackend, &MediaBackend::setEqLevel8);
    connect(settings, &Settings::eqBLevel9Changed, &bmMediaBackend, &MediaBackend::setEqLevel9);
    connect(settings, &Settings::eqBLevel10Changed, &bmMediaBackend, &MediaBackend::setEqLevel10);
    connect(settings, &Settings::eqKBypassChanged, &kMediaBackend, &MediaBackend::setEqBypass);
    connect(settings, &Settings::eqKLevel1Changed, &kMediaBackend, &MediaBackend::setEqLevel1);
    connect(settings, &Settings::eqKLevel2Changed, &kMediaBackend, &MediaBackend::setEqLevel2);
    connect(settings, &Settings::eqKLevel3Changed, &kMediaBackend, &MediaBackend::setEqLevel3);
    connect(settings, &Settings::eqKLevel4Changed, &kMediaBackend, &MediaBackend::setEqLevel4);
    connect(settings, &Settings::eqKLevel5Changed, &kMediaBackend, &MediaBackend::setEqLevel5);
    connect(settings, &Settings::eqKLevel6Changed, &kMediaBackend, &MediaBackend::setEqLevel6);
    connect(settings, &Settings::eqKLevel7Changed, &kMediaBackend, &MediaBackend::setEqLevel7);
    connect(settings, &Settings::eqKLevel8Changed, &kMediaBackend, &MediaBackend::setEqLevel8);
    connect(settings, &Settings::eqKLevel9Changed, &kMediaBackend, &MediaBackend::setEqLevel9);
    connect(settings, &Settings::eqKLevel10Changed, &kMediaBackend, &MediaBackend::setEqLevel10);
    connect(settings, &Settings::enforceAspectRatioChanged, &kMediaBackend, &MediaBackend::setEnforceAspectRatio);
    connect(settings, &Settings::enforceAspectRatioChanged, &bmMediaBackend, &MediaBackend::setEnforceAspectRatio);
    connect(settings, &Settings::mplxModeChanged, &kMediaBackend, &MediaBackend::setMplxMode);
    connect(settings, &Settings::videoOffsetChanged, [&] (auto offsetMs) {
        kMediaBackend.setVideoOffset(offsetMs);
        bmMediaBackend.setVideoOffset(offsetMs);
    });

    kMediaBackend.setEnforceAspectRatio(settings->enforceAspectRatio());
    bmMediaBackend.setEnforceAspectRatio(settings->enforceAspectRatio());

    kMediaBackend.setEqBypass(settings->eqKBypass());
    kMediaBackend.setEqLevel1(settings->eqKLevel1());
    kMediaBackend.setEqLevel2(settings->eqKLevel2());
    kMediaBackend.setEqLevel3(settings->eqKLevel3());
    kMediaBackend.setEqLevel4(settings->eqKLevel4());
    kMediaBackend.setEqLevel5(settings->eqKLevel5());
    kMediaBackend.setEqLevel6(settings->eqKLevel6());
    kMediaBackend.setEqLevel7(settings->eqKLevel7());
    kMediaBackend.setEqLevel8(settings->eqKLevel8());
    kMediaBackend.setEqLevel9(settings->eqKLevel9());
    kMediaBackend.setEqLevel10(settings->eqKLevel10());
    bmMediaBackend.setEqBypass(settings->eqBBypass());
    bmMediaBackend.setEqLevel1(settings->eqBLevel1());
    bmMediaBackend.setEqLevel2(settings->eqBLevel2());
    bmMediaBackend.setEqLevel3(settings->eqBLevel3());
    bmMediaBackend.setEqLevel4(settings->eqBLevel4());
    bmMediaBackend.setEqLevel5(settings->eqBLevel5());
    bmMediaBackend.setEqLevel6(settings->eqBLevel6());
    bmMediaBackend.setEqLevel7(settings->eqBLevel7());
    bmMediaBackend.setEqLevel8(settings->eqBLevel8());
    bmMediaBackend.setEqLevel9(settings->eqBLevel9());
    bmMediaBackend.setEqLevel10(settings->eqBLevel10());
    connect(ui->lineEdit, &CustomLineEdit::escapePressed, ui->lineEdit, &CustomLineEdit::clear);
    connect(ui->lineEditBmSearch, &CustomLineEdit::escapePressed, ui->lineEditBmSearch, &CustomLineEdit::clear);
    connect(qModel, &QueueModel::songDroppedWithoutSinger, this, &MainWindow::songDropNoSingerSel);
    connect(ui->splitter_3, &QSplitter::splitterMoved, [&] () { autosizeViews(); });
    connect(ui->splitterBm, &QSplitter::splitterMoved, [&] () { autosizeBmViews(); });
    checker = new UpdateChecker(this);
    connect(checker, &UpdateChecker::newVersionAvailable, this, &MainWindow::newVersionAvailable);
    checker->checkForUpdates();
    connect(&m_timerButtonFlash, &QTimer::timeout, this, &MainWindow::timerButtonFlashTimeout);
    m_timerButtonFlash.start(1000);
    ui->pushButtonIncomingRequests->setVisible(settings->requestServerEnabled());
    connect(settings, &Settings::requestServerEnabledChanged, ui->pushButtonIncomingRequests, &QPushButton::setVisible);
    connect(ui->actionSong_Shop, &QAction::triggered, [&] () { show(); });
    qInfo() << "Initial UI stup complete";
    connect(qModel, &QueueModel::filesDroppedOnSinger, this, &MainWindow::filesDroppedOnQueue);
    connect(settings, &Settings::applicationFontChanged, this, &MainWindow::appFontChanged);
    QApplication::processEvents();
    QApplication::processEvents();
    appFontChanged(settings->applicationFont());
    QTimer::singleShot(500, [&] () {
       autosizeViews();
       autosizeBmViews();
    });
    scutAddSinger.setKey(QKeySequence(Qt::Key_Insert));
    scutSearch.setKey((QKeySequence(Qt::Key_Slash)));
    scutRegulars.setKey(QKeySequence(Qt::CTRL + Qt::Key_R));
    scutRequests.setKey(QKeySequence(Qt::CTRL + Qt::Key_Q));
    connect(&scutAddSinger, &QShortcut::activated, this, &MainWindow::on_buttonAddSinger_clicked);
    connect(&scutSearch, &QShortcut::activated, this, &MainWindow::scutSearchActivated);
    connect(&scutRegulars, &QShortcut::activated, this, &MainWindow::on_buttonRegulars_clicked);
    connect(&scutRequests, &QShortcut::activated, this, &MainWindow::on_pushButtonIncomingRequests_clicked);
    connect(bmPlModel, &BmPlTableModel::bmSongMoved, this, &MainWindow::bmSongMoved);
    connect(songbookApi, &OKJSongbookAPI::alertRecieved, this, &MainWindow::showAlert);
    connect(settings, &Settings::cdgShowCdgWindowChanged, this, &MainWindow::cdgVisibilityChanged);
    connect(settings, &Settings::rotationShowNextSongChanged, [&] () { resizeRotation(); });
    SfxEntryList list = settings->getSfxEntries();
    qInfo() << "SfxEntryList size: " << list.size();
    foreach (SfxEntry entry, list) {
       addSfxButton(entry.path, entry.name);
    }
    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::tableViewRotationCurrentChanged);
    rotModel->setCurrentSinger(settings->currentRotationPosition());
    rotDelegate->setCurrentSinger(settings->currentRotationPosition());
    updateRotationDuration();
    connect(&m_timerSlowUiUpdate, &QTimer::timeout, this, &MainWindow::updateRotationDuration);
    m_timerSlowUiUpdate.start(10000);
    connect(qModel, &QueueModel::queueModified, [&] () { updateRotationDuration(); });
    connect(settings, &Settings::rotationDurationSettingsModified, this, &MainWindow::updateRotationDuration);
    cdgWindow->setShowBgImage(true);
    lazyDurationUpdater = new LazyDurationUpdateController(this);
    if (settings->dbLazyLoadDurations())
        lazyDurationUpdater->getDurations();
    if (settings->showCdgWindow())
        ui->btnToggleCdgWindow->setText("Hide CDG Window");
    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::rotationSelectionChanged);
    previewEnabled = settings->previewEnabled();
    connect(settings, &Settings::previewEnabledChanged, this, &MainWindow::previewEnabledChanged);

    ui->groupBoxNowPlaying->setVisible(settings->showMainWindowNowPlaying());
    ui->groupBoxSoundClips->setVisible(settings->showMainWindowSoundClips());
    if (settings->showMainWindowSoundClips())
    {
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Ignored);
        ui->groupBoxSoundClips->setVisible(true);
    }
    else
    {
        ui->groupBoxSoundClips->setVisible(false);
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Expanding);
    }
    ui->videoPreview->setVisible(settings->showMainWindowVideo());
    ui->actionNow_Playing->setChecked(settings->showMainWindowNowPlaying());
    ui->actionSound_Clips->setChecked(settings->showMainWindowSoundClips());
    ui->actionVideo_Output_2->setChecked(settings->showMainWindowVideo());
    ui->actionMultiplex_Controls->setChecked(settings->showMplxControls());
    ui->widgetMplxControls->setVisible(settings->showMplxControls());
    switch (settings->mainWindowVideoSize()) {
    case Settings::Small:
        on_actionVideoSmall_triggered();
        break;
    case Settings::Medium:
        on_actionVideoMedium_triggered();
        break;
    case Settings::Large:
        on_actionVideoLarge_triggered();
        break;
    }
    ui->labelVolume->setPixmap(QIcon::fromTheme("player-volume").pixmap(QSize(22,22)));
    ui->labelVolumeBm->setPixmap(QIcon::fromTheme("player-volume").pixmap(QSize(22,22)));
    updateIcons();
    ui->menuTesting->menuAction()->setVisible(settings->testingEnabled());
}

void MainWindow::play(const QString &karaokeFilePath, const bool &k2k)
{
    khTmpDir->remove();
    delete khTmpDir;
    khTmpDir = new QTemporaryDir();
    if (kMediaBackend.state() != MediaBackend::PausedState)
    {
        if (kMediaBackend.state() == MediaBackend::PlayingState)
        {
            if (settings->karaokeAutoAdvance())
            {
                kAASkip = true;
                cdgWindow->showAlert(false);
            }
            kMediaBackend.stop();
            ui->spinBoxTempo->setValue(100);
        }
        if (karaokeFilePath.endsWith(".zip", Qt::CaseInsensitive))
        {
            MzArchive archive(karaokeFilePath);
            if ((archive.checkCDG()) && (archive.checkAudio()))
            {
                if (archive.checkAudio())
                {
                    if (!archive.extractAudio(khTmpDir->path(), "tmp" + archive.audioExtension()))
                    {
                        m_timerTest.stop();
                        QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract audio file."),QMessageBox::Ok);
                        return;
                    }
                    if (!archive.extractCdg(khTmpDir->path(), "tmp.cdg"))
                    {
                        m_timerTest.stop();
                        QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract CDG file."),QMessageBox::Ok);
                        return;
                    }
                    QString audioFile = khTmpDir->path() + QDir::separator() + "tmp" + archive.audioExtension();
                    QString cdgFile = khTmpDir->path() + QDir::separator() + "tmp.cdg";
                    qInfo() << "Extracted audio file size: " << QFileInfo(audioFile).size();
                    cdgWindow->setShowBgImage(false);
                    setShowBgImage(false);
                    qInfo() << "Setting karaoke backend source file to: " << audioFile;
                    kMediaBackend.setMediaCdg(cdgFile, audioFile);
                    if (!k2k)
                        bmMediaBackend.fadeOut(!settings->bmKCrossFade());
                    qInfo() << "Beginning playback of file: " << audioFile;
                    bmMediaBackend.videoMute(true);
                    kMediaBackend.play();
                    kMediaBackend.fadeInImmediate();
                }
            }
            else
            {
                QMessageBox::warning(this, tr("Bad karaoke file"),tr("Zip file does not contain a valid karaoke track.  CDG or audio file missing or corrupt."),QMessageBox::Ok);
                return;
            }
        }
        else if (karaokeFilePath.endsWith(".cdg", Qt::CaseInsensitive))
        {
            QString cdgTmpFile = "tmp.cdg";
            QString audTmpFile = "tmp.mp3";
            QFile cdgFile(karaokeFilePath);
            if (!cdgFile.exists())
            {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file missing."),QMessageBox::Ok);
                return;
            }
            else if (cdgFile.size() == 0)
            {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"),QMessageBox::Ok);
                return;
            }
            QString audiofn = findMatchingAudioFile(karaokeFilePath);
            if (audiofn == "")
            {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file missing."),QMessageBox::Ok);
                return;
            }
            QFile audioFile(audiofn);
            if (audioFile.size() == 0)
            {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file contains no data"),QMessageBox::Ok);
                return;
            }
            cdgFile.copy(khTmpDir->path() + QDir::separator() + cdgTmpFile);
            QFile::copy(audiofn, khTmpDir->path() + QDir::separator() + audTmpFile);
            kMediaBackend.setMediaCdg(khTmpDir->path() + QDir::separator() + cdgTmpFile, khTmpDir->path() + QDir::separator() + audTmpFile);
            if (!k2k)
                bmMediaBackend.fadeOut(!settings->bmKCrossFade());
            bmMediaBackend.videoMute(true);
            kMediaBackend.play();
            kMediaBackend.fadeInImmediate();
        }
        else
        {
            // Close CDG if open to avoid double video playback
            qInfo() << "Playing non-CDG video file: " << karaokeFilePath;
            QString tmpFileName = khTmpDir->path() + QDir::separator() + "tmpvid." + karaokeFilePath.right(4);
            QFile::copy(karaokeFilePath, tmpFileName);
            qInfo() << "Playing temporary copy to avoid bad filename stuff w/ gstreamer: " << tmpFileName;
            kMediaBackend.setMedia(tmpFileName);
            if (!k2k)
                bmMediaBackend.fadeOut();
            bmMediaBackend.videoMute(true);
            kMediaBackend.play();
            kMediaBackend.fadeInImmediate();
        }
        kMediaBackend.setTempo(ui->spinBoxTempo->value());
        if (settings->recordingEnabled())
        {
            qInfo() << "Starting recording";
            QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm");
            audioRecorder.record(curSinger + " - " + curArtist + " - " + curTitle + " - " + timeStamp);
        }
    }
    else if (kMediaBackend.state() == MediaBackend::PausedState)
    {
        if (settings->recordingEnabled())
            audioRecorder.unpause();
        kMediaBackend.play();
        kMediaBackend.fadeIn(false);
    }
    k2kTransition = false;
    if (settings->karaokeAutoAdvance())
        kAASkip = false;

    ui->tableViewQueue->setAttribute(Qt::WA_AcceptDrops, false);
    ui->tableViewQueue->setAttribute(Qt::WA_AcceptDrops, true);
}

MainWindow::~MainWindow()
{
    m_shuttingDown = true;
    cdgWindow->stopTicker();
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    lazyDurationUpdater->stopWork();
    settings->bmSetVolume(ui->sliderBmVolume->value());
    settings->setAudioVolume(ui->sliderVolume->value());
    qInfo() << "Saving volumes - K: " << settings->audioVolume() << " BM: " << settings->bmVolume();
    qInfo() << "Saving window and widget sizing and positioning info";
    settings->saveSplitterState(ui->splitter);
    settings->saveSplitterState(ui->splitter_2);
    settings->saveSplitterState(ui->splitter_3);
    settings->saveColumnWidths(ui->tableViewDB);
    settings->saveColumnWidths(ui->tableViewRotation);
    settings->saveColumnWidths(ui->tableViewQueue);   
    settings->saveWindowState(requestsDialog);
    settings->saveWindowState(regularSingersDialog);
    settings->saveWindowState(dlgSongShop);
    settings->saveWindowState(dlgSongShop);
    settings->saveWindowState(dbDialog);
    settings->saveWindowState(settingsDialog);
    settings->saveWindowState(this);
    settings->saveSplitterState(ui->splitterBm);
    settings->saveColumnWidths(ui->tableViewBmDb);
    settings->saveColumnWidths(ui->tableViewBmPlaylist);
    settings->bmSetPlaylistIndex(ui->comboBoxBmPlaylists->currentIndex());
    settings->sync();
    regularSingersDialog->close();
    qInfo() << "Deleting non-owned objects";
    delete ui;
    delete khTmpDir;
    delete dlgSongShop;
    delete requestsDialog;
    if(_singular->isAttached())
        _singular->detach();

    qInfo() << "OpenKJ mainwindow destructor complete";
}

void MainWindow::search()
{
    dbModel->search(ui->lineEdit->text());
}

void MainWindow::databaseUpdated()
{
    dbModel->refreshCache();
    search();
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    ui->tableViewDB->horizontalHeader()->resizeSection(4,75);
    settings->restoreColumnWidths(ui->tableViewDB);
    requestsDialog->databaseUpdateComplete();
    autosizeViews();
    lazyDurationUpdater->stopWork();
    lazyDurationUpdater->deleteLater();
    lazyDurationUpdater = new LazyDurationUpdateController(this);
    lazyDurationUpdater->getDurations();
}

void MainWindow::databaseCleared()
{
    lazyDurationUpdater->stopWork();
    dbModel->refreshCache();
    dbModel->select();
    rotModel->select();
    qModel->setSinger(-1);
    ui->tableViewQueue->reset();
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    ui->tableViewDB->horizontalHeader()->resizeSection(4,75);
    autosizeViews();
    rotationDataChanged();


}

void MainWindow::on_buttonStop_clicked()
{
    if (kMediaBackend.state() == MediaBackend::PlayingState)
    {
        if (settings->showSongPauseStopWarning())
        {
            QMessageBox msgBox(this);
            QCheckBox *cb = new QCheckBox("Show warning on pause/stop in the future");
            cb->setChecked(settings->showSongPauseStopWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Stop currenly playing karaoke song?");
            msgBox.setInformativeText("There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, settings, &Settings::setShowSongPauseStopWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
            {
                return;
            }
        }
    }
    kAASkip = true;
    cdgWindow->showAlert(false);
    audioRecorder.stop();
    if (settings->bmKCrossFade())
    {
        bmMediaBackend.fadeIn(false);
        kMediaBackend.stop();
    }
    else
    {
        kMediaBackend.stop();
        bmMediaBackend.fadeIn();
    }
//    ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
}

void MainWindow::on_buttonPause_clicked()
{
    if (kMediaBackend.state() == MediaBackend::PausedState)
    {
        kMediaBackend.play();
    }
    else if (kMediaBackend.state() == MediaBackend::PlayingState)
    {
        if (settings->showSongPauseStopWarning())
        {
            QMessageBox msgBox(this);
            QCheckBox *cb = new QCheckBox("Show warning on pause/stop in the future");
            cb->setChecked(settings->showSongPauseStopWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Pause currenly playing karaoke song?");
            msgBox.setInformativeText("There is currently a karaoke song playing.  If you continue, the current song will be paused.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, settings, &Settings::setShowSongPauseStopWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
            {
                return;
            }
        }
        kMediaBackend.pause();
    }
}

void MainWindow::on_lineEdit_returnPressed()
{
    ui->tableViewDB->scrollToTop();
    search();
}

void MainWindow::on_tableViewDB_doubleClicked(const QModelIndex &index)
{
    if (qModel->singer() >= 0)
    {
        qModel->songAdd(index.sibling(index.row(),0).data().toInt());
        updateRotationDuration();
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("No singer selected.  You must select a singer before you can double-click to add to a queue.");
        msgBox.exec();
    }
}

void MainWindow::on_buttonAddSinger_clicked()
{
    dlgAddSinger->show();
}


void MainWindow::on_tableViewRotation_doubleClicked(const QModelIndex &index)
{
    if (index.column() < 3)
    {
        k2kTransition = false;
        int singerId = index.sibling(index.row(),0).data().toInt();
        QString nextSongPath = rotModel->nextSongPath(singerId);
        if (nextSongPath != "")
        {
            if ((kMediaBackend.state() == MediaBackend::PlayingState) && (settings->showSongInterruptionWarning()))
            {
                QMessageBox msgBox(this);
                QCheckBox *cb = new QCheckBox("Show this warning in the future");
                cb->setChecked(settings->showSongInterruptionWarning());
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText("Interrupt currenly playing karaoke song?");
                msgBox.setInformativeText("There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
                QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
                msgBox.addButton(QMessageBox::Cancel);
                msgBox.setCheckBox(cb);
                connect(cb, &QCheckBox::toggled, settings, &Settings::setShowSongInterruptionWarning);
                msgBox.exec();
                if (msgBox.clickedButton() != yesButton)
                {
                    return;
                }
                k2kTransition = true;
            }
            if (kMediaBackend.state() == MediaBackend::PausedState)
            {
                if (settings->karaokeAutoAdvance())
                {
                    kAASkip = true;
                    cdgWindow->showAlert(false);
                }
                audioRecorder.stop();
                kMediaBackend.stop(true);
            }
 //           play(nextSongPath);
 //           kAudioBackend.setPitchShift(rotModel->nextSongKeyChg(singerId));
            rotDelegate->setCurrentSinger(singerId);
            rotModel->setCurrentSinger(singerId);
            curSinger = rotModel->getSingerName(singerId);
            curArtist = rotModel->nextSongArtist(singerId);
            curTitle = rotModel->nextSongTitle(singerId);
            ui->labelArtist->setText(curArtist);
            ui->labelTitle->setText(curTitle);
            ui->labelSinger->setText(curSinger);
            play(nextSongPath, k2kTransition);
            kMediaBackend.setPitchShift(rotModel->nextSongKeyChg(singerId));
            qModel->songSetPlayed(rotModel->nextSongQueueId(singerId));
        }
    }
}

void MainWindow::on_tableViewRotation_clicked(const QModelIndex &index)
{
    if (index.column() == 4)
    {
        if (settings->showSingerRemovalWarning())
        {
            QMessageBox msgBox(this);
            QCheckBox *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings->showSingerRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Are you sure you want to remove this singer?");
            msgBox.setInformativeText("Unless this singer is a tracked regular, you will be unable retrieve any queue data for this singer once they are deleted.");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, settings, &Settings::setShowSingerRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
            {
                return;
            }
        }
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
}

void MainWindow::on_tableViewQueue_doubleClicked(const QModelIndex &index)
{
    k2kTransition = false;
    if (kMediaBackend.state() == MediaBackend::PlayingState)
    {
        if (settings->showSongInterruptionWarning())
        {
            QMessageBox msgBox(this);
            QCheckBox *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings->showSongInterruptionWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Interrupt currenly playing karaoke song?");
            msgBox.setInformativeText("There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, settings, &Settings::setShowSongInterruptionWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
            {
                return;
            }
        }
        k2kTransition = true;
    }
    if (kMediaBackend.state() == MediaBackend::PausedState)
    {
        if (settings->karaokeAutoAdvance())
        {
            kAASkip = true;
            cdgWindow->showAlert(false);
        }
        audioRecorder.stop();
        kMediaBackend.stop(true);
    }
    curSinger = rotModel->getSingerName(index.sibling(index.row(),1).data().toInt());
    curArtist = index.sibling(index.row(),3).data().toString();
    curTitle = index.sibling(index.row(),4).data().toString();
    ui->labelSinger->setText(curSinger);
    ui->labelArtist->setText(curArtist);
    ui->labelTitle->setText(curTitle);
    rotModel->setCurrentSinger(index.sibling(index.row(),1).data().toInt());
    rotDelegate->setCurrentSinger(index.sibling(index.row(),1).data().toInt());
    play(index.sibling(index.row(), 6).data().toString(), k2kTransition);
    kMediaBackend.setPitchShift(index.sibling(index.row(),7).data().toInt());
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

void MainWindow::songDroppedOnSinger(const int &singerId, const int &songId, const int &dropRow)
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
        if ((settings->showQueueRemovalWarning()) && (!qModel->getSongPlayed(index.sibling(index.row(),0).data().toInt())))
        {
            QMessageBox msgBox(this);
            QCheckBox *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings->showQueueRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Removing un-played song from queue");
            msgBox.setInformativeText("This song has not been played yet, are you sure you want to remove it?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, settings, &Settings::setShowQueueRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton)
            {
                qModel->songDelete(index.sibling(index.row(),0).data().toInt());
            }
        }
        else
        {
            qModel->songDelete(index.sibling(index.row(),0).data().toInt());
        }
    }
}

void MainWindow::notify_user(const QString &message)
{
    QMessageBox msgbox(this);
    msgbox.setText(message);
    msgbox.exec();
}

void MainWindow::on_buttonClearRotation_clicked()
{
    if (m_testMode)
    {
        settings->setCurrentRotationPosition(-1);
        rotModel->clearRotation();
        rotDelegate->setCurrentSinger(-1);
        qModel->setSinger(-1);
        return;
    }
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("This action will clear all rotation singers and queues. This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton)
    {
        settings->setCurrentRotationPosition(-1);
        rotModel->clearRotation();
        rotDelegate->setCurrentSinger(-1);
        qModel->setSinger(-1);
    }    
}

void MainWindow::clearQueueSort()
{
    ui->tableViewQueue->sortByColumn(-1, Qt::AscendingOrder);
}


void MainWindow::on_buttonClearQueue_clicked()
{
    if (m_testMode)
    {
        qModel->clearQueue();
        return;
    }
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

void MainWindow::on_spinBoxKey_valueChanged(const int &arg1)
{
    if ((kMediaBackend.state() == MediaBackend::PlayingState) || (kMediaBackend.state() == MediaBackend::PausedState))
    {
        kMediaBackend.setPitchShift(arg1);
        if (arg1 > 0)
            ui->spinBoxKey->setPrefix("+");
        else
            ui->spinBoxKey->setPrefix("");
    }
    else
        ui->spinBoxKey->setValue(0);
    QTimer::singleShot(20, [&] () {
        ui->spinBoxKey->findChild<QLineEdit*>()->deselect();
    });
}

void MainWindow::audioBackend_positionChanged(const qint64 &position)
{
    if (kMediaBackend.state() == MediaBackend::PlayingState)
    {
        if (!sliderPositionPressed)
        {
            ui->sliderProgress->setMaximum(kMediaBackend.duration());
            ui->sliderProgress->setValue(position);
        }
        ui->labelElapsedTime->setText(kMediaBackend.msToMMSS(position));
        ui->labelRemainTime->setText(kMediaBackend.msToMMSS(kMediaBackend.duration() - position));
        remainSecs = (kMediaBackend.duration() - position) / 1000;
    }
}

void MainWindow::audioBackend_durationChanged(const qint64 &duration)
{
    ui->labelTotalTime->setText(kMediaBackend.msToMMSS(duration));
}

void MainWindow::audioBackend_stateChanged(const MediaBackend::State &state)
{
    if (m_shuttingDown)
        return;
    qInfo() << "MainWindow - audioBackend_stateChanged(" << state << ") triggered";
    if (state == MediaBackend::StoppedState)
    {
        qInfo() << "MainWindow - audio backend state is now STOPPED";
        if (ui->labelTotalTime->text() == "0:00")
        {
            qInfo() << "MainWindow - UI is already reset, bailing out";
            return;
        }
        qInfo() << "KAudio entered StoppedState";
        bmMediaBackend.videoMute(false);
        audioRecorder.stop();
        if (k2kTransition)
            return;
        ui->labelArtist->setText("None");
        ui->labelTitle->setText("None");
        ui->labelSinger->setText("None");
        ui->labelElapsedTime->setText("0:00");
        ui->labelRemainTime->setText("0:00");
        ui->labelTotalTime->setText("0:00");
        ui->sliderProgress->setValue(0);
        ui->spinBoxTempo->setValue(100);
        setShowBgImage(true);
        cdgWindow->setShowBgImage(true);
        cdgWindow->triggerBg();
        if (state == m_lastAudioState)
            return;
        m_lastAudioState = state;
        bmMediaBackend.fadeIn(false);
        if (settings->karaokeAutoAdvance())
        {
            qInfo() << " - Karaoke Autoplay is enabled";
            if (kAASkip == true)
            {
                kAASkip = false;
                qInfo() << " - Karaoke Autoplay set to skip, bailing out";
            }
            else
            {
                int nextSinger = -1;
                int nextPos;
                QString nextSongPath;
                bool empty = false;
                int loops = 0;
                while ((nextSongPath == "") && (!empty))
                {
                    if (loops > rotModel->singerCount)
                    {
                        empty = true;
                    }
                    else
                    {
                        int curSinger = rotModel->currentSinger();
                        int curPos = rotModel->getSingerPosition(curSinger);
                        if ((curPos + 1) < rotModel->singerCount)
                        {
                            nextPos = curPos + 1;
                        }
                        else
                        {
                            nextPos = 0;
                        }
                        nextSinger = rotModel->singerIdAtPosition(nextPos);
                        rotModel->setCurrentSinger(nextSinger);
                        rotDelegate->setCurrentSinger(nextSinger);
                        nextSongPath = rotModel->nextSongPath(nextSinger);
                        loops++;
                    }
                }
                if (empty)
                    qInfo() << "KaraokeAA - No more songs to play, giving up";
                else
                {
                    kAANextSinger = nextSinger;
                    kAANextSongPath = nextSongPath;
                    qInfo() << "KaraokeAA - Will play: " << rotModel->getSingerName(nextSinger) << " - " << nextSongPath;
                    qInfo() << "KaraokeAA - Starting " << settings->karaokeAATimeout() << " second timer";
                    m_timerKaraokeAA.start(settings->karaokeAATimeout() * 1000);
                    cdgWindow->setNextSinger(rotModel->getSingerName(nextSinger));
                    cdgWindow->setNextSong(rotModel->nextSongArtist(nextSinger) + " - " + rotModel->nextSongTitle(nextSinger));
                    cdgWindow->setCountdownSecs(settings->karaokeAATimeout());
                    cdgWindow->showAlert(true);
                }
            }
        }
    }
    if (state == MediaBackend::EndOfMediaState)
    {
        qInfo() << "KAudio entered EndOfMediaState";
        audioRecorder.stop();
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        bmMediaBackend.videoMute(false);
        kMediaBackend.stop(true);
        bmMediaBackend.fadeIn(false);
        cdgWindow->setShowBgImage(true);
    }
    if (state == MediaBackend::PausedState)
    {
        qInfo() << "KAudio entered PausedState";
            audioRecorder.pause();
    }
    if (state == MediaBackend::PlayingState)
    {
        qInfo() << "KAudio entered PlayingState";
        m_lastAudioState = state;
        bmMediaBackend.videoMute(true);
        cdgWindow->setShowBgImage(false);
    }
    if (state == MediaBackend::UnknownState)
    {
        qInfo() << "KAudio entered UnknownState";
    }
    rotationDataChanged();
}

void MainWindow::sfxAudioBackend_positionChanged(const qint64 &position)
{
    ui->sliderSfxPos->setValue(position);
}

void MainWindow::sfxAudioBackend_durationChanged(const qint64 &duration)
{
    ui->sliderSfxPos->setMaximum(duration);
}

void MainWindow::sfxAudioBackend_stateChanged(const MediaBackend::State &state)
{
    if (state == MediaBackend::EndOfMediaState)
    {
        ui->sliderSfxPos->setValue(0);
        sfxMediaBackend.stop();
    }
    if (state == MediaBackend::StoppedState || state == MediaBackend::UnknownState)
        ui->sliderSfxPos->setValue(0);
}

void MainWindow::on_sliderProgress_sliderMoved(const int &position)
{
    Q_UNUSED(position);
}

void MainWindow::on_buttonRegulars_clicked()
{
    regularSingersDialog->show();
}

void MainWindow::rotationDataChanged()
{
    if (m_shuttingDown)
        return;
    if (settings->rotationShowNextSong())
        resizeRotation();
    updateRotationDuration();
    QString sep = "•";
    requestsDialog->rotationChanged();
    QString statusBarText = "Singers: ";
    statusBarText += QString::number(rotModel->rowCount());
    rotDelegate->setSingerCount(rotModel->rowCount());
    labelSingerCount.setText(statusBarText);
    QString tickerText;
    if (settings->tickerCustomString() != "")
    {
        tickerText += settings->tickerCustomString() + " " + sep + " ";
        QString cs = rotModel->getSingerName(rotModel->currentSinger());
        int nsPos;
        if (cs == "")
        {
            cs = rotModel->getSingerName(rotModel->singerIdAtPosition(0));
            if (cs == "")
                cs = "[nobody]";
            nsPos = 0;
        }
        else
            nsPos = rotModel->getSingerPosition(rotModel->currentSinger());
        QString ns = "[nobody]";
        if (rotModel->rowCount() > 0)
        {
            if (nsPos + 1 < rotModel->rowCount())
                nsPos++;
            else
                nsPos = 0;
            ns = rotModel->getSingerName(rotModel->singerIdAtPosition(nsPos));
        }
        tickerText.replace("%cs", cs);
        tickerText.replace("%ns", ns);
        tickerText.replace("%rc", QString::number(rotModel->rowCount()));
        if (ui->labelArtist->text() == "None" && ui->labelTitle->text() == "None")
            tickerText.replace("%curSong", "None");
        else
            tickerText.replace("%curSong", ui->labelArtist->text() + " - " + ui->labelTitle->text());
        tickerText.replace("%curArtist", ui->labelArtist->text());
        tickerText.replace("%curTitle", ui->labelTitle->text());
        tickerText.replace("%curSinger", cs);
        tickerText.replace("%nextSinger", ns);

    }
    if (settings->tickerShowRotationInfo())
    {
        tickerText += "Singers: ";
        tickerText += QString::number(rotModel->rowCount());
        tickerText += " " + sep + " Current: ";
        int displayPos;
        QString curSinger = rotModel->getSingerName(rotModel->currentSinger());
        if (curSinger != "")
        {
            tickerText += curSinger;
            displayPos = rotModel->getSingerPosition(rotModel->currentSinger());
        }
        else
        {
            tickerText += "None ";
            displayPos = -1;
        }
        int listSize;
        if (settings->tickerFullRotation() || (rotModel->rowCount() < settings->tickerShowNumSingers()))
        {
            if (curSinger == "")
                listSize = rotModel->rowCount();
            else
                listSize = rotModel->rowCount() - 1;
            if (listSize > 0)
                tickerText += " " + sep + " Upcoming: ";
        }
        else
        {
            listSize = settings->tickerShowNumSingers();
            tickerText += " " + sep + " Next ";
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
            if (i < listSize - 1)
                tickerText += " ";
        }
       // tickerText += "|";
    }
    cdgWindow->setTickerText(tickerText);
}

void MainWindow::silenceDetected()
{
    qInfo() << "Detected silence.  Cur Pos: " << kMediaBackend.position() << " Last CDG update pos: " << kMediaBackend.getCdgLastDraw();
    if (kMediaBackend.isCdgMode() && kMediaBackend.getCdgLastDraw() < kMediaBackend.position())
    {
        kMediaBackend.rawStop();
        if (settings->karaokeAutoAdvance())
            kAASkip = false;
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        bmMediaBackend.fadeIn();
    }
}

void MainWindow::silenceDetectedBm()
{
    if (bmMediaBackend.position() > 10000 && bmMediaBackend.position() < (bmMediaBackend.duration() - 3))
    {
        qInfo() << "Break music silence detected, reporting EndOfMediaState to trigger playlist advance";
        bmMediaStateChanged(MediaBackend::EndOfMediaState);
    }
}

void MainWindow::audioBackendChanged(const int &index)
{
    Q_UNUSED(index)
    //kAudioBackend = audioBackends->at(index);
}

void MainWindow::on_tableViewDB_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewDB->indexAt(pos);
    if (index.isValid())
    {
        dbRtClickFile = index.sibling(index.row(), 5).data().toString();
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", this, &MainWindow::previewCdg);
        contextMenu.addSeparator();
        contextMenu.addAction("Edit", this, &MainWindow::editSong);
        contextMenu.addAction("Mark bad", this, &MainWindow::markSongBad);
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
           contextMenu.addAction("Rename", this, &MainWindow::renameSinger);
           contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::sfxButton_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    SoundFxButton *btn = (SoundFxButton*)sender();
    lastRtClickedSfxBtn.path = btn->buttonData().toString();
    lastRtClickedSfxBtn.name = btn->text();
    QMenu contextMenu(this);
    contextMenu.addAction("Remove", this, &MainWindow::removeSfxButton);
    contextMenu.exec(QCursor::pos());
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
    int selCount = ui->tableViewQueue->selectionModel()->selectedRows().size();
    if (selCount == 1)
    {
        QModelIndex index = ui->tableViewQueue->indexAt(pos);
        if (index.isValid())
        {
            dbRtClickFile = index.sibling(index.row(), 6).data().toString();
            m_rtClickQueueSongId = index.sibling(index.row(), 0).data().toInt();
            dlgKeyChange->setActiveSong(m_rtClickQueueSongId);
            QMenu contextMenu(this);
            contextMenu.addAction("Preview", this, &MainWindow::previewCdg);
            contextMenu.addAction("Set Key Change", this, &MainWindow::setKeyChange);
            contextMenu.addAction("Toggle played", this, &MainWindow::toggleQueuePlayed);
            contextMenu.exec(QCursor::pos());
        }
    }
    else if (selCount > 1)
    {
        QMenu contextMenu(this);
        contextMenu.addAction("Set Played", this, &MainWindow::setMultiPlayed);
        contextMenu.addAction("Set Unplayed", this, &MainWindow::setMultiUnplayed);
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_sliderProgress_sliderPressed()
{
    sliderPositionPressed = true;
}

void MainWindow::on_sliderProgress_sliderReleased()
{
    if (ui->spinBoxTempo->value() != 100 && kMediaBackend.isCdgMode())
    {
        sliderPositionPressed = false;
        QMessageBox::information(this, "Seek Aborted","Seeking disabled while playing CDG files with tempo change.  Aborted.",QMessageBox::Ok);
        return;
    }
    kMediaBackend.setPosition(ui->sliderProgress->value());
    sliderPositionPressed = false;
}

void MainWindow::setKeyChange()
{
    dlgKeyChange->show();
}

void MainWindow::toggleQueuePlayed()
{
    qModel->songSetPlayed(m_rtClickQueueSongId, !qModel->getSongPlayed(m_rtClickQueueSongId));
    updateRotationDuration();
}

void MainWindow::regularNameConflict(const QString &name)
{
    QMessageBox::warning(this, "Regular singer exists!","A regular singer named " + name + " already exists. You must either delete or rename the existing regular first, or rename the new regular singer and try again. The operation has been cancelled.",QMessageBox::Ok);
}

void MainWindow::regularAddError(const QString &errorText)
{
    QMessageBox::warning(this, "Error adding regular singer!", errorText,QMessageBox::Ok);
}

void MainWindow::previewCdg()
{
    if (!QFile::exists(dbRtClickFile))
    {
        QMessageBox::warning(this, tr("Missing File!"), "Specified karaoke file missing, preview aborted!\n\n" + dbRtClickFile,QMessageBox::Ok);
        return;
    }
    auto *videoPreview = new DlgVideoPreview(dbRtClickFile, this);
    videoPreview->setAttribute(Qt::WA_DeleteOnClose);
    videoPreview->show();
//    DlgCdgPreview *cdgPreviewDialog = new DlgCdgPreview(this);
//    cdgPreviewDialog->setAttribute(Qt::WA_DeleteOnClose);
//    cdgPreviewDialog->setSourceFile(dbRtClickFile);
//    cdgPreviewDialog->preview();
}

void MainWindow::editSong()
{
    bool isCdg = false;
    if (QFileInfo(dbRtClickFile).suffix().toLower() == "cdg")
        isCdg = true;
    QString mediaFile;
    if (isCdg)
        mediaFile = DbUpdateThread::findMatchingAudioFile(dbRtClickFile);
    SourceDirTableModel model;
    SourceDir *srcDir = model.getDirByPath(dbRtClickFile);
    if (srcDir->getIndex() == -1)
    {

    }
    int rowId;
    QString artist;
    QString title;
    QString songId;
    QSqlQuery query;
    query.prepare("SELECT songid,artist,title,discid FROM dbsongs WHERE path = :path LIMIT 1");
    query.bindValue(":path", dbRtClickFile);
    query.exec();
    if (!query.next())
        qInfo() << "Unable to find song in db!";
    artist = query.value("artist").toString();
    title = query.value("title").toString();
    songId = query.value("discid").toString();
    rowId = query.value("songid").toInt();
    qInfo() << "db song match: " << rowId << ": " << artist << " - " << title << " - " << songId;
    bool allowRename = true;
    bool showSongId = true;
    if (srcDir->getPattern() == SourceDir::AT || srcDir->getPattern() == SourceDir::TA)
        showSongId = false;
    if (srcDir->getPattern() == SourceDir::CUSTOM || srcDir->getPattern() == SourceDir::METADATA)
        allowRename = false;
    if (srcDir->getIndex() == -1)
    {
        allowRename = false;
        QMessageBox msgBoxErr;
        msgBoxErr.setText("Unable to find a configured source path containing the file.");
        msgBoxErr.setInformativeText("You won't be able to rename the file.  To fix this, ensure that a source directory is configured in the database settings which contains this file.");
        msgBoxErr.setStandardButtons(QMessageBox::Ok);
        msgBoxErr.exec();
    }
    DlgEditSong dlg(artist, title, songId, showSongId, allowRename, this);
    int result = dlg.exec();
    if (result != QDialog::Accepted)
        return;
    if (artist == dlg.artist() && title == dlg.title() && songId == dlg.songId())
        return;
    if (dlg.renameFile())
    {
        if (!QFileInfo(dbRtClickFile).isWritable())
        {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Unable to rename file");
            msgBoxErr.setInformativeText("Unable to rename file, your user does not have write permissions. Operation cancelled.");
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
        if (isCdg)
        {
            if (!QFileInfo(mediaFile).isWritable())
            {
                QMessageBox msgBoxErr;
                msgBoxErr.setText("Unable to rename file");
                msgBoxErr.setInformativeText("Unable to rename file, your user does not have write permissions. Operation cancelled.");
                msgBoxErr.setStandardButtons(QMessageBox::Ok);
                msgBoxErr.exec();
                return;
            }
        }
        QString newFn;
        QString newMediaFn;
        bool unsupported = false;
        switch (srcDir->getPattern()) {
        case SourceDir::SAT:
            newFn = dlg.songId() + " - " + dlg.artist() + " - " + dlg.title() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            if (isCdg)
                newMediaFn = dlg.songId() + " - " + dlg.artist() + " - " + dlg.title() + "." + QFileInfo(mediaFile).completeSuffix();
            break;
        case SourceDir::STA:
            newFn = dlg.songId() + " - " + dlg.title() + " - " + dlg.artist() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            if (isCdg)
                newMediaFn = dlg.songId() + " - " + dlg.title() + " - " + dlg.artist() + "." + QFileInfo(mediaFile).completeSuffix();
            break;
        case SourceDir::ATS:
            newFn = dlg.artist() + " - " + dlg.title() + " - " + dlg.songId() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            if (isCdg)
                newMediaFn = dlg.artist() + " - " + dlg.title() + " - " + dlg.songId() + "." + QFileInfo(mediaFile).completeSuffix();
            break;
        case SourceDir::TAS:
            newFn = dlg.title() + " - " + dlg.artist() + " - " + dlg.songId() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            if (isCdg)
                newMediaFn = dlg.title() + " - " + dlg.artist() + " - " + dlg.songId() + "." + QFileInfo(mediaFile).completeSuffix();
            break;
        case SourceDir::S_T_A:
            newFn = dlg.songId() + "_" + dlg.title() + "_" + dlg.artist() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            if (isCdg)
                newMediaFn = dlg.songId() + "_" + dlg.title() + "_" + dlg.artist() + "." + QFileInfo(mediaFile).completeSuffix();
            break;
        case SourceDir::AT:
            newFn = dlg.artist() + " - " + dlg.title() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            if (isCdg)
                newMediaFn = dlg.artist() + " - " + dlg.title() + "." + QFileInfo(mediaFile).completeSuffix();
            break;
        case SourceDir::TA:
            newFn = dlg.title() + " - " + dlg.artist() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            if (isCdg)
                newMediaFn = dlg.title() + " - " + dlg.artist() + "." + QFileInfo(mediaFile).completeSuffix();
            break;
        case SourceDir::CUSTOM:
            unsupported = true;
            break;
        case SourceDir::METADATA:
            unsupported = true;
            break;
        }
        if (unsupported)
        {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Unable to rename file");
            msgBoxErr.setInformativeText("Unable to rename file, renaming custom and metadata based source files is not supported. Operation cancelled.");
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
        if (QFile::exists(newFn))
        {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Unable to rename file");
            msgBoxErr.setInformativeText("Unable to rename file, a file by that name already exists in the same directory. Operation cancelled.");
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
        if (isCdg)
        {
            if (QFile::exists(newMediaFn))
            {
                QMessageBox msgBoxErr;
                msgBoxErr.setText("Unable to rename file");
                msgBoxErr.setInformativeText("Unable to rename file, a file by that name already exists in the same directory. Operation cancelled.");
                msgBoxErr.setStandardButtons(QMessageBox::Ok);
                msgBoxErr.exec();
                return;
            }
        }
        QString newFilePath = QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn;
        if (newFilePath != dbRtClickFile)
        {
            if (!QFile::rename(dbRtClickFile, QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn))
            {
                QMessageBox msgBoxErr;
                msgBoxErr.setText("Error while renaming file!");
                msgBoxErr.setInformativeText("An unknown error occurred while renaming the file. Operation cancelled.");
                msgBoxErr.setStandardButtons(QMessageBox::Ok);
                msgBoxErr.exec();
                return;
            }
            if (isCdg)
            {
                if (!QFile::rename(mediaFile, QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newMediaFn))
                {
                    QMessageBox msgBoxErr;
                    msgBoxErr.setText("Error while renaming file!");
                    msgBoxErr.setInformativeText("An unknown error occurred while renaming the file. Operation cancelled.");
                    msgBoxErr.setStandardButtons(QMessageBox::Ok);
                    msgBoxErr.exec();
                    return;
                }
            }
        }
        qInfo() << "New filename: " << newFn;
        query.prepare("UPDATE dbsongs SET artist = :artist, title = :title, discid = :songid, path = :path, filename = :filename, searchstring = :searchstring WHERE songid = :rowid");
        QString newArtist = dlg.artist();
        QString newTitle = dlg.title();
        QString newSongId = dlg.songId();
        QString newPath = QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn;
        QString newSearchString = QFileInfo(newPath).completeBaseName() + " " + newArtist + " " + newTitle + " " + newSongId;
        query.bindValue(":artist", newArtist);
        query.bindValue(":title", newTitle);
        query.bindValue(":songid", newSongId);
        query.bindValue(":path", newPath);
        query.bindValue(":filename", newFn);
        query.bindValue(":searchstring", newSearchString);
        query.bindValue(":rowid", rowId);
        query.exec();
        qInfo() << query.lastError();
        if (!query.lastError().isValid())
        {
            query.prepare("UPDATE mem.dbsongs SET artist = :artist, title = :title, discid = :songid, path = :path, filename = :filename, searchstring = :searchstring WHERE songid = :rowid");
            query.bindValue(":artist", newArtist);
            query.bindValue(":title", newTitle);
            query.bindValue(":songid", newSongId);
            query.bindValue(":path", newPath);
            query.bindValue(":filename", newFn);
            query.bindValue(":searchstring", newSearchString);
            query.bindValue(":rowid", rowId);
            query.exec();
            QMessageBox msgBoxInfo;
            msgBoxInfo.setText("Edit successful");
            msgBoxInfo.setInformativeText("The file has been renamed and the database has been updated successfully.");
            msgBoxInfo.setStandardButtons(QMessageBox::Ok);
            msgBoxInfo.exec();
            dbModel->select();
            return;
        }
        else
        {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Error while updating the database!");
            msgBoxErr.setInformativeText(query.lastError().text());
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
            //QFile::rename(QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn, dbRtClickFile);
        }
    }
    else
    {
        query.prepare("UPDATE dbsongs SET artist = :artist, title = :title, discid = :songid, searchstring = :searchstring WHERE songid = :rowid");
        QString newArtist = dlg.artist();
        QString newTitle = dlg.title();
        QString newSongId = dlg.songId();
        QString newSearchString = QFileInfo(dbRtClickFile).completeBaseName() + " " + newArtist + " " + newTitle + " " + newSongId;
        query.bindValue(":artist", newArtist);
        query.bindValue(":title", newTitle);
        query.bindValue(":songid", newSongId);
        query.bindValue(":searchstring", newSearchString);
        query.bindValue(":rowid", rowId);
        query.exec();
        qInfo() << query.lastError();
        if (!query.lastError().isValid())
        {
            query.prepare("UPDATE mem.dbsongs SET artist = :artist, title = :title, discid = :songid, searchstring = :searchstring WHERE songid = :rowid");
            query.bindValue(":artist", newArtist);
            query.bindValue(":title", newTitle);
            query.bindValue(":songid", newSongId);
            query.bindValue(":searchstring", newSearchString);
            query.bindValue(":rowid", rowId);
            query.exec();
            QMessageBox msgBoxInfo;
            msgBoxInfo.setText("Edit successful");
            msgBoxInfo.setInformativeText("The database has been updated successfully.");
            msgBoxInfo.setStandardButtons(QMessageBox::Ok);
            msgBoxInfo.exec();
            dbModel->select();
            return;
        }
        else
        {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Error while updating the database!");
            msgBoxErr.setInformativeText(query.lastError().text());
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
    }

}

void MainWindow::markSongBad()
{
    QMessageBox msgBox;
    msgBox.setText("Marking song as bad");
    msgBox.setInformativeText("Would you like mark the file as bad in the DB, or remove it from disk permanently?");
    QPushButton *markBadButton = msgBox.addButton(tr("Mark Bad"), QMessageBox::ActionRole);
    QPushButton *removeFileButton = msgBox.addButton(tr("Remove File"), QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);

    msgBox.exec();

    if (msgBox.clickedButton() == markBadButton) {
        QSqlQuery query;
        query.exec("UPDATE dbsongs SET 'discid'=\"!!BAD!!\" WHERE path == \"" + dbRtClickFile + "\"");
        databaseUpdated();
        // connect
    } else if (msgBox.clickedButton() == removeFileButton) {
        bool isCdg = false;
        if (QFileInfo(dbRtClickFile).suffix().toLower() == "cdg")
            isCdg = true;
        QString mediaFile;
        if (isCdg)
            mediaFile = DbUpdateThread::findMatchingAudioFile(dbRtClickFile);
        QFile file(dbRtClickFile);
        if (file.remove())
        {
            if (isCdg)
            {
                if (!QFile::remove(mediaFile))
                {
                    QMessageBox msgBoxErr;
                    msgBoxErr.setText("Error deleting file");
                    msgBoxErr.setInformativeText("The cdg file was deleted, but there was an error while deleting the matching media file.  You will need to manually remove the file.");
                    msgBoxErr.setStandardButtons(QMessageBox::Ok);
                    msgBoxErr.exec();
                }
            }
            QSqlQuery query;
            query.exec("UPDATE dbsongs SET 'discid'=\"!!BAD!!\" WHERE path == \"" + dbRtClickFile + "\"");
            databaseUpdated();
        }
        else
        {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Error deleting file");
            msgBoxErr.setInformativeText("Unable to remove the file.  Please check file permissions.  Operation cancelled.");
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
        }
    } else if (msgBox.clickedButton() == cancelButton){
        //abort
    }
}

void MainWindow::setShowBgImage(const bool &show)
{
    if (show)
    {
        ui->videoPreview->useDefaultBackground();
    }
}

void MainWindow::onBgImageChange()
{
   if (kMediaBackend.state() == MediaBackend::StoppedState)
       cdgWindow->setShowBgImage(true);
}

void MainWindow::karaokeAATimerTimeout()
{
    qInfo() << "KaraokeAA - timer timeout";
    m_timerKaraokeAA.stop();
    cdgWindow->showAlert(false);
    if (kAASkip)
    {
        qInfo() << "KaraokeAA - Aborted via stop button";
        kAASkip = false;
    }
    else
    {
        curSinger = rotModel->getSingerName(kAANextSinger);
        curArtist = rotModel->nextSongArtist(kAANextSinger);
        curTitle = rotModel->nextSongTitle(kAANextSinger);
        ui->labelArtist->setText(curArtist);
        ui->labelTitle->setText(curTitle);
        ui->labelSinger->setText(curSinger);
        play(kAANextSongPath);
        kMediaBackend.setPitchShift(rotModel->nextSongKeyChg(kAANextSinger));
        qModel->songSetPlayed(rotModel->nextSongQueueId(kAANextSinger));
    }
}

void MainWindow::timerButtonFlashTimeout()
{
    static QString normalSS = " \
        QPushButton { \
            border: 2px solid #8f8f91; \
            border-radius: 6px; \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); \
            min-width: 80px; \
            padding-left: 5px; \
            padding-right: 5px; \
            padding-top: 3px; \
            padding-bottom: 3px; \
        } \
        QPushButton:pressed { \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa); \
        } \
        QPushButton:flat { \
            border: none; /* no border for a flat push button */ \
        } \
        QPushButton:default { \
            border-color: navy; /* make the default button prominent */ \
        }";

    static QString blinkSS = " \
        QPushButton { \
            border: 2px solid #8f8f91; \
            border-radius: 6px; \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f700, stop: 1 #dadb00); \
            min-width: 80px; \
            padding-left: 5px; \
            padding-right: 5px; \
            padding-top: 3px; \
            padding-bottom: 3px; \
        } \
        QPushButton:pressed { \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa); \
        } \
        QPushButton:flat { \
            border: none; /* no border for a flat push button */ \
        } \
        QPushButton:default { \
            border-color: navy; /* make the default button prominent */ \
        } \
    ";

    if (requestsDialog->numRequests() > 0)
    {
        static bool flashed = false;
        if (settings->theme() != 0)
        {
            QColor normal = this->palette().button().color();
            QColor blink = QColor("yellow");
            QColor blinkTxt = QColor("black");
            QColor normalTxt = this->palette().buttonText().color();
            QPalette palette = QPalette(ui->pushButtonIncomingRequests->palette());
            palette.setColor(QPalette::Button, (flashed) ? normal : blink);
            palette.setColor(QPalette::ButtonText, (flashed) ? normalTxt : blinkTxt);
            ui->pushButtonIncomingRequests->setPalette(palette);
            ui->pushButtonIncomingRequests->setText(" Requests (" + QString::number(requestsDialog->numRequests()) + ") ");
            flashed = !flashed;
        }
        else
        {
            ui->pushButtonIncomingRequests->setText(" Requests (" + QString::number(requestsDialog->numRequests()) + ") ");
            ui->pushButtonIncomingRequests->setStyleSheet((flashed) ? normalSS : blinkSS);
            flashed = !flashed;
        }
        update();
    }
    else if (ui->pushButtonIncomingRequests->text() != "Requests")
    {
        if (settings->theme() != 0)
        {
            ui->pushButtonIncomingRequests->setPalette(this->palette());
            ui->pushButtonIncomingRequests->setText("Requests");
        }
        else
        {
            ui->pushButtonIncomingRequests->setStyleSheet(normalSS);
            ui->pushButtonIncomingRequests->setText(" Requests ");
        }
        update();
    }
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
    bmDbModel->refreshCache();
    bmDbModel->select();
    ui->comboBoxBmPlaylists->setCurrentIndex(0);
    ui->tableViewBmDb->setColumnHidden(0, true);
    ui->tableViewBmDb->setColumnHidden(3, true);
    ui->tableViewBmDb->setColumnHidden(6, true);
    ui->tableViewBmDb->setColumnHidden(4, !settings->bmShowFilenames());
    ui->tableViewBmDb->horizontalHeader()->resizeSection(4, 100);
    ui->tableViewBmDb->setColumnHidden(1, !settings->bmShowMetadata());
    ui->tableViewBmDb->setColumnHidden(2, !settings->bmShowMetadata());
    ui->tableViewBmDb->horizontalHeader()->resizeSection(1, 100);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(2, 100);
    ui->tableViewBmDb->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(5,75);
}

void MainWindow::bmDbCleared()
{
    bmDbModel->refreshCache();
    bmDbModel->select();
    bmAddPlaylist("Default");
    ui->comboBoxBmPlaylists->setCurrentIndex(0);
    ui->tableViewBmDb->setColumnHidden(0, true);
    ui->tableViewBmDb->setColumnHidden(3, true);
    ui->tableViewBmDb->setColumnHidden(6, true);
    ui->tableViewBmDb->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->tableViewBmDb->setColumnHidden(4, !settings->bmShowFilenames());
    ui->tableViewBmDb->horizontalHeader()->resizeSection(4, 100);
    ui->tableViewBmDb->setColumnHidden(1, !settings->bmShowMetadata());
    ui->tableViewBmDb->setColumnHidden(2, !settings->bmShowMetadata());
    ui->tableViewBmDb->horizontalHeader()->resizeSection(1, 100);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(2, 100);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(5,75);
}

void MainWindow::bmShowMetadata(const bool &checked)
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


void MainWindow::bmShowFilenames(const bool &checked)
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

void MainWindow::bmMediaStateChanged(const MediaBackend::State &newState)
{
    static MediaBackend::State lastState = MediaBackend::StoppedState;
    if (newState != lastState)
    {
        lastState = newState;
        if (newState == MediaBackend::EndOfMediaState)
        {
            if (ui->checkBoxBmBreak->isChecked())
            {
                bmMediaBackend.stop(true);
                ui->checkBoxBmBreak->setChecked(false);
                return;
            }
            if (bmCurrentPosition < bmPlModel->rowCount() - 1)
                bmCurrentPosition++;
            else
                bmCurrentPosition = 0;
            bmMediaBackend.stop(true);
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
            qInfo() << "Break music auto-advancing to song: " << path;
            bmMediaBackend.setMedia(path);
            bmMediaBackend.play();
            //bmMediaBackend.setVolume(ui->sliderBmVolume->value());
            if (kMediaBackend.state() == MediaBackend::PlayingState)
                bmMediaBackend.fadeOutImmediate();
            ui->labelBmPlaying->setText(song);
            ui->labelBmNext->setText(nextSong);
            bmPlDelegate->setCurrentSong(bmCurrentPosition);
            bmPlModel->select();
        }
        if (newState == MediaBackend::StoppedState)
        {
            ui->labelBmPlaying->setText("None");
            ui->labelBmNext->setText("None");
            ui->labelBmDuration->setText("00:00");
            ui->labelBmRemaining->setText("00:00");
            ui->labelBmPosition->setText("00:00");
            ui->sliderBmPosition->setValue(0);
            if (kMediaBackend.state() != MediaBackend::PlayingState)
            {
                setShowBgImage(true);
                cdgWindow->setShowBgImage(true);
                cdgWindow->triggerBg();
            }
        }
    }
}

void MainWindow::bmMediaPositionChanged(const qint64 &position)
{
    if (!sliderBmPositionPressed)
    {
        ui->sliderBmPosition->setValue(position);
    }
    ui->labelBmPosition->setText(QTime(0,0,0,0).addMSecs(position).toString("m:ss"));
    ui->labelBmRemaining->setText(QTime(0,0,0,0).addMSecs(bmMediaBackend.duration() - position).toString("m:ss"));
}

void MainWindow::bmMediaDurationChanged(const qint64 &duration)
{
    ui->sliderBmPosition->setMaximum(duration);
    ui->labelBmDuration->setText(QTime(0,0,0,0).addMSecs(duration).toString("m:ss"));
}

void MainWindow::on_tableViewBmPlaylist_clicked(const QModelIndex &index)
{
    if (index.column() == 7)
    {
        if (bmCurrentPosition == index.row())
        {
            if (bmMediaBackend.state() != MediaBackend::PlayingState && bmMediaBackend.state() != MediaBackend::PausedState)
            {
                bmPlDelegate->setCurrentSong(-1);
                bmCurrentPosition = -1;
                bmPlModel->deleteSong(index.row());
                return;
            }
            QMessageBox msgBox;
            msgBox.setWindowTitle("Unable to remove");
            msgBox.setText("The playlist song you are trying to remove is currently playing and can not be removed.");
            msgBox.exec();
            return;
        }
        int pos = index.row();
        bmPlModel->deleteSong(pos);
        if (bmCurrentPosition > pos)
        {
            qInfo() << "deleted item, moving curpos - delPos:" << pos << " curPos:" << bmCurrentPosition;
            bmCurrentPosition--;
            bmPlDelegate->setCurrentSong(bmCurrentPosition);
        }
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
        ui->labelBmNext->setText(nextSong);

    }
}

void MainWindow::on_comboBoxBmPlaylists_currentIndexChanged(const int &index)
{
    bmCurrentPlaylist = bmPlaylistsModel->index(index, 0).data().toInt();
    bmPlModel->setCurrentPlaylist(bmCurrentPlaylist);
}

void MainWindow::on_checkBoxBmBreak_toggled(const bool &checked)
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
        nextSong = "None - Stopping after current song";
    ui->labelBmNext->setText(nextSong);
}

void MainWindow::on_tableViewBmDb_doubleClicked(const QModelIndex &index)
{
    int songId = index.sibling(index.row(), 0).data().toInt();
    bmPlModel->addSong(songId);
}

void MainWindow::on_buttonBmStop_clicked()
{
    bmMediaBackend.stop(false);
}

void MainWindow::on_lineEditBmSearch_returnPressed()
{
    bmDbModel->search(ui->lineEditBmSearch->text());
}

void MainWindow::on_tableViewBmPlaylist_doubleClicked(const QModelIndex &index)
{
    if (bmMediaBackend.state() == MediaBackend::PlayingState)
        bmMediaBackend.stop(false);
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
    bmMediaBackend.setMedia(path);
    bmMediaBackend.play();
    if (kMediaBackend.state() != MediaBackend::PlayingState)
        bmMediaBackend.fadeInImmediate();
    ui->labelBmPlaying->setText(song);
    ui->labelBmNext->setText(nextSong);
    bmPlDelegate->setCurrentSong(index.row());
    bmPlModel->select();
}

void MainWindow::on_buttonBmPause_clicked(const bool &checked)
{
    if (checked)
        bmMediaBackend.pause();
    else
        bmMediaBackend.play();
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

void MainWindow::refreshSongDbCache()
{
    QSqlQuery query;
    query.exec("DETACH DATABASE mem");
    query.exec("ATTACH DATABASE ':memory:' AS mem");
    query.exec("CREATE TABLE mem.dbsongs AS SELECT * FROM main.dbsongs");
}

void MainWindow::on_actionDisplay_Metadata_toggled(const bool &arg1)
{
    ui->tableViewBmDb->setColumnHidden(1, !arg1);
    ui->tableViewBmDb->setColumnHidden(2, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(3, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(4, !arg1);
    settings->bmSetShowMetadata(arg1);
    autosizeBmViews();
}

void MainWindow::on_actionDisplay_Filenames_toggled(const bool &arg1)
{
    ui->tableViewBmDb->setColumnHidden(4, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(5, !arg1);
    settings->bmSetShowFilenames(arg1);
    autosizeBmViews();
}

void MainWindow::on_actionShow_Debug_Log_toggled(const bool &arg1)
{
    debugDialog->setVisible(arg1);
    settings->setLogVisible(arg1);
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
        QFileInfo fi(importFile);
        QString importPath = fi.absoluteDir().path();
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
            else if (QFile(importPath + "/" + files.at(i)).exists())
            {
                reader.setMedia(importPath + "/" + files.at(i).toLocal8Bit());
                QString duration = QString::number(reader.getDuration() / 1000);
                QString artist = reader.getArtist();
                QString title = reader.getTitle();
                QString filename = QFileInfo(files.at(i)).fileName();
                QString path = importPath + "/" + files.at(i);
                QString searchstring = artist + " " + title + " " + filename;
                QString queryString = "INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) VALUES(:artist,:title,:path,:filename,:duration,:searchstring)";
                query.prepare(queryString);
                query.bindValue(":artist", artist);
                query.bindValue(":title", title);
                query.bindValue(":path", path);
                query.bindValue(":filename", filename);
                query.bindValue(":duration", duration);
                query.bindValue(":searchstring", searchstring);
                query.exec();
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
            {
                songIds.push_back(songId);
            }
            else
            {
                songId = bmPlModel->getSongIdByFilePath(importPath + "/" + files.at(i));
                if (songId >= 0)
                {
                    songIds.push_back(songId);
                }
            }

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

void MainWindow::on_actionAbout_triggered()
{
    QString title;
    QString text;
    QString date = QString::fromLocal8Bit(__DATE__) + " " + QString(__TIME__);
    title = "About OpenKJ";
    text = "OpenKJ\n\nVersion: " + QString(OKJ_VERSION_STRING) + " " + QString(OKJ_VERSION_BRANCH) + "\nBuilt: " + date + "\nLicense: GPL v3+";
    QMessageBox::about(this,title,text);
}


void MainWindow::on_pushButtonMplxLeft_toggled(const bool &checked)
{
    if (checked)
        settings->setMplxMode(Multiplex_LeftChannel);
}

void MainWindow::on_pushButtonMplxBoth_toggled(const bool &checked)
{
    if (checked)
        settings->setMplxMode(Multiplex_Normal);
}

void MainWindow::on_pushButtonMplxRight_toggled(const bool &checked)
{
    if (checked)
        settings->setMplxMode(Multiplex_RightChannel);
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    if (!settings->progressiveSearchEnabled())
        return;
    ui->tableViewDB->scrollToTop();
    static QString lastVal;
    if (arg1.trimmed() != lastVal)
    {
        dbModel->search(arg1);
        lastVal = arg1.trimmed();
    }
}

void MainWindow::cdgOffsetChanged(const int &offset)
{
    cdgOffset = offset;
}

void MainWindow::setMultiPlayed()
{
    QModelIndexList indexes = ui->tableViewQueue->selectionModel()->selectedIndexes();
    QModelIndex index;

    foreach(index, indexes) {
        if (index.column() == 0)
        {
            int queueId = index.sibling(index.row(), 0).data().toInt();
            qInfo() << "Selected row: " << index.row() << " queueId: " << queueId;
            qModel->songSetPlayed(queueId);
        }
    }
}

void MainWindow::setMultiUnplayed()
{
    QModelIndexList indexes = ui->tableViewQueue->selectionModel()->selectedIndexes();
    QModelIndex index;

    foreach(index, indexes) {
        if (index.column() == 0)
        {
            int queueId = index.sibling(index.row(), 0).data().toInt();
            qInfo() << "Selected row: " << index.row() << " queueId: " << queueId;
            qModel->songSetPlayed(queueId,false);
        }
    }
}


void MainWindow::on_spinBoxTempo_valueChanged(const int &arg1)
{
    kMediaBackend.setTempo(arg1);
    QTimer::singleShot(20, [&] () {
        ui->spinBoxTempo->findChild<QLineEdit*>()->deselect();
    });
}

void MainWindow::on_actionSongbook_Generator_triggered()
{
    dlgBookCreator->show();
}

void MainWindow::on_actionEqualizer_triggered()
{
    dlgEq->show();
}

void MainWindow::audioError(const QString &msg)
{
    QMessageBox msgBox;
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("Audio playback error! - " + msg);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if (kMediaBackend.state() == MediaBackend::PlayingState)
    {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Are you sure you want to exit?");
        msgBox.setInformativeText("There is currently a karaoke song playing.  If you continue, the current song will be stopped. Are you sure?");
        QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();
        if (msgBox.clickedButton() != yesButton)
        {
            event->ignore();
            return;
        }
    }
    if (!settings->cdgWindowFullscreen())
        settings->saveWindowState(cdgWindow);
    settings->setShowCdgWindow(cdgWindow->isVisible());
    cdgWindow->setVisible(false);
    dlgSongShop->setVisible(false);
    requestsDialog->setVisible(false);
    event->accept();
}

void MainWindow::on_sliderVolume_sliderMoved(const int &position)
{
    kMediaBackend.setVolume(position);
    kMediaBackend.fadeInImmediate();
}

void MainWindow::on_sliderBmVolume_sliderMoved(const int &position)
{
    bmMediaBackend.setVolume(position);
    if (kMediaBackend.state() != MediaBackend::PlayingState)
        bmMediaBackend.fadeInImmediate();
}

void MainWindow::songDropNoSingerSel()
{
    QMessageBox msgBox;
    msgBox.setText("No singer selected.  You must select a singer before you can add songs to a queue.");
    msgBox.exec();
}

void MainWindow::newVersionAvailable(const QString &version)
{
    QMessageBox msgBox;
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("New version of OpenKJ is available: " + version);
    msgBox.setIcon(QMessageBox::Information);
    if (checker->getOS() == "Linux")
    {
        msgBox.setInformativeText("To install the update, please use your distribution's package manager or download and build the current source.");
    }
    if (checker->getOS() == "Win32" || checker->getOS() == "Win64")
    {
        msgBox.setInformativeText("You can download the new version at <a href=https://openkj.org/software>https://openkj.org/software</a>");
    }
    if (checker->getOS() == "MacOS")
    {
        msgBox.setInformativeText("You can download the new version at <a href=https://openkj.org/software>https://openkj.org/software</a>");
    }
    msgBox.exec();
}

void MainWindow::on_pushButtonIncomingRequests_clicked()
{
    requestsDialog->show();
}

void MainWindow::on_pushButtonShop_clicked()
{
    dlgSongShop->show();
    dlgSongShop->setModal(false);
}

void MainWindow::filesDroppedOnQueue(const QList<QUrl> &urls, const int &singerId, const int &position)
{
    foreach (QUrl url, urls)
    {
        QString file = url.toLocalFile();
        if (QFile(file).exists())
        {
            if (file.endsWith(".zip", Qt::CaseInsensitive))
            {
                MzArchive archive(file);
                if (!archive.isValidKaraokeFile())
                {
                    QMessageBox msgBox;
                    msgBox.setWindowTitle("Invalid karoake file!");
                    msgBox.setText("Invalid karaoke file dropped on queue");
                    msgBox.setInformativeText(file);
                    msgBox.exec();
                    continue;
                }
            }
            else if (file.endsWith(".cdg", Qt::CaseInsensitive))
            {
                QString noext = file;
                noext.chop(3);
                QString mp3_1 = noext + "mp3";
                QString mp3_2 = noext + "MP3";
                QString mp3_3 = noext + "Mp3";
                QString mp3_4 = noext + "mP3";
                if (!QFile(mp3_1).exists() && !QFile(mp3_2).exists() && !QFile(mp3_3).exists() && !QFile(mp3_4).exists())
                {
                    QMessageBox msgBox;
                    msgBox.setWindowTitle("Invalid karoake file!");
                    msgBox.setText("CDG file dropped on queue has no matching mp3 file");
                    msgBox.setInformativeText(file);
                    msgBox.exec();
                    continue;
                }
            }
            else if (!file.endsWith(".mp4", Qt::CaseInsensitive) && !file.endsWith(".mkv", Qt::CaseInsensitive) && !file.endsWith(".avi", Qt::CaseInsensitive) && !file.endsWith(".m4v", Qt::CaseInsensitive))
            {
                QMessageBox msgBox;
                msgBox.setWindowTitle("Invalid karoake file!");
                msgBox.setText("Unsupported file type dropped on queue.  Supported file types: mp3+g zip, cdg, mp4, mkv, avi");
                msgBox.setInformativeText(file);
                msgBox.exec();
                continue;
            }
            qInfo() << "Karaoke file dropped. Singer: " << singerId << " Pos: " << position << " Path: " << file;
            int songId = dbDialog->dropFileAdd(file);
            if (songId == -1)
                continue;
            qModel->songInsert(songId, position);
        }
    }
}

void MainWindow::appFontChanged(const QFont &font)
{
    auto smallerFont = font;
    smallerFont.setPointSize(font.pointSize() - 2);
    auto smallerFontBold = smallerFont;
    smallerFontBold.setBold(true);
    ui->labelTotal->setFont(smallerFont);
    ui->labelTotalTime->setFont(smallerFont);
    ui->labelElapsed->setFont(smallerFont);
    ui->labelElapsedTime->setFont(smallerFont);
    ui->labelRemain->setFont(smallerFont);
    ui->labelRemainTime->setFont(smallerFont);
    ui->labelArtistHeader->setFont(smallerFontBold);
    ui->labelArtist->setFont(smallerFont);
    ui->labelTitleHeader->setFont(smallerFontBold);
    ui->labelTitle->setFont(smallerFont);
    ui->labelSingerHeader->setFont(smallerFontBold);
    ui->labelSinger->setFont(smallerFont);

    QApplication::setFont(font, "QWidget");
    setFont(font);
    QFontMetrics fm(settings->applicationFont());
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    int cvwWidth = std::max(300, fm.horizontalAdvance("Total: 00:00  Current:00:00  Remain: 00:00"));
#else
    int cvwWidth = std::max(300, fm.width("Total: 00:00  Current:00:00  Remain: 00:00"));
#endif
    qInfo() << "Resizing videoPreview to width: " << cvwWidth;
//    ui->cdgFrame->setMinimumWidth(cvwWidth);
//    ui->cdgFrame->setMaximumWidth(cvwWidth);
//    ui->mediaFrame->setMinimumWidth(cvwWidth);
//    ui->mediaFrame->setMaximumWidth(cvwWidth);

    QSize mcbSize(fm.height(), fm.height());
    if (mcbSize.width() < 32)
    {
        mcbSize.setWidth(32);
        mcbSize.setHeight(32);
    }
//    ui->buttonStop->resize(mcbSize);
//    ui->buttonPause->resize(mcbSize);
//    ui->buttonStop->setIconSize(mcbSize);
//    ui->buttonPause->setIconSize(mcbSize);
//    ui->buttonStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
//    ui->buttonPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));

    ui->buttonBmStop->resize(mcbSize);
    ui->buttonBmPause->resize(mcbSize);
    ui->buttonBmStop->setIconSize(mcbSize);
    ui->buttonBmPause->setIconSize(mcbSize);
    ui->buttonBmStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->buttonBmPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));

//    ui->pushButton->resize(mcbSize);
//    ui->pushButton->setIcon(QIcon(QPixmap(":/Icons/system-search2.png").scaled(mcbSize)));
//    ui->pushButton->setIconSize(mcbSize);

//    ui->buttonBmSearch->resize(mcbSize);
//    ui->buttonBmSearch->setIcon(QIcon(QPixmap(":/Icons/system-search2.png").scaled(mcbSize)));
//    ui->buttonBmSearch->setIconSize(mcbSize);

//    ui->buttonAddSinger->resize(mcbSize);
//    ui->buttonAddSinger->setIcon(QIcon(QPixmap(":/Icons/breeze-dark/list-add-user.svg").scaled(mcbSize)));
//    ui->buttonAddSinger->setIconSize(mcbSize);

//    ui->buttonClearRotation->resize(mcbSize);
//    ui->buttonClearRotation->setIcon(QIcon(QPixmap(":/Icons/breeze-dark/edit-delete.svg").scaled(mcbSize)));
//    ui->buttonClearRotation->setIconSize(mcbSize);

//    ui->buttonClearQueue->resize(mcbSize);
//    ui->buttonClearQueue->setIcon(QIcon(QPixmap(":/Icons/breeze-dark/edit-delete.svg").scaled(mcbSize)));
//    ui->buttonClearQueue->setIconSize(mcbSize);

//    ui->buttonRegulars->resize(mcbSize);
//    ui->buttonRegulars->setIcon(QIcon(QPixmap(":/Icons/breeze-dark/user-others.svg").scaled(mcbSize)));
//    ui->buttonRegulars->setIconSize(mcbSize);

    autosizeViews();

}

void MainWindow::resizeRotation()
{
    int fH = QFontMetrics(settings->applicationFont()).height();
    int iconWidth = fH + fH;
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    int waitSize = QFontMetrics(settings->applicationFont()).horizontalAdvance("Wait_");
#else
    int waitSize = QFontMetrics(settings->applicationFont()).width("Wait_");
#endif
    if (waitSize > iconWidth)
        iconWidth = waitSize;
    int singerColSize = ui->tableViewRotation->width() - (iconWidth * 3) - 5;
    int songColSize = 0;
    if (!settings->rotationShowNextSong())
    {
        ui->tableViewRotation->hideColumn(2);
    }
    else
    {
        ui->tableViewRotation->showColumn(2);
        QStringList singers = rotModel->singers();
        int largestName = 0;
        for (int i=0; i < singers.size(); i++)
        {
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
            int width = QFontMetrics(settings->applicationFont()).horizontalAdvance("_" + singers.at(i) + "_");
#else
            int width = QFontMetrics(settings->applicationFont()).width("_" + singers.at(i) + "_");
#endif
            if (width > largestName)
                largestName = width;
        }
        singerColSize = largestName;
        songColSize = ui->tableViewRotation->width() - (iconWidth * 3) - singerColSize - 5;
        ui->tableViewRotation->horizontalHeader()->resizeSection(2, songColSize);
    }
    ui->tableViewRotation->horizontalHeader()->resizeSection(0, iconWidth);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableViewRotation->horizontalHeader()->resizeSection(1, singerColSize);
    ui->tableViewRotation->horizontalHeader()->resizeSection(3, iconWidth);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableViewRotation->horizontalHeader()->resizeSection(4, iconWidth);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
}

void MainWindow::autosizeViews()
{
    int fH = QFontMetrics(settings->applicationFont()).height();
    int iconWidth = fH + fH;
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    int durationColSize = QFontMetrics(settings->applicationFont()).horizontalAdvance(" Duration ");
    int songidColSize = QFontMetrics(settings->applicationFont()).horizontalAdvance(" AA0000000-0000 ");
#else
    int durationColSize = QFontMetrics(settings->applicationFont()).width(" Duration ");
    int songidColSize = QFontMetrics(settings->applicationFont()).width(" AA0000000-0000 ");
#endif
    int remainingSpace = ui->tableViewDB->width() - durationColSize - songidColSize;
    int artistColSize = (remainingSpace / 2) - 120;
    int titleColSize = (remainingSpace / 2) + 100;
    ui->tableViewDB->horizontalHeader()->resizeSection(1, artistColSize);
    ui->tableViewDB->horizontalHeader()->resizeSection(2, titleColSize);
    ui->tableViewDB->horizontalHeader()->resizeSection(4, durationColSize);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->tableViewDB->horizontalHeader()->resizeSection(3, songidColSize);

    resizeRotation();
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    int keyColSize = QFontMetrics(settings->applicationFont()).horizontalAdvance("Key") + iconWidth;
#else
    int keyColSize = QFontMetrics(settings->applicationFont()).width("Key") + iconWidth;
#endif
    remainingSpace = ui->tableViewQueue->width() - iconWidth - keyColSize - songidColSize - 16;
    artistColSize = (remainingSpace / 2);
    titleColSize = (remainingSpace / 2);
    ui->tableViewQueue->horizontalHeader()->resizeSection(3, artistColSize);
    ui->tableViewQueue->horizontalHeader()->resizeSection(4, titleColSize);
    ui->tableViewQueue->horizontalHeader()->resizeSection(5, songidColSize);
    ui->tableViewQueue->horizontalHeader()->resizeSection(8, iconWidth);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    ui->tableViewQueue->horizontalHeader()->resizeSection(7, keyColSize);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
}

void MainWindow::autosizeBmViews()
{

    int fH = QFontMetrics(settings->applicationFont()).height();
    int iconWidth = fH + fH;
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    int durationColSize = QFontMetrics(settings->applicationFont()).horizontalAdvance("Duration") + fH;
#else
    int durationColSize = QFontMetrics(settings->applicationFont()).width("Duration") + fH;
#endif
    // 4 = filename 1 = metadata artist 2 = medatada title

    int artistColSize = 0;
    int titleColSize = 0;
    int fnameColSize = 0;
    int remainingSpace = ui->tableViewBmDb->width() - durationColSize - 5;
    if (settings->bmShowMetadata() && settings->bmShowFilenames())
    {
        artistColSize = (float)remainingSpace * .25;
        titleColSize = (float)remainingSpace * .25;
        fnameColSize = (float)remainingSpace * .5;
    }
    else if (settings->bmShowMetadata())
    {
        artistColSize = remainingSpace * .5;
        titleColSize = remainingSpace * .5;
    }
    else if (settings->bmShowFilenames())
    {
        fnameColSize = remainingSpace;
    }
    ui->tableViewBmDb->horizontalHeader()->resizeSection(1, artistColSize);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(2, titleColSize);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(4, fnameColSize);
    ui->tableViewBmDb->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(5, durationColSize);

    remainingSpace = ui->tableViewBmPlaylist->width() - durationColSize - (iconWidth * 2) - 5;
    //5=filename  6=Duration 3=artist 4=title
    if (settings->bmShowMetadata() && settings->bmShowFilenames())
    {
        artistColSize = (float)remainingSpace * .25;
        titleColSize = (float)remainingSpace * .25;
        fnameColSize = (float)remainingSpace * .5;
    }
    else if (settings->bmShowMetadata())
    {
        artistColSize = remainingSpace * .5;
        titleColSize = remainingSpace * .5;
    }
    else if (settings->bmShowFilenames())
    {
        fnameColSize = remainingSpace;
    }
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(3, artistColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(4, titleColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(5, fnameColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(6, durationColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Fixed);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(2, iconWidth);
    ui->tableViewBmPlaylist->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(7, iconWidth);
}


void MainWindow::resizeEvent(QResizeEvent *event)
{

    QMainWindow::resizeEvent(event);
    autosizeViews();
    autosizeBmViews();
    if (ui->tabWidget->currentIndex() == 0)
    {
        autosizeViews();
        bNeedAutoSize = true;
        kNeedAutoSize = false;
    }
    if (ui->tabWidget->currentIndex() == 1)
    {
        autosizeBmViews();
        bNeedAutoSize = false;
        kNeedAutoSize = true;
    }
}

void MainWindow::on_tabWidget_currentChanged(const int &index)
{
    if (bNeedAutoSize && index == 1)
    {
        autosizeBmViews();
        bNeedAutoSize = false;
    }
    if (kNeedAutoSize && index == 0)
    {
        autosizeViews();
        kNeedAutoSize = false;
    }
}

void MainWindow::databaseAboutToUpdate()
{
//    this->requestsDialog->databaseAboutToUpdate();
//    dbModel->revertAll();
//    dbModel->setTable("");
//    ui->tableViewRotation->clearSelection();
//    qModel->setSinger(-1);
}

void MainWindow::bmDatabaseAboutToUpdate()
{
    bmDbModel->revertAll();
    bmDbModel->setTable("");
    bmPlaylistsModel->revertAll();
    bmPlaylistsModel->setTable("");
    dbModel->revertAll();
    dbModel->setTable("");
}

void MainWindow::scutSearchActivated()
{
    ui->lineEdit->setFocus();
}

void MainWindow::bmSongMoved(const int &oldPos, const int &newPos)
{
    QString nextSong;
    if (oldPos < bmCurrentPosition && newPos >= bmCurrentPosition)
        bmCurrentPosition--;
    else if (oldPos > bmCurrentPosition && newPos <= bmCurrentPosition)
        bmCurrentPosition++;
    else if (oldPos == bmCurrentPosition)
        bmCurrentPosition = newPos;
    bmPlDelegate->setCurrentSong(bmCurrentPosition);
    if (!ui->checkBoxBmBreak->isChecked())
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

void MainWindow::on_sliderBmPosition_sliderPressed()
{
    qInfo() << "BM slider down";
    sliderBmPositionPressed = true;
}

void MainWindow::on_sliderBmPosition_sliderReleased()
{
    bmMediaBackend.setPosition(ui->sliderBmPosition->value());
    sliderBmPositionPressed = false;
    qInfo() << "BM slider up.  Position:" << ui->sliderBmPosition->value();
}

void MainWindow::sfxButtonPressed()
{
    SoundFxButton *btn = (SoundFxButton*)sender();
    qInfo() << "SfxButton pressed: " << btn->buttonData();
    sfxMediaBackend.setMedia(btn->buttonData().toString());
    sfxMediaBackend.setVolume(ui->sliderVolume->value());
    sfxMediaBackend.play();
}

void MainWindow::on_btnAddSfx_clicked()
{
//    QString name = "a button";
//    QString path = "a path";
    QString path = QFileDialog::getOpenFileName(this,QString("Select audio file"), QStandardPaths::writableLocation(QStandardPaths::MusicLocation), QString("Audio (*.mp3 *.ogg *.wav *.wma)"));
    if (path != "")
    {
        bool ok;
        QString name = QInputDialog::getText(this, tr("Button Text"), tr("Enter button text:"), QLineEdit::Normal, QString(), &ok);
        if (!ok || name.isEmpty())
            return;
        SfxEntry entry;
        entry.name = name;
        entry.path = path;
        settings->addSfxEntry(entry);
        addSfxButton(path, name);
    }

}

void MainWindow::on_btnSfxStop_clicked()
{
    sfxMediaBackend.stop(true);
}

void MainWindow::removeSfxButton()
{
    SfxEntryList entries = settings->getSfxEntries();
    SfxEntryList newEntries;
    foreach (SfxEntry entry, entries) {
       if (entry.name == lastRtClickedSfxBtn.name && entry.path == lastRtClickedSfxBtn.path)
           continue;
       newEntries.push_back(entry);
    }
    settings->setSfxEntries(newEntries);
    refreshSfxButtons();
}

void MainWindow::showAlert(const QString &title, const QString &message)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
   // msgBox.setInformativeText(file);
    msgBox.exec();
}

void MainWindow::tableViewRotationCurrentChanged(const QModelIndex &cur, const QModelIndex &prev)
{
    Q_UNUSED(prev)
    qModel->setSinger(cur.sibling(cur.row(),0).data().toInt());
    ui->gbxQueue->setTitle(QString("Song Queue - " + rotModel->getSingerName(cur.sibling(cur.row(),0).data().toInt())));
}

void MainWindow::updateRotationDuration()
{
    QString text;
    int secs = rotModel->rotationDuration();
    if (secs > 0)
    {
        int hours = 0;
        int minutes = secs / 60;
        int seconds = secs % 60;
        if (seconds > 0)
            minutes++;
        if (minutes > 60)
        {
            hours = minutes / 60;
            minutes = minutes % 60;
            if (hours > 1)
                text = " Rotation Duration: " + QString::number(hours) + " hours " + QString::number(minutes) + " min";
            else
                text = " Rotation Duration: " + QString::number(hours) + " hour " + QString::number(minutes) + " min";
        }
        else
            text = " Rotation Duration: " + QString::number(minutes) + " min";
    }
    else
        text = " Rotation Duration: 0 min";
    labelRotationDuration.setText(text);

    // Safety check to make sure break music video is muted if karaoke is playing
    if (bmMediaBackend.videoMuted() && kMediaBackend.state() != MediaBackend::PlayingState && kMediaBackend.state() != MediaBackend::PausedState)
        bmMediaBackend.videoMute(false);
}

void MainWindow::on_btnToggleCdgWindow_clicked()
{
    if (cdgWindow->isVisible())
    {
        cdgWindow->hide();
        ui->btnToggleCdgWindow->setText("Show CDG Window");
    }
    else
    {
        cdgWindow->show();
        ui->btnToggleCdgWindow->setText("Hide CDG Window");
    }
}

void MainWindow::cdgVisibilityChanged()
{
    if (cdgWindow->isVisible() && ui->btnToggleCdgWindow->text() == "Show CDG Window")
        ui->btnToggleCdgWindow->setText("Hide CDG Window");
    else if (cdgWindow->isHidden() && ui->btnToggleCdgWindow->text() == "Hide CDG Window")
        ui->btnToggleCdgWindow->setText("Show CDG Window");
}

void MainWindow::rotationSelectionChanged(const QItemSelection &sel, const QItemSelection &desel)
{
    if (sel.size() == 0 && desel.size() > 0)
    {
        qInfo() << "Rotation Selection Cleared!";
        qModel->setSinger(-1);
        ui->tableViewRotation->reset();
        ui->gbxQueue->setTitle("Song Queue - (No Singer Selected)");
    }

    qInfo() << "Rotation Selection Changed";

}

void MainWindow::on_lineEditBmSearch_textChanged(const QString &arg1)
{
    if (!settings->progressiveSearchEnabled())
        return;
    static QString lastVal;
    if (arg1.trimmed() != lastVal)
    {
        bmDbModel->search(arg1);
        lastVal = arg1.trimmed();
    }
}


void MainWindow::on_btnRotTop_clicked()
{
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    rotModel->singerMove(curPos, 0);
    ui->tableViewRotation->selectRow(0);
    rotationDataChanged();
}

void MainWindow::on_btnRotUp_clicked()
{
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    rotModel->singerMove(curPos, curPos - 1);
    ui->tableViewRotation->selectRow(curPos - 1);
    rotationDataChanged();
}

void MainWindow::on_btnRotDown_clicked()
{
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == rotModel->singerCount - 1)
        return;
    rotModel->singerMove(curPos, curPos + 1);
    ui->tableViewRotation->selectRow(curPos + 1);
    rotationDataChanged();
}

void MainWindow::on_btnRotBottom_clicked()
{
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == rotModel->singerCount - 1)
        return;
    rotModel->singerMove(curPos, rotModel->singerCount - 1);
    ui->tableViewRotation->selectRow(rotModel->singerCount - 1);
    rotationDataChanged();
}

void MainWindow::on_btnQTop_clicked()
{
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    qModel->songMove(curPos, 0);
    ui->tableViewQueue->selectRow(0);
    rotationDataChanged();
}

void MainWindow::on_btnQUp_clicked()
{
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    qModel->songMove(curPos, curPos - 1);
    ui->tableViewQueue->selectRow(curPos - 1);
    rotationDataChanged();
}

void MainWindow::on_btnQDown_clicked()
{
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == ui->tableViewQueue->model()->rowCount() - 1)
        return;
    qModel->songMove(curPos, curPos + 1);
    ui->tableViewQueue->selectRow(curPos + 1);
    rotationDataChanged();
}

void MainWindow::on_btnQBottom_clicked()
{
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == ui->tableViewQueue->model()->rowCount() - 1)
        return;
    qModel->songMove(curPos, ui->tableViewQueue->model()->rowCount() - 1);
    ui->tableViewQueue->selectRow(ui->tableViewQueue->model()->rowCount() - 1);
    rotationDataChanged();
}

void MainWindow::on_btnBmPlRandomize_clicked()
{
    qint32 newplayinpos = bmPlModel->randomizePlaylist(bmCurrentPosition);
    bmCurrentPosition = newplayinpos;
    bmPlDelegate->setCurrentSong(bmCurrentPosition);
}



void MainWindow::on_btnPlTop_clicked()
{
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
   bmPlModel->moveSong(curPos, 0);
   ui->tableViewBmPlaylist->selectRow(0);
}

void MainWindow::on_btnPlUp_clicked()
{
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
   bmPlModel->moveSong(curPos, curPos - 1);
   ui->tableViewBmPlaylist->selectRow(curPos - 1);
}

void MainWindow::on_btnPlDown_clicked()
{
    int maxpos = ui->tableViewBmPlaylist->model()->rowCount() - 1;
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == maxpos)
        return;
   bmPlModel->moveSong(curPos, curPos + 1);
   ui->tableViewBmPlaylist->selectRow(curPos + 1);
}

void MainWindow::on_btnPlBottom_clicked()
{
    int maxpos = ui->tableViewBmPlaylist->model()->rowCount() - 1;
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == maxpos)
        return;
   bmPlModel->moveSong(curPos, maxpos);
   ui->tableViewBmPlaylist->selectRow(maxpos);
}

void MainWindow::on_actionSound_Clips_triggered(const bool &checked)
{
    if (checked)
    {
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
    else
    {
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Expanding);
    }
    ui->groupBoxSoundClips->setVisible(checked);
    settings->setShowMainWindowSoundClips(checked);
}

void MainWindow::on_actionNow_Playing_triggered(const bool &checked)
{
    ui->groupBoxNowPlaying->setVisible(checked);
    settings->setShowMainWindowNowPlaying(checked);
}

void MainWindow::on_actionVideoSmall_triggered()
{
    ui->videoPreview->setMinimumSize(QSize(256,144));
    ui->videoPreview->setMaximumSize(QSize(256,144));
    ui->mediaFrame->setMaximumWidth(300);
    ui->mediaFrame->setMinimumWidth(300);
    settings->setMainWindowVideoSize(Settings::Small);
    QTimer::singleShot(15, [&] () {autosizeViews();});
}

void MainWindow::on_actionVideoMedium_triggered()
{
    ui->videoPreview->setMinimumSize(QSize(384,216));
    ui->videoPreview->setMaximumSize(QSize(384,216));
    ui->mediaFrame->setMaximumWidth(430);
    ui->mediaFrame->setMinimumWidth(430);
    settings->setMainWindowVideoSize(Settings::Medium);
    QTimer::singleShot(15, [&] () {autosizeViews();});
}

void MainWindow::on_actionVideoLarge_triggered()
{
    ui->videoPreview->setMinimumSize(QSize(512,288));
    ui->videoPreview->setMaximumSize(QSize(512,288));
    ui->mediaFrame->setMaximumWidth(560);
    ui->mediaFrame->setMinimumWidth(560);
    settings->setMainWindowVideoSize(Settings::Large);
    QTimer::singleShot(15, [&] () {autosizeViews();});
}

void MainWindow::on_actionVideo_Output_2_triggered(const bool &checked)
{
    ui->videoPreview->setVisible(checked);
    settings->setShowMainWindowVideo(checked);
}

void MainWindow::on_actionKaraoke_torture_triggered()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
    connect(&m_timerTest, &QTimer::timeout, [&] () {
        QApplication::beep();
        static int runs = 0;
       qInfo() << "Karaoke torture test timer timeout";
       qInfo() << "num songs in db: " << dbModel->rowCount();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       int randno = QRandomGenerator::global()->bounded(0, dbModel->rowCount() - 1);
       qInfo() << "randno: " << randno;
       ui->tableViewDB->selectRow(randno);
       ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
       play(ui->tableViewDB->selectionModel()->selectedRows(5).at(0).data().toString(), true);
       ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
//       QSqlQuery query;
//       query.prepare("SELECT songid,artist,title,discid,path FROM dbsongs WHERE songid >= (abs(random()) % (SELECT max(songid) FROM dbsongs))LIMIT 1");
//       query.exec();
//       if (!query.next())
//           qInfo() << "Unable to find song in db!";
//       auto artist = query.value("artist").toString();
//       auto title = query.value("title").toString();
//       auto songId = query.value("discid").toString();
//       auto path = query.value("path").toString();
//       qInfo() << "Torture test playing: " << path;
    });
    m_timerTest.start(3000);
#endif
}

void MainWindow::on_actionK_B_torture_triggered()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
    connect(&m_timerTest, &QTimer::timeout, [&] () {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        if (playing)
        {
            on_buttonStop_clicked();
            playing = false;
            ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
            return;
        }
       qInfo() << "Karaoke torture test timer timeout";
       qInfo() << "num songs in db: " << dbModel->rowCount();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       ui->tableViewDB->scrollToBottom();
       int randno = QRandomGenerator::global()->bounded(0, dbModel->rowCount() - 1);
       qInfo() << "randno: " << randno;
       ui->tableViewDB->selectRow(randno);
       ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
       play(ui->tableViewDB->selectionModel()->selectedRows(5).at(0).data().toString(), false);
       ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
       playing = true;
//       QSqlQuery query;
//       query.prepare("SELECT songid,artist,title,discid,path FROM dbsongs WHERE songid >= (abs(random()) % (SELECT max(songid) FROM dbsongs))LIMIT 1");
//       query.exec();
//       if (!query.next())
//           qInfo() << "Unable to find song in db!";
//       auto artist = query.value("artist").toString();
//       auto title = query.value("title").toString();
//       auto songId = query.value("discid").toString();
//       auto path = query.value("path").toString();
//       qInfo() << "Torture test playing: " << path;
    });
    m_timerTest.start(2000);
#endif
}

void MainWindow::on_actionBurn_in_triggered()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
    m_testMode = true;
    emit ui->buttonClearRotation->clicked();
    for (auto i=0; i<21; i++)
    {
        auto singerName = "Test Singer " + QString::number(i);
        rotModel->singerAdd(singerName);
        rotModel->regularDelete(singerName);
    }
    for (auto i=0; i<11; i++)
    {
        ui->tableViewRotation->selectRow(i);
        emit ui->tableViewRotation->clicked(ui->tableViewRotation->selectionModel()->selectedRows(3).at(0));
    }
    connect(&m_timerTest, &QTimer::timeout, [&] () {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        if (playing)
        {
            on_buttonStop_clicked();
            playing = false;
            ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
            return;
        }

        rotModel->singerMove(QRandomGenerator::global()->bounded(0, 19), QRandomGenerator::global()->bounded(0, 19));
        ui->tableViewRotation->selectRow(QRandomGenerator::global()->bounded(0, 19));

        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        int randno = QRandomGenerator::global()->bounded(0, dbModel->rowCount() - 1);
        qInfo() << "randno: " << randno;
        ui->tableViewDB->selectRow(randno);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        emit ui->tableViewDB->doubleClicked(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        ui->tableViewQueue->selectRow(qModel->rowCount() - 1);
        ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        if (qModel->rowCount() > 2)
        {
            auto newPos = QRandomGenerator::global()->bounded(0,qModel->rowCount() - 1);
            qModel->songMove(qModel->rowCount() - 1, newPos);
            ui->tableViewQueue->selectRow(newPos);
            ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        }
        auto idx = ui->tableViewQueue->selectionModel()->selectedRows().at(0);
        emit ui->tableViewQueue->doubleClicked(idx);
        ui->tableViewQueue->selectRow(idx.row());
        playing = true;
        qInfo() << "Burn in test cycle: " << ++runs;
    });
    m_timerTest.start(12000);
#endif
}

void MainWindow::on_actionMultiplex_Controls_triggered(bool checked)
{
    ui->widgetMplxControls->setVisible(checked);
    settings->setShowMplxControls(checked);
}

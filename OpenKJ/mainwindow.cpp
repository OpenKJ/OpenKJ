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
#include "audiobackendgstreamer.h"
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QImageReader>
#include "okarchive.h"
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

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

Settings *settings;
OKJSongbookAPI *songbookApi;
int remainSecs = 240;



QString MainWindow::GetRandomString() const
{
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   const int randomStringLength = 12; // assuming you want random strings of 12 characters

   QString randomString;
   for(int i=0; i<randomStringLength; ++i)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}

void MainWindow::addSfxButton(QString filename, QString label, bool reset)
{
    static int numButtons = 0;
    if (reset)
        numButtons = 0;
    //numButtons = ui->sfxButtonGrid->children().count();
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
    connect(button, SIGNAL(clicked(bool)), this, SLOT(sfxButtonPressed()));
    connect(button, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(sfxButton_customContextMenuRequested(QPoint)));
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
        logContents->append(QString("DEBG: " + localMsg + " (" + context.function + ")"));
        break;
    case QtInfoMsg:
        fprintf(stderr, "INFO: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "INFO: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString("INFO: " + localMsg + " (" + context.function + ")"));
        break;
    case QtWarningMsg:
        fprintf(stderr, "WARN: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "WARN: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString("WARN: " + localMsg + " (" + context.function + ")"));
        break;
    case QtCriticalMsg:
        fprintf(stderr, "CRIT: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "CRIT: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString("CRIT: " + localMsg + " (" + context.function + ")"));
        break;
    case QtFatalMsg:
        fprintf(stderr, "FATAL!!: %s (%s)\n", localMsg.constData(), context.function);
        if (loggingEnabled) logStream << "FATAL!!: " << localMsg << " (" << context.function << ")\n";
        logContents->append(QString("FATAL!!: " + localMsg + " (" + context.function + ")"));
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
//    QString logDir = settings->logDir();
//    QDir dir;
//    QString logFilePath;
//    QString filename = "openkj-debug-" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm") + "-log";
//    dir.mkpath(logDir);
//  //  m_filename = filename;
//#ifdef Q_OS_WIN
//    logFilePath = logDir + QDir::separator() + filename;
//#else
//    logFilePath = logDir + QDir::separator() + filename;
//#endif
    logFile = new QFile();
//    logFile->open(QFile::WriteOnly);
    qInstallMessageHandler(myMessageOutput);
    kNeedAutoSize = false;
    bNeedAutoSize = true;
    _singular = new QSharedMemory("SharedMemorySingleInstanceProtectorOpenKJ", this);
    shop = new SongShop(this);
    blinkRequestsBtn = false;
    kAASkip = false;
    kAANextSinger = -1;
    kAANextSongPath = "";
    m_lastAudioState = AbstractAudioBackend::StoppedState;
    sliderPositionPressed = false;
    sliderBmPositionPressed = false;
    m_rtClickQueueSongId = -1;
    m_rtClickRotationSingerId = -1;
    k2kTransition = false;
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");
    ui->setupUi(this);
    ui->actionShow_Debug_Log->setChecked(settings->logShow());
#ifdef Q_OS_WIN
    ui->sliderBmPosition->setMaximumHeight(12);
    ui->sliderBmVolume->setMaximumWidth(12);
    ui->sliderProgress->setMaximumHeight(12);
    ui->sliderVolume->setMaximumWidth(12);
#endif
    db = new KhDb(this);
    labelSingerCount = new QLabel(ui->statusBar);
    labelRotationDuration = new QLabel(ui->statusBar);
    khDir = new QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!khDir->exists())
    {
        khDir->mkpath(khDir->absolutePath());
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
    database.setDatabaseName(khDir->absolutePath() + QDir::separator() + "openkj.sqlite");
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

    sortColDB = 1;
    sortDirDB = 0;
    qInfo() << "Creating dbModel";
    dbModel = new DbTableModel(this, database);
    dbModel->select();
    qInfo() << "Creating queueModel";
    qModel = new QueueModel(this, database);
    qModel->select();
    qInfo() << "Creating qDelegate";
    qDelegate = new QueueItemDelegate(this);
    qInfo() << "creating rotModel";
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
    qInfo() << "Creating dbDialog";
    dbDialog = new DlgDatabase(database, this);
    qInfo() << "Creating dlgKeyChange";
    dlgKeyChange = new DlgKeyChange(qModel, this);
    qInfo() << "Creating regularSingersDialog";
    regularSingersDialog = new DlgRegularSingers(rotModel, this);
    qInfo() << "Creating regularExportDialog";
    regularExportDialog = new DlgRegularExport(rotModel, this);
    qInfo() << "Creating regularImportDialog";
    regularImportDialog = new DlgRegularImport(rotModel, this);
    qInfo() << "Creating requestsDialog";
    requestsDialog = new DlgRequests(rotModel);
    requestsDialog->setModal(false);
    qInfo() << "Creating dlgBookCreator";
    dlgBookCreator = new DlgBookCreator(this);
    qInfo() << "Creating dlgEq";
    dlgEq = new DlgEq(this);
    qInfo() << "Creating dlgAddSinger";
    dlgAddSinger = new DlgAddSinger(rotModel, this);
    qInfo() << "Creating dlgSongShop";
    dlgSongShop = new DlgSongShop(shop);
    dlgSongShop->setModal(false);
    qInfo() << "Creating CDG object";
    cdg = new CDG;
    ui->tableViewDB->setModel(dbModel);
    ui->tableViewDB->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewDB));
    qInfo() << "Creating dbDelegate";
    dbDelegate = new DbItemDelegate(this);
    ui->tableViewDB->setItemDelegate(dbDelegate);
//    ipcClient = new KhIPCClient("bmControl",this);
    qInfo() << "Creating break music audio backend object";
    bmAudioBackend = new AudioBackendGstreamer(false, this, "BM");
    bmAudioBackend->setName("break");
    qInfo() << "Creating karaoke audio backend object";
    kAudioBackend = new AudioBackendGstreamer(true, this, "KA");
    kAudioBackend->setName("karaoke");
    if (kAudioBackend->canFade())
        kAudioBackend->setUseFader(settings->audioUseFader());
    if (!kAudioBackend->canPitchShift())
    {
        ui->spinBoxKey->hide();
        ui->lblKey->hide();
        ui->tableViewQueue->hideColumn(7);
    }
    if (!kAudioBackend->canChangeTempo())
    {
        ui->spinBoxTempo->hide();
        ui->lblTempo->hide();
    }
    sfxAudioBackend = new AudioBackendGstreamer(false, this, "SFX");
    qInfo() << "Creating audio recorder object";
    audioRecorder = new AudioRecorder(this);
    qInfo() << "Creating settings dialog";
    settingsDialog = new DlgSettings(kAudioBackend, bmAudioBackend, this);
    qInfo() << "Creating CDG output dialog";
    cdgWindow = new DlgCdg(kAudioBackend, bmAudioBackend, 0, Qt::Window);
    connect(rotModel, SIGNAL(songDroppedOnSinger(int,int,int)), this, SLOT(songDroppedOnSinger(int,int,int)));
    connect(kAudioBackend, SIGNAL(volumeChanged(int)), ui->sliderVolume, SLOT(setValue(int)));
    connect(dbDialog, SIGNAL(databaseUpdateComplete()), this, SLOT(databaseUpdated()));
    connect(dbDialog, SIGNAL(databaseAboutToUpdate()), this, SLOT(databaseAboutToUpdate()));
    connect(dbDialog, SIGNAL(databaseSongAdded()), dbModel, SLOT(select()));
    connect(dbDialog, SIGNAL(databaseSongAdded()), requestsDialog, SLOT(databaseSongAdded()));
    connect(dbDialog, SIGNAL(databaseCleared()), this, SLOT(databaseCleared()));
    connect(dbDialog, SIGNAL(databaseCleared()), regularSingersDialog, SLOT(regularsChanged()));
    connect(kAudioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(audioBackend_positionChanged(qint64)));
    connect(kAudioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(audioBackend_durationChanged(qint64)));
    connect(kAudioBackend, SIGNAL(stateChanged(AbstractAudioBackend::State)), this, SLOT(audioBackend_stateChanged(AbstractAudioBackend::State)));
    connect(kAudioBackend, SIGNAL(pitchChanged(int)), ui->spinBoxKey, SLOT(setValue(int)));
    connect(sfxAudioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(sfxAudioBackend_positionChanged(qint64)));
    connect(sfxAudioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(sfxAudioBackend_durationChanged(qint64)));
    connect(sfxAudioBackend, SIGNAL(stateChanged(AbstractAudioBackend::State)), this, SLOT(sfxAudioBackend_stateChanged(AbstractAudioBackend::State)));
    connect(rotModel, SIGNAL(rotationModified()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(tickerOutputModeChanged()), this, SLOT(rotationDataChanged()));
    connect(settings, SIGNAL(audioBackendChanged(int)), this, SLOT(audioBackendChanged(int)));
    connect(settings, SIGNAL(cdgBgImageChanged()), this, SLOT(onBgImageChange()));
    connect(kAudioBackend, SIGNAL(audioError(QString)), this, SLOT(audioError(QString)));
    connect(bmAudioBackend, SIGNAL(audioError(QString)), this, SLOT(audioError(QString)));
    connect(kAudioBackend, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()));
    connect(bmAudioBackend, SIGNAL(silenceDetected()), this, SLOT(silenceDetectedBm()));
    connect(settingsDialog, SIGNAL(audioUseFaderChanged(bool)), kAudioBackend, SLOT(setUseFader(bool)));
    connect(shop, SIGNAL(karaokeSongDownloaded(QString)), dbDialog, SLOT(singleSongAdd(QString)));
    kAudioBackend->setUseFader(settings->audioUseFader());
    connect(settingsDialog, SIGNAL(audioSilenceDetectChanged(bool)), kAudioBackend, SLOT(setUseSilenceDetection(bool)));
    kAudioBackend->setUseSilenceDetection(settings->audioDetectSilence());

    connect(settingsDialog, SIGNAL(audioUseFaderChangedBm(bool)), bmAudioBackend, SLOT(setUseFader(bool)));
    bmAudioBackend->setUseFader(settings->audioUseFaderBm());
    connect(settingsDialog, SIGNAL(audioSilenceDetectChangedBm(bool)), bmAudioBackend, SLOT(setUseSilenceDetection(bool)));
    bmAudioBackend->setUseSilenceDetection(settings->audioDetectSilenceBm());

    connect(settingsDialog, SIGNAL(audioDownmixChanged(bool)), kAudioBackend, SLOT(setDownmix(bool)));
    connect(settingsDialog, SIGNAL(audioDownmixChangedBm(bool)), bmAudioBackend, SLOT(setDownmix(bool)));
    kAudioBackend->setDownmix(settings->audioDownmix());
    bmAudioBackend->setDownmix(settings->audioDownmixBm());
    connect(qModel, SIGNAL(queueModified(int)), rotModel, SLOT(queueModified(int)));
    connect(requestsDialog, SIGNAL(addRequestSong(int,int,int)), qModel, SLOT(songAdd(int,int,int)));
    connect(settings, SIGNAL(tickerCustomStringChanged()), this, SLOT(rotationDataChanged()));
    qInfo() << "Setting backgrounds on CDG displays";
    ui->cdgVideoWidget->setKeepAspect(true);
    cdgWindow->setShowBgImage(true);
    setShowBgImage(true);
    qInfo() << "Restoring window and listview states";
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
    if ((settings->cdgWindowFullscreen()) && (settings->showCdgWindow()))
    {
        qInfo() << "Making CDG window fullscreen";
        cdgWindow->makeFullscreen();
    }
    qInfo() << "Refreshing rotation data";
    rotationDataChanged();
    qInfo() << "Adjusting karaoke listviews column visibility and state";
    ui->tableViewDB->hideColumn(0);
    ui->tableViewDB->hideColumn(5);
    ui->tableViewDB->hideColumn(6);
    ui->tableViewDB->hideColumn(7);
//    ui->tableViewDB->horizontalHeader()->resizeSection(4,75);
    ui->tableViewQueue->hideColumn(0);
    ui->tableViewQueue->hideColumn(1);
    ui->tableViewQueue->hideColumn(2);
    ui->tableViewQueue->hideColumn(6);
    ui->tableViewQueue->hideColumn(9);
    ui->tableViewQueue->hideColumn(10);
    ui->tableViewQueue->hideColumn(11);
    ui->tableViewQueue->hideColumn(12);
    if (!kAudioBackend->canPitchShift())
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
    qInfo() << "Adding singer count to status bar";
    ui->statusBar->addWidget(labelSingerCount);
    ui->statusBar->addWidget(labelRotationDuration);



    bmCurrentPosition = 0;
    bmAudioBackend->setUseFader(true);
    bmAudioBackend->setUseSilenceDetection(true);

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
//    settings->restoreColumnWidths(ui->tableViewBmDb);
//    settings->restoreColumnWidths(ui->tableViewBmPlaylist);
    ui->tableViewBmDb->setColumnHidden(0, true);
    ui->tableViewBmDb->setColumnHidden(3, true);
    ui->tableViewBmDb->setColumnHidden(6, true);

    ui->tableViewBmPlaylist->setColumnHidden(0, true);
    ui->tableViewBmPlaylist->setColumnHidden(1, true);

    settings->restoreSplitterState(ui->splitter_3);

//    if (settings->bmShowFilenames())
//    {
//        ui->tableViewBmDb->showColumn(4);
//        ui->actionDisplay_Filenames->setChecked(true);
//    }
//    else
//    {
//        ui->tableViewBmDb->hideColumn(4);
//        ui->actionDisplay_Filenames->setChecked(false);
//    }
//    if (settings->bmShowMetadata())
//    {
//        ui->tableViewBmDb->showColumn(1);
//        ui->tableViewBmDb->showColumn(2);
//        ui->actionDisplay_Filenames->setChecked(true);
//    }
//    else
//    {
//        ui->tableViewBmDb->hideColumn(1);
//        ui->tableViewBmDb->hideColumn(2);
//        ui->actionDisplay_Filenames->setChecked(false);
//    }

    on_actionDisplay_Filenames_toggled(settings->bmShowFilenames());
    on_actionDisplay_Metadata_toggled(settings->bmShowMetadata());


    qInfo() << "Connecting signals & slots";
//    connect(ui->lineEditBmSearch, SIGNAL(textChanged(QString)), bmDbModel, SLOT(search(QString)));
    connect(bmAudioBackend, SIGNAL(stateChanged(AbstractAudioBackend::State)), this, SLOT(bmMediaStateChanged(AbstractAudioBackend::State)));
    connect(bmAudioBackend, SIGNAL(positionChanged(qint64)), this, SLOT(bmMediaPositionChanged(qint64)));
    connect(bmAudioBackend, SIGNAL(durationChanged(qint64)), this, SLOT(bmMediaDurationChanged(qint64)));
    connect(bmAudioBackend, SIGNAL(volumeChanged(int)), ui->sliderBmVolume, SLOT(setValue(int)));
    connect(bmDbDialog, SIGNAL(bmDbUpdated()), this, SLOT(bmDbUpdated()));
    connect(bmDbDialog, SIGNAL(bmDbCleared()), this, SLOT(bmDbCleared()));
    connect(bmDbDialog, SIGNAL(bmDbAboutToUpdate()), this, SLOT(bmDatabaseAboutToUpdate()));
    connect(bmAudioBackend, SIGNAL(newVideoFrame(QImage, QString)), this, SLOT(videoFrameReceived(QImage, QString)));
    connect(kAudioBackend, SIGNAL(newVideoFrame(QImage,QString)), this, SLOT(videoFrameReceived(QImage,QString)));
    qInfo() << "Setting up volume sliders";
    ui->sliderBmVolume->setValue(initialBMVol);
    bmAudioBackend->setVolume(initialBMVol);
    ui->sliderVolume->setValue(initialKVol);
    kAudioBackend->setVolume(initialKVol);
    qInfo() << "Restoring multiplex button states";
    if (settings->mplxMode() == Multiplex_Normal)
        ui->pushButtonMplxBoth->setChecked(true);
    else if (settings->mplxMode() == Multiplex_LeftChannel)
        ui->pushButtonMplxLeft->setChecked(true);
    else if (settings->mplxMode() == Multiplex_RightChannel)
        ui->pushButtonMplxRight->setChecked(true);
    qInfo() << "Creating karaoke autoplay timer";
    karaokeAATimer = new QTimer(this);
    connect(karaokeAATimer, SIGNAL(timeout()), this, SLOT(karaokeAATimerTimeout()));
    ui->actionAutoplay_mode->setChecked(settings->karaokeAutoAdvance());
    connect(ui->actionAutoplay_mode, SIGNAL(toggled(bool)), settings, SLOT(setKaraokeAutoAdvance(bool)));
    connect(settings, SIGNAL(karaokeAutoAdvanceChanged(bool)), ui->actionAutoplay_mode, SLOT(setChecked(bool)));

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
            bmAudioBackend->setMedia(path);
            bmAudioBackend->play();
            bmAudioBackend->setVolume(ui->sliderBmVolume->value());
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
    connect(settings, SIGNAL(cdgDisplayOffsetChanged(int)), this, SLOT(cdgOffsetChanged(int)));

    connect(settings, SIGNAL(eqBBypassChanged(bool)), bmAudioBackend, SLOT(setEqBypass(bool)));
    connect(settings, SIGNAL(eqBLevel1Changed(int)), bmAudioBackend, SLOT(setEqLevel1(int)));
    connect(settings, SIGNAL(eqBLevel2Changed(int)), bmAudioBackend, SLOT(setEqLevel2(int)));
    connect(settings, SIGNAL(eqBLevel3Changed(int)), bmAudioBackend, SLOT(setEqLevel3(int)));
    connect(settings, SIGNAL(eqBLevel4Changed(int)), bmAudioBackend, SLOT(setEqLevel4(int)));
    connect(settings, SIGNAL(eqBLevel5Changed(int)), bmAudioBackend, SLOT(setEqLevel5(int)));
    connect(settings, SIGNAL(eqBLevel6Changed(int)), bmAudioBackend, SLOT(setEqLevel6(int)));
    connect(settings, SIGNAL(eqBLevel7Changed(int)), bmAudioBackend, SLOT(setEqLevel7(int)));
    connect(settings, SIGNAL(eqBLevel8Changed(int)), bmAudioBackend, SLOT(setEqLevel8(int)));
    connect(settings, SIGNAL(eqBLevel9Changed(int)), bmAudioBackend, SLOT(setEqLevel9(int)));
    connect(settings, SIGNAL(eqBLevel10Changed(int)), bmAudioBackend, SLOT(setEqLevel10(int)));
    connect(settings, SIGNAL(eqKBypassChanged(bool)), kAudioBackend, SLOT(setEqBypass(bool)));
    connect(settings, SIGNAL(eqKLevel1Changed(int)), kAudioBackend, SLOT(setEqLevel1(int)));
    connect(settings, SIGNAL(eqKLevel2Changed(int)), kAudioBackend, SLOT(setEqLevel2(int)));
    connect(settings, SIGNAL(eqKLevel3Changed(int)), kAudioBackend, SLOT(setEqLevel3(int)));
    connect(settings, SIGNAL(eqKLevel4Changed(int)), kAudioBackend, SLOT(setEqLevel4(int)));
    connect(settings, SIGNAL(eqKLevel5Changed(int)), kAudioBackend, SLOT(setEqLevel5(int)));
    connect(settings, SIGNAL(eqKLevel6Changed(int)), kAudioBackend, SLOT(setEqLevel6(int)));
    connect(settings, SIGNAL(eqKLevel7Changed(int)), kAudioBackend, SLOT(setEqLevel7(int)));
    connect(settings, SIGNAL(eqKLevel8Changed(int)), kAudioBackend, SLOT(setEqLevel8(int)));
    connect(settings, SIGNAL(eqKLevel9Changed(int)), kAudioBackend, SLOT(setEqLevel9(int)));
    connect(settings, SIGNAL(eqKLevel10Changed(int)), kAudioBackend, SLOT(setEqLevel10(int)));
    kAudioBackend->setEqBypass(settings->eqKBypass());
    kAudioBackend->setEqLevel1(settings->eqKLevel1());
    kAudioBackend->setEqLevel2(settings->eqKLevel2());
    kAudioBackend->setEqLevel3(settings->eqKLevel3());
    kAudioBackend->setEqLevel4(settings->eqKLevel4());
    kAudioBackend->setEqLevel5(settings->eqKLevel5());
    kAudioBackend->setEqLevel6(settings->eqKLevel6());
    kAudioBackend->setEqLevel7(settings->eqKLevel7());
    kAudioBackend->setEqLevel8(settings->eqKLevel8());
    kAudioBackend->setEqLevel9(settings->eqKLevel9());
    kAudioBackend->setEqLevel10(settings->eqKLevel10());
    bmAudioBackend->setEqBypass(settings->eqBBypass());
    bmAudioBackend->setEqLevel1(settings->eqBLevel1());
    bmAudioBackend->setEqLevel2(settings->eqBLevel2());
    bmAudioBackend->setEqLevel3(settings->eqBLevel3());
    bmAudioBackend->setEqLevel4(settings->eqBLevel4());
    bmAudioBackend->setEqLevel5(settings->eqBLevel5());
    bmAudioBackend->setEqLevel6(settings->eqBLevel6());
    bmAudioBackend->setEqLevel7(settings->eqBLevel7());
    bmAudioBackend->setEqLevel8(settings->eqBLevel8());
    bmAudioBackend->setEqLevel9(settings->eqBLevel9());
    bmAudioBackend->setEqLevel10(settings->eqBLevel10());
    connect(ui->lineEdit, SIGNAL(escapePressed()), ui->lineEdit, SLOT(clear()));
    connect(ui->lineEditBmSearch, SIGNAL(escapePressed()), ui->lineEditBmSearch, SLOT(clear()));
    connect(qModel, SIGNAL(songDroppedWithoutSinger()), this, SLOT(songDropNoSingerSel()));
    connect(ui->splitter_3, SIGNAL(splitterMoved(int,int)), this, SLOT(autosizeViews()));
    connect(ui->splitterBm, SIGNAL(splitterMoved(int,int)), this, SLOT(autosizeBmViews()));

    checker = new UpdateChecker(this);
    connect(checker, SIGNAL(newVersionAvailable(QString)), this, SLOT(newVersionAvailable(QString)));
    checker->checkForUpdates();
    timerButtonFlash = new QTimer(this);
    connect(timerButtonFlash, SIGNAL(timeout()), this, SLOT(timerButtonFlashTimeout()));
    timerButtonFlash->start(1000);
    ui->pushButtonIncomingRequests->setVisible(settings->requestServerEnabled());
    connect(settings, SIGNAL(requestServerEnabledChanged(bool)), ui->pushButtonIncomingRequests, SLOT(setVisible(bool)));
    connect(ui->actionSong_Shop, SIGNAL(triggered(bool)), dlgSongShop, SLOT(show()));
    qInfo() << "Initial UI stup complete";
    connect(qModel, SIGNAL(filesDroppedOnSinger(QList<QUrl>,int,int)), this, SLOT(filesDroppedOnQueue(QList<QUrl>,int,int)));
    connect(settings, SIGNAL(applicationFontChanged(QFont)), this, SLOT(appFontChanged(QFont)));
    QApplication::processEvents();
    QApplication::processEvents();
    appFontChanged(settings->applicationFont());
    startupOneShot = new QTimer(this);
    connect(startupOneShot, SIGNAL(timeout()), this, SLOT(autosizeViews()));
    connect(startupOneShot, SIGNAL(timeout()), this, SLOT(autosizeBmViews()));
    startupOneShot->setSingleShot(true);
    startupOneShot->start(500);
    scutAddSinger = new QShortcut(this);
    scutSearch = new QShortcut(this);
    scutRegulars = new QShortcut(this);
    scutRequests = new QShortcut(this);
    scutAddSinger->setKey(QKeySequence(Qt::Key_Insert));
    scutSearch->setKey((QKeySequence(Qt::Key_Slash)));
    scutRegulars->setKey(QKeySequence(Qt::CTRL + Qt::Key_R));
    scutRequests->setKey(QKeySequence(Qt::CTRL + Qt::Key_Q));
    connect(scutAddSinger, SIGNAL(activated()), this, SLOT(on_buttonAddSinger_clicked()));
    connect(scutSearch, SIGNAL(activated()), this, SLOT(scutSearchActivated()));
    connect(scutRegulars, SIGNAL(activated()), this, SLOT(on_buttonRegulars_clicked()));
    connect(scutRequests, SIGNAL(activated()), this, SLOT(on_pushButtonIncomingRequests_clicked()));
    connect(bmPlModel, SIGNAL(bmSongMoved(int,int)), this, SLOT(bmSongMoved(int,int)));
    connect(songbookApi, SIGNAL(alertRecieved(QString, QString)), this, SLOT(showAlert(QString, QString)));
    connect(settings, SIGNAL(cdgShowCdgWindowChanged(bool)), this, SLOT(cdgVisibilityChanged()));
    connect(settings, SIGNAL(rotationShowNextSongChanged(bool)), this, SLOT(resizeRotation()));
//    addSfxButton("some looooong string", "Some Name");
//    addSfxButton("second string", "Second Button");
//    addSfxButton("third string", "Third Button");
//    for (int i=0; i<9; i++)
//        addSfxButton("Button " + QString::number(i), "Button " + QString::number(i));
    SfxEntryList list = settings->getSfxEntries();
    qInfo() << "SfxEntryList size: " << list.size();
    foreach (SfxEntry entry, list) {
       addSfxButton(entry.path, entry.name);
    }
    connect(ui->tableViewRotation->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(tableViewRotationCurrentChanged(QModelIndex, QModelIndex)));
    rotModel->setCurrentSinger(settings->currentRotationPosition());
    rotDelegate->setCurrentSinger(settings->currentRotationPosition());
    updateRotationDuration();
    slowUiUpdateTimer = new QTimer(this);
    connect(slowUiUpdateTimer, SIGNAL(timeout()), this, SLOT(updateRotationDuration()));
    slowUiUpdateTimer->start(10000);
    connect(qModel, SIGNAL(queueModified(int)), this, SLOT(updateRotationDuration()));
    connect(settings, SIGNAL(rotationDurationSettingsModified()), this, SLOT(updateRotationDuration()));
    cdgWindow->setShowBgImage(true);
    lazyDurationUpdater = new LazyDurationUpdateController(this);
    if (settings->dbLazyLoadDurations())
        lazyDurationUpdater->getDurations();
    if (settings->showCdgWindow())
        ui->btnToggleCdgWindow->setText("Hide CDG Window");
    connect(ui->tableViewRotation->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(rotationSelectionChanged(QItemSelection, QItemSelection)));
}

QString MainWindow::findMatchingAudioFile(QString cdgFilePath)
{
    qInfo() << "findMatchingAudioFile(" << cdgFilePath << ") called";
    QStringList audioExtensions;
    audioExtensions.append("mp3");
    audioExtensions.append("wav");
    audioExtensions.append("ogg");
    audioExtensions.append("mov");
    audioExtensions.append("flac");
    QFileInfo cdgInfo(cdgFilePath);
    QDir srcDir = cdgInfo.absoluteDir();
    QDirIterator it(srcDir);
    while (it.hasNext())
    {
        it.next();
        if (it.fileInfo().completeBaseName() != cdgInfo.completeBaseName())
            continue;
        if (it.fileInfo().suffix().toLower() == "cdg")
            continue;
        QString ext;
        foreach (ext, audioExtensions)
        {
            if (it.fileInfo().suffix().toLower() == ext)
            {
                qInfo() << "findMatchingAudioFile found match: " << it.filePath();
                return it.filePath();
            }
        }
    }
    qInfo() << "findMatchingAudioFile found no matches";
    return QString();
}


void MainWindow::play(QString karaokeFilePath, bool k2k)
{
    khTmpDir->remove();
    delete khTmpDir;
    khTmpDir = new QTemporaryDir();
    if (kAudioBackend->state() != AbstractAudioBackend::PausedState)
    {
        if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
        {
            if (settings->karaokeAutoAdvance())
            {
                kAASkip = true;
                cdgWindow->showAlert(false);
            }
            kAudioBackend->stop();
        }
        if (karaokeFilePath.endsWith(".zip", Qt::CaseInsensitive))
        {
            OkArchive archive(karaokeFilePath);
            if ((archive.checkCDG()) && (archive.checkAudio()))
            {
                if (archive.checkAudio())
                {
                    if (!archive.extractAudio(khTmpDir->path(), "tmp" + archive.audioExtension()))
                    {
                        QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract audio file."),QMessageBox::Ok);
                        return;
                    }
                    QString audioFile = khTmpDir->path() + QDir::separator() + "tmp" + archive.audioExtension();
                    qInfo() << "Extracted audio file size: " << QFileInfo(audioFile).size();
                    qInfo() << "Loading CDG data";
                    cdg->open(archive.getCDGData());
                    qInfo() << "Processing CDG data";
                    cdg->process();
                    qInfo() << "CDG Processed. - Duration: " << cdg->duration() << " Last CDG draw command time: " << cdg->lastCDGUpdate();
                    cdgWindow->setShowBgImage(false);
                    setShowBgImage(false);
                    qInfo() << "Setting karaoke backend source file to: " << audioFile;
                    kAudioBackend->setMedia(audioFile);
                    //                ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_OUT);
                    if (!k2k)
                        bmAudioBackend->fadeOut(!settings->bmKCrossFade());
                    qInfo() << "Beginning playback of file: " << audioFile;
                    kAudioBackend->play();
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
            static QString prevCdgTmpFile;
            static QString prevAudioTmpFile;
            //QFile::remove(khTmpDir->path() + QDir::separator() + prevCdgTmpFile);
            //QFile::remove(khTmpDir->path() + QDir::separator() + prevAudioTmpFile);
            QString tmpString = GetRandomString();
            QString cdgTmpFile = tmpString + ".cdg";
            QString audTmpFile = tmpString + ".mp3";
            prevCdgTmpFile = cdgTmpFile;
            prevAudioTmpFile = audTmpFile;
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
            QString audiofn = findMatchingAudioFile(karaokeFilePath);
//            QString baseFn = karaokeFilePath;
//            baseFn.chop(3);
//            if (QFile::exists(baseFn + "mp3"))
//                audiofn = baseFn + "mp3";
//            else if (QFile::exists(baseFn + "Mp3"))
//                audiofn = baseFn + "Mp3";
//            else if (QFile::exists(baseFn + "MP3"))
//                audiofn = baseFn + "MP3";
//            else if (QFile::exists(baseFn + "mP3"))
//                audiofn = baseFn + "mP3";
            if (audiofn == "")
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file missing."),QMessageBox::Ok);
                return;
            }
            QFile audioFile(audiofn);
            if (audioFile.size() == 0)
            {
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file contains no data"),QMessageBox::Ok);
                return;
            }
            cdgFile.copy(khTmpDir->path() + QDir::separator() + cdgTmpFile);
            QFile::copy(audiofn, khTmpDir->path() + QDir::separator() + audTmpFile);
            cdg->open(khTmpDir->path() + QDir::separator() + cdgTmpFile);
            cdg->process();
            kAudioBackend->setMedia(khTmpDir->path() + QDir::separator() + audTmpFile);
//            ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_OUT);
            if (!k2k)
                bmAudioBackend->fadeOut(!settings->bmKCrossFade());
            kAudioBackend->play();
        }
        else
        {
            // Close CDG if open to avoid double video playback
            cdg->reset();
            qInfo() << "Playing non-CDG video file: " << karaokeFilePath;
            QString tmpFileName = khTmpDir->path() + QDir::separator() + "tmpvid." + karaokeFilePath.right(4);
            QFile::copy(karaokeFilePath, tmpFileName);
            qInfo() << "Playing temporary copy to avoid bad filename stuff w/ gstreamer: " << tmpFileName;
            kAudioBackend->setMedia(tmpFileName);
            if (!k2k)
                bmAudioBackend->fadeOut();
            kAudioBackend->play();
        }
        if (settings->recordingEnabled())
        {
            qInfo() << "Starting recording";
            QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm");
            audioRecorder->record(curSinger + " - " + curArtist + " - " + curTitle + " - " + timeStamp);
        }
    }
    else if (kAudioBackend->state() == AbstractAudioBackend::PausedState)
    {
        if (settings->recordingEnabled())
            audioRecorder->unpause();
        kAudioBackend->play();
    }
    k2kTransition = false;
    if (settings->karaokeAutoAdvance())
        kAASkip = false;
}

MainWindow::~MainWindow()
{
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
    qInfo() << "Deleting non-owned objects";
    delete cdg;
    delete khDir;
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
    if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
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
            connect(cb, SIGNAL(toggled(bool)), settings, SLOT(setShowSongPauseStopWarning(bool)));
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
            {
                return;
            }
        }
    }
    kAASkip = true;
    cdgWindow->showAlert(false);
    audioRecorder->stop();
    if (settings->bmKCrossFade())
    {
        bmAudioBackend->fadeIn(false);
        kAudioBackend->stop();
    }
    else
    {
        kAudioBackend->stop();
        bmAudioBackend->fadeIn();
    }
//    ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
}

void MainWindow::on_buttonPause_clicked()
{
    if (kAudioBackend->state() == AbstractAudioBackend::PausedState)
    {
        kAudioBackend->play();
    }
    else if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
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
            connect(cb, SIGNAL(toggled(bool)), settings, SLOT(setShowSongPauseStopWarning(bool)));
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
            {
                return;
            }
        }
        kAudioBackend->pause();
    }
}

void MainWindow::on_lineEdit_returnPressed()
{
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
            if ((kAudioBackend->state() == AbstractAudioBackend::PlayingState) && (settings->showSongInterruptionWarning()))
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
                connect(cb, SIGNAL(toggled(bool)), settings, SLOT(setShowSongInterruptionWarning(bool)));
                msgBox.exec();
                if (msgBox.clickedButton() != yesButton)
                {
                    return;
                }
                k2kTransition = true;
            }
            if (kAudioBackend->state() == AbstractAudioBackend::PausedState)
            {
                if (settings->karaokeAutoAdvance())
                {
                    kAASkip = true;
                    cdgWindow->showAlert(false);
                }
                audioRecorder->stop();
                kAudioBackend->stop(true);
            }
 //           play(nextSongPath);
 //           kAudioBackend->setPitchShift(rotModel->nextSongKeyChg(singerId));
            rotDelegate->setCurrentSinger(singerId);
            rotModel->setCurrentSinger(singerId);
            curSinger = rotModel->getSingerName(singerId);
            curArtist = rotModel->nextSongArtist(singerId);
            curTitle = rotModel->nextSongTitle(singerId);
            ui->labelArtist->setText(curArtist);
            ui->labelTitle->setText(curTitle);
            ui->labelSinger->setText(curSinger);
            play(nextSongPath, k2kTransition);
            kAudioBackend->setPitchShift(rotModel->nextSongKeyChg(singerId));
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
            connect(cb, SIGNAL(toggled(bool)), settings, SLOT(setShowSingerRemovalWarning(bool)));
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
    if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
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
            connect(cb, SIGNAL(toggled(bool)), settings, SLOT(setShowSongInterruptionWarning(bool)));
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
            {
                return;
            }
        }
        k2kTransition = true;
    }
    if (kAudioBackend->state() == AbstractAudioBackend::PausedState)
    {
        if (settings->karaokeAutoAdvance())
        {
            kAASkip = true;
            cdgWindow->showAlert(false);
        }
        audioRecorder->stop();
        kAudioBackend->stop(true);
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
    kAudioBackend->setPitchShift(index.sibling(index.row(),7).data().toInt());
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
            connect(cb, SIGNAL(toggled(bool)), settings, SLOT(setShowQueueRemovalWarning(bool)));
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
        settings->setCurrentRotationPosition(-1);
        rotModel->clearRotation();
        rotDelegate->setCurrentSinger(-1);
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
    if ((kAudioBackend->state() == AbstractAudioBackend::PlayingState) || (kAudioBackend->state() == AbstractAudioBackend::PausedState))
    {
        kAudioBackend->setPitchShift(arg1);
        if (arg1 > 0)
            ui->spinBoxKey->setPrefix("+");
        else
            ui->spinBoxKey->setPrefix("");
    }
    else
        ui->spinBoxKey->setValue(0);
}

void MainWindow::audioBackend_positionChanged(qint64 position)
{
    if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
    {
        if (cdg->isOpen() && cdg->lastCDGUpdate() >= position)
        {
                QVideoFrame frame = cdg->videoFrameByTime(position + cdgOffset);
                ui->cdgVideoWidget->videoSurface()->present(frame);
                cdgWindow->updateCDG(frame);
        }
        if (!sliderPositionPressed)
        {
            ui->sliderProgress->setMaximum(kAudioBackend->duration());
            ui->sliderProgress->setValue(position);
        }
        ui->labelElapsedTime->setText(kAudioBackend->msToMMSS(position));
        ui->labelRemainTime->setText(kAudioBackend->msToMMSS(kAudioBackend->duration() - position));
        remainSecs = (kAudioBackend->duration() - position) / 1000;
    }
}

void MainWindow::audioBackend_durationChanged(qint64 duration)
{
    ui->labelTotalTime->setText(kAudioBackend->msToMMSS(duration));
}

void MainWindow::audioBackend_stateChanged(AbstractAudioBackend::State state)
{
    qInfo() << "MainWindow - audioBackend_stateChanged(" << state << ") triggered";
    if (state == AbstractAudioBackend::StoppedState)
    {
        qInfo() << "MainWindow - audio backend state is now STOPPED";
        if (ui->labelTotalTime->text() == "0:00")
        {
            qInfo() << "MainWindow - UI is already reset, bailing out";
            return;
        }
        qInfo() << "KAudio entered StoppedState";
        audioRecorder->stop();
        cdg->reset();
        if (k2kTransition)
            return;
        ui->labelArtist->setText("None");
        ui->labelTitle->setText("None");
        ui->labelSinger->setText("None");
        ui->labelElapsedTime->setText("0:00");
        ui->labelRemainTime->setText("0:00");
        ui->labelTotalTime->setText("0:00");
        ui->sliderProgress->setValue(0);
        ui->cdgVideoWidget->clear();
        ui->spinBoxTempo->setValue(100);
        setShowBgImage(true);
        cdgWindow->setShowBgImage(true);
        cdgWindow->triggerBg();
        if (state == m_lastAudioState)
            return;
        m_lastAudioState = state;
        bmAudioBackend->fadeIn(false);
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
                    karaokeAATimer->start(settings->karaokeAATimeout() * 1000);
                    cdgWindow->setNextSinger(rotModel->getSingerName(nextSinger));
                    cdgWindow->setNextSong(rotModel->nextSongArtist(nextSinger) + " - " + rotModel->nextSongTitle(nextSinger));
                    cdgWindow->setCountdownSecs(settings->karaokeAATimeout());
                    cdgWindow->showAlert(true);
                }
            }
        }
    }
    if (state == AbstractAudioBackend::EndOfMediaState)
    {
        cdg->reset();
        qInfo() << "KAudio entered EndOfMediaState";
        audioRecorder->stop();
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        kAudioBackend->stop(true);
        bmAudioBackend->fadeIn(false);
        cdgWindow->setShowBgImage(true);
    }
    if (state == AbstractAudioBackend::PausedState)
    {
        qInfo() << "KAudio entered PausedState";
            audioRecorder->pause();
    }
    if (state == AbstractAudioBackend::PlayingState)
    {
        qInfo() << "KAudio entered PlayingState";
        m_lastAudioState = state;
        cdgWindow->setShowBgImage(false);
    }
    if (state == AbstractAudioBackend::UnknownState)
    {
        qInfo() << "KAudio entered UnknownState";
    }
    rotationDataChanged();
}

void MainWindow::sfxAudioBackend_positionChanged(qint64 position)
{
    ui->sliderSfxPos->setValue(position);
}

void MainWindow::sfxAudioBackend_durationChanged(qint64 duration)
{
    ui->sliderSfxPos->setMaximum(duration);
}

void MainWindow::sfxAudioBackend_stateChanged(AbstractAudioBackend::State state)
{
    if (state == AbstractAudioBackend::EndOfMediaState)
    {
        ui->sliderSfxPos->setValue(0);
        sfxAudioBackend->stop();
    }
    if (state == AbstractAudioBackend::StoppedState || state == AbstractAudioBackend::UnknownState)
        ui->sliderSfxPos->setValue(0);
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
    if (settings->rotationShowNextSong())
        resizeRotation();
    updateRotationDuration();
    requestsDialog->rotationChanged();
    QString statusBarText = "Singers: ";
    statusBarText += QString::number(rotModel->rowCount());
    rotDelegate->setSingerCount(rotModel->rowCount());
    labelSingerCount->setText(statusBarText);
    QString tickerText;
    if (settings->tickerCustomString() != "")
    {
        tickerText += settings->tickerCustomString() + " |";
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
        tickerText += " Singers: ";
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
            if (i < listSize - 1)
                tickerText += "  ";
        }
       // tickerText += "|";
    }
    cdgWindow->setTickerText(tickerText);
}

void MainWindow::silenceDetected()
{
    qInfo() << "Detected silence.  Cur Pos: " << kAudioBackend->position() << " Last CDG update pos: " << cdg->lastCDGUpdate();
    if (cdg->isOpen() && cdg->lastCDGUpdate() < kAudioBackend->position())
    {
        kAudioBackend->rawStop();
        if (settings->karaokeAutoAdvance())
            kAASkip = false;
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        bmAudioBackend->fadeIn();
    }
}

void MainWindow::silenceDetectedBm()
{
    if (bmAudioBackend->position() > 10000)
    {
        qInfo() << "Break music silence detected";
        bmMediaStateChanged(AbstractAudioBackend::EndOfMediaState);
    }
}

void MainWindow::audioBackendChanged(int index)
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
        contextMenu.addAction("Preview", this, SLOT(previewCdg()));
        contextMenu.addSeparator();
        contextMenu.addAction("Edit", this, SLOT(editSong()));
        contextMenu.addAction("Mark bad", this, SLOT(markSongBad()));
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

void MainWindow::sfxButton_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    SoundFxButton *btn = (SoundFxButton*)sender();
    lastRtClickedSfxBtn.path = btn->buttonData().toString();
    lastRtClickedSfxBtn.name = btn->text();
    QMenu contextMenu(this);
    contextMenu.addAction("Remove", this, SLOT(removeSfxButton()));
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
            contextMenu.addAction("Preview", this, SLOT(previewCdg()));
            contextMenu.addAction("Set Key Change", this, SLOT(setKeyChange()));
            contextMenu.addAction("Toggle played", this, SLOT(toggleQueuePlayed()));
            contextMenu.exec(QCursor::pos());
        }
    }
    else if (selCount > 1)
    {
        QMenu contextMenu(this);
        contextMenu.addAction("Set Played", this, SLOT(setMultiPlayed()));
        contextMenu.addAction("Set Unplayed", this, SLOT(setMultiUnplayed()));
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_sliderProgress_sliderPressed()
{
    sliderPositionPressed = true;
}

void MainWindow::on_sliderProgress_sliderReleased()
{
    kAudioBackend->setPosition(ui->sliderProgress->value());
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
    DlgCdgPreview *cdgPreviewDialog = new DlgCdgPreview(this);
    cdgPreviewDialog->setAttribute(Qt::WA_DeleteOnClose);
    cdgPreviewDialog->setSourceFile(dbRtClickFile);
    cdgPreviewDialog->preview();
}

void MainWindow::editSong()
{
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
        QString newFn;
        bool unsupported = false;
        switch (srcDir->getPattern()) {
        case SourceDir::SAT:
            newFn = dlg.songId() + " - " + dlg.artist() + " - " + dlg.title() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            break;
        case SourceDir::STA:
            newFn = dlg.songId() + " - " + dlg.title() + " - " + dlg.artist() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            break;
        case SourceDir::ATS:
            newFn = dlg.artist() + " - " + dlg.title() + " - " + dlg.songId() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            break;
        case SourceDir::TAS:
            newFn = dlg.title() + " - " + dlg.artist() + " - " + dlg.songId() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            break;
        case SourceDir::S_T_A:
            newFn = dlg.songId() + "_" + dlg.title() + "_" + dlg.artist() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            break;
        case SourceDir::AT:
            newFn = dlg.artist() + " - " + dlg.title() + "." + QFileInfo(dbRtClickFile).completeSuffix();
            break;
        case SourceDir::TA:
            newFn = dlg.title() + " - " + dlg.artist() + "." + QFileInfo(dbRtClickFile).completeSuffix();
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
        db->songMarkBad(dbRtClickFile);
        databaseUpdated();
        // connect
    } else if (msgBox.clickedButton() == removeFileButton) {
        QFile file(dbRtClickFile);
        if (file.remove())
        {
            db->songMarkBad(dbRtClickFile);
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
   if (kAudioBackend->state() == AbstractAudioBackend::StoppedState)
       cdgWindow->setShowBgImage(true);
}

void MainWindow::karaokeAATimerTimeout()
{
    qInfo() << "KaraokeAA - timer timeout";
    karaokeAATimer->stop();
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
        kAudioBackend->setPitchShift(rotModel->nextSongKeyChg(kAANextSinger));
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
                ui->checkBoxBmBreak->setChecked(false);
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
            if (kAudioBackend->state() != AbstractAudioBackend::PlayingState)
            {
                ui->cdgVideoWidget->clear();
                setShowBgImage(true);
                cdgWindow->setShowBgImage(true);
                cdgWindow->triggerBg();
            }
        }
    }
}

void MainWindow::bmMediaPositionChanged(qint64 position)
{
    if (!sliderBmPositionPressed)
    {
        ui->sliderBmPosition->setValue(position);
    }
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
        if (bmCurrentPosition == index.row())
        {
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
    bmAudioBackend->stop(false);
}

void MainWindow::on_lineEditBmSearch_returnPressed()
{
    bmDbModel->search(ui->lineEditBmSearch->text());
}

void MainWindow::on_tableViewBmPlaylist_doubleClicked(const QModelIndex &index)
{
    if (bmAudioBackend->state() == AbstractAudioBackend::PlayingState)
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

void MainWindow::refreshSongDbCache()
{
    QSqlQuery query;
    query.exec("DETACH DATABASE mem");
    query.exec("ATTACH DATABASE ':memory:' AS mem");
    query.exec("CREATE TABLE mem.dbsongs AS SELECT * FROM main.dbsongs");
}

void MainWindow::on_actionDisplay_Metadata_toggled(bool arg1)
{
    ui->tableViewBmDb->setColumnHidden(1, !arg1);
    ui->tableViewBmDb->setColumnHidden(2, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(3, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(4, !arg1);
    settings->bmSetShowMetadata(arg1);
    autosizeBmViews();
}

void MainWindow::on_actionDisplay_Filenames_toggled(bool arg1)
{
    ui->tableViewBmDb->setColumnHidden(4, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(5, !arg1);
    settings->bmSetShowFilenames(arg1);
    autosizeBmViews();
}

void MainWindow::on_actionShow_Debug_Log_toggled(bool arg1)
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

void MainWindow::videoFrameReceived(QImage frame, QString backendName)
{
    if (backendName == "break" && kAudioBackend->state() == AbstractAudioBackend::PlayingState)
        return;
    //QImage img = frame.copy();
    // We shouldn't have an open CDG file while karaoke video is playing, if one is open, close it
    if (cdg->isOpen())
        cdg->reset();
    ui->cdgVideoWidget->videoSurface()->present(QVideoFrame(frame));
    cdgWindow->updateCDG(frame);
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


void MainWindow::on_pushButtonMplxLeft_toggled(bool checked)
{
    if (checked)
        settings->setMplxMode(Multiplex_LeftChannel);
}

void MainWindow::on_pushButtonMplxBoth_toggled(bool checked)
{
    if (checked)
        settings->setMplxMode(Multiplex_Normal);
}

void MainWindow::on_pushButtonMplxRight_toggled(bool checked)
{
    if (checked)
        settings->setMplxMode(Multiplex_RightChannel);
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    if (!settings->progressiveSearchEnabled())
        return;
    static QString lastVal;
    if (arg1.trimmed() != lastVal)
    {
        dbModel->search(arg1);
        lastVal = arg1.trimmed();
    }
}

void MainWindow::cdgOffsetChanged(int offset)
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


void MainWindow::on_spinBoxTempo_valueChanged(int arg1)
{
    kAudioBackend->setTempo(arg1);
    cdg->setTempo(arg1);
}

void MainWindow::on_actionSongbook_Generator_triggered()
{
    dlgBookCreator->show();
}

void MainWindow::on_actionEqualizer_triggered()
{
    dlgEq->show();
}

void MainWindow::audioError(QString msg)
{
    QMessageBox msgBox;
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("Audio playback error! - " + msg);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
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

void MainWindow::on_sliderVolume_sliderMoved(int position)
{
    kAudioBackend->setVolume(position);
}

void MainWindow::on_sliderBmVolume_sliderMoved(int position)
{
    bmAudioBackend->setVolume(position);
}

void MainWindow::songDropNoSingerSel()
{
    QMessageBox msgBox;
    msgBox.setText("No singer selected.  You must select a singer before you can add songs to a queue.");
    msgBox.exec();
}

void MainWindow::newVersionAvailable(QString version)
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

void MainWindow::filesDroppedOnQueue(QList<QUrl> urls, int singerId, int position)
{
    foreach (QUrl url, urls)
    {
        QString file = url.toLocalFile();
        if (QFile(file).exists())
        {
            if (file.endsWith(".zip", Qt::CaseInsensitive))
            {
                OkArchive archive(file);
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

void MainWindow::appFontChanged(QFont font)
{
    QApplication::setFont(font, "QWidget");
    setFont(font);
    QFontMetrics fm(settings->applicationFont());
    int cvwWidth = max(300, fm.width("Total: 00:00  Current:00:00  Remain: 00:00"));
    qInfo() << "Resizing cdgVideoWidget to width: " << cvwWidth;
    ui->cdgVideoWidget->arResize(cvwWidth);
    ui->cdgFrame->setMinimumWidth(cvwWidth);
    ui->cdgFrame->setMaximumWidth(cvwWidth);
    ui->mediaFrame->setMinimumWidth(cvwWidth);
    ui->mediaFrame->setMaximumWidth(cvwWidth);

    QSize mcbSize(fm.height(), fm.height());
    ui->buttonStop->resize(mcbSize);
    ui->buttonPause->resize(mcbSize);
    ui->buttonStop->setIconSize(mcbSize);
    ui->buttonPause->setIconSize(mcbSize);
    ui->buttonStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->buttonPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));

    ui->buttonBmStop->resize(mcbSize);
    ui->buttonBmPause->resize(mcbSize);
    ui->buttonBmStop->setIconSize(mcbSize);
    ui->buttonBmPause->setIconSize(mcbSize);
    ui->buttonBmStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->buttonBmPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));

    ui->pushButton->resize(mcbSize);
    ui->pushButton->setIcon(QIcon(QPixmap(":/Icons/system-search2.png").scaled(mcbSize)));
    ui->pushButton->setIconSize(mcbSize);

    ui->buttonBmSearch->resize(mcbSize);
    ui->buttonBmSearch->setIcon(QIcon(QPixmap(":/Icons/system-search2.png").scaled(mcbSize)));
    ui->buttonBmSearch->setIconSize(mcbSize);

    ui->buttonAddSinger->resize(mcbSize);
    ui->buttonAddSinger->setIcon(QIcon(QPixmap(":/icons/Icons/list-add-user.png").scaled(mcbSize)));
    ui->buttonAddSinger->setIconSize(mcbSize);

    ui->buttonClearRotation->resize(mcbSize);
    ui->buttonClearRotation->setIcon(QIcon(QPixmap(":/icons/Icons/edit-clear.png").scaled(mcbSize)));
    ui->buttonClearRotation->setIconSize(mcbSize);

    ui->buttonClearQueue->resize(mcbSize);
    ui->buttonClearQueue->setIcon(QIcon(QPixmap(":/icons/Icons/edit-clear.png").scaled(mcbSize)));
    ui->buttonClearQueue->setIconSize(mcbSize);

    ui->buttonRegulars->resize(mcbSize);
    ui->buttonRegulars->setIcon(QIcon(QPixmap(":/icons/Icons/emblem-favorite.png").scaled(mcbSize)));
    ui->buttonRegulars->setIconSize(mcbSize);

    autosizeViews();

}

void MainWindow::resizeRotation()
{
    int fH = QFontMetrics(settings->applicationFont()).height();
    int iconWidth = fH + fH;
    int waitSize = QFontMetrics(settings->applicationFont()).width("Wait_");
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
            int width = QFontMetrics(settings->applicationFont()).width("_" + singers.at(i) + "_");
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
    int durationColSize = QFontMetrics(settings->applicationFont()).width(" Duration ");
    int songidColSize = QFontMetrics(settings->applicationFont()).width(" AA0000000-0000 ");
    int remainingSpace = ui->tableViewDB->width() - durationColSize - songidColSize;
    int artistColSize = (remainingSpace / 2) - 120;
    int titleColSize = (remainingSpace / 2) + 100;
    ui->tableViewDB->horizontalHeader()->resizeSection(1, artistColSize);
    ui->tableViewDB->horizontalHeader()->resizeSection(2, titleColSize);
    ui->tableViewDB->horizontalHeader()->resizeSection(4, durationColSize);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->tableViewDB->horizontalHeader()->resizeSection(3, songidColSize);

    resizeRotation();

    int keyColSize = QFontMetrics(settings->applicationFont()).width("Key") + iconWidth;
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
    int durationColSize = QFontMetrics(settings->applicationFont()).width("Duration") + fH;

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

void MainWindow::on_tabWidget_currentChanged(int index)
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

void MainWindow::bmSongMoved(int oldPos, int newPos)
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
    bmAudioBackend->setPosition(ui->sliderBmPosition->value());
    sliderBmPositionPressed = false;
    qInfo() << "BM slider up.  Position:" << ui->sliderBmPosition->value();
}

void MainWindow::sfxButtonPressed()
{
    SoundFxButton *btn = (SoundFxButton*)sender();
    qInfo() << "SfxButton pressed: " << btn->buttonData();
    sfxAudioBackend->setMedia(btn->buttonData().toString());
    sfxAudioBackend->setVolume(ui->sliderVolume->value());
    sfxAudioBackend->play();
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
    sfxAudioBackend->stop(true);
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

void MainWindow::showAlert(QString title, QString message)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
   // msgBox.setInformativeText(file);
    msgBox.exec();
}

void MainWindow::tableViewRotationCurrentChanged(QModelIndex cur, QModelIndex prev)
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
    labelRotationDuration->setText(text);
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

void MainWindow::rotationSelectionChanged(QItemSelection sel, QItemSelection desel)
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

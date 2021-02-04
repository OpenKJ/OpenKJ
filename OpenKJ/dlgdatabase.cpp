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

#include "dlgdatabase.h"
#include "ui_dlgdatabase.h"
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QSqlQuery>
#include <QMessageBox>
#include "dbupdatethread.h"
#include "settings.h"
#include <QStandardPaths>
#include <QFileSystemWatcher>

extern Settings settings;

DlgDatabase::DlgDatabase(QSqlDatabase db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDatabase)
{
    this->db = db;
    ui->setupUi(this);
    sourcedirmodel = new SourceDirTableModel();
    sourcedirmodel->loadFromDB();
    ui->tableViewFolders->setModel(sourcedirmodel);
    ui->tableViewFolders->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableViewFolders->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    selectedRow = -1;
    customPatternsDlg = new DlgCustomPatterns(this);
    dbUpdateDlg = new DlgDbUpdate(this);
    if (settings.dbDirectoryWatchEnabled())
    {
        QStringList sourceDirs = sourcedirmodel->getSourceDirs();
        QString path;
        foreach (path, sourceDirs)
        {
            QFileInfo finfo(path);
            if (finfo.isDir() && finfo.isReadable())
            {
                fsWatcher.addPath(path);
                qInfo() << "Adding watch to path: " << path;
                QDirIterator it(path, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    QString subPath = it.next();
                    if (!it.fileInfo().isDir() || subPath.endsWith("/.") || subPath.endsWith("/.."))
                        continue;
                    qInfo() << "Adding watch to subpath: " << subPath;
                    fsWatcher.addPath(subPath);
                }
            }
        }
        connect(&fsWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(directoryChanged(QString)));
    }
}

DlgDatabase::~DlgDatabase()
{
    fsWatcher.removePaths(fsWatcher.directories());
    delete sourcedirmodel;
    delete ui;
}

void DlgDatabase::singleSongAdd(const QString& path)
{
    qInfo() << "singleSongAdd(" << path << ") called";
    DbUpdateThread *updateThread = new DbUpdateThread(QSqlDatabase::cloneDatabase(QSqlDatabase::database(), "threaddb"),this);
    updateThread->addSingleTrack(path);
    delete updateThread;
    emit databaseUpdateComplete();
    //emit databaseSongAdded();
}

int DlgDatabase::dropFileAdd(const QString &path)
{
    DbUpdateThread *updateThread = new DbUpdateThread(QSqlDatabase::cloneDatabase(QSqlDatabase::database(), "threaddb"),this);
    int songId = updateThread->addDroppedFile(path);
    delete updateThread;
    return songId;
}

void DlgDatabase::on_buttonNew_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this);
    if (fileName != "")
    {
        bool okPressed = false;
        QStringList items;
        QSqlQuery query;
        query.exec("SELECT * FROM custompatterns ORDER BY name");
        while (query.next())
        {
            QString name = query.value("name").toString();
            items << QString(tr("Custom: ") + name);
        }


        items << tr("SongID - Artist - Title") << tr("SongID - Title - Artist") << tr("Artist - Title - SongID") << tr("Title - Artist - SongID") << tr("Artist - Title") << tr("Title - Artist") << tr("SongID_Title_Artist") << tr("Media Tags");
        QString selected = QInputDialog::getItem(this,"Select a file naming pattern","Pattern",items,0,false,&okPressed);
        if (okPressed)
        {
            int pattern = 0;
            int customPattern = -1;
            if (selected == tr("SongID - Artist - Title")) pattern = SourceDir::SAT;
            if (selected == tr("SongID - Title - Artist")) pattern = SourceDir::STA;
            if (selected == tr("Artist - Title - SongID")) pattern = SourceDir::ATS;
            if (selected == tr("Title - Artist - SongID")) pattern = SourceDir::TAS;
            if (selected == tr("Artist - Title")) pattern = SourceDir::AT;
            if (selected == tr("Title - Artist")) pattern = SourceDir::TA;
            if (selected == tr("Media Tags")) pattern = SourceDir::METADATA;
            if (selected == tr("SongID_Title_Artist")) pattern = SourceDir::S_T_A;
            if (selected.contains(tr("Custom")))
            {
                pattern = SourceDir::CUSTOM;
                QString name = selected.split(": ").at(1);
                query.exec("SELECT patternid FROM custompatterns WHERE name == \"" + name + "\"");
                if (query.first())
                    customPattern = query.value(0).toInt();
            }
            sourcedirmodel->addSourceDir(fileName, pattern, customPattern);
        }
    }
}

void DlgDatabase::on_buttonClose_clicked()
{
    settings.saveColumnWidths(ui->tableViewFolders);
    ui->tableViewFolders->clearSelection();
    hide();
}

void DlgDatabase::on_buttonDelete_clicked()
{
    if (selectedRow >= 0)
    {
    int index = ui->tableViewFolders->currentIndex().row();
    sourcedirmodel->delSourceDir(index);
    selectedRow = -1;
    ui->tableViewFolders->clearSelection();
    }
}

void DlgDatabase::on_tableViewFolders_clicked(const QModelIndex &index)
{
    selectedRow = index.row();
}

void DlgDatabase::on_buttonUpdate_clicked()
{
    if (selectedRow >= 0)
    {
        DbUpdateThread *updateThread = new DbUpdateThread(QSqlDatabase::cloneDatabase(QSqlDatabase::database(), "threaddb"),this);
        //emit databaseAboutToUpdate();
        dbUpdateDlg->reset();
        connect(updateThread, SIGNAL(progressMessage(QString)), dbUpdateDlg, SLOT(addProgressMsg(QString)));
        connect(updateThread, SIGNAL(stateChanged(QString)), dbUpdateDlg, SLOT(changeStatusTxt(QString)));
        connect(updateThread, SIGNAL(progressMaxChanged(int)), dbUpdateDlg, SLOT(setProgressMax(int)));
        connect(updateThread, SIGNAL(progressChanged(int)), dbUpdateDlg, SLOT(changeProgress(int)));
        dbUpdateDlg->changeDirectory(sourcedirmodel->getDirByIndex(selectedRow).getPath());
        dbUpdateDlg->show();
//        QMessageBox msgBox;
//        msgBox.setStandardButtons(0);
//        msgBox.setText("Updating Database, please wait...");
//        msgBox.show();
        QApplication::processEvents();
        updateThread->setPath(sourcedirmodel->getDirByIndex(selectedRow).getPath());
        updateThread->setPattern(sourcedirmodel->getDirByIndex(selectedRow).getPattern());
        QApplication::processEvents();
        updateThread->startUnthreaded();
//        while (updateThread->isRunning())
//        {
//            QApplication::processEvents();
//        }
        emit databaseUpdateComplete();
        QApplication::processEvents();
        dbUpdateDlg->changeStatusTxt(tr("Database update complete!"));
        dbUpdateDlg->setProgressMax(100);
        dbUpdateDlg->changeProgress(100);
        QApplication::processEvents();
        showDbUpdateErrors(updateThread->getErrors());
        QMessageBox::information(this, tr("Update Complete"), tr("Database update complete."));
        dbUpdateDlg->hide();
    }
}

void DlgDatabase::on_buttonUpdateAll_clicked()
{
    //emit databaseAboutToUpdate();
    DbUpdateThread *updateThread = new DbUpdateThread(QSqlDatabase::cloneDatabase(QSqlDatabase::database(), "threaddb"),this);
    dbUpdateDlg->reset();
    connect(updateThread, SIGNAL(progressMessage(QString)), dbUpdateDlg, SLOT(addProgressMsg(QString)));
    connect(updateThread, SIGNAL(stateChanged(QString)), dbUpdateDlg, SLOT(changeStatusTxt(QString)));
    connect(updateThread, SIGNAL(progressMaxChanged(int)), dbUpdateDlg, SLOT(setProgressMax(int)));
    connect(updateThread, SIGNAL(progressChanged(int)), dbUpdateDlg, SLOT(changeProgress(int)));
    dbUpdateDlg->show();

    //QMessageBox msgBox;
    //msgBox.setStandardButtons(0);
    //msgBox.setText("Updating Database, please wait...");
    //msgBox.show();
    for (int i=0; i < sourcedirmodel->size(); i++)
    {
        //msgBox.setInformativeText("Processing path: " + sourcedirmodel->getDirByIndex(i)->getPath());
        dbUpdateDlg->changeDirectory(sourcedirmodel->getDirByIndex(i).getPath());
        updateThread->setPath(sourcedirmodel->getDirByIndex(i).getPath());
        updateThread->setPattern(sourcedirmodel->getDirByIndex(i).getPattern());
        updateThread->startUnthreaded();
//        while (updateThread->isRunning())
//        {
//            QApplication::processEvents();
//        }
    }
//    msgBox.setInformativeText("Reloading song database into cache");
    emit databaseUpdateComplete();
//    msgBox.hide();
    showDbUpdateErrors(updateThread->getErrors());
    dbUpdateDlg->hide();
    QMessageBox::information(this, tr("Update Complete"), tr("Database update complete."));
    delete(updateThread);
    emit databaseUpdateComplete();
}

void DlgDatabase::on_btnClearDatabase_clicked()
{
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure?"));
    msgBox.setInformativeText(tr("Clearing the song database will also clear the rotation and all saved regular singer data.  If you have not already done so, you may want to export your regular singers before performing this operation.  This operation can not be undone."));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        QSqlQuery query;
        query.exec("DELETE FROM dbSongs");
        query.exec("DELETE FROM regularsongs");
        query.exec("DELETE FROM regularsingers");
        query.exec("DELETE FROM queuesongs");
        query.exec("DELETE FROM rotationsingers");
        emit databaseCleared();
        QMessageBox::information(this, tr("Database cleared"), tr("Song database, regular singers, and all rotation data has been cleared."));
    }
}

void DlgDatabase::dbupdate_thread_finished()
{
}

void DlgDatabase::showDbUpdateErrors(QStringList errors)
{
    if (errors.count() > 0)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Some files were skipped due to problems"));
        msgBox.setDetailedText(errors.join("\n"));
        QSpacerItem* horizontalSpacer = new QSpacerItem(600, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        QGridLayout* layout = (QGridLayout*)msgBox.layout();
        layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
        msgBox.exec();
    }
}

void DlgDatabase::on_btnCustomPatterns_clicked()
{
    customPatternsDlg->show();
}

void DlgDatabase::on_btnExport_clicked()
{
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "dbexport.csv";
    qDebug() << "Default save location: " << defaultFilePath;
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select DB export filename"), defaultFilePath, "(*.csv)");
    if (saveFilePath != "")
    {
        QFile csvFile(saveFilePath);
        if (!csvFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, tr("Error saving file"), tr("Unable to open selected file for writing.  Please verify that you have the proper permissions to write to that location."),QMessageBox::Close);
            return;
        }
        QSqlQuery query;
        query.exec("SELECT * from dbsongs ORDER BY artist,title,filename");
        while (query.next())
        {
            QString artist = query.value("artist").toString();
            QString title = query.value("title").toString();
            QString songId = query.value("discid").toString();
            QString filepath = query.value("path").toString();
            QString data = "\"" + artist + "\",\"" + title + "\",\"" + songId + "\",\"" + filepath + "\"" + "\n";
            csvFile.write(data.toLocal8Bit().data());
        }
        csvFile.close();
    }
}

void DlgDatabase::directoryChanged(QString dirPath)
{
    if (!settings.dbDirectoryWatchEnabled())
        return;
    DbUpdateThread *dbthread = new DbUpdateThread(db, this);
    qInfo() << "Directory changed fired for dir: " << dirPath;
    QDirIterator it(dirPath);
    while (it.hasNext()) {
        QString file = it.next();
        QFileInfo fi(file);
        if (fi.isDir())
            continue;
        if (file == dirPath + "/." || file == dirPath + "/..")
            continue;
        if (fi.suffix().toLower() != "zip" && fi.suffix().toLower() != "cdg")
        {
            continue;
        }
        if (dbthread->dbEntryExists(file))
        {
            continue;
        }
        qInfo() << "Detected new file: " << file;
        qInfo() << "Adding file to the database";
        dbthread->addSingleTrack(file);
        emit databaseUpdateComplete();
    }
    delete(dbthread);

}

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

extern Settings *settings;

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
}

DlgDatabase::~DlgDatabase()
{
    delete sourcedirmodel;
    delete ui;
}

void DlgDatabase::singleSongAdd(QString path)
{
    DbUpdateThread *updateThread = new DbUpdateThread(this);
    updateThread->addSingleTrack(path);
    delete updateThread;
    emit databaseUpdated();
}

int DlgDatabase::dropFileAdd(QString path)
{
    DbUpdateThread *updateThread = new DbUpdateThread(this);
    int songId = updateThread->addDroppedFile(path);
    delete updateThread;
    emit databaseUpdated();
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
//            g_artistRegex = query.value("artistregex").toString();
//            g_titleRegex  = query.value("titleregex").toString();
//            g_discIdRegex = query.value("discidregex").toString();
//            g_artistCaptureGrp = query.value("artistcapturegrp").toInt();
//            g_titleCaptureGrp  = query.value("titlecapturegrp").toInt();
//            g_discIdCaptureGrp = query.value("discidcapturegrp").toInt();
            items << QString("Custom: " + name);
        }


        items << "DiscID - Artist - Title" << "DiscID - Title - Artist" << "Artist - Title - DiscID" << "Title - Artist - DiscID" << "Artist - Title" << "Title - Artist" << "Media Tags";
        QString selected = QInputDialog::getItem(this,"Select a file naming pattern","Pattern",items,0,false,&okPressed);
        if (okPressed)
        {
            int pattern = 0;
            int customPattern = -1;
            if (selected == "DiscID - Artist - Title") pattern = SourceDir::DAT;
            if (selected == "DiscID - Title - Artist") pattern = SourceDir::DTA;
            if (selected == "Artist - Title - DiscID") pattern = SourceDir::ATD;
            if (selected == "Title - Artist - DiscID") pattern = SourceDir::TAD;
            if (selected == "Artist - Title") pattern = SourceDir::AT;
            if (selected == "Title - Artist") pattern = SourceDir::TA;
            if (selected == "Media Tags") pattern = SourceDir::METADATA;
            if (selected.contains("Custom"))
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
    settings->saveColumnWidths(ui->tableViewFolders);
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
        DbUpdateThread *updateThread = new DbUpdateThread(this);
        dbUpdateDlg->reset();
        connect(updateThread, SIGNAL(progressMessage(QString)), dbUpdateDlg, SLOT(addProgressMsg(QString)));
        connect(updateThread, SIGNAL(stateChanged(QString)), dbUpdateDlg, SLOT(changeStatusTxt(QString)));
        connect(updateThread, SIGNAL(progressMaxChanged(int)), dbUpdateDlg, SLOT(setProgressMax(int)));
        connect(updateThread, SIGNAL(progressChanged(int)), dbUpdateDlg, SLOT(changeProgress(int)));
        dbUpdateDlg->changeDirectory(sourcedirmodel->getDirByIndex(selectedRow)->getPath());
        dbUpdateDlg->show();
//        QMessageBox msgBox;
//        msgBox.setStandardButtons(0);
//        msgBox.setText("Updating Database, please wait...");
//        msgBox.show();
        QApplication::processEvents();
        updateThread->setPath(sourcedirmodel->getDirByIndex(selectedRow)->getPath());
        updateThread->setPattern(sourcedirmodel->getDirByIndex(selectedRow)->getPattern());
        updateThread->start();
        while (updateThread->isRunning())
        {
            QApplication::processEvents();
        }
        emit databaseUpdated();
//        msgBox.hide();

        showDbUpdateErrors(updateThread->getErrors());
        dbUpdateDlg->hide();
        QMessageBox::information(this, "Update Complete", "Database update complete.");
    }
}

void DlgDatabase::on_buttonUpdateAll_clicked()
{
    DbUpdateThread *updateThread = new DbUpdateThread(this);
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
        dbUpdateDlg->changeDirectory(sourcedirmodel->getDirByIndex(i)->getPath());
        updateThread->setPath(sourcedirmodel->getDirByIndex(i)->getPath());
        updateThread->setPattern(sourcedirmodel->getDirByIndex(i)->getPattern());
        updateThread->start();
        while (updateThread->isRunning())
        {
            QApplication::processEvents();
        }
    }
//    msgBox.setInformativeText("Reloading song database into cache");
    emit databaseUpdated();
//    msgBox.hide();
    showDbUpdateErrors(updateThread->getErrors());
    dbUpdateDlg->hide();
    QMessageBox::information(this, "Update Complete", "Database update complete.");
    delete(updateThread);
}

void DlgDatabase::on_btnClearDatabase_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("Clearing the song database will also clear the rotation and all saved regular singer data.  If you have not already done so, you may want to export your regular singers before performing this operation.  This operation can not be undone.");
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
        QMessageBox::information(this, "Database cleared", "Song database, regular singers, and all rotation data has been cleared.");
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
        msgBox.setText("Some files were skipped due to problems");
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
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select DB export filename"), defaultFilePath, tr("(*.csv)"));
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
            QString discid = query.value("discid").toString();
            QString filepath = query.value("path").toString();
            QString data = "\"" + artist + "\",\"" + title + "\",\"" + discid + "\",\"" + filepath + "\"" + "\n";
            csvFile.write(data.toLocal8Bit().data());
        }
        csvFile.close();
    }
}

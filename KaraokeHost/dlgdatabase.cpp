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

#include "dlgdatabase.h"
#include "ui_dlgdatabase.h"
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QSqlQuery>
#include <QMessageBox>
#include "dbupdatethread.h"
#include "khsettings.h"

extern KhSettings *settings;

DlgDatabase::DlgDatabase(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDatabase)
{
    ui->setupUi(this);
    sourcedirmodel = new SourceDirTableModel();
    sourcedirmodel->loadFromDB();
    ui->treeViewFolders->setModel(sourcedirmodel);
    selectedRow = -1;
    settings->restoreColumnWidths(ui->treeViewFolders);
    dlgDurationScan = new DlgDurationScan(this);
}

DlgDatabase::~DlgDatabase()
{
    delete sourcedirmodel;
    delete ui;
}



void DlgDatabase::on_buttonNew_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this);
    if (fileName != "")
    {
        bool okPressed = false;
        QStringList items;
        items << "DiscID - Artist - Title" << "DiscID - Title - Artist" << "Artist - Title - DiscID" << "Title - Artist - DiscID" << "Artist - Title" << "Title - Artist";
        QString selected = QInputDialog::getItem(this,"Select a file naming pattern","Pattern",items,0,false,&okPressed);
        if (okPressed)
        {
            int pattern = 0;
            if (selected == "DiscID - Artist - Title") pattern = SourceDir::DAT;
            if (selected == "DiscID - Title - Artist") pattern = SourceDir::DTA;
            if (selected == "Artist - Title - DiscID") pattern = SourceDir::ATD;
            if (selected == "Title - Artist - DiscID") pattern = SourceDir::TAD;
            if (selected == "Artist - Title") pattern = SourceDir::AT;
            if (selected == "Title - Artist") pattern = SourceDir::TA;
            sourcedirmodel->addSourceDir(fileName, pattern);
        }
    }
}

void DlgDatabase::on_buttonClose_clicked()
{
    settings->saveColumnWidths(ui->treeViewFolders);
    ui->treeViewFolders->clearSelection();
    hide();
}

void DlgDatabase::on_buttonDelete_clicked()
{
    if (selectedRow >= 0)
    {
    int index = ui->treeViewFolders->currentIndex().row();
    sourcedirmodel->delSourceDir(index);
    selectedRow = -1;
    ui->treeViewFolders->clearSelection();
    }
}

void DlgDatabase::on_treeViewFolders_clicked(const QModelIndex &index)
{
    selectedRow = index.row();
}

void DlgDatabase::on_buttonUpdate_clicked()
{
    if (selectedRow >= 0)
    {
        DbUpdateThread updateThread(this);
        QMessageBox msgBox;
        msgBox.setStandardButtons(0);
        msgBox.setText("Updating Database, please wait...");
        msgBox.show();
        QApplication::processEvents();
        updateThread.setPath(sourcedirmodel->getDirByIndex(selectedRow)->getPath());
        updateThread.setPattern(sourcedirmodel->getDirByIndex(selectedRow)->getPattern());
        updateThread.start();
        while (updateThread.isRunning())
        {
            QApplication::processEvents();
        }
        emit databaseUpdated();
        msgBox.hide();
        QMessageBox::information(this, "Update Complete", "Database update complete.");
    }
}

void DlgDatabase::on_buttonUpdateAll_clicked()
{
    DbUpdateThread updateThread(this);
    QMessageBox msgBox;
    msgBox.setStandardButtons(0);
    msgBox.setText("Updating Database, please wait...");
    msgBox.show();
    for (int i=0; i < sourcedirmodel->size(); i++)
    {
        msgBox.setInformativeText("Processing path: " + sourcedirmodel->getDirByIndex(i)->getPath());
        updateThread.setPath(sourcedirmodel->getDirByIndex(i)->getPath());
        updateThread.setPattern(sourcedirmodel->getDirByIndex(i)->getPattern());
        updateThread.start();
        while (updateThread.isRunning())
        {
            QApplication::processEvents();
        }
    }
    msgBox.setInformativeText("Reloading song database into cache");
    emit databaseUpdated();
    msgBox.hide();
    QMessageBox::information(this, "Update Complete", "Database update complete.");
}

void DlgDatabase::on_btnClearDatabase_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText("Clearing the song database will also clear the rotation and all saved regular singer data.  If you have not already done so, you may want to export your regular singers before performing this operation.  This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);

    msgBox.exec();

    if (msgBox.clickedButton() == yesButton) {
        QSqlQuery query;
        query.exec("DELETE FROM dbSongs");
        emit databaseCleared();
        QMessageBox::information(this, "Database cleared", "Song database, regular singers, and all rotation data has been cleared.");
    } else if (msgBox.clickedButton() == cancelButton) {
        // Cancelled
    }
}

void DlgDatabase::dbupdate_thread_finished()
{
}

void DlgDatabase::on_pushButtonGetDurations_clicked()
{
    dlgDurationScan->show();
}

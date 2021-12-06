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
#include "dbupdater.h"
#include <QStandardPaths>

DlgDatabase::DlgDatabase(TableModelKaraokeSongs &dbModel, QWidget *parent) :
    QDialog(parent),
    m_dbModel(dbModel),
    ui(new Ui::DlgDatabase)
{
    ui->setupUi(this);
    sourcedirmodel = new TableModelKaraokeSourceDirs();
    sourcedirmodel->loadFromDB();
    ui->tableViewFolders->setModel(sourcedirmodel);
    ui->tableViewFolders->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableViewFolders->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(ui->tableViewFolders->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DlgDatabase::on_foldersSelectionChanged);
    updateButtonsState();
    customPatternsDlg = new DlgCustomPatterns(this);
    dbUpdateDlg = new DlgDbUpdate(this);

    if (m_settings.dbDirectoryWatchEnabled()) {
        m_directoryMonitor = new DirectoryMonitor(this, sourcedirmodel->getSourceDirs());
        connect(m_directoryMonitor, &DirectoryMonitor::databaseUpdateComplete, this, &DlgDatabase::databaseUpdateComplete);
    }
}

DlgDatabase::~DlgDatabase()
{
    delete m_directoryMonitor;
    delete sourcedirmodel;
    delete ui;
}

void DlgDatabase::singleSongAdd(const QString& path)
{
    qInfo() << "singleSongAdd(" << path << ") called";
    DbUpdater updater;
    updater.addFilesToDatabase(QStringList() << path);
    emit databaseUpdateComplete();
    //emit databaseSongAdded();
}

void DlgDatabase::on_buttonNew_clicked()
{
#ifdef Q_OS_LINUX
    QString fileName = QFileDialog::getExistingDirectory(
            this,
            "Select a karaoke source dir",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
            );
#else
    QString fileName = QFileDialog::getExistingDirectory(
            this,
            "Select a karaoke source dir",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            QFileDialog::ShowDirsOnly
            );
#endif
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
    updateButtonsState();
}

void DlgDatabase::on_buttonClose_clicked()
{
    m_settings.saveColumnWidths(ui->tableViewFolders);
    ui->tableViewFolders->clearSelection();
    hide();
}

void DlgDatabase::on_buttonDelete_clicked()
{
    int index = ui->tableViewFolders->currentIndex().row();
    if (index >= 0)
    {
        sourcedirmodel->delSourceDir(index);
        updateButtonsState();
    }
}

void DlgDatabase::on_buttonUpdate_clicked()
{
    scan(false);
}

void DlgDatabase::on_buttonUpdateAll_clicked()
{
    scan(true);
}

void DlgDatabase::scan(bool scanAllPaths)
{
    QStringList paths;
    DbUpdater::ProcessingOptions processingOptions = DbUpdater::ProcessingOption::PrepareForRemovalOfMissing;

    if (scanAllPaths) {
        processingOptions |= DbUpdater::ProcessingOption::FixMovedFilesSearchInWholeDB;
        for (int i=0; i < sourcedirmodel->size(); i++)
            paths.append(sourcedirmodel->getDirByIndex(i).getPath());
    }
    else {
        processingOptions |= DbUpdater::ProcessingOption::FixMovedFiles;
        int index = ui->tableViewFolders->currentIndex().row();
        if (index >= 0) {
            paths.append(sourcedirmodel->getDirByIndex(index).getPath());
        }
    }

    if (paths.isEmpty())
        return;

    dbUpdateDlg->reset();
    DbUpdater updater;
    connect(&updater, &DbUpdater::progressMessage, dbUpdateDlg, &DlgDbUpdate::addLogMsg);
    connect(&updater, &DbUpdater::stateChanged, dbUpdateDlg, &DlgDbUpdate::changeStatusTxt);
    connect(&updater, &DbUpdater::progressChanged, dbUpdateDlg, &DlgDbUpdate::changeProgress);
    dbUpdateDlg->show();
    QApplication::processEvents();

    updater.process(paths, processingOptions);

    emit databaseUpdateComplete();
    showDbUpdateErrors(updater.getErrors());

    if (updater.missingFilesCount() > 0) {
        QString text = "There are %1 file(s) in the database that are no longer present on disk. Do you want to remove them from the database?";
        if (!scanAllPaths) {
            text += "\n\nIf the files have been been moved to another path in the database, "
                    "select 'No' and then 'Update all' to detect the new location and update the database.";
        }

        QMessageBox msgBox;
        msgBox.setText(tr("Remove missing files from database?"));
        msgBox.setInformativeText(tr(qPrintable(text)).arg(updater.missingFilesCount()));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.addButton(QMessageBox::No);
        QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
        msgBox.exec();
        if (msgBox.clickedButton() == yesButton) {
            updater.removeMissingFilesFromDatabase();
        }
    }

    dbUpdateDlg->hide();
    QMessageBox::information(this, tr("Update Complete"), tr("Database update complete."));
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

void DlgDatabase::showDbUpdateErrors(const QStringList& errors)
{
    if (errors.count() > 0)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Some files were skipped due to problems"));
        msgBox.setDetailedText(errors.join("\n"));
        auto horizontalSpacer = new QSpacerItem(600, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        auto layout = (QGridLayout*)msgBox.layout();
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
#ifdef Q_OS_LINUX
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select DB export filename"), defaultFilePath, "(*.csv)",
                                                        nullptr, QFileDialog::DontUseNativeDialog);
#else
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select DB export filename"), defaultFilePath, "(*.csv)",
                                                        nullptr);
#endif
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

void DlgDatabase::on_foldersSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    updateButtonsState();
}

void DlgDatabase::updateButtonsState()
{
    bool hasSelectedRow = ui->tableViewFolders->selectionModel()->selectedRows().count() > 0;
    ui->buttonUpdate->setEnabled(hasSelectedRow);
    ui->buttonDelete->setEnabled(hasSelectedRow);

    auto model = ui->tableViewFolders->model();
    ui->buttonUpdateAll->setEnabled(model && model->rowCount() > 0);
}


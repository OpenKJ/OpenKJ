/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
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

#include "bmdbdialog.h"
#include "ui_bmdbdialog.h"
#include "bmdbupdatethread.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>

BmDbDialog::BmDbDialog(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::BmDbDialog) {
    ui->setupUi(this);
    m_pathsModel.setTable("bmsrcdirs");
    m_pathsModel.select();
    ui->tableViewPaths->setModel(&m_pathsModel);
    m_pathsModel.sort(0, Qt::AscendingOrder);
    connect(ui->pushButtonAdd, &QPushButton::clicked, this, &BmDbDialog::pushButtonAddClicked);
    connect(ui->pushButtonClearDb, &QPushButton::clicked, this, &BmDbDialog::pushButtonClearDbClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &BmDbDialog::close);
    connect(ui->pushButtonDelete, &QPushButton::clicked, this, &BmDbDialog::pushButtonDeleteClicked);
    connect(ui->pushButtonUpdate, &QPushButton::clicked, this, &BmDbDialog::pushButtonUpdateClicked);
    connect(ui->pushButtonUpdateAll, &QPushButton::clicked, this, &BmDbDialog::pushButtonUpdateAllClicked);
}

// This is here instead of the header to make moc & std::unique_ptr happy
BmDbDialog::~BmDbDialog() = default;

void BmDbDialog::pushButtonAddClicked() {
    QString fileName = QFileDialog::getExistingDirectory(
            this,
            "Select a meadia source dir",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog
            );
    if (fileName != "") {
        m_pathsModel.insertRow(m_pathsModel.rowCount());
        m_pathsModel.setData(m_pathsModel.index(m_pathsModel.rowCount() - 1, 0), fileName);
        m_pathsModel.submitAll();
    }
}


void BmDbDialog::pushButtonUpdateClicked() {
    if (ui->tableViewPaths->selectionModel()->selectedIndexes().empty())
        return;
    auto selIndex = ui->tableViewPaths->selectionModel()->selectedIndexes().at(0);
    auto path = selIndex.sibling(selIndex.row(), 0).data().toString();
    m_dbUpdateDlg.reset();
    m_dbUpdateDlg.show();
    auto thread = new BmDbUpdateThread(this);
    thread->setPath(path);
    m_dbUpdateDlg.changeDirectory(path);
    connect(thread, &BmDbUpdateThread::progressMessage, &m_dbUpdateDlg, &DlgDbUpdate::addProgressMsg);
    connect(thread, &BmDbUpdateThread::stateChanged, &m_dbUpdateDlg, &DlgDbUpdate::changeStatusTxt);
    connect(thread, &BmDbUpdateThread::progressMaxChanged, &m_dbUpdateDlg, &DlgDbUpdate::setProgressMax);
    connect(thread, &BmDbUpdateThread::progressChanged, &m_dbUpdateDlg, &DlgDbUpdate::changeProgress);
    thread->startUnthreaded();
    QMessageBox::information(this, tr("Update Complete"), tr("Database update complete."));
    m_dbUpdateDlg.hide();
    emit bmDbUpdated();
    delete (thread);
}

void BmDbDialog::pushButtonUpdateAllClicked() {
    m_dbUpdateDlg.reset();
    m_dbUpdateDlg.show();
    for (int i = 0; i < m_pathsModel.rowCount(); i++) {
        QApplication::processEvents();
        auto thread = new BmDbUpdateThread(this);
        thread->setPath(m_pathsModel.data(m_pathsModel.index(i, 0)).toString());
        m_dbUpdateDlg.changeDirectory(m_pathsModel.data(m_pathsModel.index(i, 0)).toString());
        connect(thread, &BmDbUpdateThread::progressMessage, &m_dbUpdateDlg, &DlgDbUpdate::addProgressMsg);
        connect(thread, &BmDbUpdateThread::stateChanged, &m_dbUpdateDlg, &DlgDbUpdate::changeStatusTxt);
        connect(thread, &BmDbUpdateThread::progressMaxChanged, &m_dbUpdateDlg, &DlgDbUpdate::setProgressMax);
        connect(thread, &BmDbUpdateThread::progressChanged, &m_dbUpdateDlg, &DlgDbUpdate::changeProgress);
        thread->startUnthreaded();
        delete (thread);
    }
    QMessageBox::information(this, tr("Update Complete"), tr("Database update complete."));
    m_dbUpdateDlg.hide();
    emit bmDbUpdated();
}

void BmDbDialog::pushButtonClearDbClicked() {
    QMessageBox msgBox(QMessageBox::Warning, "Are you sure?",
                       "Clearing the database will also clear all playlists.  If you have not already done so, "
                       "you may want to export your playlists before performing this operation.",
                       QMessageBox::Cancel | QMessageBox::Yes);
    if (msgBox.exec() != QMessageBox::Yes)
        return;
    QSqlQuery query;
    query.exec("DELETE FROM bmplaylists");
    query.exec("DELETE FROM bmplsongs");
    query.exec("DELETE FROM bmsongs");
    query.exec("UPDATE sqlite_sequence SET seq = 0");
    query.exec("VACUUM");
    m_pathsModel.select();
    emit bmDbCleared();
}

void BmDbDialog::pushButtonDeleteClicked() {
    if (ui->tableViewPaths->selectionModel()->selectedIndexes().empty())
        return;
    m_pathsModel.removeRow(ui->tableViewPaths->selectionModel()->selectedIndexes().at(0).row());
    m_pathsModel.select();
    m_pathsModel.submitAll();
}



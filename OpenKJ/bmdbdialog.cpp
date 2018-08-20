/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
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
#include "tagreader.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDebug>
#include <QtConcurrent>

BmDbDialog::BmDbDialog(QSqlDatabase db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BmDbDialog)
{
    m_db = db;
    ui->setupUi(this);
    pathsModel = new QSqlTableModel(this, db);
    pathsModel->setTable("bmsrcdirs");
    pathsModel->select();
    ui->tableViewPaths->setModel(pathsModel);
    pathsModel->sort(0, Qt::AscendingOrder);
    selectedDirectoryIdx = -1;
    dbUpdateDlg = new DlgDbUpdate(this);
}

BmDbDialog::~BmDbDialog()
{
    delete ui;
}

void BmDbDialog::on_pushButtonAdd_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this);
    if (fileName != "")
    {
        pathsModel->insertRow(pathsModel->rowCount());
        pathsModel->setData(pathsModel->index(pathsModel->rowCount() - 1, 0), fileName);
        pathsModel->submitAll();
    }
}



void BmDbDialog::on_pushButtonUpdate_clicked()
{
    if (selectedDirectoryIdx >= 0)
    {
        //emit bmDbAboutToUpdate();
        dbUpdateDlg->reset();
        dbUpdateDlg->show();
        //QMessageBox *msgBox = new QMessageBox(this);
        //msgBox->setStandardButtons(0);
        //msgBox->setText("Updating Database, please wait...");
        //msgBox->show();
        BmDbUpdateThread *thread = new BmDbUpdateThread(QSqlDatabase::cloneDatabase(QSqlDatabase::database(), "bmThreadDb"),this);
        thread->setPath(pathsModel->data(pathsModel->index(selectedDirectoryIdx, 0)).toString());
        dbUpdateDlg->changeDirectory(pathsModel->data(pathsModel->index(selectedDirectoryIdx, 0)).toString());
        connect(thread, SIGNAL(progressMessage(QString)), dbUpdateDlg, SLOT(addProgressMsg(QString)));
        connect(thread, SIGNAL(stateChanged(QString)), dbUpdateDlg, SLOT(changeStatusTxt(QString)));
        connect(thread, SIGNAL(progressMaxChanged(int)), dbUpdateDlg, SLOT(setProgressMax(int)));
        connect(thread, SIGNAL(progressChanged(int)), dbUpdateDlg, SLOT(changeProgress(int)));
        thread->startUnthreaded();
//        while (thread->isRunning())
//            QApplication::processEvents();
        //msgBox->close();
        QMessageBox::information(this, tr("Update Complete"), tr("Database update complete."));
        dbUpdateDlg->hide();
        emit bmDbUpdated();
        delete(thread);
        //delete msgBox;
    }
}

void BmDbDialog::on_pushButtonUpdateAll_clicked()
{
    dbUpdateDlg->reset();
    dbUpdateDlg->show();
    //emit bmDbAboutToUpdate();
    //QMessageBox *msgBox = new QMessageBox(this);
    //msgBox->setStandardButtons(0);
    //msgBox->setText("Updating Database, please wait...");
    //msgBox->show();
    for (int i=0; i < pathsModel->rowCount(); i++)
    {
        QApplication::processEvents();
        BmDbUpdateThread *thread = new BmDbUpdateThread(QSqlDatabase::cloneDatabase(QSqlDatabase::database(), "bmThreadDb"),this);
        thread->setPath(pathsModel->data(pathsModel->index(i, 0)).toString());
        dbUpdateDlg->changeDirectory(pathsModel->data(pathsModel->index(i, 0)).toString());
        connect(thread, SIGNAL(progressMessage(QString)), dbUpdateDlg, SLOT(addProgressMsg(QString)));
        connect(thread, SIGNAL(stateChanged(QString)), dbUpdateDlg, SLOT(changeStatusTxt(QString)));
        connect(thread, SIGNAL(progressMaxChanged(int)), dbUpdateDlg, SLOT(setProgressMax(int)));
        connect(thread, SIGNAL(progressChanged(int)), dbUpdateDlg, SLOT(changeProgress(int)));
        thread->startUnthreaded();
//        while (thread->isRunning())
//            QApplication::processEvents();
        delete(thread);
    }
    //msgBox->close();
    QMessageBox::information(this, tr("Update Complete"), tr("Database update complete."));
    dbUpdateDlg->hide();
    emit bmDbUpdated();
}

void BmDbDialog::on_pushButtonClose_clicked()
{
    close();
}

void BmDbDialog::on_pushButtonClearDb_clicked()
{
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure?"));
    msgBox.setInformativeText(tr("Clearing the database will also clear all playlists.  If you have not already done so, you may want to export your playlists before performing this operation.  This operation can not be undone."));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        QSqlQuery query;
        query.exec("DELETE FROM bmplaylists");
        query.exec("DELETE FROM bmplsongs");
        query.exec("DELETE FROM bmsongs");
        query.exec("UPDATE sqlite_sequence SET seq = 0");
        query.exec("VACUUM");
        pathsModel->select();
        emit bmDbCleared();
    }
}

void BmDbDialog::on_pushButtonDelete_clicked()
{
    pathsModel->removeRow(selectedDirectoryIdx);
    pathsModel->select();
    pathsModel->submitAll();
}

void BmDbDialog::on_tableViewPaths_clicked(const QModelIndex &index)
{
    if (index.row() >= 0)
        selectedDirectoryIdx = index.row();
    else
        selectedDirectoryIdx = -1;
}



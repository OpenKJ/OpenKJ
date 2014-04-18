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

#include "databasedialog.h"
#include "ui_databasedialog.h"
#include "databaseupdatethread.h"
#include <QFileDialog>
#include <QMessageBox>

DatabaseDialog::DatabaseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DatabaseDialog)
{

    ui->setupUi(this);
    srcDirs = new BmSourceDirs(this);
    sourcedirmodel = new SourceDirTableModel(srcDirs, this);
    ui->treeViewPaths->setModel(sourcedirmodel);
}

DatabaseDialog::~DatabaseDialog()
{
    delete ui;
}

void DatabaseDialog::on_pushButtonAdd_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this);
    if (fileName != "")
    {
        sourcedirmodel->layoutAboutToBeChanged();
        srcDirs->add(fileName);
        sourcedirmodel->layoutChanged();
        // Directory selected
    }
}

void DatabaseDialog::on_pushButtonUpdateAll_clicked()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(0);
    msgBox->setText("Updating Database, please wait...");
    msgBox->show();
    for (int i=0; i < srcDirs->size(); i++)
    {
        QApplication::processEvents();
        DatabaseUpdateThread thread;
        thread.setPath(srcDirs->at(i)->getPath());
        thread.start();
        while (thread.isRunning())
            QApplication::processEvents();
    }
    msgBox->close();
    delete msgBox;
}

void DatabaseDialog::on_pushButtonClose_clicked()
{
    close();
}

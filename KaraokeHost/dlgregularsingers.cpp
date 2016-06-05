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

#include "dlgregularsingers.h"
#include "ui_dlgregularsingers.h"
#include <QDebug>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSqlQuery>

DlgRegularSingers::DlgRegularSingers(RotationModel *rotationModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRegularSingers)
{
    m_rtClickRegSingerId = -1;
    ui->setupUi(this);
    regModel = new QSqlTableModel(this);
    regModel->setTable("regularsingers");
    regModel->sort(1, Qt::AscendingOrder);
    ui->tableViewRegulars->setModel(regModel);
    regDelegate = new RegItemDelegate(this);
    ui->tableViewRegulars->setItemDelegate(regDelegate);
    ui->comboBoxAddPos->addItem("Fair");
    ui->comboBoxAddPos->addItem("Bottom");
    ui->comboBoxAddPos->addItem("Next");
    rotModel = rotationModel;
    ui->tableViewRegulars->hideColumn(0);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableViewRegulars->horizontalHeader()->resizeSection(3, 20);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableViewRegulars->horizontalHeader()->resizeSection(4, 20);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    regModel->setHeaderData(2, Qt::Horizontal, "Songs");
    regModel->setHeaderData(3, Qt::Horizontal, "");
    regModel->setHeaderData(4, Qt::Horizontal, "");
    regModel->select();
    connect(rotModel, SIGNAL(regularsModified()), regModel, SLOT(select()));
}

DlgRegularSingers::~DlgRegularSingers()
{
    delete ui;
}

void DlgRegularSingers::regularsChanged()
{
    regModel->select();
}

void DlgRegularSingers::on_btnClose_clicked()
{
    close();
}

void DlgRegularSingers::on_tableViewRegulars_clicked(const QModelIndex &index)
{
    if (index.column() == 3)
    {
        if (rotModel->singerExists(index.sibling(index.row(), 1).data().toString()))
        {
            QMessageBox::warning(this, tr("Naming conflict"), tr("A rotation singer already exists with the same name as the regular you're attempting to add. Action aborted."), QMessageBox::Close);
            return;
        }
        if ((ui->comboBoxAddPos->currentText() == "Next") && (rotModel->currentSinger() != -1))
            rotModel->regularLoad(index.sibling(index.row(), 0).data().toInt(), rotModel->ADD_NEXT);
        else if ((ui->comboBoxAddPos->currentText() == "Fair") && (rotModel->currentSinger() != -1))
            rotModel->regularLoad(index.sibling(index.row(), 0).data().toInt(), rotModel->ADD_FAIR);
        else
            rotModel->regularLoad(index.sibling(index.row(), 0).data().toInt(), rotModel->ADD_BOTTOM);
        return;
    }
    if (index.column() == 4)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Are you sure you want to delete this regular singer?");
        msgBox.setInformativeText("This will completely remove the regular singer from the database and can not be undone.  Note that if the singer is already loaded they won't be deleted from the rotation but regular tracking will be disabled.");
        QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();
        if (msgBox.clickedButton() == yesButton)
        {
            rotModel->regularDelete(index.sibling(index.row(), 0).data().toInt());
            regModel->select();
        }
    }
}

void DlgRegularSingers::on_tableViewRegulars_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewRegulars->indexAt(pos);
    if (index.isValid())
    {
        m_rtClickRegSingerId = index.sibling(index.row(),0).data().toInt();
        QMenu contextMenu(this);
        contextMenu.addAction("Rename", this, SLOT(renameRegSinger()));
        contextMenu.exec(QCursor::pos());
    }
}

void DlgRegularSingers::editSingerDuplicateError()
{
    QMessageBox::warning(this, tr("Duplicate Name"), tr("A regular singer by that name already exists, edit cancelled."),QMessageBox::Close);
}

void DlgRegularSingers::renameRegSinger()
{
    bool ok;
    QString currentName = rotModel->getRegularName(m_rtClickRegSingerId);
    QString name = QInputDialog::getText(this, "Rename regular singer", "New name:", QLineEdit::Normal, currentName, &ok);
    if (ok && !name.isEmpty())
    {
        if ((name.toLower() == currentName.toLower()) && (name != currentName))
        {
            // changing capitalization only
            rotModel->regularSetName(m_rtClickRegSingerId, name);
        }
        else if (rotModel->regularExists(name))
        {
            QMessageBox::warning(this, "Regular singer exists!","A regular singer named " + name + " already exists. Please choose a unique name and try again. The operation has been cancelled.",QMessageBox::Ok);
        }
        else
        {
            rotModel->regularSetName(m_rtClickRegSingerId, name);
        }
        regModel->select();
    }
}

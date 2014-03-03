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

#include "regularsingersdialog.h"
#include "ui_regularsingersdialog.h"
#include <QDebug>
#include <QSqlQuery>

RegularSingersDialog::RegularSingersDialog(KhRegularSingers *regSingers, KhRotationSingers *rotSingers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegularSingersDialog)
{
    ui->setupUi(this);
    regularSingerModel = new RegularSingerModel(regSingers, this);
    ui->treeViewRegulars->setModel(regularSingerModel);
    ui->treeViewRegulars->hideColumn(0);
    ui->treeViewRegulars->setColumnWidth(3,20);
    ui->treeViewRegulars->setColumnWidth(4,20);
    ui->treeViewRegulars->setColumnWidth(5,20);
    ui->treeViewRegulars->header()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
    ui->treeViewRegulars->header()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->treeViewRegulars->header()->setSectionResizeMode(3,QHeaderView::Fixed);
    ui->treeViewRegulars->header()->setSectionResizeMode(4,QHeaderView::Fixed);
    ui->treeViewRegulars->header()->setSectionResizeMode(5,QHeaderView::Fixed);
    ui->comboBoxAddPos->addItem("Fair");
    ui->comboBoxAddPos->addItem("Bottom");
    ui->comboBoxAddPos->addItem("Next");
    m_rotSingers = rotSingers;
    m_regSingers = regSingers;
}

RegularSingersDialog::~RegularSingersDialog()
{
    delete ui;
}

void RegularSingersDialog::on_btnClose_clicked()
{
    close();
}

void RegularSingersDialog::on_treeViewRegulars_clicked(const QModelIndex &index)
{
    if (index.column() == 3)
    {
        // Add to rotation
        qDebug() << "Add to rotation clicked on row " << index.row();
        addRegularToRotation(index.row());

    }
    else if (index.column() == 4)
    {
        // Rename regular
        qDebug() << "Rename singer clicked on row " << index.row();
    }
    else if (index.column() == 5)
    {
        // Delete regular
        qDebug() << "Delete singer clicked on row " << index.row();
        emit regularSingerDeleted(regularSingerModel->getRegularSingerByListIndex(index.row())->getIndex());
        regularSingerModel->removeByListIndex(index.row());
    }
}

void RegularSingersDialog::addRegularToRotation(int ListIndex)
{
    if (m_rotSingers->singerExists(m_regSingers->at(ListIndex)->getName()))
    {
        //Singer with name already exists in rotation
    }
    else
    {
        QSqlQuery query("BEGIN TRANSACTION");
        m_rotSingers->singerAdd(m_regSingers->at(ListIndex)->getName());
        KhSinger *rotSinger = m_rotSingers->getSingers()->at(m_rotSingers->getSingers()->size() -1);
        KhRegularSinger *regSinger = m_regSingers->at(ListIndex);
        regSinger->getRegSongs()->sort();
        for (int i=0; i < regSinger->getRegSongs()->getRegSongs()->size(); i++)
        {
            KhRegularSong *regSong = regSinger->getRegSongs()->getRegSongs()->at(i);
            rotSinger->addSongAtEnd(regSong->getSongIndex());
            rotSinger->getQueueSongs()->at(i)->setRegSingerIndex(regSinger->getIndex());
            rotSinger->getQueueSongs()->at(i)->setRegSong(true);
            rotSinger->getQueueSongs()->at(i)->setRegSongIndex(regSong->getRegSongIndex());
        }
        rotSinger->setRegular(true);
        rotSinger->setRegularIndex(regSinger->getIndex());
        query.exec("COMMIT TRANSACTION");
    }
}

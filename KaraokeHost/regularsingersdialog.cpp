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

RegularSingersDialog::RegularSingersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegularSingersDialog)
{
    ui->setupUi(this);
    regularSingerModel = new RegularSingerModel(this);
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
}

RegularSingersDialog::~RegularSingersDialog()
{
    delete ui;
}

void RegularSingersDialog::on_btnClose_clicked()
{
    close();
}

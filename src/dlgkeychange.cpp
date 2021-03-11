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

#include "dlgkeychange.h"
#include "ui_dlgkeychange.h"

DlgKeyChange::DlgKeyChange(TableModelQueueSongs *queueModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgKeyChange)
{
    qModel = queueModel;
    m_activeSong = -1;
    ui->setupUi(this);
}

void DlgKeyChange::setActiveSong(int songId)
{
    m_activeSong = songId;
    ui->spinBoxKey->setValue(qModel->getKey(m_activeSong));
}

DlgKeyChange::~DlgKeyChange()
{
    delete ui;
}

void DlgKeyChange::on_buttonBox_accepted()
{
    qModel->setKey(m_activeSong, ui->spinBoxKey->value());
    close();
}

void DlgKeyChange::on_spinBoxKey_valueChanged(int arg1)
{
    if (arg1 > 0)
        ui->spinBoxKey->setPrefix("+");
    else
        ui->spinBoxKey->setPrefix("");
}

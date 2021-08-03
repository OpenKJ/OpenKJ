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

#include "dlgregularsingers.h"
#include "ui_dlgregularsingers.h"
#include <QDebug>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSqlQuery>


DlgRegularSingers::DlgRegularSingers(TableModelRotation *rotationModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRegularSingers)
{
    m_rtClickHistorySingerId = -1;
    ui->setupUi(this);
    m_settings.restoreWindowState(this);
    ui->tableViewRegulars->setModel(&m_historySingersModel);
    ui->tableViewRegulars->setItemDelegate(&m_historySingersDelegate);
    ui->comboBoxAddPos->addItem("Fair");
    ui->comboBoxAddPos->addItem("Bottom");
    ui->comboBoxAddPos->addItem("Next");
    ui->comboBoxAddPos->setCurrentIndex(m_settings.lastSingerAddPositionType());
    connect(ui->comboBoxAddPos, qOverload<int>(&QComboBox::currentIndexChanged), &m_settings, &Settings::setLastSingerAddPositionType);
    m_rotModel = rotationModel;
    ui->tableViewRegulars->hideColumn(0);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableViewRegulars->horizontalHeader()->resizeSection(3, 20);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableViewRegulars->horizontalHeader()->resizeSection(4, 20);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}

DlgRegularSingers::~DlgRegularSingers()
{
    delete ui;
}

void DlgRegularSingers::regularsChanged()
{
    m_historySingersModel.loadSingers();
    //regModel->select();
}

void DlgRegularSingers::on_btnClose_clicked()
{
    close();
}

void DlgRegularSingers::on_tableViewRegulars_clicked(const QModelIndex &index)
{
    if (index.column() == 3)
    {
        on_tableViewRegulars_doubleClicked(index);
        return;
    }
    if (index.column() == 4)
    {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Are you sure you want to delete this singer's history?"));
        msgBox.setInformativeText(tr("This will completely remove this singer's song history from the database and can not be undone."));
        QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();
        if (msgBox.clickedButton() == yesButton)
        {
            m_historySingersModel.deleteHistory(index.sibling(index.row(), 0).data().toInt());
        }
    }
}

void DlgRegularSingers::on_tableViewRegulars_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableViewRegulars->indexAt(pos);
    if (index.isValid())
    {
        m_rtClickHistorySingerId = index.sibling(index.row(),0).data().toInt();
        QMenu contextMenu(this);
        contextMenu.addAction(tr("Rename"), this, &DlgRegularSingers::renameHistorySinger);
        contextMenu.exec(QCursor::pos());
    }
}

void DlgRegularSingers::renameHistorySinger()
{
    bool ok;
    QString currentName = m_historySingersModel.getName(m_rtClickHistorySingerId);
    QString name = QInputDialog::getText(this, tr("Rename history singer"), tr("New name:"), QLineEdit::Normal, currentName, &ok);

    if (name.isEmpty() || !ok || name == currentName)
        return;
    if (m_historySingersModel.exists(name) && name.toLower() != currentName.toLower())
    {
        QMessageBox::warning(
                    this,
                    tr("A history singer with this name already exists!"),
                    tr("A history singer named ") + name + tr(" already exists. Please choose a unique name and try again. "
                                                              "The operation has been cancelled."),
                    QMessageBox::Ok
                    );
        return;
    }
    if (m_rotModel->singerExists(currentName))
    {
        if (!m_rotModel->singerExists(name))
        {
            auto answer = QMessageBox::question(this,
                                                "Rename matching rotation singer?",
                                                "This singer appears to currently be in the rotation, and no singer matching the new name "
                                                "exists.  Would you like to rename the roation singer as well?",
                                                QMessageBox::StandardButtons(QMessageBox::Yes|QMessageBox::No),
                                                QMessageBox::Yes
                                                );
            if (answer == QMessageBox::No)
                return;
            m_rotModel->singerSetName(m_rotModel->getSingerId(currentName), name);
        }
        else
        {
            auto answer = QMessageBox::question(this,
                                                "Can't rename the matching rotation singer!",
                                                "This singer appears to currently be in the rotation, but another rotation singer also "
                                                "exists which conflicts with the new name. OpenKJ will be unable to rename the matching "
                                                "rotation singer and history data may be lost. Would you like to rename the singer anyway?"
                                                );
            if (answer == QMessageBox::No)
                return;
        }
    }
    m_historySingersModel.rename(m_rtClickHistorySingerId, name);
}

void DlgRegularSingers::on_lineEditSearch_textChanged(const QString &arg1)
{
    m_historySingersModel.filter(arg1);
    //proxyModel->setFilterString(QString("%1").arg(arg1));
    //proxyModel->sort(1);
}

void DlgRegularSingers::on_tableViewRegulars_doubleClicked(const QModelIndex &index)
{
    QString singerName = index.sibling(index.row(), 1).data().toString();
    if (m_rotModel->singerExists(singerName))
    {
        if (m_rotModel->singerIsRegular(m_rotModel->getSingerId(singerName)))
        {
            QMessageBox::warning(this, "This regular singer is already in the rotation",
                                 "This regular singer already exists in the current rotation.\n\nAction cancelled.");
            return;
        }
        auto answer = QMessageBox::question(this,
                                            "A singer by this name already exists in the rotation",
                                            "A singer with this name is currently in the rotation, but they aren't marked as a regular.\n"
                                            "Would you like load this regular's song history for the matching rotation singer?",
                                            QMessageBox::StandardButtons(QMessageBox::Yes|QMessageBox::Cancel),
                                            QMessageBox::Yes
                                            );
        if (answer == QMessageBox::Yes)
        {
            int singerId = m_rotModel->getSingerId(singerName);
            m_rotModel->singerMakeRegular(singerId);
        }
        return;
    }
    if ((ui->comboBoxAddPos->currentIndex() == 2) && (m_rotModel->currentSinger() != -1))
        m_rotModel->singerAdd(singerName, m_rotModel->ADD_NEXT);
    else if ((ui->comboBoxAddPos->currentIndex() == 0) && (m_rotModel->currentSinger() != -1))
        m_rotModel->singerAdd(singerName, m_rotModel->ADD_FAIR);
    else
        m_rotModel->singerAdd(singerName, m_rotModel->ADD_BOTTOM);
    m_rotModel->singerMakeRegular(m_rotModel->getSingerId(singerName));
}


void DlgRegularSingers::closeEvent([[maybe_unused]]QCloseEvent *event)
{
    m_settings.saveWindowState(this);
    hide();
}

void DlgRegularSingers::toggleVisibility() {
    if (!isVisible())
        ui->comboBoxAddPos->setCurrentIndex(m_settings.lastSingerAddPositionType());
    setVisible(!isVisible());

}

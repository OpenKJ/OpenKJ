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

#include "dlgregularexport.h"
#include "ui_dlgregularexport.h"
#include <QFileDialog>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>
#include <QXmlStreamWriter>
#include <QSqlQuery>
#include <QApplication>

DlgRegularExport::DlgRegularExport(RotationModel *rotationModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRegularExport)
{
    ui->setupUi(this);
    regModel = new QSqlTableModel(this);
    regModel->setTable("regularsingers");
    regModel->sort(1, Qt::AscendingOrder);
    regModel->select();
    rotModel = rotationModel;
    ui->tableViewRegulars->setModel(regModel);
    ui->tableViewRegulars->hideColumn(0);
    ui->tableViewRegulars->hideColumn(2);
    ui->tableViewRegulars->hideColumn(3);
    ui->tableViewRegulars->hideColumn(4);
    ui->tableViewRegulars->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    connect(rotModel, SIGNAL(regularsModified()), regModel, SLOT(select()));
}

DlgRegularExport::~DlgRegularExport()
{
    delete ui;
}

void DlgRegularExport::on_pushButtonClose_clicked()
{
    close();
}

void DlgRegularExport::on_pushButtonExport_clicked()
{
    QModelIndexList selList = ui->tableViewRegulars->selectionModel()->selectedRows();
    QList<int> selRegs;
    for (int i=0; i < selList.size(); i++)
    {
        selRegs << selList.at(i).data().toInt();
    }
    if (selRegs.size() > 0)
    {
        QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "KhRegularSingersExport.xml";
        qDebug() << "Default save location: " << defaultFilePath;
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select file to save regulars to"), defaultFilePath, "(*.xml)");
        if (saveFilePath != "")
        {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setStandardButtons(0);
            msgBox->setText(tr("Exporting regular singers, please wait..."));
            msgBox->show();
            exportSingers(selRegs, saveFilePath);
            msgBox->close();
            delete msgBox;
            QMessageBox::information(this, tr("Export complete"), tr("Regular singer export complete."));
            ui->tableViewRegulars->clearSelection();
        }
    }
}

void DlgRegularExport::on_pushButtonExportAll_clicked()
{
    ui->tableViewRegulars->selectAll();
    QModelIndexList selList = ui->tableViewRegulars->selectionModel()->selectedRows();
    QList<int> selRegs;
    for (int i=0; i < selList.size(); i++)
    {
        selRegs << selList.at(i).data().toInt();
    }
    if (selRegs.size() > 0)
    {
        QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "KhRegularSingersExport.xml";
        qDebug() << "Default save location: " << defaultFilePath;
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select file to save regulars to"), defaultFilePath, "(*.xml)");
        if (saveFilePath != "")
        {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setStandardButtons(0);
            msgBox->setText(tr("Exporting regular singers, please wait..."));
            msgBox->show();
            exportSingers(selRegs, saveFilePath);
            msgBox->close();
            delete msgBox;
            QMessageBox::information(this, tr("Export complete"), tr("Regular singer export complete."));
            ui->tableViewRegulars->clearSelection();
        }
    }
}

void DlgRegularExport::exportSingers(QList<int> regSingerIds, QString savePath)
{
    QFile *xmlFile = new QFile(savePath);
    xmlFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QXmlStreamWriter xml(xmlFile);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("regulars");
    for (int r=0; r < regSingerIds.size(); r++)
    {
        QApplication::processEvents();
        int regSingerId = regSingerIds.at(r);
        xml.writeStartElement("singer");
        xml.writeAttribute("name", rotModel->getRegularName(regSingerId));
        QSqlQuery query;
        query.exec("SELECT dbsongs.artist,dbsongs.title,dbsongs.discid,regularsongs.keychg FROM regularsongs,dbsongs WHERE dbsongs.songid == regularsongs.songid AND regularsongs.regsingerid == " + QString::number(regSingerId) + " ORDER BY regularsongs.position");
        while (query.next())
        {
            QApplication::processEvents();
            xml.writeStartElement("song");
            xml.writeAttribute("artist", query.value(0).toString());
            xml.writeAttribute("title", query.value(1).toString());
            xml.writeAttribute("discid", query.value(2).toString());
            xml.writeAttribute("key", query.value(3).toString());
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    xmlFile->close();
}

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

#include "dlgregularimport.h"
#include "ui_dlgregularimport.h"
#include <QFileDialog>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSqlQuery>
#include <QXmlStreamReader>
#include <QApplication>

DlgRegularImport::DlgRegularImport(RotationModel *rotationModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRegularImport)
{
    ui->setupUi(this);
    curImportFile = "";
    rotModel = rotationModel;
}

DlgRegularImport::~DlgRegularImport()
{
    delete ui;
}

void DlgRegularImport::on_pushButtonSelectFile_clicked()
{
    QString importFile = QFileDialog::getOpenFileName(this,tr("Select file to load regulars from"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("(*.xml)"));
    if (importFile != "")
    {
        curImportFile = importFile;
        QStringList singers = loadSingerList(importFile);
        ui->listWidgetRegulars->clear();
        ui->listWidgetRegulars->addItems(singers);
    }
}

void DlgRegularImport::on_pushButtonClose_clicked()
{
    close();
}

void DlgRegularImport::on_pushButtonImport_clicked()
{
    if (ui->listWidgetRegulars->selectedItems().size() > 0)
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setStandardButtons(0);
        msgBox->setText("Importing regular singers, please wait...");
        msgBox->show();
        for (int i=0; i < ui->listWidgetRegulars->selectedItems().size(); i++)
        {
            msgBox->setInformativeText("Importing singer: " + ui->listWidgetRegulars->selectedItems().at(i)->text());
            importSinger(ui->listWidgetRegulars->selectedItems().at(i)->text());
        }
        msgBox->close();
        delete msgBox;
        QMessageBox::information(this, "Import complete", "Regular singer import complete.");
        ui->listWidgetRegulars->clearSelection();
    }
}

void DlgRegularImport::on_pushButtonImportAll_clicked()
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(0);
    msgBox->setText("Importing regular singers, please wait...");
    msgBox->show();
    ui->listWidgetRegulars->selectAll();
    for (int i=0; i < ui->listWidgetRegulars->selectedItems().size(); i++)
    {
        msgBox->setInformativeText("Importing singer: " + ui->listWidgetRegulars->selectedItems().at(i)->text());
        importSinger(ui->listWidgetRegulars->selectedItems().at(i)->text());
    }
    msgBox->close();
    delete msgBox;
    QMessageBox::information(this, "Import complete", "Regular singer import complete.");
        ui->listWidgetRegulars->clearSelection();
}

QStringList DlgRegularImport::loadSingerList(QString fileName)
{
    QStringList singers;
    QFile *xmlFile = new QFile(fileName);
    xmlFile->open(QIODevice::ReadOnly);
    QXmlStreamReader xml(xmlFile);
    while (!xml.isEndDocument())
    {
        xml.readNext();
        if ((xml.isStartElement()) && (xml.name() == "singer"))
            singers << xml.attributes().value("name").toString();
    }
    xmlFile->close();
    singers.sort();
    return singers;
}

void DlgRegularImport::importSinger(QString name)
{
    if (rotModel->regularExists(name))
    {
        QMessageBox::warning(this, tr("Naming conflict"),QString("A regular singer named \"" + name + "\" already exists.  Please remove or rename the existing regular singer and try again."));
        return;
    }
    QFile *xmlFile = new QFile(curImportFile);
    xmlFile->open(QIODevice::ReadOnly);
    QXmlStreamReader xml(xmlFile);
    bool done = false;
    while ((!xml.isEndDocument()) && (!done))
    {
        QApplication::processEvents();
        xml.readNext();
        if ((xml.isStartElement()) && (xml.name() == "singer") && (xml.attributes().value("name") == name))
        {
            int regSingerId = rotModel->regularAdd(name);
            xml.readNext();
            int position = 0;
            while ((xml.name() != "singer") && (!xml.isEndDocument()))
            {
                if ((xml.isStartElement()) && (xml.name() == "song"))
                {
                    QApplication::processEvents();
                    QSqlQuery query;
                    QString discId = xml.attributes().value("discid").toString();
                    QString artist = xml.attributes().value("artist").toString();
                    QString title = xml.attributes().value("title").toString();
                    QString keyChg = xml.attributes().value("key").toString();
                    QString sql = "SELECT songid FROM dbsongs WHERE artist == \"" + artist + "\" AND title == \"" + title + "\" AND discid == \"" + discId + "\" LIMIT 1";
                    query.exec(sql);
                    if (query.first())
                    {
                        QString songId = query.value(0).toString();
                        sql = "INSERT INTO regularsongs (regsingerid, songid, keychg, position) VALUES(" + QString::number(regSingerId) + "," + songId + "," + keyChg + "," + QString::number(position) + ")";
                        query.exec(sql);
                        position++;
                    }
                    else
                       QMessageBox::warning(this, tr("No song match found"),QString("An exact song DB match for the song \"" + discId + " - " + artist + " - " + title + "\" could not be found while importing singer \"" + name + "\", skipping import for this song."));
                }
                xml.readNext();
            }
            done = true;
        }
    }
    xmlFile->close();
}

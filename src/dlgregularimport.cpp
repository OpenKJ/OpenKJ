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

#include "dlgregularimport.h"
#include "ui_dlgregularimport.h"
#include <QFileDialog>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSqlQuery>
#include <QXmlStreamReader>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


DlgRegularImport::DlgRegularImport(TableModelKaraokeSongs &karaokeSongsModel, QWidget *parent) :
    m_karaokeSongsModel(karaokeSongsModel),
    QDialog(parent),
    ui(new Ui::DlgRegularImport)
{
    ui->setupUi(this);
    m_curImportFile = "";
}

DlgRegularImport::~DlgRegularImport()
{
    delete ui;
}

void DlgRegularImport::on_pushButtonSelectFile_clicked()
{
    QString importFile = QFileDialog::getOpenFileName(this,tr("Select file to load regulars from"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "OpenKJ export files (*.xml *json)",
                                                      nullptr, QFileDialog::DontUseNativeDialog);
    if (importFile != "")
    {
        m_curImportFile = importFile;
        QStringList singers;
        if (m_curImportFile.endsWith("xml", Qt::CaseInsensitive))
            singers = legacyLoadSingerList(importFile);
        else
            singers = loadSingerList(importFile);
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
    QStringList errors;
    if (ui->listWidgetRegulars->selectedItems().size() < 1)
        return;

    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(QFlags<QMessageBox::StandardButton>());
    msgBox->setText(tr("Importing regular singers, please wait..."));
    msgBox->show();
    for (int i=0; i < ui->listWidgetRegulars->selectedItems().size(); i++)
    {
        QString name = ui->listWidgetRegulars->selectedItems().at(i)->text();
        if (m_historySingersModel.exists(name))
        {
            QMessageBox msgBox;
            auto mergeBtn = msgBox.addButton("Merge", QMessageBox::ActionRole);
            auto replaceBtn = msgBox.addButton("Replace", QMessageBox::DestructiveRole);
            auto skipBtn = msgBox.addButton("Skip", QMessageBox::RejectRole);
            msgBox.setDefaultButton(mergeBtn);
            msgBox.setWindowTitle("Naming conflict");
            msgBox.setText("An existing singer named \"" + name + "\" already exists.\nHow would you like to proceed?");
            msgBox.exec();
            if (msgBox.clickedButton() == skipBtn)
                continue;
            if (msgBox.clickedButton() == replaceBtn)
            {
                m_historySingersModel.deleteHistory(m_historySingersModel.getId(name));
            }
        }
        msgBox->setInformativeText(tr("Importing singer: ") + name);
        if (m_curImportFile.endsWith("xml", Qt::CaseInsensitive))
            errors.append(legacyImportSinger(ui->listWidgetRegulars->selectedItems().at(i)->text()));
        else
            errors.append(importSinger(ui->listWidgetRegulars->selectedItems().at(i)->text()));
    }
    msgBox->close();
    delete msgBox;

    if (errors.size() > 0)
    {
        QMessageBox msgBox;
        msgBox.addButton(QMessageBox::StandardButton::Ok);
        msgBox.setDetailedText(errors.join("\n"));
        msgBox.setText("Some songs could not be imported because there were not matching songs in your database");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }

    QMessageBox::information(this, tr("Import complete"), tr("Regular singer import complete."));
    ui->listWidgetRegulars->clearSelection();
}

void DlgRegularImport::on_pushButtonImportAll_clicked()
{
    QStringList errors;
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(QFlags<QMessageBox::StandardButton>());
    msgBox->setText(tr("Importing regular singers, please wait..."));
    msgBox->show();
    ui->listWidgetRegulars->selectAll();
    for (int i=0; i < ui->listWidgetRegulars->selectedItems().size(); i++)
    {
        QString name = ui->listWidgetRegulars->selectedItems().at(i)->text();
        if (m_historySingersModel.exists(name))
        {
            QMessageBox msgBox;
            auto mergeBtn = msgBox.addButton("Merge", QMessageBox::ActionRole);
            auto replaceBtn = msgBox.addButton("Replace", QMessageBox::DestructiveRole);
            auto skipBtn = msgBox.addButton("Skip", QMessageBox::RejectRole);
            msgBox.setDefaultButton(mergeBtn);
            msgBox.setWindowTitle("Naming conflict");
            msgBox.setText("An existing singer named \"" + name + "\" already exists.\nHow would you like to proceed?");
            msgBox.exec();
            if (msgBox.clickedButton() == skipBtn)
                continue;
            if (msgBox.clickedButton() == replaceBtn)
            {
                m_historySingersModel.deleteHistory(m_historySingersModel.getId(name));
            }
        }
        msgBox->setInformativeText(tr("Importing singer: ") + ui->listWidgetRegulars->selectedItems().at(i)->text());
        if (m_curImportFile.endsWith("xml", Qt::CaseInsensitive))
            errors.append(legacyImportSinger(ui->listWidgetRegulars->selectedItems().at(i)->text()));
        else
            errors.append(importSinger(ui->listWidgetRegulars->selectedItems().at(i)->text()));
    }
    msgBox->close();
    delete msgBox;

    if (errors.size() > 0)
    {
        QMessageBox msgBox;
        msgBox.addButton(QMessageBox::StandardButton::Ok);
        msgBox.setDetailedText(errors.join("\n"));
        msgBox.setText("Some songs could not be imported because there were not matching songs in your database");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }

    QMessageBox::information(this, tr("Import complete"), tr("Regular singer import complete."));
        ui->listWidgetRegulars->clearSelection();
}

QStringList DlgRegularImport::legacyLoadSingerList(const QString &fileName)
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

QStringList DlgRegularImport::loadSingerList(const QString &filename)
{
    m_curImportFile = filename;
    QStringList singers;
    QFile importFile(filename);
    importFile.open(QFile::ReadOnly);
    auto contents = importFile.readAll();
    auto jDoc = QJsonDocument::fromJson(contents);
    auto array = jDoc.array();
    std::for_each(array.begin(), array.end(), [&singers] (QJsonValueRef singer) {
        singers.push_back(singer.toObject().value("name").toString());
    });
    return singers;
}

QStringList DlgRegularImport::legacyImportSinger(const QString &name)
{
    QStringList missingSongs;
    QFile *xmlFile = new QFile(m_curImportFile);
    xmlFile->open(QIODevice::ReadOnly);
    QXmlStreamReader xml(xmlFile);
    bool done = false;
    while ((!xml.isEndDocument()) && (!done))
    {
        QApplication::processEvents();
        xml.readNext();
        if ((xml.isStartElement()) && (xml.name() == "singer") && (xml.attributes().value("name") == name))
        {
            xml.readNext();
            int position = 0;
            while ((xml.name() != "singer") && (!xml.isEndDocument()))
            {
                if ((xml.isStartElement()) && (xml.name() == "song"))
                {
                    QApplication::processEvents();
                    QSqlQuery query;
                    QString songId = xml.attributes().value("discid").toString();
                    QString artist = xml.attributes().value("artist").toString();
                    QString title = xml.attributes().value("title").toString();
                    int keyChg = xml.attributes().value("key").toInt();

                    QString sql = "SELECT path FROM dbsongs WHERE artist == \"" + artist + "\" AND title == \"" + title + "\" AND discid == \"" + songId + "\" LIMIT 1";
                    query.exec(sql);
                    if (query.first())
                    {
                        QString path = query.value(0).toString();
                        m_historySongsModel.saveSong(name,path,artist,title,songId,keyChg);
                        position++;
                    }
                    else
                    {
                        QString vendorPart;
                        for (int i=0; i < songId.size(); i++)
                        {
                            QChar character = songId.at(i);
                            if (character.isLetter())
                                vendorPart.append(character);
                            else
                                break;
                        }
                       sql = "SELECT path FROM dbsongs WHERE artist == \"" + artist + "\" AND title == \"" + title + "\" AND discid LIKE \"%" + vendorPart + "%\" LIMIT 1";
                       query.exec(sql);
                       if (query.first())
                       {
                           QString path = query.value(0).toString();
                           m_historySongsModel.saveSong(name,path,artist,title,songId,keyChg);
                           position++;
                       }
                       else
                       {
                           query.prepare("SELECT path FROM dbsongs WHERE discid = :discid");
                           query.bindValue(":discid", songId);
                           query.exec();
                           if (query.first())
                           {
                               QString path = query.value(0).toString();
                               m_historySongsModel.saveSong(name,path,artist,title,songId,keyChg);
                           }
                           else
                               missingSongs.append("Song: \"" + songId + " - " + artist + " - " + title + "\" Missing for singer: " + name);
                       }
                    }
                }
                xml.readNext();
            }
            done = true;
        }
    }
    xmlFile->close();
    return missingSongs;
}

QStringList DlgRegularImport::importSinger(const QString &name)
{
    QStringList missingFiles;
    QFile importFile(m_curImportFile);
    importFile.open(QFile::ReadOnly);
    auto contents = importFile.readAll();
    auto jDoc = QJsonDocument::fromJson(contents);
    auto array = jDoc.array();
    auto match = std::find_if(array.begin(), array.end(), [&name] (QJsonValueRef singer) {
        return (singer.toObject().value("name").toString() == name);
    });
    if (match == array.end())
        return QStringList();
    auto songs = match->toObject().value("songs").toArray();
    std::for_each(songs.begin(), songs.end(), [&] (QJsonValueRef song) {
        m_historySongsModel.saveSong(
                    name,
                    song.toObject().value("filepath").toString(),
                    song.toObject().value("artist").toString(),
                    song.toObject().value("title").toString(),
                    song.toObject().value("songid").toString(),
                    song.toObject().value("keychange").toInt(),
                    song.toObject().value("plays").toInt(),
                    QDateTime::fromString(song.toObject().value("lastplay").toString())
                    );
    });
    return missingFiles;
}


void DlgRegularImport::closeEvent([[maybe_unused]]QCloseEvent *event)
{
    deleteLater();
}


void DlgRegularImport::done(int)
{
    deleteLater();
}

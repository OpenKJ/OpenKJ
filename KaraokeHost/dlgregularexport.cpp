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

DlgRegularExport::DlgRegularExport(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRegularExport)
{
    ui->setupUi(this);
    regModel = new QSqlTableModel(this);
    regModel->setTable("regularsingers");
    regModel->select();
    rotModel = new RotationModel(this);
    ui->treeViewRegulars->setModel(regModel);
    ui->treeViewRegulars->hideColumn(0);
    ui->treeViewRegulars->hideColumn(2);
    ui->treeViewRegulars->hideColumn(3);
    ui->treeViewRegulars->header()->setSectionResizeMode(1,QHeaderView::Stretch);
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
    QModelIndexList selList = ui->treeViewRegulars->selectionModel()->selectedRows();
    QList<int> selRegs;
    for (int i=0; i < selList.size(); i++)
    {
        selRegs << selList.at(i).data().toInt();
    }
    if (selRegs.size() > 0)
    {
        QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "KhRegularSingersExport.xml";
        qDebug() << "Default save location: " << defaultFilePath;
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select file to save regulars to"), defaultFilePath, tr("(*.xml)"));
        if (saveFilePath != "")
        {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setStandardButtons(0);
            msgBox->setText("Exporting regular singers, please wait...");
            msgBox->show();
            exportSingers(selRegs, saveFilePath);
            msgBox->close();
            delete msgBox;
            QMessageBox::information(this, "Export complete", "Regular singer export complete.");
            ui->treeViewRegulars->clearSelection();
        }
    }
}

void DlgRegularExport::on_pushButtonExportAll_clicked()
{
    ui->treeViewRegulars->selectAll();
    QModelIndexList selList = ui->treeViewRegulars->selectionModel()->selectedRows();
    QList<int> selRegs;
    for (int i=0; i < selList.size(); i++)
    {
        selRegs << selList.at(i).data().toInt();
    }
    if (selRegs.size() > 0)
    {
        QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "KhRegularSingersExport.xml";
        qDebug() << "Default save location: " << defaultFilePath;
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select file to save regulars to"), defaultFilePath, tr("(*.xml)"));
        if (saveFilePath != "")
        {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setStandardButtons(0);
            msgBox->setText("Exporting regular singers, please wait...");
            msgBox->show();
            exportSingers(selRegs, saveFilePath);
            msgBox->close();
            delete msgBox;
            QMessageBox::information(this, "Export complete", "Regular singer export complete.");
            ui->treeViewRegulars->clearSelection();
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

#include "dlgregularexport.h"
#include "ui_dlgregularexport.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>

DlgRegularExport::DlgRegularExport(KhRegularSingers *regularSingers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgRegularExport)
{
    regSingers = regularSingers;
    ui->setupUi(this);
    regSingersModel = new RegularSingerModel(regSingers, this);
    ui->treeViewRegulars->setModel(regSingersModel);
    ui->treeViewRegulars->hideColumn(0);
    ui->treeViewRegulars->hideColumn(3);
    ui->treeViewRegulars->hideColumn(4);
    ui->treeViewRegulars->hideColumn(5);
    ui->treeViewRegulars->header()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
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
            regSingers->exportSingers(selRegs, saveFilePath);
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
            regSingers->exportSingers(selRegs, saveFilePath);
            msgBox->close();
            delete msgBox;
            QMessageBox::information(this, "Export complete", "Regular singer export complete.");
            ui->treeViewRegulars->clearSelection();
        }
    }
}

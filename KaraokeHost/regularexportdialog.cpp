#include "regularexportdialog.h"
#include "ui_regularexportdialog.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

RegularExportDialog::RegularExportDialog(KhRegularSingers *regularSingers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegularExportDialog)
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

RegularExportDialog::~RegularExportDialog()
{
    delete ui;
}

void RegularExportDialog::on_pushButtonClose_clicked()
{
    close();
}

void RegularExportDialog::on_pushButtonExport_clicked()
{
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "KhRegularSingersExport.xml";
    qDebug() << "Default save location: " << defaultFilePath;
    QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Select file to save regulars to"), defaultFilePath, tr("(*.xml)"));
    if (saveFilePath != "")
    {
        QModelIndexList selList = ui->treeViewRegulars->selectionModel()->selectedRows();
        QList<int> selRegs;
        for (int i=0; i < selList.size(); i++)
        {
            selRegs << selList.at(i).data().toInt();
        }
        if (selRegs.size() > 0)
            regSingers->exportSingers(selRegs, saveFilePath);
    }
}

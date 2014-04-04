#include "regularimportdialog.h"
#include "ui_regularimportdialog.h"
#include <QFileDialog>
#include <QStandardPaths>

RegularImportDialog::RegularImportDialog(KhRegularSingers *regSingersPtr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegularImportDialog)
{
    ui->setupUi(this);
    regSingers = regSingersPtr;
}

RegularImportDialog::~RegularImportDialog()
{
    delete ui;
}

void RegularImportDialog::on_pushButtonSelectFile_clicked()
{
    QString importFile = QFileDialog::getOpenFileName(this,tr("Select file to load regulars from"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), tr("(*.xml)"));
    if (importFile != "")
    {
        QStringList singers = regSingers->importLoadSingerList(importFile);
        ui->listWidgetRegulars->clear();
        ui->listWidgetRegulars->addItems(singers);
    }
}

void RegularImportDialog::on_pushButtonClose_clicked()
{
    close();
}

void RegularImportDialog::on_pushButtonImport_clicked()
{

}

void RegularImportDialog::on_pushButtonImportAll_clicked()
{

}

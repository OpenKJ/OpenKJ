#include "dlgdbupdate.h"
#include "ui_dlgdbupdate.h"

DlgDbUpdate::DlgDbUpdate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgDbUpdate)
{
    ui->setupUi(this);
    reset();
}

DlgDbUpdate::~DlgDbUpdate()
{
    delete ui;
}

void DlgDbUpdate::addProgressMsg(QString msg)
{
    ui->textEdit->append(msg);
}

void DlgDbUpdate::changeStatusTxt(QString txt)
{
    ui->lblCurrentActivity->setText(txt);
}

void DlgDbUpdate::changeProgress(int progress)
{
    ui->progressBar->setValue(progress);
}

void DlgDbUpdate::setProgressMax(int max)
{
    ui->progressBar->setMaximum(max);
}

void DlgDbUpdate::changeDirectory(QString dir)
{
    ui->lblDirectory->setText(dir);
}

void DlgDbUpdate::reset()
{
    ui->lblDirectory->setText("[none]");
    ui->progressBar->setValue(0);
    ui->textEdit->clear();
    ui->lblCurrentActivity->setText("");

}

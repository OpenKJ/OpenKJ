#include "dlgpassword.h"
#include "ui_dlgpassword.h"
#include "settings.h"

#include <QMessageBox>

DlgPassword::DlgPassword(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgPassword)
{
    ui->setupUi(this);
    ui->label->hide();
}

DlgPassword::~DlgPassword()
{
    delete ui;
}

void DlgPassword::on_pushButtonOk_clicked()
{
    Settings settings;
    if (settings.chkPassword(ui->lineEditPassword->text()))
    {
        ui->label->hide();
        password = ui->lineEditPassword->text();
        accept();
    }
    else
    {
        ui->label->show();
    }
}

void DlgPassword::on_pushButtonCancel_clicked()
{
    reject();
}

void DlgPassword::on_pushButtonReset_clicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Clear password?"));
    msgBox.setText(tr("Warning, this will erase all secured account and credit card data."));
    if (msgBox.exec() == QDialog::Accepted)
    {
        Settings settings;
        settings.clearPassword();
    }

}

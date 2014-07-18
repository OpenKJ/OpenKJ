#include "dlgkeychange.h"
#include "ui_dlgkeychange.h"

DlgKeyChange::DlgKeyChange(QueueModel *queueModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgKeyChange)
{
    qModel = queueModel;
    m_activeSong = -1;
    ui->setupUi(this);
}

void DlgKeyChange::setActiveSong(int songId)
{
    m_activeSong = songId;
    ui->spinBoxKey->setValue(qModel->getSongKey(m_activeSong));
}

DlgKeyChange::~DlgKeyChange()
{
    delete ui;
}

void DlgKeyChange::on_buttonBox_accepted()
{
    qModel->songSetKey(m_activeSong, ui->spinBoxKey->value());
    close();
}

void DlgKeyChange::on_spinBoxKey_valueChanged(int arg1)
{
    if (arg1 > 0)
        ui->spinBoxKey->setPrefix("+");
    else
        ui->spinBoxKey->setPrefix("");
}
